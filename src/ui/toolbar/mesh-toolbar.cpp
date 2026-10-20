// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file Mesh toolbar
 */
/*
 * Authors:
 *   bulia byak <bulia@dr.com>
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *   Abhishek Sharma
 *   Tavmjong Bah <tavjong@free.fr>
 *   Vaibhav Malik <vaibhavmalik2018@gmail.com>
 *
 * Copyright (C) 2012 Tavmjong Bah
 * Copyright (C) 2007 Johan Engelen
 * Copyright (C) 2005 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "mesh-toolbar.h"

#include <glibmm/i18n.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/liststore.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/togglebutton.h>

#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "object/sp-defs.h"
#include "object/sp-mesh-gradient.h"
#include "selection.h"
#include "style.h"
#include "ui/builder-utils.h"
#include "ui/dialog-run.h"
#include "ui/icon-names.h"
#include "ui/simple-pref-pusher.h"
#include "ui/tools/mesh-tool.h"
#include "ui/util.h"
#include "ui/widget/drop-down-list.h"
#include "ui/widget/spinbutton.h"

using Inkscape::DocumentUndo;
using Inkscape::UI::Tools::MeshTool;

namespace Inkscape::UI::Toolbar {
namespace {

// Get a list of selected meshes taking into account fill/stroke toggles
std::vector<SPMeshGradient *> ms_get_dt_selected_gradients(Selection *selection)
{
    std::vector<SPMeshGradient *> ms_selected;

    auto prefs = Preferences::get();
    bool edit_fill   = prefs->getBool("/tools/mesh/edit_fill",   true);
    bool edit_stroke = prefs->getBool("/tools/mesh/edit_stroke", true);

    for (auto item : selection->items()) {
        auto style = item->style;
        if (!style) {
            continue;
        }

        if (edit_fill && style->fill.isPaintserver()) {
            auto server = item->style->getFillPaintServer();
            if (auto mesh = cast<SPMeshGradient>(server)) {
                ms_selected.push_back(mesh);
            }
        }

        if (edit_stroke && style->stroke.isPaintserver()) {
            auto server = item->style->getStrokePaintServer();
            if (auto mesh = cast<SPMeshGradient>(server)) {
                ms_selected.push_back(mesh);
            }
        }
    }

    return ms_selected;
}

/*
 * Get the current selection status from the desktop
 */
void ms_read_selection(Selection *selection,
                       SPMeshGradient *&ms_selected,
                       bool &ms_selected_multi,
                       SPMeshType &ms_type,
                       bool &ms_type_multi)
{
    ms_selected = nullptr;
    ms_selected_multi = false;
    ms_type = SP_MESH_TYPE_COONS;
    ms_type_multi = false;

    bool first = true;

    // Read desktop selection, taking into account fill/stroke toggles
    for (auto const &mesh : ms_get_dt_selected_gradients(selection)) {
        if (first) {
            ms_selected = mesh;
            ms_type = mesh->type;
            first = false;
        } else {
            if (ms_selected != mesh) {
                ms_selected_multi = true;
            }
            if (ms_type != mesh->type) {
                ms_type_multi = true;
            }
        }
    }
}

} // namespace

MeshToolbar::MeshToolbar()
    : MeshToolbar{create_builder("toolbar-mesh.ui")}
{}

MeshToolbar::MeshToolbar(Glib::RefPtr<Gtk::Builder> const &builder)
    : Toolbar{get_widget<Gtk::Box>(builder, "mesh-toolbar")}
    , _row_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_row_item")}
    , _col_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_col_item")}
    , _edit_fill_btn{&get_widget<Gtk::ToggleButton>(builder, "_edit_fill_btn")}
    , _edit_stroke_btn{&get_widget<Gtk::ToggleButton>(builder, "_edit_stroke_btn")}
    , _show_handles_btn{&get_widget<Gtk::ToggleButton>(builder, "show_handles_btn")}
    , _select_type_item(get_derived_widget<UI::Widget::DropDownList>(builder, "type-selector"))
{
    auto prefs = Preferences::get();

    // Configure the types combo box.
    _select_type_item.append(C_("Type", "Coons"));
    _select_type_item.append(_("Bicubic"));
    _select_type_item.set_selected(0);
    _select_type_item.signal_changed().connect([this] { type_changed(_select_type_item.get_selected()); });

    // Setup the spin buttons.
    setup_derived_spin_button(_row_item, "mesh_rows", 1, &MeshToolbar::row_changed);
    setup_derived_spin_button(_col_item, "mesh_cols", 1, &MeshToolbar::col_changed);

    _row_item.set_custom_numeric_menu_data({
        {1, ""},
        {2, ""},
        {3, ""},
        {4, ""},
        {5, ""},
        {6, ""},
        {7, ""},
        {8, ""},
        {9, ""},
        {10, ""},
    });

    _col_item.set_custom_numeric_menu_data({
        {1, ""},
        {2, ""},
        {3, ""},
        {4, ""},
        {5, ""},
        {6, ""},
        {7, ""},
        {8, ""},
        {9, ""},
        {10, ""},
    });

    // Configure mode buttons
    int mode = prefs->getInt("/tools/mesh/mesh_geometry", SP_MESH_GEOMETRY_NORMAL);

    int btn_index = 0;
    for (auto &item : children(get_widget<Gtk::Box>(builder, "new_type_buttons_box"))) {
        auto &btn = dynamic_cast<Gtk::ToggleButton &>(item);
        btn.set_active(btn_index == mode);
        btn.signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &MeshToolbar::new_geometry_changed), btn_index++));
    }

    mode = prefs->getInt("/tools/mesh/newfillorstroke");

    btn_index = 0;
    for (auto &item : children(get_widget<Gtk::Box>(builder, "new_fillstroke_buttons_box"))) {
        auto &btn = dynamic_cast<Gtk::ToggleButton &>(item);
        btn.set_active(btn_index == mode);
        btn.signal_clicked().connect(
            sigc::bind(sigc::mem_fun(*this, &MeshToolbar::new_fillstroke_changed), btn_index++));
    }

    // Edit fill mesh.
    _edit_fill_pusher.reset(new UI::SimplePrefPusher(_edit_fill_btn, "/tools/mesh/edit_fill"));

    // Edit stroke mesh.
    _edit_stroke_pusher.reset(new UI::SimplePrefPusher(_edit_stroke_btn, "/tools/mesh/edit_stroke"));

    // Show/hide side and tensor handles.
    _show_handles_pusher.reset(new UI::SimplePrefPusher(_show_handles_btn, "/tools/mesh/show_handles"));

    _initMenuBtns();

    // Signals.
    _edit_fill_btn->signal_toggled().connect(sigc::mem_fun(*this, &MeshToolbar::toggle_fill_stroke));
    _edit_stroke_btn->signal_toggled().connect(sigc::mem_fun(*this, &MeshToolbar::toggle_fill_stroke));
    _show_handles_btn->signal_toggled().connect(sigc::mem_fun(*this, &MeshToolbar::toggle_handles));
    get_widget<Gtk::Button>(builder, "toggle_sides_btn")
        .signal_clicked()
        .connect(sigc::mem_fun(*this, &MeshToolbar::toggle_sides));
    get_widget<Gtk::Button>(builder, "make_elliptical_btn")
        .signal_clicked()
        .connect(sigc::mem_fun(*this, &MeshToolbar::make_elliptical));
    get_widget<Gtk::Button>(builder, "pick_colors_btn")
        .signal_clicked()
        .connect(sigc::mem_fun(*this, &MeshToolbar::pick_colors));
    get_widget<Gtk::Button>(builder, "scale_mesh_btn")
        .signal_clicked()
        .connect(sigc::mem_fun(*this, &MeshToolbar::fit_mesh));

    get_widget<Gtk::Button>(builder, "warning_btn").signal_clicked().connect([this] { warning_popup(); });
}

MeshToolbar::~MeshToolbar() = default;

void MeshToolbar::setDesktop(SPDesktop *desktop)
{
    if (_desktop) {
        c_selection_changed.disconnect();
        c_selection_modified.disconnect();
        c_subselection_changed.disconnect();
        c_defs_release.disconnect();
        c_defs_modified.disconnect();
    }

    Toolbar::setDesktop(desktop);

    if (_desktop) {
        // connect to selection modified and changed signals
        auto sel = desktop->getSelection();
        auto document = desktop->getDocument();

        c_selection_changed = sel->connectChanged([this] (auto) { selection_changed(); });
        c_selection_modified = sel->connectModified([this] (auto, auto) { selection_changed(); });

        c_defs_release = document->getDefs()->connectRelease([this] (auto) { selection_changed(); });
        c_defs_modified = document->getDefs()->connectModified([this] (auto, auto) { selection_changed(); });
        selection_changed();
    }
}

void MeshToolbar::setup_derived_spin_button(UI::Widget::SpinButton &btn, Glib::ustring const &name,
                                            double default_value, ValueChangedMemFun value_changed_mem_fun)
{
    auto const path = "/tools/mesh/" + name;
    auto const val = Preferences::get()->getDouble(path, default_value);

    auto adj = btn.get_adjustment();
    adj->set_value(val);
    adj->signal_value_changed().connect(sigc::mem_fun(*this, value_changed_mem_fun));

    btn.setDefocusTarget(this);
}

void MeshToolbar::new_geometry_changed(int mode)
{
    Preferences::get()->setInt("/tools/mesh/mesh_geometry", mode);
}

void MeshToolbar::new_fillstroke_changed(int mode)
{
    Preferences::get()->setInt("/tools/mesh/newfillorstroke", mode);
}

void MeshToolbar::row_changed()
{
    if (_blocker.pending()) {
        return;
    }

    auto guard = _blocker.block();

    int rows = _row_item.get_adjustment()->get_value();

    Preferences::get()->setInt("/tools/mesh/mesh_rows", rows);
}

void MeshToolbar::col_changed()
{
    if (_blocker.pending()) {
        return;
    }

    auto guard = _blocker.block();

    int cols = _col_item.get_adjustment()->get_value();

    Preferences::get()->setInt("/tools/mesh/mesh_cols", cols);
}

void MeshToolbar::toggle_fill_stroke()
{
    auto prefs = Preferences::get();
    prefs->setBool("/tools/mesh/edit_fill", _edit_fill_btn->get_active());
    prefs->setBool("/tools/mesh/edit_stroke", _edit_stroke_btn->get_active());

    if (auto mt = get_mesh_tool()) {
        auto drag = mt->get_drag();
        drag->updateDraggers();
        drag->updateLines();
        drag->updateLevels();
        selection_changed(); // Need to update Type widget
    }
}

void MeshToolbar::toggle_handles()
{
    auto prefs = Preferences::get();
    prefs->setBool("/tools/mesh/show_handles", _show_handles_btn->get_active());

    if (auto mt = get_mesh_tool()) {
        mt->get_drag()->refreshDraggers();
    }
}

/*
 * Core function, setup all the widgets whenever something changes on the desktop
 */
void MeshToolbar::selection_changed()
{
    if (_blocker.pending()) {
        return;
    }

    if (!_desktop) {
        return;
    }

    auto selection = _desktop->getSelection();
    if (selection) {
        SPMeshGradient *ms_selected = nullptr;
        SPMeshType ms_type = SP_MESH_TYPE_COONS;
        bool ms_selected_multi = false;
        bool ms_type_multi = false;
        ms_read_selection(selection, ms_selected, ms_selected_multi, ms_type, ms_type_multi);

        _select_type_item.set_sensitive(!ms_type_multi);
        auto guard = _blocker.block();
        _select_type_item.set_selected(ms_type);
    }
}

void MeshToolbar::warning_popup()
{
    char *msg = _("Mesh gradients are part of SVG 2:\n"
                  "* Syntax may change.\n"
                  "* Web browser implementation is not guaranteed.\n"
                  "\n"
                  "For web: convert to bitmap (Edit->Make bitmap copy).\n"
                  "For print: export to PDF.");
    auto dialog = std::make_unique<Gtk::MessageDialog>(msg, false, Gtk::MessageType::WARNING, Gtk::ButtonsType::OK, true);
    dialog_show_modal_and_selfdestruct(std::move(dialog), get_root());
}

/**
 * Sets mesh type: Coons, Bicubic
 */
void MeshToolbar::type_changed(int mode)
{
    if (_blocker.pending()) {
        return;
    }

    auto selection = _desktop->getSelection();
    auto meshes = ms_get_dt_selected_gradients(selection);

    auto type = static_cast<SPMeshType>(mode);
    for (auto &mesh : meshes) {
        mesh->type = type;
        mesh->type_set = true;
        mesh->updateRepr();
    }

    if (!meshes.empty()) {
        DocumentUndo::done(_desktop->getDocument(), RC_("Undo", "Set mesh type"), INKSCAPE_ICON("mesh-gradient"));
    }
}

void MeshToolbar::toggle_sides()
{
    if (auto mt = get_mesh_tool()) {
        mt->corner_operation(MG_CORNER_SIDE_TOGGLE);
    }
}

void MeshToolbar::make_elliptical()
{
    if (auto mt = get_mesh_tool()) {
        mt->corner_operation(MG_CORNER_SIDE_ARC);
    }
}

void MeshToolbar::pick_colors()
{
    if (auto mt = get_mesh_tool()) {
        mt->corner_operation(MG_CORNER_COLOR_PICK);
    }
}

void MeshToolbar::fit_mesh()
{
    if (auto mt = get_mesh_tool()) {
        mt->fit_mesh_in_bbox();
    }
}

Tools::MeshTool *MeshToolbar::get_mesh_tool() const
{
    if (!_desktop) {
        return nullptr;
    }
    return dynamic_cast<Tools::MeshTool *>(_desktop->getTool());
}

} // namespace Inkscape::UI::Toolbar

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

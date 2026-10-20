// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file Connector toolbar
 */
/* Authors:
 *   MenTaLguY <mental@rydia.net>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Frank Felfe <innerspace@iname.com>
 *   John Cliff <simarilius@yahoo.com>
 *   David Turner <novalis@gnu.org>
 *   Josh Andler <scislac@scislac.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *   Tavmjong Bah <tavmjong@free.fr>
 *   Abhishek Sharma
 *   Kris De Gussem <Kris.DeGussem@gmail.com>
 *   Vaibhav Malik <vaibhavmalik2018@gmail.com>
 *
 * Copyright (C) 2004 David Turner
 * Copyright (C) 2003 MenTaLguY
 * Copyright (C) 1999-2011 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "connector-toolbar.h"

#include <glibmm/i18n.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/togglebutton.h>

#include "conn-avoid-ref.h"
#include "desktop.h"
#include "document-undo.h"
#include "enums.h"
#include "layer-manager.h"
#include "object/algorithms/graphlayout.h"
#include "object/sp-namedview.h"
#include "object/sp-path.h"
#include "selection.h"
#include "ui/builder-utils.h"
#include "ui/icon-names.h"
#include "ui/tools/connector-tool.h"

using Inkscape::DocumentUndo;

namespace Inkscape::UI::Toolbar {

ConnectorToolbar::ConnectorToolbar()
    : ConnectorToolbar{create_builder("toolbar-connector.ui")}
{}

ConnectorToolbar::ConnectorToolbar(Glib::RefPtr<Gtk::Builder> const &builder)
    : Toolbar{get_widget<Gtk::Box>(builder, "connector-toolbar")}
    , _orthogonal_btn{get_widget<Gtk::ToggleButton>             (builder, "_orthogonal_btn")}
    , _curvature_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_curvature_item")}
    , _spacing_item  {get_derived_widget<UI::Widget::SpinButton>(builder, "_spacing_item")}
    , _length_item   {get_derived_widget<UI::Widget::SpinButton>(builder, "_length_item")}
    , _directed_btn  {get_widget<Gtk::ToggleButton>             (builder, "_directed_btn")}
    , _overlap_btn   {get_widget<Gtk::ToggleButton>             (builder, "_overlap_btn")}
{
    auto prefs = Preferences::get();

    setup_derived_spin_button(_curvature_item, "curvature", defaultConnCurvature, &ConnectorToolbar::curvature_changed);
    setup_derived_spin_button(_spacing_item, "spacing", defaultConnSpacing, &ConnectorToolbar::spacing_changed);
    setup_derived_spin_button(_length_item, "length", 100, &ConnectorToolbar::length_changed);

    // Values auto-calculated.
    _curvature_item.set_custom_numeric_menu_data({});
    _spacing_item.set_custom_numeric_menu_data({});
    _length_item.set_custom_numeric_menu_data({});

    // Orthogonal connectors toggle button
    _orthogonal_btn.set_active(prefs->getBool("/tools/connector/orthogonal"));

    // Directed edges toggle button
    _directed_btn.set_active(prefs->getBool("/tools/connector/directedlayout"));

    // Avoid overlaps toggle button
    _overlap_btn.set_active(prefs->getBool("/tools/connector/avoidoverlaplayout"));

    // Signals.
    get_widget<Gtk::Button>(builder, "avoid_btn")
        .signal_clicked()
        .connect(sigc::mem_fun(*this, &ConnectorToolbar::path_set_avoid));
    get_widget<Gtk::Button>(builder, "ignore_btn")
        .signal_clicked()
        .connect(sigc::mem_fun(*this, &ConnectorToolbar::path_set_ignore));
    get_widget<Gtk::Button>(builder, "graph_btn")
        .signal_clicked()
        .connect(sigc::mem_fun(*this, &ConnectorToolbar::graph_layout));

    _orthogonal_btn.signal_toggled().connect(sigc::mem_fun(*this, &ConnectorToolbar::orthogonal_toggled));
    _directed_btn.signal_toggled().connect(sigc::mem_fun(*this, &ConnectorToolbar::directed_graph_layout_toggled));
    _overlap_btn.signal_toggled().connect(sigc::mem_fun(*this, &ConnectorToolbar::nooverlaps_graph_layout_toggled));

    _initMenuBtns();
}

void ConnectorToolbar::setDesktop(SPDesktop *desktop)
{
    if (_desktop) {
        _repr->removeObserver(*this);
        GC::release(_repr);
        _repr = nullptr;

        _selection_changed_conn.disconnect();
    }

    Toolbar::setDesktop(desktop);

    if (_desktop) {
        _selection_changed_conn = _desktop->getSelection()->connectChanged(sigc::mem_fun(*this, &ConnectorToolbar::_selectionChanged));

        // Watch for changes to the connector-spacing attribute in the XML.
        _repr = desktop->getNamedView()->getRepr();
        GC::anchor(_repr);
        _repr->addObserver(*this);
        // _repr->synthesizeEvents(*this); // BAD
    }
}

void ConnectorToolbar::setup_derived_spin_button(UI::Widget::SpinButton &btn, Glib::ustring const &name,
                                                 double default_value, ValueChangedMemFun value_changed_mem_fun)
{
    auto const adj = btn.get_adjustment();
    auto const path = "/tools/connector/" + name;
    auto const val = Preferences::get()->getDouble(path, default_value);
    adj->set_value(val);
    adj->signal_value_changed().connect(sigc::mem_fun(*this, value_changed_mem_fun));
    btn.setDefocusTarget(this);
}

void ConnectorToolbar::path_set_avoid()
{
    UI::Tools::cc_selection_set_avoid(_desktop, true);
}

void ConnectorToolbar::path_set_ignore()
{
    UI::Tools::cc_selection_set_avoid(_desktop, false);
}

void ConnectorToolbar::orthogonal_toggled()
{
    auto doc = _desktop->getDocument();

    if (!DocumentUndo::getUndoSensitive(doc)) {
        return;
    }

    // quit if run by the _changed callbacks
    if (_blocker.pending()) {
        return;
    }

    // in turn, prevent callbacks from responding
    auto guard = _blocker.block();

    bool is_orthog = _orthogonal_btn.get_active();
    auto value = is_orthog ? "orthogonal" : "polyline";

    bool modified = false;
    for (auto item : _desktop->getSelection()->items()) {
        if (UI::Tools::cc_item_is_connector(item)) {
            item->setAttribute( "inkscape:connector-type", value);
            item->getAvoidRef().handleSettingChange();
            modified = true;
        }
    }

    if (!modified) {
        Preferences::get()->setBool("/tools/connector/orthogonal", is_orthog);
    } else {
        DocumentUndo::done(doc, is_orthog ? RC_("Undo", "Set connector type: orthogonal"): RC_("Undo", "Set connector type: polyline"), INKSCAPE_ICON("draw-connector"));
    }
}

void ConnectorToolbar::curvature_changed()
{
    auto doc = _desktop->getDocument();

    if (!DocumentUndo::getUndoSensitive(doc)) {
        return;
    }

    // quit if run by the _changed callbacks
    if (_blocker.pending()) {
        return;
    }

    // in turn, prevent callbacks from responding
    auto guard = _blocker.block();

    auto newValue = _curvature_item.get_adjustment()->get_value();
    char value[G_ASCII_DTOSTR_BUF_SIZE];
    g_ascii_dtostr(value, G_ASCII_DTOSTR_BUF_SIZE, newValue);

    bool modified = false;
    for (auto item : _desktop->getSelection()->items()) {
        if (UI::Tools::cc_item_is_connector(item)) {
            item->setAttribute( "inkscape:connector-curvature", value);
            item->getAvoidRef().handleSettingChange();
            modified = true;
        }
    }

    if (!modified) {
        Preferences::get()->setDouble(Glib::ustring("/tools/connector/curvature"), newValue);
    } else {
        DocumentUndo::done(doc, RC_("Undo", "Change connector curvature"), INKSCAPE_ICON("draw-connector"));
    }
}

void ConnectorToolbar::spacing_changed()
{
    auto doc = _desktop->getDocument();

    if (!DocumentUndo::getUndoSensitive(doc)) {
        return;
    }

    auto repr = _desktop->getNamedView()->getRepr();

    if (!repr->attribute("inkscape:connector-spacing") &&
        _spacing_item.get_adjustment()->get_value() == defaultConnSpacing) {
        // Don't need to update the repr if the attribute doesn't
        // exist and it is being set to the default value -- as will
        // happen at startup.
        return;
    }

    // quit if run by the attr_changed listener
    if (_blocker.pending()) {
        return;
    }

    // in turn, prevent listener from responding
    auto guard = _blocker.block();

    repr->setAttributeCssDouble("inkscape:connector-spacing", _spacing_item.get_adjustment()->get_value());
    _desktop->getNamedView()->updateRepr();

    bool modified = false;
    for (auto item : get_avoided_items(_desktop->layerManager().currentRoot(), _desktop)) {
        auto m = Geom::identity();
        avoid_item_move(&m, item);
        modified = true;
    }

    if (modified) {
        DocumentUndo::done(doc, RC_("Undo", "Change connector spacing"), INKSCAPE_ICON("draw-connector"));
    }
}

void ConnectorToolbar::graph_layout()
{
    auto prefs = Preferences::get();

    // hack for clones, see comment in align-and-distribute.cpp
    int saved_compensation = prefs->getInt("/options/clonecompensation/value", SP_CLONE_COMPENSATION_UNMOVED);
    prefs->setInt("/options/clonecompensation/value", SP_CLONE_COMPENSATION_UNMOVED);

    auto tmp = _desktop->getSelection()->items();
    auto vec = std::vector<SPItem *>{tmp.begin(), tmp.end()};
    graphlayout(vec);

    prefs->setInt("/options/clonecompensation/value", saved_compensation);

    DocumentUndo::done(_desktop->getDocument(), RC_("Undo", "Arrange connector network"), INKSCAPE_ICON("dialog-align-and-distribute"));
}

void ConnectorToolbar::length_changed()
{
    Preferences::get()->setDouble("/tools/connector/length", _length_item.get_adjustment()->get_value());
}

void ConnectorToolbar::directed_graph_layout_toggled()
{
    Preferences::get()->setBool("/tools/connector/directedlayout", _directed_btn.get_active());
}

void ConnectorToolbar::_selectionChanged(Selection *selection)
{
    if (auto path = cast<SPPath>(selection->singleItem())) {
        _orthogonal_btn.set_active(path->connEndPair.isOrthogonal());
        _curvature_item.get_adjustment()->set_value(path->connEndPair.getCurvature());
    }
}

void ConnectorToolbar::nooverlaps_graph_layout_toggled()
{
    Preferences::get()->setBool("/tools/connector/avoidoverlaplayout", _overlap_btn.get_active());
}

void ConnectorToolbar::notifyAttributeChanged(XML::Node &repr, GQuark name, Util::ptr_shared, Util::ptr_shared)
{
    if (_blocker.pending()) {
        return;
    }

    static auto const connector_spacing_quark = g_quark_from_static_string("inkscape:connector-spacing");
    if (name == connector_spacing_quark) {
        _spacing_item.get_adjustment()->set_value(_desktop->getNamedView()->connector_spacing);
    }
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

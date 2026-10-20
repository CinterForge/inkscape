// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file LPE toolbar
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

#include "lpe-toolbar.h"

#include <gtkmm/liststore.h>
#include "live_effects/lpe-line_segment.h"
#include "selection.h"
#include "ui/builder-utils.h"
#include "ui/dialog/dialog-container.h"
#include "ui/tools/lpe-tool.h"
#include "ui/util.h"
#include "ui/widget/unit-tracker.h"

using Inkscape::UI::Widget::UnitTracker;
using Inkscape::Util::Unit;
using Inkscape::Util::Quantity;
using Inkscape::DocumentUndo;
using Inkscape::UI::Tools::ToolBase;
using Inkscape::UI::Tools::LpeTool;

namespace Inkscape::UI::Toolbar {

LPEToolbar::LPEToolbar()
    : LPEToolbar{create_builder("toolbar-lpe.ui")}
{}

LPEToolbar::LPEToolbar(Glib::RefPtr<Gtk::Builder> const &builder)
    : Toolbar{get_widget<Gtk::Box>(builder, "lpe-toolbar")}
    , _show_bbox_btn(get_widget<Gtk::ToggleButton>(builder, "_show_bbox_btn"))
    , _bbox_from_selection_btn(get_widget<Gtk::ToggleButton>(builder, "_bbox_from_selection_btn"))
    , _measuring_btn(get_widget<Gtk::ToggleButton>(builder, "_measuring_btn"))
    , _open_lpe_dialog_btn(get_widget<Gtk::ToggleButton>(builder, "_open_lpe_dialog_btn"))
    , _tracker{std::make_unique<UnitTracker>(Util::UNIT_TYPE_LINEAR)}
    , _line_segment_combo(get_derived_widget<UI::Widget::DropDownList>(builder, "line-type"))
{
    auto prefs = Preferences::get();

    // Combo box to choose line segment type
    for (auto item : {_("Closed"), _("Open start"), _("Open end"), _("Open both")}) {
        _line_segment_combo.append(item);
    }
    _line_segment_combo.set_selected(0);
    _line_segment_combo.signal_changed().connect([this] { change_line_segment_type(_line_segment_combo.get_selected()); });

    // Configure mode buttons
    int btn_index = 0;
    for (auto &item : children(get_widget<Gtk::Box>(builder, "mode_buttons_box"))) {
        auto &btn = dynamic_cast<Gtk::ToggleButton &>(item);
        _mode_buttons.push_back(&btn);
        btn.signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &LPEToolbar::mode_changed), btn_index++));
    }

    int mode = prefs->getInt("/tools/lpetool/mode", 0);
    _mode_buttons[mode]->set_active();

    // Add the units menu
    _units_item = _tracker->create_unit_dropdown();
    _units_item->signal_changed().connect(sigc::mem_fun(*this, &LPEToolbar::unit_changed));
    _units_item->set_sensitive(prefs->getBool("/tools/lpetool/show_measuring_info", true));
    get_widget<Gtk::Box>(builder, "units_box").append(*_units_item);

    // Set initial states
    _show_bbox_btn.set_active(prefs->getBool("/tools/lpetool/show_bbox", true));
    _bbox_from_selection_btn.set_active(false);
    _measuring_btn.set_active(prefs->getBool("/tools/lpetool/show_measuring_info", true));
    _open_lpe_dialog_btn.set_active(false);

    // Signals.
    _show_bbox_btn.signal_toggled().connect(sigc::mem_fun(*this, &LPEToolbar::toggle_show_bbox));
    _bbox_from_selection_btn.signal_toggled().connect(sigc::mem_fun(*this, &LPEToolbar::toggle_set_bbox));
    _measuring_btn.signal_toggled().connect(sigc::mem_fun(*this, &LPEToolbar::toggle_show_measuring_info));
    _open_lpe_dialog_btn.signal_toggled().connect(sigc::mem_fun(*this, &LPEToolbar::open_lpe_dialog));

    _initMenuBtns();
}

LPEToolbar::~LPEToolbar() = default;

void LPEToolbar::setDesktop(SPDesktop *desktop)
{
    if (_desktop) {
        c_selection_modified.disconnect();
        c_selection_changed.disconnect();
    }

    Toolbar::setDesktop(desktop);

    if (_desktop) {
        // Watch selection
        c_selection_modified = desktop->getSelection()->connectModified(sigc::mem_fun(*this, &LPEToolbar::sel_modified));
        c_selection_changed = desktop->getSelection()->connectChanged(sigc::mem_fun(*this, &LPEToolbar::sel_changed));
        sel_changed(desktop->getSelection());
    }
}

void LPEToolbar::setActiveUnit(Util::Unit const *unit)
{
    _tracker->setActiveUnit(unit);
}

void LPEToolbar::setMode(int mode)
{
    _mode_buttons[mode]->set_active();
    mode_changed(mode); // set_active() does not trigger callback, call it here.
}

// this is called when the mode is changed via the toolbar (i.e., one of the subtool buttons is pressed)
void LPEToolbar::mode_changed(int mode)
{
    using namespace LivePathEffect;

    auto const tool = _desktop->getTool();
    if (!SP_LPETOOL_CONTEXT(tool)) {
        return;
    }

    // only take action if run by the attr_changed listener
    if (_blocker.pending()) {
        return;
    }

    // in turn, prevent listener from responding
    auto guard = _blocker.block();

    EffectType type = lpesubtools[mode].type;

    auto const lc = SP_LPETOOL_CONTEXT(_desktop->getTool());
    bool success = UI::Tools::lpetool_try_construction(lc->getDesktop(), type);
    if (success) {
        // since the construction was already performed, we set the state back to inactive
        _mode_buttons[0]->set_active();
        mode = 0;
    } else {
        // switch to the chosen subtool
        SP_LPETOOL_CONTEXT(_desktop->getTool())->mode = type;
    }

    if (DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
        Preferences::get()->setInt("/tools/lpetool/mode", mode);
    }
}

void LPEToolbar::toggle_show_bbox()
{
    Preferences::get()->setBool("/tools/lpetool/show_bbox", _show_bbox_btn.get_active());

    if (auto const lc = dynamic_cast<LpeTool *>(_desktop->getTool())) {
        lc->reset_limiting_bbox();
    }
}

void LPEToolbar::toggle_set_bbox()
{
    auto selection = _desktop->getSelection();

    auto bbox = selection->visualBounds();

    if (bbox) {
        Geom::Point A(bbox->min());
        Geom::Point B(bbox->max());

        A *= _desktop->doc2dt();
        B *= _desktop->doc2dt();

        // TODO: should we provide a way to store points in prefs?
        auto prefs = Preferences::get();
        prefs->setDouble("/tools/lpetool/bbox_upperleftx", A[Geom::X]);
        prefs->setDouble("/tools/lpetool/bbox_upperlefty", A[Geom::Y]);
        prefs->setDouble("/tools/lpetool/bbox_lowerrightx", B[Geom::X]);
        prefs->setDouble("/tools/lpetool/bbox_lowerrighty", B[Geom::Y]);

        SP_LPETOOL_CONTEXT(_desktop->getTool())->reset_limiting_bbox();
    }

    _bbox_from_selection_btn.set_active(false);
}

void LPEToolbar::change_line_segment_type(int mode)
{
    using namespace LivePathEffect;

    // quit if run by the attr_changed listener
    if (_blocker.pending()) {
        return;
    }

    // in turn, prevent listener from responding
    auto guard = _blocker.block();

    auto line_seg = dynamic_cast<LPELineSegment *>(_currentlpe);

    if (_currentlpeitem && line_seg) {
        line_seg->end_type.param_set_value(static_cast<LivePathEffect::EndType>(mode));
        sp_lpe_item_update_patheffect(_currentlpeitem, true, true);
    }
}

void LPEToolbar::toggle_show_measuring_info()
{
    auto const lc = dynamic_cast<LpeTool *>(_desktop->getTool());
    if (!lc) {
        return;
    }

    bool show = _measuring_btn.get_active();
    Preferences::get()->setBool("/tools/lpetool/show_measuring_info",  show);
    lc->show_measuring_info(show);
    _units_item->set_sensitive(show);
}

void LPEToolbar::unit_changed()
{
    if (!_desktop) {
        return;
    }

    Preferences::get()->setString("/tools/lpetool/unit", _tracker->getActiveUnit()->abbr);

    if (auto const lc = SP_LPETOOL_CONTEXT(_desktop->getTool())) {
        lc->delete_measuring_items();
        lc->create_measuring_items();
    }
}

void LPEToolbar::open_lpe_dialog()
{
    if (dynamic_cast<LpeTool *>(_desktop->getTool())) {
        _desktop->getContainer()->new_dialog("LivePathEffect");
    } else {
        std::cerr << "LPEToolbar::open_lpe_dialog: LPEToolbar active but current tool is not LPE tool!" << std::endl;
    }
    _open_lpe_dialog_btn.set_active(false);
}

void LPEToolbar::sel_modified(Selection *selection, guint /*flags*/)
{
    auto const tool = selection->desktop()->getTool();
    if (auto const lc = SP_LPETOOL_CONTEXT(tool)) {
        lc->update_measuring_items();
    }
}

void LPEToolbar::sel_changed(Selection *selection)
{
    using namespace LivePathEffect;
    auto const tool = selection->desktop()->getTool();
    auto const lc = SP_LPETOOL_CONTEXT(tool);
    if (!lc) {
        return;
    }

    lc->delete_measuring_items();
    lc->create_measuring_items(selection);

    // activate line segment combo box if a single item with LPELineSegment is selected
    auto lpeitem = cast<SPLPEItem>(selection->singleItem());
    if (lpeitem && UI::Tools::lpetool_item_has_construction(lpeitem)) {

        auto lpe = lpeitem->getCurrentLPE();
        if (lpe && lpe->effectType() == LINE_SEGMENT) {
            auto lpels = static_cast<LPELineSegment *>(lpe);
            _currentlpe = lpe;
            _currentlpeitem = lpeitem;
            _line_segment_combo.set_sensitive();
            _line_segment_combo.set_selected(lpels->end_type.get_value());
        } else {
            _currentlpe = nullptr;
            _currentlpeitem = nullptr;
            _line_segment_combo.set_sensitive(false);
        }

    } else {
        _currentlpe = nullptr;
        _currentlpeitem = nullptr;
        _line_segment_combo.set_sensitive(false);
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :

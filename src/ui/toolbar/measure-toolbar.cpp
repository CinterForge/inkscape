// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file Measure toolbar
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

#include "measure-toolbar.h"

#include <gtkmm/togglebutton.h>

#include "desktop.h"
#include "document-undo.h"
#include "object/sp-namedview.h"
#include "ui/builder-utils.h"
#include "ui/tools/measure-tool.h"
#include "ui/widget/generic/spin-button.h"
#include "ui/widget/spinbutton.h"
#include "ui/widget/unit-tracker.h"

using Inkscape::UI::Widget::UnitTracker;
using Inkscape::Util::Unit;
using Inkscape::DocumentUndo;
using Inkscape::UI::Tools::MeasureTool;

static MeasureTool *get_measure_tool(SPDesktop *desktop)
{
    if (desktop) {
        return dynamic_cast<MeasureTool *>(desktop->getTool());
    }
    return nullptr;
}

namespace Inkscape::UI::Toolbar {

MeasureToolbar::MeasureToolbar()
    : MeasureToolbar{create_builder("toolbar-measure.ui")}
{}

MeasureToolbar::MeasureToolbar(Glib::RefPtr<Gtk::Builder> const &builder)
    : Toolbar{get_widget<Gtk::Box>(builder, "measure-toolbar")}
    , _tracker{std::make_unique<UnitTracker>(Util::UNIT_TYPE_LINEAR)}
    , _font_size_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_font_size_item")}
    , _precision_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_precision_item")}
    , _scale_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_scale_item")}
    , _only_selected_btn{get_widget<Gtk::ToggleButton>(builder, "_only_selected_btn")}
    , _ignore_1st_and_last_btn{get_widget<Gtk::ToggleButton>(builder, "_ignore_1st_and_last_btn")}
    , _inbetween_btn{get_widget<Gtk::ToggleButton>(builder, "_inbetween_btn")}
    , _show_hidden_btn{get_widget<Gtk::ToggleButton>(builder, "_show_hidden_btn")}
    , _all_layers_btn{get_widget<Gtk::ToggleButton>(builder, "_all_layers_btn")}
    , _offset_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_offset_item")}
{
    auto prefs = Preferences::get();

    auto unit_menu = _tracker->create_unit_dropdown();
    unit_menu->signal_changed().connect(sigc::mem_fun(*this, &MeasureToolbar::unit_changed));
    get_widget<Gtk::Box>(builder, "unit_menu_box").append(*unit_menu);

    setup_derived_spin_button(_font_size_item, "fontsize", 10.0, &MeasureToolbar::fontsize_value_changed);
    setup_derived_spin_button(_precision_item, "precision", 2, &MeasureToolbar::precision_value_changed);
    setup_derived_spin_button(_scale_item, "scale", 100.0, &MeasureToolbar::scale_value_changed);
    setup_derived_spin_button(_offset_item, "offset", 5.0, &MeasureToolbar::offset_value_changed);

    // Values auto-calculated.
    _font_size_item.set_custom_numeric_menu_data({});
    _precision_item.set_custom_numeric_menu_data({});
    _scale_item.set_custom_numeric_menu_data({});
    _offset_item.set_custom_numeric_menu_data({});

    // Signals.
    _only_selected_btn.set_active(prefs->getBool("/tools/measure/only_selected", false));
    _only_selected_btn.signal_toggled().connect(sigc::mem_fun(*this, &MeasureToolbar::toggle_only_selected));

    _ignore_1st_and_last_btn.set_active(prefs->getBool("/tools/measure/ignore_1st_and_last", true));
    _ignore_1st_and_last_btn.signal_toggled().connect(
        sigc::mem_fun(*this, &MeasureToolbar::toggle_ignore_1st_and_last));

    _inbetween_btn.set_active(prefs->getBool("/tools/measure/show_in_between", true));
    _inbetween_btn.signal_toggled().connect(sigc::mem_fun(*this, &MeasureToolbar::toggle_show_in_between));

    _show_hidden_btn.set_active(prefs->getBool("/tools/measure/show_hidden", true));
    _show_hidden_btn.signal_toggled().connect(sigc::mem_fun(*this, &MeasureToolbar::toggle_show_hidden));

    _all_layers_btn.set_active(prefs->getBool("/tools/measure/all_layers", true));
    _all_layers_btn.signal_toggled().connect(sigc::mem_fun(*this, &MeasureToolbar::toggle_all_layers));

    auto& settings = get_widget<Gtk::Popover>(builder, "settings-popover");
    settings.set_child(_settings);

    get_widget<Gtk::Button>(builder, "reverse_btn")
        .signal_clicked()
        .connect(sigc::mem_fun(*this, &MeasureToolbar::reverse_knots));

    get_widget<Gtk::Button>(builder, "to_phantom_btn")
        .signal_clicked()
        .connect(sigc::mem_fun(*this, &MeasureToolbar::to_phantom));

    get_widget<Gtk::Button>(builder, "to_guides_btn")
        .signal_clicked()
        .connect(sigc::mem_fun(*this, &MeasureToolbar::to_guides));

    get_widget<Gtk::Button>(builder, "to_item_btn")
        .signal_clicked()
        .connect(sigc::mem_fun(*this, &MeasureToolbar::to_item));

    get_widget<Gtk::Button>(builder, "mark_dimension_btn")
        .signal_clicked()
        .connect(sigc::mem_fun(*this, &MeasureToolbar::to_mark_dimension));

    _initMenuBtns();
}

MeasureToolbar::~MeasureToolbar() = default;

void MeasureToolbar::setDesktop(SPDesktop *desktop)
{
    Toolbar::setDesktop(desktop);

    if (_desktop) {
        if (!_unit_set) {
            auto default_unit = desktop->getNamedView()->getDisplayUnit();
            _tracker->setActiveUnitByAbbr(Preferences::get()->getString("/tools/measure/unit", default_unit->abbr).c_str());
            _unit_set = true;
        }
    }
    _settings.setDesktop(desktop);
}

void MeasureToolbar::setup_derived_spin_button(UI::Widget::SpinButton &btn, Glib::ustring const &name,
                                               double default_value, ValueChangedMemFun const value_changed_mem_fun)
{
    auto adj = btn.get_adjustment();
    auto const path = "/tools/measure/" + name;
    auto const val = Preferences::get()->getDouble(path, default_value);
    adj->set_value(val);
    adj->signal_value_changed().connect(sigc::mem_fun(*this, value_changed_mem_fun));
    btn.setDefocusTarget(this);
}

void MeasureToolbar::fontsize_value_changed()
{
    if (!DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
        return;
    }
    Preferences::get()->setDouble("/tools/measure/fontsize", _font_size_item.get_adjustment()->get_value());
    if (auto mt = get_measure_tool(_desktop)) {
        mt->showCanvasItems();
    }
}

void MeasureToolbar::unit_changed()
{
    Glib::ustring const unit = _tracker->getActiveUnit()->abbr;
    Preferences::get()->setString("/tools/measure/unit", unit);
    if (auto mt = get_measure_tool(_desktop)) {
        mt->showCanvasItems();
    }
}

void MeasureToolbar::precision_value_changed()
{
    if (!DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
        return;
    }
    Preferences::get()->setInt("/tools/measure/precision", _precision_item.get_adjustment()->get_value());
    if (auto mt = get_measure_tool(_desktop)) {
        mt->showCanvasItems();
    }
}

void MeasureToolbar::scale_value_changed()
{
    if (!DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
        return;
    }
    Preferences::get()->setDouble("/tools/measure/scale", _scale_item.get_adjustment()->get_value());
    if (auto mt = get_measure_tool(_desktop)) {
        mt->showCanvasItems();
    }
}

void MeasureToolbar::offset_value_changed()
{
    if (!DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
        return;
    }
    Preferences::get()->setDouble("/tools/measure/offset", _offset_item.get_adjustment()->get_value());
    if (auto mt = get_measure_tool(_desktop)) {
        mt->showCanvasItems();
    }
}

void MeasureToolbar::toggle_only_selected()
{
    bool active = _only_selected_btn.get_active();
    Preferences::get()->setBool("/tools/measure/only_selected", active);
    _desktop->messageStack()->flash(INFORMATION_MESSAGE, active ? _("Measures only selected.") : _("Measure all."));
    if (auto mt = get_measure_tool(_desktop)) {
        mt->showCanvasItems();
    }
}

void MeasureToolbar::toggle_ignore_1st_and_last()
{
    bool active = _ignore_1st_and_last_btn.get_active();
    Preferences::get()->setBool("/tools/measure/ignore_1st_and_last", active);
    _desktop->messageStack()->flash(INFORMATION_MESSAGE, active ? _("Start and end measures inactive.") : _("Start and end measures active."));
    if (auto mt = get_measure_tool(_desktop)) {
        mt->showCanvasItems();
    }
}

void MeasureToolbar::toggle_show_in_between()
{
    bool active = _inbetween_btn.get_active();
    Preferences::get()->setBool("/tools/measure/show_in_between", active);
    _desktop->messageStack()->flash(INFORMATION_MESSAGE, active ? _("Compute all elements.") : _("Compute max length."));
    if (auto mt = get_measure_tool(_desktop)) {
        mt->showCanvasItems();
    }
}

void MeasureToolbar::toggle_show_hidden()
{
    bool active = _show_hidden_btn.get_active();
    Preferences::get()->setBool("/tools/measure/show_hidden", active);
    _desktop->messageStack()->flash(INFORMATION_MESSAGE, active ? _("Show all crossings.") : _("Show visible crossings."));
    if (auto mt = get_measure_tool(_desktop)) {
        mt->showCanvasItems();
    }
}

void MeasureToolbar::toggle_all_layers()
{
    bool active = _all_layers_btn.get_active();
    Preferences::get()->setBool("/tools/measure/all_layers", active);
    _desktop->messageStack()->flash(INFORMATION_MESSAGE, active ? _("Use all layers in the measure.") : _("Use current layer in the measure."));
    if (auto mt = get_measure_tool(_desktop)) {
        mt->showCanvasItems();
    }
}

void MeasureToolbar::reverse_knots()
{
    if (auto mt = get_measure_tool(_desktop)) {
        mt->reverseKnots();
    }
}

void MeasureToolbar::to_phantom()
{
    if (auto mt = get_measure_tool(_desktop)) {
        mt->toPhantom();
    }
}

void MeasureToolbar::to_guides()
{
    if (auto mt = get_measure_tool(_desktop)) {
        mt->toGuides();
    }
}

void MeasureToolbar::to_item()
{
    if (auto mt = get_measure_tool(_desktop)) {
        mt->toItem();
    }
}

void MeasureToolbar::to_mark_dimension()
{
    if (auto mt = get_measure_tool(_desktop)) {
        mt->toMarkDimension();
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

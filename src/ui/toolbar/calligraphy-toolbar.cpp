// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file Calligraphy toolbar
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

#include "calligraphy-toolbar.h"

#include <glibmm/i18n.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/togglebutton.h>

#include "ui/builder-utils.h"
#include "ui/dialog/calligraphic-profile-rename.h"
#include "ui/simple-pref-pusher.h"
#include "ui/widget/combo-tool-item.h"
#include "ui/widget/spinbutton.h"
#include "ui/widget/unit-tracker.h"

using Inkscape::UI::Widget::UnitTracker;
using Inkscape::Util::Quantity;
using Inkscape::Util::Unit;
using Inkscape::Util::UnitTable;

namespace Inkscape::UI::Toolbar {

static std::vector<Glib::ustring> get_presets_list()
{
    return Preferences::get()->getAllDirs("/tools/calligraphic/preset");
}

CalligraphyToolbar::CalligraphyToolbar()
    : CalligraphyToolbar{create_builder("toolbar-calligraphy.ui")}
{}

CalligraphyToolbar::CalligraphyToolbar(Glib::RefPtr<Gtk::Builder> const &builder)
    : Toolbar{get_widget<Gtk::Box>(builder, "calligraphy-toolbar")}
    , _tracker{std::make_unique<UnitTracker>(Util::UNIT_TYPE_LINEAR)}
    , _profile_selector_combo{get_derived_widget<UI::Widget::DropDownList>(builder, "_profile_selector_combo")}
    , _width_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_width_item")}
    , _thinning_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_thinning_item")}
    , _mass_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_mass_item")}
    , _angle_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_angle_item")}
    , _usetilt_btn{&get_widget<Gtk::ToggleButton>(builder, "_usetilt_btn")}
    , _flatness_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_flatness_item")}
    , _cap_rounding_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_cap_rounding_item")}
    , _tremor_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_tremor_item")}
    , _wiggle_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_wiggle_item")}
{
    auto prefs = Preferences::get();

    auto percent = Unit::create("%");
    _tracker->prependUnit(percent.get());
    if (prefs->getBool("/tools/calligraphic/abs_width")) {
        _tracker->setActiveUnitByLabel(prefs->getString("/tools/calligraphic/unit"));
    }

    auto &usepressure_btn = get_widget<Gtk::ToggleButton>(builder, "usepressure_btn");
    auto &tracebackground_btn = get_widget<Gtk::ToggleButton>(builder, "tracebackground_btn");

    // Setup the spin buttons.
    setup_derived_spin_button(_width_item, "width", 15.118, &CalligraphyToolbar::width_value_changed);
    setup_derived_spin_button(_thinning_item, "thinning", 10, &CalligraphyToolbar::velthin_value_changed);
    setup_derived_spin_button(_mass_item, "mass", 2.0, &CalligraphyToolbar::mass_value_changed);
    setup_derived_spin_button(_angle_item, "angle", 30, &CalligraphyToolbar::angle_value_changed);
    setup_derived_spin_button(_flatness_item, "flatness", -90, &CalligraphyToolbar::flatness_value_changed);
    setup_derived_spin_button(_cap_rounding_item, "cap_rounding", 0.0, &CalligraphyToolbar::cap_rounding_value_changed);
    setup_derived_spin_button(_tremor_item, "tremor", 0.0, &CalligraphyToolbar::tremor_value_changed);
    setup_derived_spin_button(_wiggle_item, "wiggle", 0.0, &CalligraphyToolbar::wiggle_value_changed);

    _width_item.set_custom_numeric_menu_data({
        { 1, _("(hairline)")},
        { 3, ""},
        { 5, ""},
        {10, ""},
        {15, _("(default)")},
        {20, ""},
        {30, ""},
        {50, ""},
        {75, ""},
        {100, _("(broad stroke)")}
    });

    _thinning_item.set_custom_numeric_menu_data({
        {-100, _("(speed blows up stroke)")},
        { -40, ""},
        { -20, ""},
        { -10, _("(slight widening)")},
        {   0, _("(constant width)")},
        {  10, _("(slight thinning, default)")},
        {  20, ""},
        {  40, ""},
        { 100, _("(speed deflates stroke)")}
    });

    _mass_item.set_custom_numeric_menu_data({
        {  0, _("(no inertia)")},
        {  2, _("(slight smoothing, default)")},
        { 10, _("(noticeable lagging)")},
        { 20, ""},
        { 50, ""},
        {100, _("(maximum inertia)")}
    });

    _angle_item.set_custom_numeric_menu_data({
        {-90, _("(left edge up)")},
        {-60, ""},
        {-30, ""},
        {  0, _("(horizontal)")},
        { 30, _("(default)")},
        { 60, ""},
        { 90, _("(right edge up)")}
    });

    // Fixation
    _flatness_item.set_custom_numeric_menu_data({
        {  0, _("(perpendicular to stroke, \"brush\")")},
        { 20, ""},
        { 40, ""},
        { 60, ""},
        { 90, _("(almost fixed, default)")},
        {100, _("(fixed by Angle, \"pen\")")}
    });

    _cap_rounding_item.set_custom_numeric_menu_data({
        {  0, _("(blunt caps, default)")},
        {0.3, _("(slightly bulging)")},
        {0.5, ""},
        {1.0, ""},
        {1.4, _("(approximately round)")},
        {5.0, _("(long protruding caps)")}
    });

    _tremor_item.set_custom_numeric_menu_data({
        {  0, _("(smooth line)")},
        { 10, _("(slight tremor)")},
        { 20, _("(noticeable tremor)")},
        { 40, ""},
        { 60, ""},
        {100, _("(maximum tremor)")}
    });

    _wiggle_item.set_custom_numeric_menu_data({
        {  0, _("(no wiggle)")},
        { 20, _("(slight deviation)")},
        { 40, ""},
        { 60, ""},
        {100, _("(wild waves and curls)")}
    });

    // Configure the calligraphic profile combo box text.
    build_presets_list();
    _profile_selector_combo.signal_changed().connect(sigc::mem_fun(*this, &CalligraphyToolbar::change_profile));

    // Unit menu.
    auto unit_menu = _tracker->create_unit_dropdown();
    get_widget<Gtk::Box>(builder, "unit_menu_box").append(*unit_menu);
    unit_menu->signal_changed().connect(sigc::mem_fun(*this, &CalligraphyToolbar::unit_changed));

    // Use pressure button.
    _widget_map["usepressure"] = &usepressure_btn;
    _usepressure_pusher = std::make_unique<SimplePrefPusher>(&usepressure_btn, "/tools/calligraphic/usepressure");
    usepressure_btn.signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &CalligraphyToolbar::on_pref_toggled),
                                                         &usepressure_btn, "/tools/calligraphic/usepressure"));

    // Tracebackground button.
    _widget_map["tracebackground"] = &tracebackground_btn;
    _tracebackground_pusher = std::make_unique<SimplePrefPusher>(&tracebackground_btn, "/tools/calligraphic/tracebackground");
    tracebackground_btn.signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &CalligraphyToolbar::on_pref_toggled),
                                                             &tracebackground_btn,
                                                             "/tools/calligraphic/tracebackground"));

    // Use tilt button.
    _widget_map["usetilt"] = _usetilt_btn;
    _usetilt_pusher = std::make_unique<SimplePrefPusher>(_usetilt_btn, "/tools/calligraphic/usetilt");
    _usetilt_btn->signal_toggled().connect(sigc::mem_fun(*this, &CalligraphyToolbar::tilt_state_changed));
    _angle_item.set_sensitive(!prefs->getBool("/tools/calligraphic/usetilt", true));
    _usetilt_btn->set_active(prefs->getBool("/tools/calligraphic/usetilt", true));

    // Signals.
    get_widget<Gtk::Button>(builder, "profile_edit_btn")
        .signal_clicked()
        .connect(sigc::mem_fun(*this, &CalligraphyToolbar::edit_profile));

    _initMenuBtns();
}

CalligraphyToolbar::~CalligraphyToolbar() = default;

void CalligraphyToolbar::setup_derived_spin_button(UI::Widget::SpinButton &btn, Glib::ustring const &name,
                                                   double default_value, ValueChangedMemFun value_changed_mem_fun)
{
    auto const prefs = Preferences::get();
    auto const path = "/tools/calligraphic/" + name;
    auto const val = prefs->getDouble(path, default_value);
    auto adj = btn.get_adjustment();

    if (name == "width") {
        auto const unit = UnitTable::get().getUnit(prefs->getString("/tools/calligraphic/unit"));
        adj = Gtk::Adjustment::create(Quantity::convert(val, "px", unit), 0.001, 100, 1.0, 10.0);
        btn.set_adjustment(adj);
    } else {
        adj->set_value(val);
    }

    adj->signal_value_changed().connect(sigc::mem_fun(*this, value_changed_mem_fun));

    _widget_map[name] = adj.get();
    _tracker->addAdjustment(adj->gobj());
    btn.setDefocusTarget(this);
}

void CalligraphyToolbar::width_value_changed()
{
    auto const unit = _tracker->getActiveUnit();
    auto prefs = Preferences::get();
    prefs->setBool("/tools/calligraphic/abs_width", _tracker->getCurrentLabel() != "%");
    prefs->setDouble("/tools/calligraphic/width",
                     Quantity::convert(_width_item.get_adjustment()->get_value(), unit, "px"));
    update_presets_list();
}

void CalligraphyToolbar::velthin_value_changed()
{
    Preferences::get()->setDouble("/tools/calligraphic/thinning", _thinning_item.get_adjustment()->get_value());
    update_presets_list();
}

void CalligraphyToolbar::angle_value_changed()
{
    Preferences::get()->setDouble("/tools/calligraphic/angle", _angle_item.get_adjustment()->get_value());
    update_presets_list();
}

void CalligraphyToolbar::flatness_value_changed()
{
    Preferences::get()->setDouble("/tools/calligraphic/flatness", _flatness_item.get_adjustment()->get_value());
    update_presets_list();
}

void CalligraphyToolbar::cap_rounding_value_changed()
{
    Preferences::get()->setDouble("/tools/calligraphic/cap_rounding", _cap_rounding_item.get_adjustment()->get_value());
    update_presets_list();
}

void CalligraphyToolbar::tremor_value_changed()
{
    Preferences::get()->setDouble("/tools/calligraphic/tremor", _tremor_item.get_adjustment()->get_value());
    update_presets_list();
}

void CalligraphyToolbar::wiggle_value_changed()
{
    Preferences::get()->setDouble("/tools/calligraphic/wiggle", _wiggle_item.get_adjustment()->get_value());
    update_presets_list();
}

void CalligraphyToolbar::mass_value_changed()
{
    Preferences::get()->setDouble("/tools/calligraphic/mass", _mass_item.get_adjustment()->get_value());
    update_presets_list();
}

void CalligraphyToolbar::on_pref_toggled(Gtk::ToggleButton *item, Glib::ustring const &path)
{
    Preferences::get()->setBool(path, item->get_active());
    update_presets_list();
}

void CalligraphyToolbar::update_presets_list()
{
    if (_presets_blocked) {
        return;
    }

    auto presets = get_presets_list();

    int index = 1;  // 0 is for no preset.
    for (auto i = presets.begin(); i != presets.end(); ++i, ++index) {
        bool match = true;

        auto preset = Preferences::get()->getAllEntries(*i);
        for (auto &j : preset) {
            auto const entry_name = j.getEntryName();
            if (entry_name == "id" || entry_name == "name") {
                continue;
            }

            if (auto widget = _widget_map[entry_name.data()]) {
                if (auto adj = dynamic_cast<Gtk::Adjustment *>(widget)) {
                    double v = j.getDouble();
                    if (std::abs(adj->get_value() - v) > 1e-6) {
                        match = false;
                        break;
                    }
                } else if (auto toggle = dynamic_cast<Gtk::ToggleButton *>(widget)) {
                    bool v = j.getBool();
                    if (toggle->get_active() != v) {
                        match = false;
                        break;
                    }
                }
            }
        }

        if (match) {
            // newly added item is at the same index as the
            // save command, so we need to change twice for it to take effect
            _profile_selector_combo.set_selected(0);
            _profile_selector_combo.set_selected(index);
            return;
        }
    }

    // no match found
    _profile_selector_combo.set_selected(0);
}

void CalligraphyToolbar::tilt_state_changed()
{
    _angle_item.set_sensitive(!_usetilt_btn->get_active());
    on_pref_toggled(_usetilt_btn, "/tools/calligraphic/usetilt");
}

void CalligraphyToolbar::build_presets_list()
{
    _presets_blocked = true;

    _profile_selector_combo.remove_all();
    _profile_selector_combo.append(_("No preset"));

    // iterate over all presets to populate the list
    for (auto const &preset : get_presets_list()) {
        auto const preset_name = Preferences::get()->getString(preset + "/name");
        if (!preset_name.empty()) {
            _profile_selector_combo.append(_(preset_name.data()));
        }
    }

    _presets_blocked = false;

    update_presets_list();
}

void CalligraphyToolbar::change_profile()
{
    auto mode = _profile_selector_combo.get_selected();

    if (_presets_blocked) {
        return;
    }

    // mode is one-based so we subtract 1
    auto const presets = get_presets_list();

    Glib::ustring preset_path = "";
    if (mode - 1 < presets.size()) {
        preset_path = presets.at(mode - 1);
    }

    if (!preset_path.empty()) {
        _presets_blocked = true; //temporarily block the selector so no one will updadte it while we're reading it

        auto preset = Preferences::get()->getAllEntries(preset_path);

        // Shouldn't this be std::map?
        for (auto &i : preset) {
            auto const entry_name = i.getEntryName();
            if (entry_name == "id" || entry_name == "name") {
                continue;
            }
            if (auto widget = _widget_map[entry_name.data()]) {
                if (auto adj = dynamic_cast<Gtk::Adjustment *>(widget)) {
                    adj->set_value(i.getDouble());
                } else if (auto toggle = dynamic_cast<Gtk::ToggleButton *>(widget)) {
                    toggle->set_active(i.getBool());
                } else {
                    g_warning("Unknown widget type for preset: %s\n", entry_name.data());
                }
            } else {
                g_warning("Bad key found in a preset record: %s\n", entry_name.data());
            }
        }

        _presets_blocked = false;
    }
}

void CalligraphyToolbar::edit_profile()
{
    save_profile(nullptr);
}

void CalligraphyToolbar::unit_changed()
{
    auto const unit = _tracker->getActiveUnit();
    auto prefs = Preferences::get();
    prefs->setBool("/tools/calligraphic/abs_width", _tracker->getCurrentLabel() != "%");
    prefs->setDouble("/tools/calligraphic/width",
                     std::clamp(prefs->getDouble("/tools/calligraphic/width"), Quantity::convert(0.001, unit, "px"),
                           Quantity::convert(100, unit, "px")));
    prefs->setString("/tools/calligraphic/unit", unit->abbr);
}

void CalligraphyToolbar::save_profile(GtkWidget *)
{
    using Dialog::CalligraphicProfileRename;
    auto prefs = Preferences::get();
    if (! _desktop) {
        return;
    }

    if (_presets_blocked) {
        return;
    }

    auto current_profile_name = _profile_selector_combo.get_string(_profile_selector_combo.get_selected());

    if (current_profile_name == _("No preset")) {
        current_profile_name = "";
    }

    CalligraphicProfileRename::show(_desktop, current_profile_name);
    if (!CalligraphicProfileRename::applied()) {
        // dialog cancelled
        update_presets_list();
        return;
    }
    Glib::ustring new_profile_name = CalligraphicProfileRename::getProfileName();

    if (new_profile_name.empty()) {
        // empty name entered
        update_presets_list();
        return;
    }

    _presets_blocked = true;

    // If there's a preset with the given name, find it and set save_path appropriately
    auto presets = get_presets_list();
    int total_presets = presets.size();
    int new_index = -1;
    Glib::ustring save_path; // profile pref path without a trailing slash

    for (int i = 0; i < presets.size(); i++) {
        auto &p = presets[i];
        auto const name = prefs->getString(p + "/name");
        if (!name.empty() && (new_profile_name == name || current_profile_name == name)) {
            new_index = i;
            save_path = p;
            break;
        }
    }

    if (CalligraphicProfileRename::deleted() && new_index != -1) {
        prefs->remove(save_path);
        _presets_blocked = false;
        build_presets_list();
        return;
    }

    if (new_index == -1) {
        // no preset with this name, create
        new_index = total_presets + 1;
        auto const profile_id = g_strdup_printf("/dcc%d", new_index);
        save_path = Glib::ustring("/tools/calligraphic/preset") + profile_id;
        g_free(profile_id);
    }

    for (auto const &[widget_name, widget] : _widget_map) {
        if (widget) {
            if (auto adj = dynamic_cast<Gtk::Adjustment *>(widget)) {
                prefs->setDouble(save_path + "/" + widget_name, adj->get_value());
            } else if (auto toggle = dynamic_cast<Gtk::ToggleButton *>(widget)) {
                prefs->setBool(save_path + "/" + widget_name, toggle->get_active());
            } else {
                g_warning("Unknown widget type for preset: %s\n", widget_name.c_str());
            }
        } else {
            g_warning("Bad key when writing preset: %s\n", widget_name.c_str());
        }
    }
    prefs->setString(save_path + "/name", new_profile_name);

    _presets_blocked = true;
    build_presets_list();
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

// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file Tweak toolbar
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

#include "tweak-toolbar.h"

#include <glibmm/i18n.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/togglebutton.h>

#include "ui/builder-utils.h"
#include "ui/tools/tweak-tool.h"
#include "ui/util.h"
#include "ui/widget/spinbutton.h"

namespace Inkscape::UI::Toolbar {

TweakToolbar::TweakToolbar()
    : TweakToolbar{create_builder("toolbar-tweak.ui")}
{}

TweakToolbar::TweakToolbar(Glib::RefPtr<Gtk::Builder> const &builder)
    : Toolbar{get_widget<Gtk::Box>(builder, "tweak-toolbar")}
    , _width_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_width_item")}
    , _force_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_force_item")}
    , _fidelity_box{get_widget<Gtk::Box>(builder, "_fidelity_box")}
    , _fidelity_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_fidelity_item")}
    , _pressure_btn{get_widget<Gtk::ToggleButton>(builder, "_pressure_btn")}
    , _channels_box{get_widget<Gtk::Box>(builder, "_channels_box")}
    , _doh_btn{get_widget<Gtk::ToggleButton>(builder, "_doh_btn")}
    , _dos_btn{get_widget<Gtk::ToggleButton>(builder, "_dos_btn")}
    , _dol_btn{get_widget<Gtk::ToggleButton>(builder, "_dol_btn")}
    , _doo_btn{get_widget<Gtk::ToggleButton>(builder, "_doo_btn")}
{
    setup_derived_spin_button(_width_item, "width", 15, &TweakToolbar::width_value_changed);
    setup_derived_spin_button(_force_item, "force", 20, &TweakToolbar::force_value_changed);
    setup_derived_spin_button(_fidelity_item, "fidelity", 50, &TweakToolbar::fidelity_value_changed);

    _width_item.set_custom_numeric_menu_data({
        {1, _("(pinch tweak)")},
        {2, ""},
        {3, ""},
        {5, ""},
        {10, ""},
        {15, _("(default)")},
        {30, ""},
        {50, ""},
        {75, ""},
        {100, _("(broad tweak)")}
    });

    _force_item.set_custom_numeric_menu_data({
        {1, _("(minimum force)")},
        {5, ""},
        {10, ""},
        {20, _("(default)")},
        {30, ""},
        {50, ""},
        {70, ""},
        {100, _("(maximum force)")}
    });

    _fidelity_item.set_custom_numeric_menu_data({
        {10, _("(rough, simplified)")},
        {25, ""},
        {35, ""},
        {50, _("(default)")},
        {60, ""},
        {80, ""},
        {100, _("(fine, but many nodes)")}
    });

    // Configure mode buttons
    int btn_index = 0;
    for (auto &item : children(get_widget<Gtk::Box>(builder, "mode_buttons_box"))) {
        auto &btn = dynamic_cast<Gtk::ToggleButton &>(item);
        _mode_buttons.push_back(&btn);
        btn.signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &TweakToolbar::mode_changed), btn_index++));
    }

    auto prefs = Inkscape::Preferences::get();

    // Pressure button.
    _pressure_btn.signal_toggled().connect(sigc::mem_fun(*this, &TweakToolbar::pressure_state_changed));
    _pressure_btn.set_active(prefs->getBool("/tools/tweak/usepressure", true));

    // Set initial mode.
    int mode = prefs->getIntLimited("/tools/tweak/mode", 0, 0, _mode_buttons.size() - 1);
    mode = std::clamp<int>(mode, 0, _mode_buttons.size() - 1);
    _mode_buttons[mode]->set_active();

    // Configure channel buttons.
    // TRANSLATORS: H, S, L, and O stands for:
    // Hue, Saturation, Lighting and Opacity respectively.
    _doh_btn.signal_toggled().connect(sigc::mem_fun(*this, &TweakToolbar::toggle_doh));
    _doh_btn.set_active(prefs->getBool("/tools/tweak/doh", true));
    _dos_btn.signal_toggled().connect(sigc::mem_fun(*this, &TweakToolbar::toggle_dos));
    _dos_btn.set_active(prefs->getBool("/tools/tweak/dos", true));
    _dol_btn.signal_toggled().connect(sigc::mem_fun(*this, &TweakToolbar::toggle_dol));
    _dol_btn.set_active(prefs->getBool("/tools/tweak/dol", true));
    _doo_btn.signal_toggled().connect(sigc::mem_fun(*this, &TweakToolbar::toggle_doo));
    _doo_btn.set_active(prefs->getBool("/tools/tweak/doo", true));

    // Elements must be hidden after being initially visible.
    if (mode == Inkscape::UI::Tools::TWEAK_MODE_COLORPAINT || mode == Inkscape::UI::Tools::TWEAK_MODE_COLORJITTER) {
        _fidelity_box.set_visible(false);
    } else {
        _channels_box.set_visible(false);
    }

    _initMenuBtns();
}

TweakToolbar::~TweakToolbar() = default;

void TweakToolbar::setup_derived_spin_button(UI::Widget::SpinButton &btn, Glib::ustring const &name,
                                             double default_value, ValueChangedMemFun value_changed_mem_fun)
{
    auto const path = "/tools/tweak/" + name;
    auto const val = Preferences::get()->getDouble(path, default_value);

    auto adj = btn.get_adjustment();
    adj->set_value(val);
    adj->signal_value_changed().connect(sigc::mem_fun(*this, value_changed_mem_fun));

    btn.setDefocusTarget(this);
}

void TweakToolbar::setMode(int mode)
{
    _mode_buttons[mode]->set_active();
    mode_changed(mode); // set_active() does not trigger callback, call it here.
}

void TweakToolbar::width_value_changed()
{
    Preferences::get()->setDouble("/tools/tweak/width", _width_item.get_adjustment()->get_value() * 0.01);
}

void TweakToolbar::force_value_changed()
{
    Preferences::get()->setDouble("/tools/tweak/force", _force_item.get_adjustment()->get_value() * 0.01);
}

void TweakToolbar::mode_changed(int mode)
{
    Preferences::get()->setInt("/tools/tweak/mode", mode);

    bool flag = mode == Inkscape::UI::Tools::TWEAK_MODE_COLORPAINT ||
                mode == Inkscape::UI::Tools::TWEAK_MODE_COLORJITTER;

    _channels_box.set_visible(flag);

    _fidelity_box.set_visible(!flag);
}

void TweakToolbar::fidelity_value_changed()
{
    Preferences::get()->setDouble("/tools/tweak/fidelity", _fidelity_item.get_adjustment()->get_value() * 0.01);
}

void TweakToolbar::pressure_state_changed()
{
    Preferences::get()->setBool("/tools/tweak/usepressure", _pressure_btn.get_active());
}

void TweakToolbar::toggle_doh()
{
    Preferences::get()->setBool("/tools/tweak/doh", _doh_btn.get_active());
}

void TweakToolbar::toggle_dos()
{
    Preferences::get()->setBool("/tools/tweak/dos", _dos_btn.get_active());
}

void TweakToolbar::toggle_dol()
{
    Preferences::get()->setBool("/tools/tweak/dol", _dol_btn.get_active());
}

void TweakToolbar::toggle_doo()
{
    Preferences::get()->setBool("/tools/tweak/doo", _doo_btn.get_active());
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

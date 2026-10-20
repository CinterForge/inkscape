// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_TOOLBAR_CALLIGRAPHY_TOOLBAR_H
#define INKSCAPE_UI_TOOLBAR_CALLIGRAPHY_TOOLBAR_H

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

#include "toolbar.h"

namespace Gtk {
class Builder;
class ComboBoxText;
class ToggleButton;
} // namespace Gtk

namespace Inkscape::UI {
class SimplePrefPusher;
namespace Widget {
class DropDownList;
class SpinButton;
class UnitTracker;
} // namespace Widget
} // namespace Inkscape::UI

namespace Inkscape::UI::Toolbar {

class CalligraphyToolbar : public Toolbar
{
public:
    CalligraphyToolbar();
    ~CalligraphyToolbar() override;

private:
    CalligraphyToolbar(Glib::RefPtr<Gtk::Builder> const &builder);

    using ValueChangedMemFun = void (CalligraphyToolbar::*)();

    std::unique_ptr<UI::Widget::UnitTracker> _tracker;
    bool _presets_blocked = false;

    UI::Widget::DropDownList &_profile_selector_combo;
    UI::Widget::SpinButton &_width_item;

    UI::Widget::SpinButton &_thinning_item;
    UI::Widget::SpinButton &_mass_item;

    UI::Widget::SpinButton &_angle_item;
    Gtk::ToggleButton *_usetilt_btn;

    UI::Widget::SpinButton &_flatness_item;

    UI::Widget::SpinButton &_cap_rounding_item;

    UI::Widget::SpinButton &_tremor_item;
    UI::Widget::SpinButton &_wiggle_item;

    std::map<std::string, Glib::Object *> _widget_map;

    // TODO: Check if these can be moved to the constructor.
    std::unique_ptr<SimplePrefPusher> _tracebackground_pusher;
    std::unique_ptr<SimplePrefPusher> _usepressure_pusher;
    std::unique_ptr<SimplePrefPusher> _usetilt_pusher;

    void setup_derived_spin_button(UI::Widget::SpinButton &btn, Glib::ustring const &name, double default_value,
                                   ValueChangedMemFun value_changed_mem_fun);
    void width_value_changed();
    void velthin_value_changed();
    void angle_value_changed();
    void flatness_value_changed();
    void cap_rounding_value_changed();
    void tremor_value_changed();
    void wiggle_value_changed();
    void mass_value_changed();
    void build_presets_list();
    void change_profile();
    void save_profile(GtkWidget *widget);
    void edit_profile();
    void update_presets_list();
    void tilt_state_changed();
    void unit_changed();
    void on_pref_toggled(Gtk::ToggleButton *item, Glib::ustring const &path);
};

} // namespace Inkscape::UI::Toolbar

#endif // INKSCAPE_UI_TOOLBAR_CALLIGRAPHY_TOOLBAR_H

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

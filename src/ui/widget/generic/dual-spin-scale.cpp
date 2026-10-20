// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * Derived from and replaces SpinSlider
 */
/*
 * Author:
 *
 * Copyright (C) 2007 Nicholas Bishop <nicholasbishop@gmail.com>
 *               2008 Felipe C. da S. Sanches <juca@members.fsf.org>
 *               2017 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

/*
 * Derived from and replaces SpinSlider
 */

#include "dual-spin-scale.h"

#include <glibmm/stringutils.h>
#include <gtkmm/adjustment.h>

#include "ui/pack.h"

namespace Inkscape::UI::Widget {

DualSpinScale::DualSpinScale(Glib::ustring label1, Glib::ustring label2,
                             double value, double lower, double upper,
                             double step_increment, int digits,
                             Glib::ustring const &tip_text1, Glib::ustring const &tip_text2)
{
    set_name("DualSpinScale");

    _s1.set_adjustment_values(lower, upper, step_increment);
    _s1.set_digits(digits);
    _s1.set_value(value);
    _s1.set_tooltip_text(tip_text1);

    _s2.set_adjustment_values(lower, upper, step_increment);
    _s2.set_digits(digits);
    _s2.set_value(value);
    _s2.set_tooltip_text(tip_text2);

    _s1.get_adjustment()->signal_value_changed().connect(_signal_value_changed.make_slot());
    _s2.get_adjustment()->signal_value_changed().connect(_signal_value_changed.make_slot());
    _s1.get_adjustment()->signal_value_changed().connect(sigc::mem_fun(*this, &DualSpinScale::update_linked));

    _link.set_has_frame(false);
    _link.set_focus_on_click(false);
    _link.set_focusable(false);
    _link.add_css_class("link-edit-button");
    _link.set_valign(Gtk::Align::CENTER);
    _link.signal_clicked().connect(sigc::mem_fun(*this, &DualSpinScale::link_toggled));

    auto const vb = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
    vb->append(_s1);
    _s1.set_margin_bottom(3);
    vb->append(_s2);
    UI::pack_start(*this, *vb);
    UI::pack_start(*this, _link, false, false);
    set_linked(true);
    _s2.set_sensitive(false);
}

void DualSpinScale::set_linked(bool link) {
    _linked = link;
    _link.set_image_from_icon_name(_linked ? "entries-linked" : "entries-unlinked", Gtk::IconSize::NORMAL);
}

sigc::signal<void ()>& DualSpinScale::signal_value_changed()
{
    return _signal_value_changed;
}

const SpinScale& DualSpinScale::get_SpinScale1() const
{
    return _s1;
}

SpinScale& DualSpinScale::get_SpinScale1()
{
    return _s1;
}

const SpinScale& DualSpinScale::get_SpinScale2() const
{
    return _s2;
}

SpinScale& DualSpinScale::get_SpinScale2()
{
    return _s2;
}

void DualSpinScale::link_toggled()
{
    _linked = !_linked;
    set_linked(_linked);
    _s2.set_sensitive(!_linked);
    update_linked();
}

void DualSpinScale::update_linked()
{
    if (_linked) {
        _s2.set_value(_s1.get_value());
    }
}

} // namespace Inkscape::UI::Widget

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

// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * Derived from and replaces SpinSlider
 */
/*
 * Author:
 *
 * Copyright (C) 2007 Nicholas Bishop <nicholasbishop@gmail.com>
 *               2017 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_WIDGET_DUAL_SPIN_SCALE_H
#define INKSCAPE_UI_WIDGET_DUAL_SPIN_SCALE_H

#include <gtkmm/button.h>

#include "spin-scale.h"

namespace Inkscape::UI::Widget {

/**
 * Contains two SpinScales for controlling number-opt-number attributes.
 *
 * @see SpinScale
 */
class DualSpinScale : public Gtk::Box
{
public:
    DualSpinScale(const Glib::ustring label1, const Glib::ustring label2,
                  double value, double lower, double upper,
                  double step_increment, int digits,
                  Glib::ustring const &tip_text1, Glib::ustring const &tip_text2);

    sigc::signal<void ()>& signal_value_changed();

    const SpinScale& get_SpinScale1() const;
    SpinScale& get_SpinScale1();

    const SpinScale& get_SpinScale2() const;
    SpinScale& get_SpinScale2();

    void set_linked(bool link);
    bool is_linked() const { return _linked; }

    std::pair<double, double> get_value() const {
        return {_s1.get_value(), (_linked ? _s1 : _s2).get_value()};
    }

    void set_value(double value1, double value2) {
        _s1.get_adjustment()->set_value(value1);
        _s2.get_adjustment()->set_value(value2);
    }

private:
    void link_toggled();
    void update_linked();
    sigc::signal<void ()> _signal_value_changed;
    SpinScale _s1, _s2;
    bool _linked = true;
    Gtk::Button _link;
};

} // namespace Inkscape::UI::Widget

#endif // INKSCAPE_UI_WIDGET_DUAL_SPIN_SCALE_H

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

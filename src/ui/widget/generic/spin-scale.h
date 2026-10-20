// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Build a scale and spin button combo
 *//*
 * Copyright (C) 2025 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_UI_WIDGET_SPIN_SCALE_H
#define SEEN_UI_WIDGET_SPIN_SCALE_H

#include <gtkmm/box.h>

#include "svg/css-ostringstream.h"

#include "scale-bar.h"
#include "spin-button.h"
#include "ui/icon-names.h"

namespace Inkscape::UI::Widget {

class SpinScale : public Gtk::Box
{
public:

    SpinScale() : Gtk::Box(Gtk::Orientation::HORIZONTAL, 2)
    {
        construct();
        set_adjustment_values(); // defaults
    }
    SpinScale(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
        : Gtk::Box(cobject)
    {
        construct();
        set_adjustment_values(); // defaults
    }

    void set_value(double new_value) { _spin.set_value(new_value); }
    void set_suffix(std::string const &suffix, bool add_half_space = false) { _spin.set_suffix(suffix, add_half_space); }
    void set_scaling_factor(double factor) { _spin.set_scaling_factor(factor); }
    void set_max_block_count(int count) { _scale.set_max_block_count(count); }

    auto get_adjustment() { return _spin.get_adjustment(); }
    void set_adjustment(Glib::RefPtr<Gtk::Adjustment> adjustment)
    {
        _spin.set_adjustment(adjustment);
        _scale.set_adjustment(adjustment);
    }
    void set_digits(int digits) { _spin.set_digits(digits); }
    void set_adjustment_values(double lower = 0.0, double upper = 100.0, double step_increment = 1.0, double page_increment = 0.0)
    {
        auto adj = get_adjustment();
        adj->set_lower(lower);
        adj->set_upper(upper);
        adj->set_step_increment(step_increment);
        adj->set_page_increment(page_increment);
        // These values mess of range and are relics of Gtk trying to use concepts from
        // scrolling in viewports for number ranges.
        adj->set_page_size(0);
    }

    double get_value() const { return _spin.get_value(); }
    auto signal_value_changed() { return _spin.signal_value_changed(); }

    std::string as_string() const
    {
        Inkscape::CSSOStringStream os;
        os << get_value();
        return os.str();
    }

    void set_icon(std::string path, std::string tooltip)
    {
        _icon.set_from_icon_name(INKSCAPE_ICON(path));
        _icon.set_tooltip_text(tooltip);
    }

private:
    void construct()
    {
        set_name("SpinScale");

        add_css_class("border-box");
        add_css_class("entry-box");
        add_css_class("std-widget-height");

        _spin.set_digits(0);
        _spin.set_halign(Gtk::Align::END);
        _spin.set_has_frame(false);

        _scale.set_valign(Gtk::Align::FILL);
        _scale.set_margin_end(2);
        _scale.set_margin_start(5);
        _scale.set_hexpand();
        _scale.set_adjustment(get_adjustment()); 

        _icon.set_margin_start(12);

        append(_icon);
        append(_scale);
        append(_spin);
    }

    ScaleBar _scale;
    InkSpinButton _spin;
    Gtk::Image _icon;
};

} // namespace Inkscape::UI::Widget

#endif /* !SEEN_UI_WIDGET_SPIN_SCALE_H */

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim:filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99:

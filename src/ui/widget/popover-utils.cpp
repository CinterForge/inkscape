// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Utilities for handling popovers (scrolling, positioning).
 *//*
 * Authors:
 *   Ayan Das <ayandazzz@outlook.com>
 *
 * Copyright (C) 2025 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "popover-utils.h"

#include <gtkmm/native.h>
#include <gtkmm/scrolledwindow.h>
#include <gdkmm/surface.h>

namespace Inkscape::UI::Widget::Utils {

void wrap_in_scrolled_window(Gtk::Popover& popover, int min_height, int min_width) {
    auto child = popover.get_child();
    if (!child || dynamic_cast<Gtk::ScrolledWindow*>(child)) {
        return;
    }

    child->reference();
    popover.unset_child();

    auto scrolled = Gtk::make_managed<Gtk::ScrolledWindow>();
    scrolled->set_child(*child);
    scrolled->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    scrolled->set_propagate_natural_height(true);
    scrolled->set_propagate_natural_width(true);
    scrolled->set_min_content_height(min_height);
    scrolled->set_min_content_width(min_width);

    popover.set_child(*scrolled);
    child->unreference();
}

bool smart_position(Gtk::Popover& popover, Gtk::Widget& anchor) {
    int natural_height = 0, natural_width = 0;
    int base_dummy = -1;

    popover.measure(Gtk::Orientation::VERTICAL, -1, base_dummy, natural_height, base_dummy, base_dummy);
    popover.measure(Gtk::Orientation::HORIZONTAL, -1, base_dummy, natural_width, base_dummy, base_dummy);

    auto native = anchor.get_native();
    if (!native) {
        return false;
    }

    auto surface = native->get_surface();
    if (!surface) {
        return false;
    }

    double surface_height = surface->get_height();
    double surface_width = surface->get_width();

    double btn_x = 0, btn_y = 0;
    if (!anchor.translate_coordinates(*dynamic_cast<Gtk::Widget*>(native), 0, 0, btn_x, btn_y)) {
        return false;
    }

    double btn_height = anchor.get_height();
    double btn_width = anchor.get_width();

    double space_top    = btn_y;
    double space_bottom = surface_height - (btn_y + btn_height);
    double space_left   = btn_x;
    double space_right  = surface_width - (btn_x + btn_width);

    if (space_bottom >= natural_height) {
        popover.set_position(Gtk::PositionType::BOTTOM);
        return true;
    } else if (space_top >= natural_height) {
        popover.set_position(Gtk::PositionType::TOP);
        return true;
    } else if(space_left >= natural_width) {
        popover.set_position(Gtk::PositionType::LEFT);
        return true;
    } else if (space_right >= natural_width) {
        popover.set_position(Gtk::PositionType::RIGHT);
        return true;
    }

    return false;
}

} // namespace Inkscape::UI::Widget::Utils

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
//vim:filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99:

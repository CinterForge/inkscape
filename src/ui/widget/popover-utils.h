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

#ifndef INKSCAPE_UI_WIDGET_POPOVER_UTILS_H
#define INKSCAPE_UI_WIDGET_POPOVER_UTILS_H

#include <gtkmm/popover.h>
#include <gtkmm/widget.h>

namespace Inkscape::UI::Widget::Utils {

/**
 * @brief Wraps the content of a popover in a scrollable window.
 *
 * This function takes the current child of the provided `Gtk::Popover` and re-parents it
 * into a `Gtk::ScrolledWindow`.
 *
 * Note: If the child is already a Gtk::ScrolledWindow,
 * the function returns immediately without making changes.
 *
 * @param popover    Reference to the Gtk::Popover whose content needs wrapping.
 * @param min_height The minimum content height (in pixels) for the scrolled window.
 * Defaults to 200.
 * @param min_width  The minimum content width (in pixels) for the scrolled window.
 * Defaults to 100.
 */
void wrap_in_scrolled_window(Gtk::Popover& popover, int min_height = 200, int min_width = 100);

/**
 * @brief Tries to position the popover based on available screen space.
 * Calculates available space in 4 directions and attempts to set the position
 * in the preference: Bottom > Top > Left > Right.
 * @param popover The popover to position.
 * @param anchor  The widget the popover is attached to.
 * @return true if a direction with sufficient space was found and set; false otherwise.
 */
bool smart_position(Gtk::Popover& popover, Gtk::Widget& anchor);

} // namespace Inkscape::UI::Widget::Utils

#endif // INKSCAPE_UI_WIDGET_POPOVER_UTILS_H

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

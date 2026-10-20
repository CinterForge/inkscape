// SPDX-License-Identifier: GPL-2.0-or-later
/* Authors:
 *   Thomas Holder
 *
 * Copyright (C) 2020 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_UI_WIDGET_SCROLL_UTILS_H
#define SEEN_INKSCAPE_UI_WIDGET_SCROLL_UTILS_H

namespace Gtk {
class Widget;
} // namespace Gtk

namespace Inkscape::UI::Widget {

/// Get the first ancestor which is scrollable.
Gtk::Widget *get_scrollable_ancestor(Gtk::Widget &widget);

/// Get the first ancestor which is scrollable.
inline Gtk::Widget const *get_scrollable_ancestor(Gtk::Widget const &widget)
{
    return get_scrollable_ancestor(const_cast<Gtk::Widget &>(widget));
}

} // namespace Inkscape::UI::Widget

#endif // SEEN_INKSCAPE_UI_WIDGET_SCROLL_UTILS_H

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

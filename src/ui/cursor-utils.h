// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Cursor utilities
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#ifndef INK_CURSOR_UTILITIES_H
#define INK_CURSOR_UTILITIES_H

#include <glibmm/refptr.h>

#include "colors/color.h"

namespace Gdk {
class Cursor;
} // namespace Gdk

namespace Gtk {
class Widget;
} // namespace Gtk

namespace Inkscape {

Glib::RefPtr<Gdk::Cursor> load_svg_cursor(Gtk::Widget &widget,
                                          std::string const &file_name,
                                          std::optional<Colors::Color> fill = {},
                                          std::optional<Colors::Color> stroke = {});

void set_svg_cursor(Gtk::Widget &widget,
                    std::string const &file_name,
                    std::optional<Colors::Color> fill = {},
                    std::optional<Colors::Color> stroke = {});

} // namespace Inkscape

#endif // INK_CURSOR_UTILITIES_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

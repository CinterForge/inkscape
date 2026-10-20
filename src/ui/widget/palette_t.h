// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief Color palette widget
 */
/* Authors:
 *   Michael Kowalski
 *
 * Copyright (C) 2021 Michael Kowalski
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_UI_WIDGET_PALETTE_T_H
#define SEEN_INKSCAPE_UI_WIDGET_PALETTE_T_H

namespace Inkscape::UI::Widget {

struct rgb_t { double r; double g; double b; };
struct palette_t { Glib::ustring name; Glib::ustring id; std::vector<rgb_t> colors; };

} // namespace Inkscape::UI::Widget

#endif // SEEN_INKSCAPE_UI_WIDGET_PALETTE_T_H

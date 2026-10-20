// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape Widget Utilities
 *
 * Author:
 *   Bryce W. Harrington <brycehar@bryceharrington.org>
 *
 * Copyright (C) 2003 Bryce Harrington
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_SPW_UTILITIES_H
#define SEEN_SPW_UTILITIES_H

#include <glibmm/ustring.h>

namespace Gtk {
class Widget;
} // namespace Gtk

/// Get string action target, if available.
Glib::ustring sp_get_action_target(Gtk::Widget* widget);

#endif // SEEN_SPW_UTILITIES_H

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

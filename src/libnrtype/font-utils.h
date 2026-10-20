// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Font selection utilities.
 */
/*
 * Authors:
 *   See Git history
 *
 * Copyright (C) 2026 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <map>
#include <string>
#include <vector>

#include <glibmm/ustring.h>

namespace Inkscape {

// Pass fontspec to and back from Pango to get a the fontspec in canonical form. TEST
Glib::ustring canonize_fontspec(Glib::ustring const &fontspec);

// Returns a map of 'tag' => 'value' from a variations string. TEST
std::map<std::string, std::string> parse_variations(const char* variations);

} // namespace Inkscape

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

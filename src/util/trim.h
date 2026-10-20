// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Inkscape::Util::trim - trim whitespace and other characters
 *
 * Authors:
 *   Rafael Siejakowski <rs@rs-math.net>
 *
 * Copyright (C) 2022 the Authors.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_TRIM_H
#define SEEN_TRIM_H

#include <glibmm/ustring.h>
#include <glibmm/regex.h>

namespace Inkscape {
namespace Util {

/**
 * @brief
 * Modifies a string in place, removing leading and trailing whitespace characters.
 * Optionally, it can remove other characters or ranges in addition to whitespace.
 *
 * @param input - a reference to a Glib::ustring which will be modified in place.
 * @param also_remove - optional range of characters to remove in addition to whitespace.
 *                      NOTE: these characters are inserted into a regex range (square brackets) and
 *                            therefore may need to be regex-escaped. It is the responsibility of
 *                            the user to pass a string that will work correctly in a regex range.
 */
inline void trim(Glib::ustring &input, Glib::ustring const &also_remove = "")
{
    auto const regex = Glib::Regex::create(Glib::ustring("^[\\s") + also_remove + "]*(.+?)[\\s"
                                                  + also_remove + "]*$");
    Glib::MatchInfo match_info;
    regex->match(input, match_info);
    if (!match_info.matches()) {
        input.clear();
        return;
    }
    input = match_info.fetch(1);
}

} // namespace Util
} // namespace Inkscape

#endif // SEEN_TRIM_H
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

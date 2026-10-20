// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Join lists of elements, while respecting internationalization.
 *
 * Authors:
 * Copyright (C) 2026 Michael Terry
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cassert>

#include <glibmm/i18n.h>

#include "join.h"

namespace Inkscape {
namespace Util {

/**
 * Join a set of items with a language-appropriate separator (think comma for English).
 *
 * Some languages use other separators ("、" in CJK). Some may use other rules (like a string
 * before each item, not just between internal items - I think Arabic might do that using "و").
 *
 * - An empty set will return an empty string.
 * - A set with one element will return just that element.
 * - The list will be joined in sort-order.
 * - This method does NOT add a joiner word like "and".
 *
 * Warning: This only handles up to a specific number of elements currently. Do not use with
 *          collections of unknown or arbitrary size.
 *
 * Example call for English locale:
 *  set("apple", "grape", "banana") => "apple, banana, grape"
 *
 * @param items strings to join
 */
Glib::ustring join_with_separator(std::set<Glib::ustring> const &items)
{
    auto len = items.size();
    auto iter = items.begin();
    if (len == 0) {
        return "";
    } else if (len == 1) {
        return *iter;
    } else if (len == 2) {
        auto a = *iter++;
        auto b = *iter++;
        // Translators: this is a list of two elements
        return Glib::ustring::compose(_("%1, %2"), a, b);
    } else if (len == 3) {
        auto a = *iter++;
        auto b = *iter++;
        auto c = *iter++;
        // Translators: this is a list of three elements
        return Glib::ustring::compose(_("%1, %2, %3"), a, b, c);
    } else if (len == 4) {
        auto a = *iter++;
        auto b = *iter++;
        auto c = *iter++;
        auto d = *iter++;
        // Translators: this is a list of four elements
        return Glib::ustring::compose(_("%1, %2, %3, %4"), a, b, c, d);
    } else if (len == 5) {
        auto a = *iter++;
        auto b = *iter++;
        auto c = *iter++;
        auto d = *iter++;
        auto e = *iter++;
        // Translators: this is a list of five elements
        return Glib::ustring::compose(_("%1, %2, %3, %4, %5"), a, b, c, d, e);
    } else if (len == 6) {
        auto a = *iter++;
        auto b = *iter++;
        auto c = *iter++;
        auto d = *iter++;
        auto e = *iter++;
        auto f = *iter++;
        // Translators: this is a list of six elements
        return Glib::ustring::compose(_("%1, %2, %3, %4, %5, %6"), a, b, c, d, e, f);
    } else {
        // If you need to handle larger sets, add more translation lines above.
        assert(false);
        return "";
    }
}

} // namespace Util
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

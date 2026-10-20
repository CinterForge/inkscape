// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Parse a string containing number ranges.
 *
 * Copyright (C) 2022 Martin Owens
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef INKSCAPE_UTIL_PARSE_INT_RANGE_H
#define INKSCAPE_UTIL_PARSE_INT_RANGE_H

#include <set>
#include <string>

namespace Inkscape {

/**
 * Parse integer ranges out of a string.
 *
 * @param input - A string containing number ranges that can either be comma
 *                separated or dash separated for non and continuous ranges.
 * @param start - Optional first number in the acceptable range.
 * @param end - The last number in the acceptable range.
 *
 * @returns a sorted set of unique numbers.
 */
std::set<unsigned> parseIntRange(std::string const &input, unsigned start = 1, unsigned end = 0);

} // namespace Inkscape

#endif // INKSCAPE_UTIL_PARSE_INT_RANGE_H

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

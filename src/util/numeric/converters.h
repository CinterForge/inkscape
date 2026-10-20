// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Utility functions to convert ascii representations to numbers
 */
#ifndef UTIL_NUM_CONVERTERS_H
#define UTIL_NUM_CONVERTERS_H
/*
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *   Mohammad Aadil Shabier
 *
 *
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <iostream>
#include <cstring>
#include <sstream>
#include <vector>

namespace Inkscape {
namespace Util {

// while calling read_number(string, false), it's not obvious, what
// that false stands for. read_number(string, NO_WARNING)
// can be more explicit.
constexpr bool NO_WARNING = false;

/* convert ascii representation to double
 * the function can only be used to convert numbers as given by gui elements that use localized representation
 * @param value ascii representation of the number
 * @return the converted number
 *
 * Setting warning to false disables conversion error warnings from
 * this function. This can be useful in places, where the input type
 * is not known beforehand. For example, see sp_feColorMatrix_set in
 * sp-fecolormatrix.cpp
 * Consider making warning = True by default since that's how it seems to be used everywhere?
 */
inline double read_number(gchar const *value, bool warning = true)
{
    if (!value) {
        g_warning("Called Inkscape::Util::read_number with value==null_ptr, this can lead to unexpected behaviour.");
        return 0;
    }
    char *end;
    double ret = g_ascii_strtod(value, &end);
    if (*end) {
        if (warning) {
            g_warning("Inkscape::Util::read_number() Unable to convert \"%s\" to number", value);
        }
        // We could leave this out, too. If strtod can't convert
        // anything, it will return zero.
        ret = 0;
    }
    return ret;
}

inline bool read_bool(gchar const *value, bool default_value)
{
    if (!value)
        return default_value;
    switch (value[0]) {
        case 't':
            if (strncmp(value, "true", 4) == 0)
                return true;
            break;
        case 'f':
            if (strncmp(value, "false", 5) == 0)
                return false;
            break;
    }
    return default_value;
}

/* convert ascii representation to double
 * the function can only be used to convert numbers as given by gui elements that use localized representation
 * numbers are delimited by space
 * @param value ascii representation of the number
 * @return the vector of the converted numbers
 */
inline std::vector<gdouble> read_vector(const gchar *value)
{
    std::vector<gdouble> v;

    gchar const *beg = value;
    while (isspace(*beg) || (*beg == ','))
        beg++;
    while (*beg) {
        char *end;
        double ret = g_ascii_strtod(beg, &end);
        if (end == beg) {
            g_warning("Inkscape::Util::read_vector() Unable to convert \"%s\" to number", beg);
            break;
        }
        v.push_back(ret);

        beg = end;
        while (isspace(*beg) || (*beg == ','))
            beg++;
    }
    return v;
}

/*
 * Format a number with any trailing zeros removed.
 */
inline std::string format_number(double val, unsigned int precision = 3)
{
    std::ostringstream out;
    out.imbue(std::locale("C"));
    out.precision(precision);
    out << std::fixed << val;
    std::string ret = out.str();

    while(ret.find(".") != std::string::npos
        && (ret.substr(ret.length() - 1, 1) == "0"
         || ret.substr(ret.length() - 1, 1) == "."))
        ret.pop_back();

    return ret;
}


} // namespace Util
} // namespace Inkscape

#endif // UTIL_NUM_CONVERTERS_H

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

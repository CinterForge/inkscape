// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2021 Martin Owens
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstring>
#include <string>
#include <glib.h>

#include "svg/svg-bool.h"

/**
 * This boolean is not an SVG specification type, but it is something
 * Inkscape uses for many of it's internal values.
 */
SVGBool::SVGBool(bool default_value)
    : _default(default_value)
{}

bool SVGBool::read(gchar const *str)
{
    if (!str) return false;

    _is_set = true;
    _value = !g_ascii_strcasecmp(str, "true") ||
             !g_ascii_strcasecmp(str, "yes") ||
             !g_ascii_strcasecmp(str, "y") ||
             (atoi(str) != 0);

    return true;
}

void SVGBool::unset() {
    _is_set = false;
}

void SVGBool::readOrUnset(gchar const *str) {
    if (!read(str)) {
        unset();
    }
}

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

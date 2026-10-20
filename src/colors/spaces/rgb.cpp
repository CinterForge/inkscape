// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2023 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "rgb.h"

#include <sstream>
#include <string>

#include "colors/cms/profile.h"
#include "colors/color.h"
#include "colors/utils.h"

namespace Inkscape::Colors::Space {

/**
 * Print the RGB color to a CSS Hex code of 6 or 8 digits.
 *
 * @arg values - A vector of doubles for each channel in the RGB space
 * @arg opacity - True if the opacity should be included in the output.
 */
std::string RGB::toString(std::vector<double> const &values, bool opacity) const
{
    return rgba_to_hex(toRGBA(values), values.size() == 4 && opacity);
}

/**
 * Parse RGB css values
 */
bool RGB::Parser::parse(std::istringstream &ss, std::vector<double> &output) const
{
    // Modern CSS syntax
    char sep0 = 0x0;
    char sep1 = '/'; // alpha separator
    int max_count = 4;

    // Legacy CSS syntax
    if (ss.str().find(',') != std::string::npos) {
        sep0 = ',';
        sep1 = ',';
	max_count = _alpha ? 4 : 3; // rgb() must have 3 and rgba() 4
    }

    bool end = false;
    while (!end && output.size() < max_count) {
        if (!append_css_value(ss, output, end, output.size() == 2 ? sep1 : sep0, output.size() == 3 ? 1.0 : 255.0))
            break;
    }
    return end;
}

} // namespace Inkscape::Colors::Space

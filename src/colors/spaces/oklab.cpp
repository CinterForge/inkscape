// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 *//*
 * Authors:
 *   Rafał Siejakowski <rs@rs-math.net>
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "oklab.h"

#include <array>
#include <cmath>

#include "colors/printer.h"

namespace Inkscape::Colors::Space {

bool OkLab::Parser::parse(std::istringstream &ss, std::vector<double> &output) const
{
    bool end = false;
    if (append_css_value(ss, output, end, ',')               // Luminance
        && append_css_value(ss, output, end, ',', MAX_SCALE) // Chroma A
        && append_css_value(ss, output, end, '/', MAX_SCALE) // Chroma B
        && (append_css_value(ss, output, end) || true)       // Optional opacity
        && end) {
        // Values are between -100% and 100% so post processed into the range 0 to 1
        output[1] = (output[1] + 1) / 2;
        output[2] = (output[2] + 1) / 2;
        return true;
    }
    return false;
}

/**
 * Print the Lab color to a CSS string.
 *
 * @arg values - A vector of doubles for each channel in the Lch space
 * @arg opacity - True if the opacity should be included in the output.
 */
std::string OkLab::toString(std::vector<double> const &values, bool opacity) const
{
    auto os = CssFuncPrinter(3, "oklab");

    os << values[0]                                  // Luminance
       << SCALE_UP(values[1], MIN_SCALE, MAX_SCALE)  // Chroma A
       << SCALE_UP(values[2], MIN_SCALE, MAX_SCALE); // Chroma B

    if (opacity && values.size() == 4)
        os << values[3]; // Optional opacity

    return os;
}

}; // namespace Inkscape::Colors::Space

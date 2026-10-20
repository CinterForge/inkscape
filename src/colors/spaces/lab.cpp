// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 *//*
 * Authors:
 *   2023 Martin Owens
 *
 * Copyright (C) 2023 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "lab.h"

#include <cmath>

#include "colors/color.h"
#include "colors/printer.h"

namespace Inkscape::Colors::Space {

/**
 * Print the Lab color to a CSS string.
 *
 * @arg values - A vector of doubles for each channel in the Lab space
 * @arg opacity - True if the opacity should be included in the output.
 */
std::string Lab::toString(std::vector<double> const &values, bool opacity) const
{
    auto os = CssFuncPrinter(3, "lab");

    os << values[0] * LUMA_SCALE                             // Luminance
       << SCALE_UP(values[1], MIN_SCALE, MAX_SCALE)  // Chroma A
       << SCALE_UP(values[2], MIN_SCALE, MAX_SCALE); // Chroma B

    if (opacity && values.size() == 4)
        os << values[3]; // Optional opacity

    return os;
}

bool Lab::Parser::parse(std::istringstream &ss, std::vector<double> &output) const
{
    // CSS Color Module 4 defines 100% as 125 for A/B in lab
    static double CSS_PERCENT_SCALE = (100.0 / 125.0);

    bool end = false;
    if (append_css_value(ss, output, end, ',', LUMA_SCALE)                  // Lightness
        && append_css_value(ss, output, end, ',', 1.0, CSS_PERCENT_SCALE) // Chroma-A
        && append_css_value(ss, output, end, '/', 1.0, CSS_PERCENT_SCALE) // Chroma-B
        && (append_css_value(ss, output, end) || true)                      // Optional opacity
        && end) {
        // The A and B portions are between -128 and +127 which is hard for append_css_value to convert
        output[1] = SCALE_DOWN(output[1], MIN_SCALE, MAX_SCALE);
        output[2] = SCALE_DOWN(output[2], MIN_SCALE, MAX_SCALE);
        return true;
    }
    return false;
}

}; // namespace Inkscape::Colors::Space

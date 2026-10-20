// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2023 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "hsv.h"


#include "colors/color.h"
#include "colors/printer.h"

namespace Inkscape::Colors::Space {

/**
 * Parse the hwb css string and convert to hsv inline, if it exists in the input string stream.
 */
bool HSV::fromHwbParser::parse(std::istringstream &ss, std::vector<double> &output) const
{
    if (HueParser::parse(ss, output)) {
        // See https://en.wikipedia.org/wiki/HWB_color_model#Converting_to_and_from_HSV
        auto scale = output[1] + output[2];
        if (scale > 1.0) {
            output[1] /= scale;
            output[2] /= scale;
        }
        output[1] = output[2] == 1.0 ? 0.0 : (1.0 - (output[1] / (1.0 - output[2])));
        output[2] = 1.0 - output[2];
        return true;
    }
    return false;
}

/**
 * Print the HSV color to a CSS hwb() string.
 *
 * @arg values - A vector of doubles for each channel in the HSV space
 * @arg opacity - True if the opacity should be included in the output.
 */
std::string HSV::toString(std::vector<double> const &values, bool opacity) const
{
    static constexpr double CSS_WB_SCALE = 100.0;
    auto oo = CssFuncPrinter(3, "hwb");

    // First entry is Hue, which is in degrees, white and black are derived
    oo << (int)(values[0] * 360)                         // Hue, degrees, 0..360
       << ((1.0 - values[1]) * values[2]) * CSS_WB_SCALE // White,        0..100
       << (1.0 - values[2]) * CSS_WB_SCALE;              // Black,        0..100

    if (opacity && values.size() == 4)
      oo << values[3]; // Optional opacity

    return oo;
}

}; // namespace Inkscape::Colors::Space

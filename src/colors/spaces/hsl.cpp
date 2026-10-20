// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2023 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "hsl.h"

#include <cmath>

#include "colors/color.h"
#include "colors/printer.h"

namespace Inkscape::Colors::Space {

/**
 * Print the HSL color to a CSS string.
 *
 * @arg values - A vector of doubles for each channel in the HSL space
 * @arg opacity - True if the opacity should be included in the output.
 */
std::string HSL::toString(std::vector<double> const &values, bool opacity) const
{
    static constexpr double CSS_SL_SCALE = 100.0;
    auto oo = CssLegacyPrinter(3, "hsl", opacity && values.size() == 4);
    return oo << (int)(values[0] * 360)          // hue, in degrees
              << values[1] * CSS_SL_SCALE << "%" // saturation, as percentage
              << values[2] * CSS_SL_SCALE << "%" // lightness, as percentage
              << values.back();                  // optional alpha
}

}; // namespace Inkscape::Colors::Space

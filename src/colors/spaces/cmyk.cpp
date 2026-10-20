// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * DeviceCMYK is NOT a color managed color space for ink values, for those
 * please see the CMS icc profile based color spaces.
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2023 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "cmyk.h"

#include "colors/color.h"
#include "colors/printer.h"

namespace Inkscape::Colors::Space {

/**
 * Print the DeviceCMYK color to a CSS Color Module Level 5 string.
 *
 * @arg values - A vector of doubles for each channel in the DeviceCMYK space
 * @arg opacity - True if the opacity should be included in the output.
 */
std::string DeviceCMYK::toString(std::vector<double> const &values, bool opacity) const
{
    auto os = CssFuncPrinter(4, "device-cmyk");
    os << values;
    if (opacity && values.size() == 5)
        os << values[4];
    return os;
}

/**
 * Return true if, using a rough hueruistic, this color could be considered to be using
 * too much ink if it was printed using the ink as specified.
 *
 * @arg input - Channel values in this space.
 */
bool DeviceCMYK::overInk(std::vector<double> const &input) const
{
    if (!input.size())
        return false;
    // Over 320% is considered over inked, see cms.cpp for details.
    return input[0] + input[1] + input[2] + input[3] > 3.2;
}

}; // namespace Inkscape::Colors::Space

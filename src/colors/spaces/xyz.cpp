// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 *//*
 * Authors:
 *   2015 Alexei Boronine (original idea, JavaScript implementation)
 *   2015 Roger Tallada (Obj-C implementation)
 *   2017 Martin Mitas (C implementation, based on Obj-C implementation)
 *   2021 Massinissa Derriche (C++ implementation for Inkscape, based on C implementation)
 *   2023 Martin Owens (New Color classes)
 *
 * Copyright (C) 2023 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "xyz.h"

#include <cmath>

#include "colors/printer.h"

namespace Inkscape::Colors::Space {

/**
 * Print the color to a CSS Color module 4 xyz-d65 color.
 *
 * @arg values - A vector of doubles for each channel in the xyz space
 * @arg opacity - True if the opacity should be included in the output.
 */
std::string XYZ::toString(std::vector<double> const &values, bool opacity) const
{
    auto os = CssColorPrinter(3, "xyz");
    os << values;
    if (opacity && values.size() == 4)
        os << values[3];
    return os;
}

std::string XYZ50::toString(std::vector<double> const &values, bool opacity) const
{
    auto os = CssColorPrinter(3, "xyz-d50");
    os << values;
    if (opacity && values.size() == 4)
        os << values[3];
    return os;
}

/**
 * Return the XYZ D50 color profile
 */
std::shared_ptr<Inkscape::Colors::CMS::Profile> const XYZ50::getProfile() const
{
    static std::shared_ptr<Colors::CMS::Profile> xyz_profile = Colors::CMS::Profile::create_xyz50();
    return xyz_profile;
}

}; // namespace Inkscape::Colors::Space

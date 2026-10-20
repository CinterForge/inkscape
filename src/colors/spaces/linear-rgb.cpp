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

#include "linear-rgb.h"


#include "colors/printer.h"

namespace Inkscape::Colors::Space {

/**
 * Return the RGB color profile, this is static for all RGB sub-types
 */
std::shared_ptr<Inkscape::Colors::CMS::Profile> const LinearRGB::getProfile() const
{
    static std::shared_ptr<Colors::CMS::Profile> linearrgb_profile = Colors::CMS::Profile::create_linearrgb();
    return linearrgb_profile;
}

/**
 * Print the RGB color to a CSS Color module 4 srgb-linear color.
 *
 * @arg values - A vector of doubles for each channel in the RGB space
 * @arg opacity - True if the opacity should be included in the output.
 */
std::string LinearRGB::toString(std::vector<double> const &values, bool opacity) const
{
    auto os = CssColorPrinter(3, "srgb-linear");
    os << values;
    if (opacity && values.size() == 4)
        os << values.back();
    return os;
}

}; // namespace Inkscape::Colors::Space

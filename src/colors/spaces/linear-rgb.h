// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLORS_SPACES_LINEARRGB_H
#define SEEN_COLORS_SPACES_LINEARRGB_H

#include "base.h"

#include <cmath>

namespace Inkscape::Colors::Space {

class LinearRGB : public ProfileSpace<true>
{
public:
    LinearRGB(): ProfileSpace(Type::linearRGB, 3, "linearRGB", "linearRGB", "color-selector-linear-rgb") {
        _svgNames.emplace_back("linearRGB");
        _svgNames.emplace_back("srgb-linear");
        _intent = RenderingIntent::RELATIVE_COLORIMETRIC;
        _intent_priority = 10;
    }
    ~LinearRGB() override = default;

    std::shared_ptr<Colors::CMS::Profile> const getProfile() const override;

    std::string toString(std::vector<double> const &values, bool opacity = true) const override;

    // NOTE: the following conversions are used by OkLab to avoid being based in the linearRGB icc profile
    /**
     * Convert a color from the a linear RGB colorspace to the sRGB colorspace.
     *
     * @param in_out[in,out] The linear RGB color converted to a RGB color.
     */
    template <typename T>
    inline static void toRGB(T const *i, T *o)
    {
        auto from_linear = [](double c)
        {
            return c <= 0.0031308 ? 12.92 * c : 1.055 * std::pow(c, 1.0 / 2.4) - 0.055;
        };
        o[0] = from_linear(i[0]);
        o[1] = from_linear(i[1]);
        o[2] = from_linear(i[2]);
    }

    /**
     * Convert from sRGB icc values to linear RGB values
     *
     * @param in_out[in,out] The RGB color converted to a linear RGB color.
     */
    template <typename T>
    inline static void fromRGB(T const *i, T *o)
    {
        auto to_linear = [](double c)
        {
            return c > 0.04045 ? std::pow((c + 0.055) / 1.055, 2.4) : c / 12.92;
        };
        o[0] = to_linear(i[0]);
        o[1] = to_linear(i[1]);
        o[2] = to_linear(i[2]);
    }

};

} // namespace Inkscape::Colors::Space

#endif // SEEN_COLORS_SPACES_LINEARRGB_H

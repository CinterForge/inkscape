// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLORS_SPACES_HSV_H
#define SEEN_COLORS_SPACES_HSV_H

#include <cmath>

#include "rgb.h"

namespace Inkscape::Colors::Space {

class HSV : public RGBBase<HSV>
{
public:
    constexpr static int ProfileChannels = 3;
    constexpr static int OutputChannels = 3;

    HSV(): RGBBase(Type::HSV, "HSV", "HSV", "color-selector-hsx") {}
    ~HSV() override = default;

    /**
     * Convert the HSV color into sRGB components used in the sRGB icc profile.
     */
    template<typename T, bool PremultipliedAlpha = false>
    inline static void spaceToProfile(T const *i, T *o)
    {
        double d;
        double e;
        double v;

        if constexpr (PremultipliedAlpha) {
            d = i[0] * 5.99999999 / i[3];
            e = i[1] / i[3];
            v = i[2] / i[3];
            o[3] = i[3];
        } else {
            d = i[0] * 5.99999999;
            e = i[1];
            v = i[2];
        }
        double f = d - std::floor(d);
        double w = v * (1.0 - i[1]);
        double q = v * (1.0 - (i[1] * f));
        double t = v * (1.0 - (i[1] * (1.0 - f)));

        if (d < 1.0) {
            o[0] = v;
            o[1] = t;
            o[2] = w;
        } else if (d < 2.0) {
            o[0] = q;
            o[1] = v;
            o[2] = w;
        } else if (d < 3.0) {
            o[0] = w;
            o[1] = v;
            o[2] = t;
        } else if (d < 4.0) {
            o[0] = w;
            o[1] = q;
            o[2] = v;
        } else if (d < 5.0) {
            o[0] = t;
            o[1] = w;
            o[2] = v;
        } else {
            o[0] = v;
            o[1] = w;
            o[2] = q;
        }
    }

    /**
     * Convert from sRGB icc values to HSV values
     */
    template<typename T, bool PremultiplyAlpha = false>
    inline static void profileToSpace(T const *i, T *o)
    {
        double r = i[0];
        double g = i[1];
        double b = i[2];

        double max = std::max(std::max(r, g), b);
        double min = std::min(std::min(r, g), b);
        double delta = max - min;

        o[2] = max;
        o[1] = max > 0 ? delta / max : 0.0;

        if (o[1] != 0.0) {
            if (r == max) {
                o[0] = (g - b) / delta;
            } else if (g == max) {
                o[0] = 2.0 + (b - r) / delta;
            } else {
                o[0] = 4.0 + (r - g) / delta;
            }
            o[0] = o[0] / 6.0;
            if (o[0] < 0)
                o[0] += 1.0;
        } else
            o[0] = 0.0;

        if constexpr (PremultiplyAlpha) {
            o[0] *= i[3];
            o[1] *= i[3];
            o[2] *= i[3];
            o[3] = i[3];
        }
    }

    std::string toString(std::vector<double> const &values, bool opacity) const override;

    class fromHwbParser : public HueParser
    {
    public:
        fromHwbParser(bool alpha)
	     : HueParser("hwb", Space::Type::HSV, alpha, 100.0)
        {}
        bool parse(std::istringstream &input, std::vector<double> &output) const override;
    };
};

} // namespace Inkscape::Colors::Space

#endif // SEEN_COLORS_SPACES_HSV_H

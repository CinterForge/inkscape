// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLORS_SPACES_HSL_H
#define SEEN_COLORS_SPACES_HSL_H

#include "rgb.h"

namespace Inkscape::Colors::Space {

static float hue_2_rgb(float v1, float v2, float h)
{
    if (h < 0)
        h += 6.0;
    if (h > 6)
        h -= 6.0;
    if (h < 1)
        return v1 + (v2 - v1) * h;
    if (h < 3)
        return v2;
    if (h < 4)
        return v1 + (v2 - v1) * (4 - h);
    return v1;
}

class HSL : public RGBBase<HSL>
{
public:
    constexpr static int OutputChannels = 3;
    constexpr static int ProfileChannels = 3;

    HSL(): RGBBase(Type::HSL, "HSL", "HSL", "color-selector-hsx") {
        _svgNames.emplace_back("hsl");
    }
    ~HSL() override = default;

    /**
     * Convert the HSL color into sRGB components used in the sRGB icc profile.
     */
    template<typename T, bool PremultipliedAlpha = false>
    inline static void spaceToProfile(T const *i, T *o)
    {
        double h;
        double s;
        double l;

        if constexpr (PremultipliedAlpha) {
            h = i[0] / i[3];
            s = i[1] / i[3];
            l = i[2] / i[3];
            o[3] = i[3];
        } else {
            h = i[0];
            s = i[1];
            l = i[2];
        }

        if (s == 0) { // Gray
            o[0] = l;
            o[1] = l;
            o[2] = l;
        } else {
            double v2;
            if (l < 0.5) {
                v2 = l * (1 + s);
            } else {
                v2 = l + s - l * s;
            }
            double v1 = 2 * l - v2;

            o[0] = hue_2_rgb(v1, v2, h * 6 + 2.0);
            o[1] = hue_2_rgb(v1, v2, h * 6);
            o[2] = hue_2_rgb(v1, v2, h * 6 - 2.0);
        }
    }

    /**
     * Convert from sRGB icc values to HSL values
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

        double h = 0;
        double s = 0;
        double l = (max + min) / 2.0;

        if (delta != 0) {
            if (l <= 0.5)
                s = delta / (max + min);
            else
                s = delta / (2 - max - min);

            if (r == max)
                h = (g - b) / delta;
            else if (g == max)
                h = 2.0 + (b - r) / delta;
            else if (b == max)
                h = 4.0 + (r - g) / delta;

            h = h / 6.0;

            if (h < 0)
                h += 1;
            if (h > 1)
                h -= 1;
        }
        if constexpr (PremultiplyAlpha) {
            o[0] = h * i[3];
            o[1] = s * i[3];
            o[2] = l * i[3];
            o[3] = i[3];
        } else {
            o[0] = h;
            o[1] = s;
            o[2] = l;
        }
    }

    std::string toString(std::vector<double> const &values, bool opacity = true) const override;

    class Parser : public HueParser
    {
    public:
        Parser(bool alpha)
	     : HueParser("hsl", Type::HSL, alpha, 100.0)
        {}
    };
};

} // namespace Inkscape::Colors::Space

#endif // SEEN_COLORS_SPACES_HSL_H

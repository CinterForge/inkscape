// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Mike Kowalski
 *
 * Copyright (C) 2025 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLORS_SPACES_OKHSV_H
#define SEEN_COLORS_SPACES_OKHSV_H

#include "oklab.h"

#include "colors/spaces/ok-color.h"

namespace Inkscape::Colors::Space {

class OkHsv : public RGBBase<OkHsv>
{
public:
    constexpr static int ProfileChannels = 3;
    constexpr static int OutputChannels = 3;

    OkHsv(): RGBBase(Type::OKHSV, "OkHsv", "OkHsv", "color-selector-okhsv") {}
    ~OkHsv() override = default;

    std::string toString(std::vector<double> const &values, bool opacity) const override { return ""; }

    template <typename T, bool PremultipliedAlpha = false>
    inline static void spaceToProfile(T const *i, T *o)
    {
        float h, s, v;
        if constexpr (PremultipliedAlpha) {
            h = static_cast<float>(std::clamp((double)i[0] / i[3], 0.0, 1.0));
            s = static_cast<float>(std::clamp((double)i[1] / i[3], 0.0, 1.0));
            v = static_cast<float>(std::clamp((double)i[2] / i[3], 0.0, 1.0));
            o[3] = i[3];
        } else {
            h = static_cast<float>(std::clamp((double)i[0], 0.0, 1.0));
            s = static_cast<float>(std::clamp((double)i[1], 0.0, 1.0));
            v = static_cast<float>(std::clamp((double)i[2], 0.0, 1.0));
        }

        ok_color::HSV hsv{h, s, v};
        auto rgb = ok_color::okhsv_to_srgb(hsv);
        o[0] = rgb.r;
        o[1] = rgb.g;
        o[2] = rgb.b;
    }

    template <typename T, bool PremultiplyAlpha = false>
    inline static void profileToSpace(T const *i, T *o)
    {
        ok_color::RGB rgb{(float)i[0], (float)i[1], (float)i[2]};
        auto hsv = ok_color::srgb_to_okhsv(rgb);

        if constexpr (PremultiplyAlpha) {
            o[0] = hsv.h * i[3];
            o[1] = hsv.s * i[3];
            o[2] = hsv.v * i[3];
            o[3] = i[3];
        } else {
            o[0] = hsv.h;
            o[1] = hsv.s;
            o[2] = hsv.v;
        }
    }

};

} // namespace Inkscape::Colors::Space

#endif // SEEN_COLORS_SPACES_OKHSV_H

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Rafał Siejakowski <rs@rs-math.net>
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLORS_SPACES_OKHSL_H
#define SEEN_COLORS_SPACES_OKHSL_H

#include <2geom/angle.h>

#include "oklab.h"
#include "oklch.h" // max_chroma

namespace Inkscape::Colors::Space {

class OkHsl : public RGBBase<OkHsl>
{
public:
    constexpr static int ProfileChannels = 3;
    constexpr static int OutputChannels = 3;

    OkHsl(): RGBBase(Type::OKHSL, "OkHsl", "OkHsl", "color-selector-okhsl", true) {}
    ~OkHsl() override = default;

    template<typename T, bool PremultipliedAlpha = false>
    inline static void spaceToProfile(T const *i, T *o)
    {
        double h, s, l;
        if constexpr (PremultipliedAlpha) {
            h = i[0] / i[3];
            s = i[1] / i[3];
            l = std::clamp((double)i[2] / i[3], 0.0, 1.0);
            o[3] = i[3];
        } else {
            h = i[0];
            s = i[1];
            l = std::clamp((double)i[2], 0.0, 1.0);
        }

        // Get max chroma for this hue and lightness and compute the absolute chroma.
        double const chromax = OkLch::max_chroma(l, h * 360.0);
        double const absolute_chroma = s * chromax;

        // Convert hue and chroma to the Cartesian a, b coordinates.
        double a, b;
        Geom::sincos(h * 2.0 * M_PI, b, a);
        o[0] = l;
        o[1] = a * absolute_chroma;
        o[2] = b * absolute_chroma;

        OkLab::toLinearRGB(o, o);
        LinearRGB::toRGB(o, o);
    }

    template<typename T, bool PremultiplyAlpha = false>
    inline static void profileToSpace(T const *i, T *o)
    {
        LinearRGB::fromRGB(i, o);
        OkLab::fromLinearRGB(o, o);

        // Compute the chroma.
        double const absolute_chroma = std::hypot(o[1], o[2]);
        if (absolute_chroma < 1e-7) {
            // It would be numerically unstable to calculate the hue for this
            // color, so we set the hue and saturation to zero (grayscale color).
            o[2] = o[0];
            o[1] = 0.0;
            o[0] = 0.0;
        } else {
            // Compute the hue (in the unit interval).
            Geom::Angle const hue_angle = std::atan2(o[2], o[1]);
            o[2] = std::clamp((double)o[0], 0.0, 1.0);
            o[0] = hue_angle.radians0() / (2.0 * M_PI);

            // Compute the linear saturation.
            double const hue_degrees = Geom::deg_from_rad(hue_angle.radians0());
            double const chromax = OkLch::max_chroma(o[2], hue_degrees);
            o[1] = (chromax == 0.0) ? 0.0 : std::clamp(absolute_chroma / chromax, 0.0, 1.0);
        }

        if constexpr (PremultiplyAlpha) {
            o[0] *= i[3];
            o[1] *= i[3];
            o[2] *= i[3];
            o[3] = i[3];
        }
    }

    std::string toString(std::vector<double> const &values, bool opacity) const override { return ""; }
};

} // namespace Inkscape::Colors::Space

#endif // SEEN_COLORS_SPACES_OKHSL_H

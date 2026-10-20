// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLORS_SPACES_OKLCH_H
#define SEEN_COLORS_SPACES_OKLCH_H

#include <2geom/angle.h>

#include "oklab.h"

namespace Inkscape::Colors::Space {

class OkLch : public RGBBase<OkLch>
{
public:
    constexpr static int ProfileChannels = 3;
    constexpr static int OutputChannels = 3;

    constexpr static double HUE_SCALE = 360;

    OkLch(): RGBBase(Type::OKLCH, "OkLch", "OkLch", "color-selector-oklch") {
        _svgNames.emplace_back("oklch");
    }
    ~OkLch() override = default;

    template<typename T, bool PremultipliedAlpha = false>
    inline static void spaceToProfile(T const *i, T *o)
    {
        // Convert a color from the the OkLch colorspace to the OKLab colorspace.
        // c and h are polar coordinates; convert to Cartesian a, b coords.
        double c, h;

        if constexpr (PremultipliedAlpha) {
            o[0] = i[0] / i[3];
            c    = i[1] / i[3];
            h    = i[2] / i[3];
            o[3] = i[3];
        } else {
            o[0] = i[0];
            c    = i[1];
            h    = i[2];
        }
        double a, b;
        Geom::sincos(Geom::Angle::from_degrees(h * HUE_SCALE), a, b);
        o[1] = b * c;
        o[2] = a * c;

        OkLab::toLinearRGB(o, o);
        LinearRGB::toRGB(o, o);
    }
    template<typename T, bool PremultiplyAlpha = false>
    inline static void profileToSpace(T const *i, T *o)
    {
        LinearRGB::fromRGB(i, o);
        OkLab::fromLinearRGB(o, o);

        // Convert a, b to polar coordinates c, h.
        double c = std::hypot(o[1], o[2]);
        if (c > 0.001) {
            Geom::Angle const hue_angle = std::atan2(o[2], o[1]);
            o[2] = Geom::deg_from_rad(hue_angle.radians0()) / HUE_SCALE;
        } else {
            o[2] = 0;
        }

        if constexpr (PremultiplyAlpha) {
            o[0] *= i[3];
            o[1] = c * i[3];
            o[2] *= i[3];
            o[3] = i[3];
        } else {
            o[1] = c;
        }
    }

    std::string toString(std::vector<double> const &values, bool opacity) const override;

public:
    class Parser : public Colors::Parser
    {
    public:
        Parser()
            : Colors::Parser("oklch", Type::OKLCH)
        {}
        bool parse(std::istringstream &input, std::vector<double> &output) const override;
    };

    static double max_chroma(double l, double h);
};

uint8_t const *render_hue_scale(double s, double l, std::array<uint8_t, 4 * 1024> *map);
uint8_t const *render_saturation_scale(double h, double l, std::array<uint8_t, 4 * 1024> *map);
uint8_t const *render_lightness_scale(double h, double s, std::array<uint8_t, 4 * 1024> *map);

} // namespace Inkscape::Colors::Space

#endif // SEEN_COLORS_SPACES_OKLCH_H

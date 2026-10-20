// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLORS_SPACES_HSLUV_H
#define SEEN_COLORS_SPACES_HSLUV_H

#include <2geom/line.h>
#include <2geom/ray.h>

#include "luv.h"

#include <2geom/line.h>
#include <2geom/ray.h>

namespace Inkscape::Colors::Space {

double max_chroma_for_lh(double l, double h);

class HSLuv : public XYZBase<HSLuv>
{
public:
    constexpr static int ProfileChannels = 3;
    constexpr static int OutputChannels = 3;

    HSLuv(): XYZBase(Type::HSLUV, "HSLuv", "HSLuv", "color-selector-hsluv") {}
    ~HSLuv() override = default;

    /**
     * Convert a color from the the HSLuv colorspace to the XYZ65 colorspace
     */
    template<typename T, bool PremultipliedAlpha = false>
    inline static void spaceToProfile(T const *i, T *o)
    {
        double h;
        double s;
        double l;
        double c;

        if constexpr (PremultipliedAlpha) {
            h = i[0] * 360 / i[3];
            s = i[1] * 100 / i[3];
            l = i[2] * 100 / i[3];
            o[3] = i[3];
        } else {
            h = i[0] * 360;
            s = i[1] * 100;
            l = i[2] * 100;
        }

        /* White and black: disambiguate chroma */
        if (l > 99.9999999 || l < 0.00000001) {
            c = 0.0;
        } else {
            c = max_chroma_for_lh(l, h) / 100.0 * s;
        }

        /* Grays: disambiguate hue */
        if (s < 0.00000001) {
            h = 0.0;
        }

        double sinhrad, coshrad;
        Geom::sincos(Geom::rad_from_deg(h), sinhrad, coshrad);
        double u = coshrad * c;
        double v = sinhrad * c;

        o[0] = l;
        o[1] = u;
        o[2] = v;

        Luv::toXYZ(o, o);
    }

    /**
     * Convert a color from the the XYZ65 colorspace to the HSLuv colorspace
     */
    template<typename T, bool PremultiplyAlpha = false>
    inline static void profileToSpace(T const *i, T *o)
    {
        Luv::fromXYZ(i, o);

        double l = o[0];
        auto uv = Geom::Point(o[1], o[2]);
        double h;
        double const c = uv.length();

        /* Grays: disambiguate hue */
        if (c < 0.00000001) {
            h = 0;
        } else {
            h = Geom::deg_from_rad(Geom::atan2(uv));
            if (h < 0.0) {
                h += 360.0;
            }
        }

        double s;

        /* White and black: disambiguate saturation */
        if (l > 99.9999999 || l < 0.00000001) {
            s = 0.0;
        } else {
            s = c / max_chroma_for_lh(l, h) * 100.0;
        }

        if constexpr (PremultiplyAlpha) {
            o[0] = h / 360 * i[3];
            o[1] = s / 100 * i[3];
            o[2] = l / 100 * i[3];
            o[3] = i[3];
        } else {
            o[0] = h / 360;
            o[1] = s / 100;
            o[2] = l / 100;
        }
    }

    std::string toString(std::vector<double> const &values, bool opacity) const override { return ""; }

    static std::array<Geom::Line, 6> get_bounds(double l);
};

} // namespace Inkscape::Colors::Space

#endif // SEEN_COLORS_SPACES_HSLUV_H

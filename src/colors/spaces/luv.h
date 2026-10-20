// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLORS_SPACES_LUV_H
#define SEEN_COLORS_SPACES_LUV_H

#include "xyz.h"

namespace Inkscape::Colors::Space {

// CIE LUV constants
constexpr double KAPPA = 903.29629629629629629630;
constexpr double EPSILON = 0.00885645167903563082;

double l2y(double l);
double y2l(double y);

class Luv : public XYZBase<Luv>
{
public:
    constexpr static int OutputChannels = 3;
    constexpr static int ProfileChannels = 3;

    constexpr static double REF_U = 0.19783000664283680764;
    constexpr static double REF_V = 0.46831999493879100370;

    // There's no CSS for LUV yet, so we don't know what scales
    // the w3c might choose to use. Our own calculations use these.
    constexpr static double LUMA_SCALE = 100;
    constexpr static double MIN_U = -100;
    constexpr static double MAX_U = 200;
    constexpr static double MIN_V = -200;
    constexpr static double MAX_V = 120;

    Luv(): XYZBase(Type::LUV, "Luv", "Luv", "color-selector-luv") {}
    ~Luv() override = default;

    /**
     * Convert a color from the the Luv colorspace to the XYZ colorspace.
     */
    template<typename T, bool PremultipliedAlpha = false>
    inline static void spaceToProfile(T const *i, T *o)
    {
        if constexpr (PremultipliedAlpha) {
            o[0] = i[0] / i[3];
            o[1] = i[1] / i[3];
            o[2] = i[2] / i[3];
            o[3] = i[3];
            scaleUp(o, o);
        } else {
            scaleUp(i, o);
        }
        toXYZ(o, o);
    }

    template<typename T>
    inline static void toXYZ(T const *i, T *o)
    {
        if (i[0] <= 0.00000001) {
            /* Black would create a divide-by-zero error. */
            o[0] = 0.0;
            o[1] = 0.0;
            o[2] = 0.0;
            return;
        }

        double var_u = i[1] / (13.0 * i[0]) + REF_U;
        double var_v = i[2] / (13.0 * i[0]) + REF_V;
        double y = l2y(i[0]);
        double x = -(9.0 * y * var_u) / ((var_u - 4.0) * var_v - var_u * var_v);
        double z = (9.0 * y - (15.0 * var_v * y) - (var_v * x)) / (3.0 * var_v);

        o[0] = x;
        o[1] = y;
        o[2] = z;
    }

    /**
     * Convert a color from the the XYZ colorspace to the Luv colorspace.
     */
    template<typename T, bool PremultiplyAlpha = false>
    inline static void profileToSpace(T const *i, T *o)
    {
        fromXYZ(i, o);
        scaleDown(o, o);

        if constexpr (PremultiplyAlpha) {
            o[0] *= i[3];
            o[1] *= i[3];
            o[2] *= i[3];
            o[3] = i[3];
        }
    }

    template<typename T>
    inline static void fromXYZ(T const *i, T *o)
    {
        double const denominator = i[0] + (15.0 * i[1]) + (3.0 * i[2]);
        double var_u = 4.0 * i[0] / denominator;
        double var_v = 9.0 * i[1] / denominator;
        double l = y2l(i[1]);
        double u = 13.0 * l * (var_u - REF_U);
        double v = 13.0 * l * (var_v - REF_V);

        o[0] = l;
        if (l < 0.00000001) {
            o[1] = 0.0;
            o[2] = 0.0;
        } else {
            o[1] = u;
            o[2] = v;
        }
    }

    template<typename T>
    static void scaleUp(T const *i, T *o)
    {
        o[0] = SCALE_UP(i[0], 0, LUMA_SCALE);
        o[1] = SCALE_UP(i[1], MIN_U, MAX_U);
        o[2] = SCALE_UP(i[2], MIN_V, MAX_V);
    }

    template<typename T>
    static void scaleDown(T const *i, T *o)
    {
        o[0] = SCALE_DOWN(i[0], 0, LUMA_SCALE);
        o[1] = SCALE_DOWN(i[1], MIN_U, MAX_U);
        o[2] = SCALE_DOWN(i[2], MIN_V, MAX_V);
    }

    static std::vector<double> fromCoordinates(std::vector<double> const &in);
    static std::vector<double> toCoordinates(std::vector<double> const &in);

    std::string toString(std::vector<double> const &values, bool opacity) const override { return ""; }
};

} // namespace Inkscape::Colors::Space

#endif // SEEN_COLORS_SPACES_LUV_H

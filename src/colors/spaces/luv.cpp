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

#include "luv.h"

#include <array>
#include <algorithm>
#include <cmath>

namespace Inkscape::Colors::Space {

/**
 * Utility function used to convert from the XYZ colorspace to CIELuv.
 * https://en.wikipedia.org/wiki/CIELUV
 *
 * @param y Y component of the XYZ color.
 * @return Luminance component of Luv color.
 */
double y2l(double y)
{
    if (y <= EPSILON)
        return y * KAPPA;
    else
        return 116.0 * std::cbrt(y) - 16.0;
}

/**
 * Utility function used to convert from CIELuv colorspace to XYZ.
 *
 * @param l Luminance component of Luv color.
 * @return Y component of the XYZ color.
 */
double l2y(double l)
{
    if (l <= 8.0) {
        return l / KAPPA;
    } else {
        double x = (l + 16.0) / 116.0;
        return (x * x * x);
    }
}

std::vector<double> Luv::fromCoordinates(std::vector<double> const &in)
{
    auto out = in;
    scaleDown(in.data(), out.data());
    return out;
}

std::vector<double> Luv::toCoordinates(std::vector<double> const &in)
{
    auto out = in;
    scaleUp(in.data(), out.data());
    return out;
}

}; // namespace Inkscape::Colors::Space

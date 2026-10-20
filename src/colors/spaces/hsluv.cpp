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

#include "hsluv.h"

#include <cmath>

namespace Inkscape::Colors::Space {

/**
 * Calculate the bounds of the Luv colors in RGB gamut.
 *
 * @param l Lightness. Between 0.0 and 100.0.
 * @return Bounds of Luv colors in RGB gamut.
 */
std::array<Geom::Line, 6> HSLuv::get_bounds(double l)
{
    std::array<Geom::Line, 6> bounds;

    double tl = l + 16.0;
    double sub1 = (tl * tl * tl) / 1560896.0;
    double sub2 = (sub1 > EPSILON ? sub1 : (l / KAPPA));
    int channel;
    int t;

    static const std::vector<double> d65[3] = {{3.24096994190452134377, -1.53738317757009345794, -0.49861076029300328366},
                                               {-0.96924363628087982613, 1.87596750150772066772, 0.04155505740717561247},
                                               {0.05563007969699360846, -0.20397695888897656435, 1.05697151424287856072}};

    for (channel = 0; channel < 3; channel++) {
        double m1 = d65[channel][0];
        double m2 = d65[channel][1];
        double m3 = d65[channel][2];

        for (t = 0; t < 2; t++) {
            double top1 = (284517.0 * m1 - 94839.0 * m3) * sub2;
            double top2 = (838422.0 * m3 + 769860.0 * m2 + 731718.0 * m1) * l * sub2 - 769860.0 * t * l;
            double bottom = (632260.0 * m3 - 126452.0 * m2) * sub2 + 126452.0 * t;

            bounds[channel * 2 + t].setCoefficients(top1, -bottom, top2);
        }
    }

    return bounds;
}

/**
 * Calculate the maximum in gamut chromaticity for the given luminance and hue.
 *
 * @param l Luminance.
 * @param h Hue.
 * @return The maximum chromaticity.
 */
double max_chroma_for_lh(double l, double h)
{
    double min_len = std::numeric_limits<double>::max();
    auto const ray = Geom::Ray(Geom::Point(0, 0), Geom::rad_from_deg(h));

    for (auto const &line : HSLuv::get_bounds(l)) {
        auto intersections = line.intersect(ray);
        if (intersections.empty()) {
            continue;
        }
        double len = intersections[0].point().length();

        if (len >= 0 && len < min_len) {
            min_len = len;
        }
    }

    return min_len;
}

}; // namespace Inkscape::Colors::Space

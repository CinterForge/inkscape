// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLORS_SPACES_OKLAB_H
#define SEEN_COLORS_SPACES_OKLAB_H

#include <2geom/math-utils.h>

#include "linear-rgb.h"
#include "rgb.h"

namespace Inkscape::Colors::Space {

/** Two-dimensional array to store a constant 3x3 matrix. */
using Matrix = const double[3][3];

// clang-format off
/** Matrix of the linear transformation from linear RGB space to linear
 * cone responses, used in the first step of RGB to OKLab conversion.
 */
Matrix LRGB2CONE = {{0.4122214708, 0.5363325363, 0.0514459929},
                    {0.2119034982, 0.6806995451, 0.1073969566},
                    {0.0883024619, 0.2817188376, 0.6299787005}};

/** The inverse of the matrix LRGB2CONE. */
Matrix CONE2LRGB = {
    {  4.0767416613479942676681908333711298900607278264432, -3.30771159040819331315866078424893188865618253342,  0.230969928729427886449650619561935920170561518112 },
    { -1.2684380040921760691815055595117506020901414005992,  2.60975740066337143024050095284233623056192338553, -0.341319396310219620992658250306535533187548361872 },
    { -0.0041960865418371092973767821251846315637521173374, -0.70341861445944960601310996913659932654899822384,  1.707614700930944853864541790660472961199090408527 }
};
/** The matrix M2 used in the second step of RGB to OKLab conversion.
 * Taken from https://bottosson.github.io/posts/oklab/ (retrieved 2022).
 */
Matrix M2 = {{0.2104542553, 0.793617785, -0.0040720468},
             {1.9779984951, -2.428592205, 0.4505937099},
             {0.0259040371, 0.7827717662, -0.808675766}};

/** The inverse of the matrix M2. The first column looks like it wants to be 1 but
 * this form is closer to the actual inverse (due to numerics). */
Matrix M2_INVERSE = {
    { 0.99999999845051981426207542502031373637162589278552,  0.39633779217376785682345989261573192476766903603,  0.215803758060758803423141461830037892590617787467 },
    { 1.00000000888176077671607524567047071276183677410134, -0.10556134232365634941095687705472233997368274024, -0.063854174771705903405254198817795633810975771082 },
    { 1.00000005467241091770129286515344610721841028698942, -0.08948418209496575968905274586339134130669669716, -1.291485537864091739948928752914772401878545675371 }
};
// clang-format on

class OkLab : public RGBBase<OkLab> {
public:
    constexpr static int ProfileChannels = 3;
    constexpr static int OutputChannels = 3;

    /* These values are technically unbounded in the actual calculations but are
     * defined between -0.4 and 0.4 by the CSS Color Module specification
     * as the reasonable upper and lower limits for display. Our internal model
     * always scales from 0 to 1 of this expected range of values.
     */
    constexpr static double MIN_SCALE = -0.4;
    constexpr static double MAX_SCALE = 0.4;

    OkLab(): RGBBase(Type::OKLAB, "OkLab", "OkLab", "color-selector-oklab", true) {
        _svgNames.emplace_back("oklab");
    }
    ~OkLab() override = default;

    template<typename T, bool PremultipliedAlpha = false>
    inline static void spaceToProfile(T const *i, T *o)
    {
        if constexpr (PremultipliedAlpha) {
            o[0] = i[0] / i[3];
            o[1] = SCALE_UP(i[1] / i[3], MIN_SCALE, MAX_SCALE);
            o[2] = SCALE_UP(i[2] / i[3], MIN_SCALE, MAX_SCALE);
            o[3] = i[3];
        } else {
            o[0] = i[0];
            o[1] = SCALE_UP(i[1], MIN_SCALE, MAX_SCALE);
            o[2] = SCALE_UP(i[2], MIN_SCALE, MAX_SCALE);
        }

        toLinearRGB(o, o);
        LinearRGB::toRGB(o, o);
    }
    template<typename T, bool PremultiplyAlpha = false>
    inline static void profileToSpace(T const *i, T *o)
    {
        LinearRGB::fromRGB(i, o);
        fromLinearRGB(o, o);

        if constexpr (PremultiplyAlpha) {
            o[0] *= i[3];
            o[1] = SCALE_DOWN(o[1], MIN_SCALE, MAX_SCALE) * i[3];
            o[2] = SCALE_DOWN(o[2], MIN_SCALE, MAX_SCALE) * i[3];
            o[3] = i[3];
        } else {
            o[1] = SCALE_DOWN(o[1], MIN_SCALE, MAX_SCALE);
            o[2] = SCALE_DOWN(o[2], MIN_SCALE, MAX_SCALE);
        }
    }

    std::string toString(std::vector<double> const &values, bool opacity) const override;

public:
    class Parser : public Colors::Parser
    {
    public:
        Parser()
            : Colors::Parser("oklab", Type::OKLAB)
        {}
        bool parse(std::istringstream &input, std::vector<double> &output) const override;
    };

    template <typename A1, typename A2>
    inline static constexpr double dot3(const A1 &a1, const A2 &a2)
    {
        return a1[0] * a2[0] + a1[1] * a2[1] + a1[2] * a2[2];
    }

    template<typename T>
    static void toLinearRGB(T const *i, T *o)
    {
        std::array<double, 3> cones;
        for (unsigned c = 0; c < 3; c++) {
            cones[c] = Geom::cube(dot3(M2_INVERSE[c], i));
        }
        for (unsigned c = 0; c < 3; c++) {
            // input is unbounded, so don't clip it in linearRGB either, or else we loose information
            o[c] = dot3(CONE2LRGB[c], cones);
        }
    }

    template<typename T>
    static void fromLinearRGB(T const *i, T *o)
    {
        std::array<double, 3> cones;
        for (unsigned c = 0; c < 3; c++) {
            cones[c] = std::cbrt(dot3(LRGB2CONE[c], i));
        }
        for (unsigned c = 0; c < 3; c++) {
            o[c] = dot3(M2[c], cones);
        }
    }
};

} // namespace Inkscape::Colors::Space

#endif // SEEN_COLORS_SPACES_OKLAB_H

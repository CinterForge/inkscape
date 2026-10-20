// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 *//*
 * Authors:
 *   Rafał Siejakowski <rs@rs-math.net>
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "oklch.h"

#include <2geom/polynomial.h>
#include <cmath>

#include "colors/color.h"
#include "colors/printer.h"

namespace Inkscape::Colors::Space {

/* Chroma is technically unbounded in the actual calculations but is
 * defined between 0.0 and 0.4 by the CSS Color Module specification
 * as the reasonable upper and lower limits for display. Our internal model
 * always scales from 0 to 1 of this expected range of values.
 */
constexpr double CHROMA_SCALE = 0.4;

/** @brief
 * Data needed to compute coefficients in the cubic polynomials which express the lines
 * of constant luminosity and hue (but varying chroma) as curves in the linear RGB space.
 */
struct ChromaLineCoefficients
{
    // Variable naming: `c%d` contains coefficients of c^%d in the polynomial, where c is
    // the OKLch chroma. l refers to the luminosity, cos and sin to the cosine and sine of
    // the hue angle. Trailing digits are exponents. For example,
    // c2.lcos2 is the coefficient of (l * cos(hue_angle)^2) in the overall coefficient of c^2.
    struct
    {
        double l2cos, l2sin;
    } c1;
    struct
    {
        double lcos2, lcossin, lsin2;
    } c2;
    struct
    {
        double cos3, cos2sin, cossin2, sin3;
    } c3;
};

// clang-format off
ChromaLineCoefficients const LAB_BOUNDS[] = {
    // Red polynomial
    {
        .c1 = {
            .l2cos = 5.83279532899080641005754476131631984,
            .l2sin = 2.3780791275435732378965655753413412
        },
        .c2 = {
            .lcos2   = 1.81614129917652075864819542521099165275,
            .lcossin = 2.11851258971260413543962953223104329409,
            .lsin2   = 1.68484527361538384522450980300698198391
        },
        .c3 = {
            .cos3    =  0.257535869797624151773507242289856932594,
            .cos2sin =  0.414490345667882332785000888243122224651,
            .cossin2 =  0.126596511492002610582126014059213892767,
            .sin3    = -0.455702039844046560333204117380816048203
        }
    },
    // Green polynomial
    {
        .c1 = {
            .l2cos = -2.243030176177044107983968331289088261,
            .l2sin = 0.00129441240977850026657772225608
        },
        .c2 = {
            .lcos2   = -0.5187087369791308621879921351291952375,
            .lcossin = -0.7820717390897833607054953914674219281,
            .lsin2   = -1.8531911425339782749638630868227383795
        },
        .c3 = {
            .cos3    = -0.0817959138495637068389017598370049459,
            .cos2sin = -0.1239788660641220973883495153116480854,
            .cossin2 =  0.0792215342150077349794741576353537047,
            .sin3    =  0.7218132301017783162780535454552058572
        }
    },
    // Blue polynomial
    {
        .c1 = {
            .l2cos = -0.2406412780923628220925350522352767957,
            .l2sin = -6.48404701978782955733370693958213669
        },
        .c2 = {
            .lcos2   = 0.015528352128452044798222201797574285162,
            .lcossin = 1.153466975472590255156068122829360981648,
            .lsin2   = 8.535379923500727607267514499627438513637
        },
        .c3 = {
            .cos3    = -0.0006573855374563134769075967180540368,
            .cos2sin = -0.0519029179849443823389557527273309386,
            .cossin2 = -0.763927972885238036962716856256210617,
            .sin3    = -3.67825541507929556013845659620477582
        }
    }
};
// clang-format on

/** Stores powers of luminance, hue cosine and hue sine angles. */
struct ConstraintMonomials
{
    double l, l2, l3, c, c2, c3, s, s2, s3;
    ConstraintMonomials(double l, double h)
        : l{l}
    {
        l2 = Geom::sqr(l);
        l3 = l2 * l;
        Geom::sincos(Geom::rad_from_deg(h), s, c);
        c2 = Geom::sqr(c);
        c3 = c2 * c;
        s2 = 1.0 - c2; // Use sin^2 = 1 - cos^2.
        s3 = s2 * s;
    }
};

/** @brief Find the coefficients of the cubic polynomial expressing the linear
 * R, G or B component as a function of OKLch chroma.
 *
 * The returned polynomial gives R(c), G(c) or B(c) for all values of c and fixed
 * values of luminance and hue.
 *
 * @param index The index of the component to evaluate (0 for R, 1 for G, 2 for B).
 * @param m The monomials in L, cos(hue) and sin(hue) needed for the calculation.
 * @return an array whose i-th element is the coefficient of c^i in the polynomial.
 */
static std::array<double, 4> component_coefficients(unsigned index, ConstraintMonomials const &m)
{
    auto const &coeffs = LAB_BOUNDS[index];
    std::array<double, 4> result;
    // Multiply the coefficients by the corresponding monomials.
    result[0] = m.l3; // The coefficient of l^3 is always 1
    result[1] = coeffs.c1.l2cos * m.l2 * m.c + coeffs.c1.l2sin * m.l2 * m.s;
    result[2] = coeffs.c2.lcos2 * m.l * m.c2 + coeffs.c2.lcossin * m.l * m.c * m.s + coeffs.c2.lsin2 * m.l * m.s2;
    result[3] =
        coeffs.c3.cos3 * m.c3 + coeffs.c3.cos2sin * m.c2 * m.s + coeffs.c3.cossin2 * m.c * m.s2 + coeffs.c3.sin3 * m.s3;
    return result;
}

/* Compute the maximum Lch chroma for the given luminosity and hue.
 *
 * Implementation notes:
 * The space of Lch colors is a complicated solid with curved faces in the
 * (L, c, h)-space. So it is not easy to find the maximum chroma for the given
 * luminosity and hue. (By maximum chroma, we mean the maximum value of c such
 * that the color oklch(L c h) still fits in the sRGB gamut.)
 *
 * We consider an abstract ray (L, c, h) where L and h are fixed and c varies
 * from 0 to infinity. Conceptually, we transform this ray to the linear RGB space,
 * which is the unit cube. The ray thus becomes a 3D cubic curve in the RGB cube
 * and the coordinates R(c), G(c) and B(c) are degree 3 polynomials in the chroma
 * variable c. The coefficients of c^i in those polynomials will depend on L and h.
 *
 * To find the smallest positive value of c for which the curve leaves the unit
 * cube, we must solve the equations R(c) = 0, R(c) = 1 and similarly for G(c)
 * and B(c). The desired value is the smallest positive solution among those 6
 * equations.
 *
 * The case of very small or very large luminosity is handled separately.
 */
double OkLch::max_chroma(double l, double h)
{
    static double const EPS = 1e-7;
    if (l < EPS || l > 1.0 - EPS) { // Black or white allow no chroma.
        return 0;
    }

    double chroma_bound = Geom::infinity();
    auto const process_root = [&](double root) -> bool {
        if (root < EPS) { // Ignore roots less than epsilon
            return false;
        }
        if (chroma_bound > root) {
            chroma_bound = root;
        }
        return true;
    };

    // Check relevant chroma constraints for all three coordinates R, G, B.
    auto const monomials = ConstraintMonomials(l, h);
    for (unsigned i = 0; i < 3; i++) {
        auto const coeffs = component_coefficients(i, monomials);
        // The cubic polynomial is coeffs[3]*c^3 + coeffs[2]*c^2 + coeffs[1]*c + coeffs[0]

        // First we solve for the R/G/B component equal to zero.
        for (double root : Geom::solve_cubic(coeffs[3], coeffs[2], coeffs[1], coeffs[0])) {
            if (process_root(root)) {
                break;
            }
        }

        // Now solve for the component equal to 1 by subtracting 1.0 from coeffs[0].
        for (double root : Geom::solve_cubic(coeffs[3], coeffs[2], coeffs[1], coeffs[0] - 1.0)) {
            if (process_root(root)) {
                break;
            }
        }
    }
    if (chroma_bound == Geom::infinity()) { // No bound was found, so everything was < EPS
        return 0;
    }
    return chroma_bound;
}

/** @brief How many intervals a color scale should be subdivided into for the chroma bounds probing.
 *
 * The reason this constant exists is because probing chroma bounds requires solving 6 cubic equations,
 * which would not be feasible for all 1024 pixels on a scale without slowing down the UI.
 * To speed things up, we subdivide the scale into COLOR_SCALE_INTERVALS intervals and linearly
 * interpolate the chroma bound on each interval. Note that the actual color interpolation is still
 * done in the OKLab space, but the computed absolute chroma may be slightly off in the middle of
 * each interval (hopefully, in an imperceptible way).
 *
 * @todo Consider rendering the color sliders asynchronously, which might make this
 *       interpolation unnecessary. We would then get full precision gradients.
 */
unsigned const COLOR_SCALE_INTERVALS = 32; // Must be a power of 2 and less than 1024.

uint8_t const *render_hue_scale(double s, double l, std::array<uint8_t, 4 * 1024> *map)
{
    auto const data = map->data();
    auto pos = data;
    unsigned const interval_length = 1024 / COLOR_SCALE_INTERVALS;

    double h = 0; // Variable hue
    double chroma_bound = OkLch::max_chroma(l, h);
    double next_chroma_bound;
    double const step = 360.0 / 1024.0;
    double const interpolation_step = 360.0 / COLOR_SCALE_INTERVALS;

    for (unsigned i = 0; i < COLOR_SCALE_INTERVALS; i++) {
        double const initial_chroma = chroma_bound * s;
        next_chroma_bound = OkLch::max_chroma(l, h + interpolation_step);
        double const final_chroma = next_chroma_bound * s;

        for (unsigned j = 0; j < interval_length; j++) {
            double const c = Geom::lerp(static_cast<double>(j) / interval_length, initial_chroma, final_chroma);
            auto rgb = *Color(Space::Type::OKLCH, {l, c, h / 360}).converted(Space::Type::RGB);
            *pos++ = (uint8_t)SP_COLOR_F_TO_U(rgb[0]);
            *pos++ = (uint8_t)SP_COLOR_F_TO_U(rgb[1]);
            *pos++ = (uint8_t)SP_COLOR_F_TO_U(rgb[2]);
            *pos++ = 0xFF;
            h += step;
        }
        chroma_bound = next_chroma_bound;
    }
    return data;
}

uint8_t const *render_saturation_scale(double h, double l, std::array<uint8_t, 4 * 1024> *map)
{
    auto const data = map->data();
    auto pos = data;
    auto chromax = OkLch::max_chroma(l, h);
    if (chromax == 0.0) { // Render black or white strip.
        uint8_t const bw = (l > 0.9) ? 0xFF : 0x00;
        for (size_t i = 0; i < 1024; i++) {
            *pos++ = bw;   // red
            *pos++ = bw;   // green
            *pos++ = bw;   // blue
            *pos++ = 0xFF; // alpha
        }
    } else { // Render strip of varying chroma.
        double const chroma_step = chromax / 1024.0;
        double c = 0.0;
        for (size_t i = 0; i < 1024; i++) {
            auto rgb = *Color(Space::Type::OKLCH, {l, c, h}).converted(Space::Type::RGB);
            *pos++ = (uint8_t)SP_COLOR_F_TO_U(rgb[0]);
            *pos++ = (uint8_t)SP_COLOR_F_TO_U(rgb[1]);
            *pos++ = (uint8_t)SP_COLOR_F_TO_U(rgb[2]);
            *pos++ = 0xFF;
            c += chroma_step;
        }
    }
    return data;
}

uint8_t const *render_lightness_scale(double h, double s, std::array<uint8_t, 4 * 1024> *map)
{
    auto const data = map->data();
    auto pos = data;
    unsigned const interval_length = 1024 / COLOR_SCALE_INTERVALS;

    double l = 0; // Variable lightness

    double chroma_bound = OkLch::max_chroma(l, h);
    double next_chroma_bound;
    double const step = 1.0 / 1024.0;
    double const interpolation_step = 1.0 / COLOR_SCALE_INTERVALS;

    for (unsigned i = 0; i < COLOR_SCALE_INTERVALS; i++) {
        double const initial_chroma = chroma_bound * s;
        next_chroma_bound = OkLch::max_chroma(l + interpolation_step, h);
        double const final_chroma = next_chroma_bound * s;

        for (unsigned j = 0; j < interval_length; j++) {
            double const c = Geom::lerp(static_cast<double>(j) / interval_length, initial_chroma, final_chroma);
            auto rgb = *Color(Space::Type::OKLCH, {l, c, h}).converted(Space::Type::RGB);
            *pos++ = (uint8_t)SP_COLOR_F_TO_U(rgb[0]);
            *pos++ = (uint8_t)SP_COLOR_F_TO_U(rgb[1]);
            *pos++ = (uint8_t)SP_COLOR_F_TO_U(rgb[2]);
            *pos++ = 0xFF;
            l += step;
        }
        chroma_bound = next_chroma_bound;
    }
    return data;
}

bool OkLch::Parser::parse(std::istringstream &ss, std::vector<double> &output) const
{
    bool end = false;
    if (append_css_value(ss, output, end, ',')                  // Luminance
        && append_css_value(ss, output, end, ',', CHROMA_SCALE) // Chroma
        && append_css_value(ss, output, end, '/', HUE_SCALE)    // Hue
        && (append_css_value(ss, output, end) || true)          // Optional opacity
        && end) {
        return true;
    }
    return false;
}

/**
 * Print the Lab color to a CSS string.
 *
 * @arg values - A vector of doubles for each channel in the Lch space
 * @arg opacity - True if the opacity should be included in the output.
 */
std::string OkLch::toString(std::vector<double> const &values, bool opacity) const
{
    auto os = CssFuncPrinter(3, "oklch");

    os << values[0]                // Luminance
       << values[1] * CHROMA_SCALE // Chroma
       << values[2] * HUE_SCALE;   // Hue

    if (opacity && values.size() == 4)
        os << values[3]; // Optional opacity

    return os;
}

}; // namespace Inkscape::Colors::Space

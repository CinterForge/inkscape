// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 AUTHORS
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_COLORS_UTILS_H
#define SEEN_COLORS_UTILS_H

#include <algorithm>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

/* Useful composition macros */

constexpr double SP_COLOR_U_TO_F(uint32_t v)
{
    return v / 255.0;
}
constexpr uint32_t SP_COLOR_F_TO_U(double v)
{
    return (unsigned int)(std::clamp(v, 0.0, 1.0) * 255. + .5);
}
constexpr uint32_t SP_RGBA32_R_U(uint32_t v)
{
    return (v >> 24) & 0xff;
}
constexpr uint32_t SP_RGBA32_G_U(uint32_t v)
{
    return (v >> 16) & 0xff;
}
constexpr uint32_t SP_RGBA32_B_U(uint32_t v)
{
    return (v >> 8) & 0xff;
}
constexpr uint32_t SP_RGBA32_A_U(uint32_t v)
{
    return v & 0xff;
}
constexpr double SP_RGBA32_R_F(uint32_t v)
{
    return SP_COLOR_U_TO_F(SP_RGBA32_R_U(v));
}
constexpr double SP_RGBA32_G_F(uint32_t v)
{
    return SP_COLOR_U_TO_F(SP_RGBA32_G_U(v));
}
constexpr double SP_RGBA32_B_F(uint32_t v)
{
    return SP_COLOR_U_TO_F(SP_RGBA32_B_U(v));
}
constexpr double SP_RGBA32_A_F(uint32_t v)
{
    return SP_COLOR_U_TO_F(SP_RGBA32_A_U(v));
}

constexpr uint32_t SP_RGBA32_U_COMPOSE(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
{
    return ((r & 0xff) << 24) | ((g & 0xff) << 16) | ((b & 0xff) << 8) | (a & 0xff);
}
constexpr uint32_t SP_RGBA32_F_COMPOSE(double r, double g, double b, double a)
{
    return SP_RGBA32_U_COMPOSE(SP_COLOR_F_TO_U(r), SP_COLOR_F_TO_U(g), SP_COLOR_F_TO_U(b), SP_COLOR_F_TO_U(a));
}
constexpr uint32_t SP_RGBA32_C_COMPOSE(uint32_t c, double o)
{
    return SP_RGBA32_U_COMPOSE(SP_RGBA32_R_U(c), SP_RGBA32_G_U(c), SP_RGBA32_B_U(c), SP_COLOR_F_TO_U(o));
}

constexpr uint32_t compose_argb32(double a, double r, double g, double b) {
    return SP_RGBA32_U_COMPOSE(SP_COLOR_F_TO_U(a), SP_COLOR_F_TO_U(r), SP_COLOR_F_TO_U(g), SP_COLOR_F_TO_U(b));
}

/**
 * A set of useful color modifying functions which do not fit as generic
 * methods on the color class itself but which are used in various places.
 */
namespace Inkscape::Colors {

class Color;

uint32_t hex_to_rgba(std::string const &value);
std::vector<double> rgba_to_values(uint32_t rgba, bool opacity);
std::string rgba_to_hex(uint32_t value, bool alpha = false, bool uppercase = false);
std::string color_to_id(std::optional<Color> const &color);
std::string desc_to_id(std::string const &desc);

Color make_contrasted_color(Color const &orig, double amount);
Color make_theme_color(Color const &orig, bool dark);
Color make_disabled_color(Color const &orig, bool dark);

double lightness(Color color);
double perceptual_lightness(double l);
double get_perceptual_lightness(Color const &color);
std::pair<double, double> get_contrasting_color(double l);

/// Premultiply a vector of colour channels, assuming alpha is last.
constexpr void premultiply(std::span<double> vec) {
    auto const alpha = vec.back();
    for (int i = 0; i < vec.size() - 1; i++) {
        vec[i] *= alpha;
    }
}
constexpr std::vector<double> premultiplied(std::vector<double> vec) {
    premultiply(vec);
    return std::move(vec);
}

/// Unpremultiply a vector of colour channels, assuming alpha is last.
constexpr void unpremultiply(std::span<double> vec) {
    auto const alpha = vec.back();
    if (alpha == 0) {
        return;
    }
    for (int i = 0; i < vec.size() - 1; i++) {
        vec[i] /= alpha;
    }
}
constexpr std::vector<double> unpremultiplied(std::vector<double> vec) {
    unpremultiply(vec);
    return std::move(vec);
}

} // namespace Inkscape::Colors

#endif // SEEN_COLORS_UTILS_H

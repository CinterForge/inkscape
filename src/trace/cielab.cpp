// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2005, 2006 by Gerald Friedland, Kristian Jantz and Lars Knipping
 * Conversion to C++ for Inkscape by Bob Jamison
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include <algorithm>
#include <cmath>
#include "cielab.h"

namespace Inkscape {
namespace Trace {

namespace {

/**
 * Convert integer RGB values into a pixel value.
 */
uint32_t getRGB(int r, int g, int b)
{
    r = std::clamp(r, 0, 255);
    g = std::clamp(g, 0, 255);
    b = std::clamp(b, 0, 255);
    return (r << 16) | (g << 8) | b;
}

/**
 * Convert float RGB values (0.0-1.0) into a pixel value.
 */
uint32_t getRGB(float r, float g, float b)
{
    return getRGB((int)(r * 256.0f),
                  (int)(g * 256.0f),
                  (int)(b * 256.0f));
}

//#########################################
//# Root approximations for large speedup.
//# By njh!
//#########################################

class Tables
{
public:
    static int constexpr SIZE = 16;
    float cbrt[SIZE + 1];
    float qn[SIZE + 1];

    static auto &get()
    {
        static Tables const instance;
        return instance;
    }

private:
    Tables();
};

template <typename T>
auto sq(T t)
{
    return t * t;
}

// Cube root.
double cbrt(double x)
{
    auto polish = [x] (double y) {
        return (2.0 * y + x / sq(y)) / 3.0;
    };
    double y = Tables::get().cbrt[(int)(x * Tables::SIZE)]; // assuming x \in [0, 1]
    y = polish(y);
    y = polish(y);
    return y;
}

// Quintic root.
double qnrt(double x)
{
    auto polish = [x] (double y) {
        return (4.0 * y + x / sq(sq(y))) / 5.0;
    };
    double y = Tables::get().qn[(int)(x * Tables::SIZE)]; // assuming x \in [0, 1]
    y = polish(y);
    y = polish(y);
    return y;
}

double pow24(double x)
{
    return sq(x * qnrt(x));
}

Tables::Tables()
{
    auto entry = [&] (int i, float x) {
        cbrt[i] = std::pow(x / SIZE, 0.3333f);
        qn[i]   = std::pow(x / SIZE, 0.2f);
    };

    entry(0, 0.5f);
    for (int i = 1; i < SIZE + 1; i++) {
        entry(i, i);
    }
}

} // namespace

CieLab::CieLab(uint32_t rgb)
{
    uint8_t ir  = (rgb >> 16) & 0xff;
    uint8_t ig  = (rgb >>  8) & 0xff;
    uint8_t ib  = (rgb      ) & 0xff;

    float fr = ir / 255.0f;
    float fg = ig / 255.0f;
    float fb = ib / 255.0f;

    // printf("fr:%f fg:%f fb:%f\n", fr, fg, fb);

    auto to_linear = [] (float &x) {
        if (x > 0.04045) {
            x = pow24((x + 0.055) / 1.055);
        } else {
            x /= 12.92;
        }
    };

    to_linear(fr);
    to_linear(fg);
    to_linear(fb);

    // Use white = D65
    float x = fr * 0.4124 + fg * 0.3576 + fb * 0.1805;
    float y = fr * 0.2126 + fg * 0.7152 + fb * 0.0722;
    float z = fr * 0.0193 + fg * 0.1192 + fb * 0.9505;

    float vx = x / 0.95047;
    float vy = y;
    float vz = z / 1.08883;

    // printf("vx:%f vy:%f vz:%f\n", vx, vy, vz);

    auto f = [] (float &x) {
        if (x > 0.008856) {
            x = cbrt(x);
        } else {
            x = (7.787 * x) + (16.0 / 116.0);
        }
    };

    f(vx);
    f(vy);
    f(vz);

    C = 0;
    L = 116.0 * vy - 16.0;
    A = 500.0 * (vx - vy);
    B = 200.0 * (vy - vz);
}

uint32_t CieLab::toRGB() const
{
    float vy = (L + 16.0) / 116.0;
    float vx = A / 500.0 + vy;
    float vz = vy - B / 200.0;

    auto finv = [] (float &x) {
        float x3 = x * x * x;
        if (x3 > 0.008856) {
            x = x3;
        } else {
            x = (x - 16.0 / 116.0) / 7.787;
        }
    };

    finv(vx);
    finv(vy);
    finv(vz);

    vx *= 0.95047; // use white = D65
    vz *= 1.08883;

    float vr = vx *  3.2406 + vy * -1.5372 + vz * -0.4986;
    float vg = vx * -0.9689 + vy *  1.8758 + vz *  0.0415;
    float vb = vx *  0.0557 + vy * -0.2040 + vz *  1.0570;

    auto from_linear = [] (float &x) {
        if (x > 0.0031308) {
            x = 1.055 * std::pow(x, 1.0 / 2.4) - 0.055;
        } else {
            x *= 12.92;
        }
    };

    from_linear(vr);
    from_linear(vg);
    from_linear(vb);

    return getRGB(vr, vg, vb);
}

float CieLab::diffSq(CieLab const &c1, CieLab const &c2)
{
    return sq(c1.L - c2.L)
         + sq(c1.A - c2.A)
         + sq(c1.B - c2.B);
}

float CieLab::diff(CieLab const &c1, CieLab const &c2)
{
    return std::sqrt(diffSq(c1, c2));
}

} // namespace Trace
} // namespace Inkscape

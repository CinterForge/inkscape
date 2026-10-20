// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2005, 2006 by Gerald Friedland, Kristian Jantz and Lars Knipping
 * Conversion to C++ for Inkscape by Bob Jamison
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef INKSCAPE_TRACE_CIELAB_H
#define INKSCAPE_TRACE_CIELAB_H

#include <cstdint>

namespace Inkscape {
namespace Trace {

class CieLab
{
public:
    CieLab()
    {
        C = 0;
        L = A = B = 0.0f;
    }

    /**
     * Construct this CieLab from an ARGB value
     */
    CieLab(uint32_t rgb);

    CieLab(float l, float a, float b)
    {
        C = 0;
        L = l;
        A = a;
        B = b;
    }

    CieLab(CieLab const &other)
    {
        C = other.C;
        L = other.L;
        A = other.A;
        B = other.B;
    }

    CieLab &operator=(CieLab const &other)
    {
        C = other.C;
        L = other.L;
        A = other.A;
        B = other.B;
        return *this;
    }

    /**
     * Retrieve a CieLab value via index.
     */
    float operator()(unsigned index) const
    {
        switch (index) {
            case 0: return L;
            case 1: return A;
            case 2: return B;
            default: return 0;
        }
    }

    void add(CieLab const &other)
    {
        C += other.C;
        L += other.L;
        A += other.A;
        B += other.B;
    }

    void mul(float scale)
    {
        L *= scale;
        A *= scale;
        B *= scale;
    }

    /**
     * Return this CieLab's value converted to an ARGB value
     */
    uint32_t toRGB() const;

    /**
     * Squared Euclidean distance between two colors in CieLab space.
     */
    static float diffSq(CieLab const &c1, CieLab const &c2);

    /**
     * Computes euclidean distance in CieLab space between two colors.
     */
    static float diff(CieLab const &c1, CieLab const &c2);

    unsigned C;
    float L;
    float A;
    float B;
};

} // namespace Trace
} // namespace Inkscape

#endif // INKSCAPE_TRACE_CIELAB_H

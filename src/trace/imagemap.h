// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef INKSCAPE_TRACE_IMAGEMAP_H
#define INKSCAPE_TRACE_IMAGEMAP_H

#include <vector>
#include <array>

namespace Inkscape {
namespace Trace {

template <typename T>
struct MapBase
{
    int width;
    int height;
    std::vector<T> pixels;

    MapBase(int width, int height)
        : width(width)
        , height(height)
        , pixels(width * height) {}

    int offset(int x, int y) const { return x + y * width; }
    T       *row(int y)       { return pixels.data() + y * width; }
    T const *row(int y) const { return pixels.data() + y * width; }
    void setPixel(int x, int y, T val) { pixels[offset(x, y)] = val; }
    T getPixel(int x, int y) const { return pixels[offset(x, y)]; }
};

/*
 * GrayMap
 */

struct GrayMap
    : MapBase<unsigned long>
{
    static unsigned long constexpr BLACK = 0;
    static unsigned long constexpr WHITE = 255 * 3;

    GrayMap(int width, int height);

    bool writePPM(char const *fileName);
};

/*
 * RgbMap
 */

struct RGB
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

struct RgbMap
    : MapBase<RGB>
{
    RgbMap(int width, int height);

    bool writePPM(char const *fileName);
};

/*
 * IndexedMap
 */

struct IndexedMap
    : MapBase<unsigned>
{
    IndexedMap(int width, int height);

    RGB getPixelValue(int x, int y) const { return clut[getPixel(x, y) % clut.size()]; }
    bool writePPM(char const *fileName);

    int nrColors;
    std::array<RGB, 256> clut; ///< Color look-up table.
};

} // namespace Trace
} // namespace Inkscape

#endif // INKSCAPE_TRACE_IMAGEMAP_H

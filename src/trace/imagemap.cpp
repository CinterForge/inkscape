// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include <cstdio>
#include "imagemap.h"

namespace Inkscape {
namespace Trace {

/*
 * GrayMap
 */

GrayMap::GrayMap(int width, int height)
    : MapBase(width, height)
{
}

bool GrayMap::writePPM(char const *fileName)
{
    if (!fileName) {
        return false;
    }

    auto f = std::fopen(fileName, "wb");
    if (!f) {
        return false;
    }

    std::fprintf(f, "P6 %d %d 255\n", width, height);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            auto pix = getPixel(x, y) / 3;
            unsigned char pixb = pix & 0xff;
            std::fputc(pixb, f);
            std::fputc(pixb, f);
            std::fputc(pixb, f);
        }
    }

    std::fclose(f);

    return true;
}

/*
 * RgbMap
 */

RgbMap::RgbMap(int width, int height)
    : MapBase(width, height)
{
}

bool RgbMap::writePPM(char const *fileName)
{
    if (!fileName) {
        return false;
    }

    auto f = std::fopen(fileName, "wb");
    if (!f) {
        return false;
    }

    std::fprintf(f, "P6 %d %d 255\n", width, height);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            auto rgb = getPixel(x, y);
            std::fputc(rgb.r, f);
            std::fputc(rgb.g, f);
            std::fputc(rgb.b, f);
        }
    }

    std::fclose(f);

    return true;
}

/*
 * IndexedMap
 */

IndexedMap::IndexedMap(int width, int height)
    : MapBase(width, height)
    , nrColors(0)
{
    clut.fill(RGB{0, 0, 0});
}

bool IndexedMap::writePPM(char const *fileName)
{
    if (!fileName) {
        return false;
    }

    auto f = std::fopen(fileName, "wb");
    if (!f) {
        return false;
    }

    std::fprintf(f, "P6 %d %d 255\n", width, height);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            auto rgb = getPixelValue(x, y);
            std::fputc(rgb.r, f);
            std::fputc(rgb.g, f);
            std::fputc(rgb.b, f);
        }
    }

    std::fclose(f);

    return true;
}

} // namespace Trace
} // namespace Inkscape

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLORS_SPACES_ENUM_H
#define SEEN_COLORS_SPACES_ENUM_H

#include <string>
#include <map>

namespace Inkscape::Colors {
enum class RenderingIntent
{
    UNKNOWN = 0,
    AUTO = 1,
    PERCEPTUAL = 2,
    RELATIVE_COLORIMETRIC = 3,
    SATURATION = 4,
    ABSOLUTE_COLORIMETRIC = 5,
    // This isn't an SVG standard value, this is an Inkscape additional
    // value that means RENDERING_INTENT_RELATIVE_COLORIMETRIC minus
    // the black point compensation. This BPC doesn't apply to any other
    // rendering intent so is safely folded in here.
    RELATIVE_COLORIMETRIC_NOBPC = 6
};

// Used in caching keys and in svg rendering-intent attributes
static std::map<RenderingIntent, std::string> intentIds = {
    {RenderingIntent::UNKNOWN, ""},
    {RenderingIntent::AUTO, "auto"},
    {RenderingIntent::PERCEPTUAL, "perceptual"},
    {RenderingIntent::SATURATION, "saturation"},
    {RenderingIntent::ABSOLUTE_COLORIMETRIC, "absolute-colorimetric"},
    {RenderingIntent::RELATIVE_COLORIMETRIC, "relative-colorimetric"},
    {RenderingIntent::RELATIVE_COLORIMETRIC_NOBPC, "relative-colorimetric-nobpc"},
};

namespace Space {
// The spaces we support are a mixture of ICC profile spaces
// and internal spaces converted to and from RGB
enum class Type
{
    NONE,      // An error of some kind, or destroyed object
    Alpha,     // A zero sized profile which forces opacity always on
    Gray,      // Grayscale, typical of some print icc profiles
    RGB,       // sRGB color space typical with SVG
    linearRGB, // linear RGB used in SVG and other transforms
    HSL,       // Hue, Saturation and Lightness, sometimes called HLS
    HSV,       // Hue, Saturation and Value, similar to HSL and HWB
    HWB,       // Hue, Whiteness and Blackness, similar to HSL and HSV
    CMYK,      // Cyan, Magenta, Yellow and Black for print
    CMY,       // CMYK without the black, used in some icc profiles
    XYZ,       // Color, Luminance and Blueness, with a D65 Whitepoint
    XYZ50,     // Same as XYZ but with a D50 Whitepoint
    YXY,
    LUV,       // Lightness and chromaticity, aka CIELUV
    LCH,       // Lunimance, Chroma and Hue, aka HCL
    LAB,       // Lightness, Green-Magenta and Blue-Yellow, aka CIELAB
    HSLUV,
    OKHSL,
    OKHSV,
    OKLCH,
    OKLAB,
    YCbCr,
    CSSNAME, // Special css-named colors
    CMS      // Special cms type
};

} // namespace Space
} // namespace Inkscape::Colors

#endif // SEEN_COLORS_SPACES_ENUM_H

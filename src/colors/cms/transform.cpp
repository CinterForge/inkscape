// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * A C++ light wrapper for lcms2 transforms
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018-2025 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "transform.h"

#include <algorithm>
#include <memory>

#include "profile.h"
#include <iostream>

namespace Inkscape::Colors::CMS {

// Color space is used in lcms2 to scale input and output values, we don't want this.
static constexpr cmsUInt32Number mask_colorspace = ~COLORSPACE_SH(0b11111);

/**
 * Returns the formatting for a profile, assuming that the data is in inkscape
 * cms api ranges and format: 64bit doubles with no scaling except xyz.
 *
 * @arg profile - The color profile which will be transformed into or out of.
 * @arg size    - Number of bytes in this format, default 0 which means 8 byte double
 * @arg decimal - True if float or double, false if int. Default is true.
 * @arg alpha   - What kind of alpha processing to do (see Alpha)
 *
 * @return The lcms2 transform format for this color profile.
 */
int Transform::lcms_color_format(std::shared_ptr<Profile> const &profile, int size, bool decimal, Alpha alpha)
{
    // Format is 64bit floating point (double) or 32bit (float)
    // Note: size of 8 will clobber channel size bit and cause errors, pass zero (see lcms API docs)
    auto format = cmsFormatterForColorspaceOfProfile(profile->getHandle(), std::clamp(size, 0, 4), decimal);

    // Add the alpha channel into the formatter
    if (alpha != Alpha::NONE) {
        // Note that this does not mean the output will have the alpha copied into it, see cmsFLAGS_COPY_ALPHA
        format |= EXTRA_SH(1);
    }

    // When translating the color channels must be pre-multiplied first
    if (alpha == Alpha::PREMULTIPLIED) {
        format |= PREMUL_SH(1);
    }

    // Masking color values can only happen to non-xyz because while we scale everything else
    // to 0.0 to 1.0, we don't actually scale our xyz which can go as high as 1.99
    if ((format & COLORSPACE_SH(PT_XYZ)) != COLORSPACE_SH(PT_XYZ)) {
        format &= mask_colorspace;
    }
    return format;
}

/**
 * Get the lcms2 intent enum from the inkscape intent enum
 *
 * @args intent - The Inkscape RenderingIntent enum
 *
 * @returns lcms intent enum, default is INTENT_PERCEPTUAL
 */
int Transform::lcms_intent(RenderingIntent intent)
{
    switch (intent) {
        case RenderingIntent::RELATIVE_COLORIMETRIC:
        case RenderingIntent::RELATIVE_COLORIMETRIC_NOBPC:
            return INTENT_RELATIVE_COLORIMETRIC;
        case RenderingIntent::SATURATION:
            return INTENT_SATURATION;
        case RenderingIntent::ABSOLUTE_COLORIMETRIC:
            return INTENT_ABSOLUTE_COLORIMETRIC;
        case RenderingIntent::PERCEPTUAL:
        case RenderingIntent::UNKNOWN:
        case RenderingIntent::AUTO:
        default:
            return INTENT_PERCEPTUAL;
    }
}

/**
 * Get the black point correction flag, if set in inkscape's itent enum
 */
int Transform::lcms_bpc(RenderingIntent intent)
{
    // Black point compensation only matters to relative colorimetric
    return intent == RenderingIntent::RELATIVE_COLORIMETRIC ? cmsFLAGS_BLACKPOINTCOMPENSATION : 0;
}

} // namespace Inkscape::Colors::CMS

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

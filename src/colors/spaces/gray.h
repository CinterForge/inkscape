// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLORS_SPACES_GRAY_H
#define SEEN_COLORS_SPACES_GRAY_H

#include "rgb.h"

namespace Inkscape::Colors::Space {

class Gray : public RGBBase<Gray>
{
public:
    constexpr static int OutputChannels = 1;
    constexpr static int ProfileChannels = 3;

    Gray(): RGBBase(Type::Gray, "Gray", "Gray", "color-selector-gray") {}

    /**
     * Convert a single gray channel into an RGB
     */
    template<typename T, bool PremultipliedAlpha = false>
    inline static void spaceToProfile(T const *i, T *o)
    {
        if constexpr (PremultipliedAlpha) {
            o[0] = i[0] / i[1];
            o[1] = i[0] / i[1];
            o[2] = i[0] / i[1];
            o[3] = i[1];
        } else {
            o[0] = i[0];
            o[1] = i[0];
            o[2] = i[0];
        }
    }

    /**
     * Convert an RGB into a gray channel using the HSL method
     */
    template<typename T, bool PremultiplyAlpha = false>
    inline static void profileToSpace(T const *i, T *o)
    {
        double max = std::max(std::max(i[0], i[1]), i[2]);
        double min = std::min(std::min(i[0], i[1]), i[2]);
        o[0] = (max + min) / 2.0; // Based on HSL conversion
        if constexpr (PremultiplyAlpha) {
            o[0] *= i[3];
            o[1] = i[3];
        }
    }

    std::string toString(std::vector<double> const &values, bool opacity) const override { return ""; }
};

} // namespace Inkscape::Colors::Space

#endif // SEEN_COLORS_SPACES_GRAY_H

// SPDX-License-Identifier: GPL-2.0-or-later
//
// CSS Level 4 gamut mapping: https://www.w3.org/TR/css-color-4/#gamut-mapping
//
// Implementation adopted from:
// https://github.com/color-js/color.js/blob/main/src/toGamut.js

#include "base.h"

namespace Inkscape::Colors {

/**
 * Given a color `origin`, returns a new color that is in gamut using
 * the CSS Gamut Mapping Algorithm. If `space` is specified, it will be in gamut
 * in `space`, and returned in `space`. Otherwise, it will be in gamut and
 * returned in the color space of `origin`.
 * @param {ColorTypes} origin
 * @param {{ space?: string | ColorSpace | undefined }} param1
 * @returns {PlainColorObject}
 */
Color to_gamut_css(const Color& origin, const std::shared_ptr<Space::AnySpace>& space);

// Check if color is outside of given color space gamut
bool out_of_gamut(const Color& color, const std::shared_ptr<Space::AnySpace>& space, double eps = 0.0001);

} // namespace

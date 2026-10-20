// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 *//*
 * Copyright (C) 2023 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "lch.h"

#include "colors/printer.h"

namespace Inkscape::Colors::Space {

/**
 * Print the Lch color to a CSS string.
 *
 * @arg values - A vector of doubles for each channel in the Lch space
 * @arg opacity - True if the opacity should be included in the output.
 */
std::string Lch::toString(std::vector<double> const &values, bool opacity) const
{
    auto os = CssFuncPrinter(3, "lch");

    os << values[0] * LUMA_SCALE   // Luminance
       << values[1] * CHROMA_SCALE // Chroma
       << values[2] * HUE_SCALE;   // Hue

    if (opacity && values.size() == 4)
        os << values[3]; // Optional opacity
    return os;
}

bool Lch::Parser::parse(std::istringstream &ss, std::vector<double> &output) const
{
    bool end = false;
    return append_css_value(ss, output, end, ',', LUMA_SCALE)      // Luminance
           && append_css_value(ss, output, end, ',', CHROMA_SCALE) // Chroma
           && append_css_value(ss, output, end, '/', HUE_SCALE)    // Hue
           && (append_css_value(ss, output, end) || true)          // Optional opacity
           && end;
}

}; // namespace Inkscape::Colors::Space

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

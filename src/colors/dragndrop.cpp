// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Jon A. Cruz <jon@joncruz.org>
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2009-2023 author
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "dragndrop.h"

#include <cstring>
#include <iostream>

#include "colors/color.h"

namespace Inkscape::Colors {

/**
 * Convert a paint into a draggable object.
 */
std::vector<char> getMIMEData(Paint const &paint, char const *mime_type)
{
    // XML Handles all types of paint
    if (std::strcmp(mime_type, mimeOSWB_COLOR) == 0) {
        auto const xml = paint_to_xml_string(paint);
        return {xml.begin(), xml.end()};
    }

    // Handle NoColor first
    if (std::holds_alternative<NoColor>(paint)) {
        if (std::strcmp(mime_type, mimeTEXT) == 0)
            return {'n', 'o', 'n', 'e'};
        if (std::strcmp(mime_type, mimeX_COLOR) == 0)
            return std::vector<char>(8); // transparent black
        return {};
    }

    auto &color = std::get<Color>(paint);
    if (std::strcmp(mime_type, mimeTEXT) == 0) {
        auto const str = color.toString();
        return {str.begin(), str.end()};
    } else if (std::strcmp(mime_type, mimeX_COLOR) == 0) {
        // X-color is only ever in RGBA
        auto const rgb = color.toRGBA();
        return {
            (char)SP_RGBA32_R_U(rgb), (char)SP_RGBA32_R_U(rgb), (char)SP_RGBA32_G_U(rgb), (char)SP_RGBA32_G_U(rgb),
            (char)SP_RGBA32_B_U(rgb), (char)SP_RGBA32_B_U(rgb), (char)SP_RGBA32_A_U(rgb), (char)SP_RGBA32_A_U(rgb),
        };
    }
    return {};
}

/**
 * Convert a dropped object into a color, if possible.
 */
Paint fromMIMEData(std::span<char const> data, char const *mime_type)
{
    if (std::strcmp(mime_type, mimeX_COLOR) == 0) {
        if (data.size() != 8)
            throw ColorError("Data is the wrong size for color mime type");
        return Color{SP_RGBA32_U_COMPOSE(data[0], data[2], data[4], data[6])};
    }

    auto const str = std::string{data.data(), data.size()};
    if (std::strcmp(mime_type, mimeTEXT) == 0) {
        if (str == "none") {
            return NoColor();
        }
        if (auto color = Color::parse(str)) {
            return *color;
        }
    }
    if (std::strcmp(mime_type, mimeOSWB_COLOR) == 0) {
        return xml_string_to_paint(str, nullptr);
    }
    throw ColorError("Unknown color data found");
}

} // namespace Inkscape::Colors

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

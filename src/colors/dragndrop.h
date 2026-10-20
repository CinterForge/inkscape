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

#ifndef INKSCAPE_COLORS_DRAGNDROP
#define INKSCAPE_COLORS_DRAGNDROP

#include <span>
#include <vector>

#include "xml-color.h"

namespace Inkscape::Colors {

inline constexpr auto mimeOSWB_COLOR = "application/x-oswb-color";
inline constexpr auto mimeX_COLOR = "application/x-color";
inline constexpr auto mimeTEXT = "text/plain";

std::vector<char> getMIMEData(Paint const &paint, char const *mime_type);
Paint fromMIMEData(std::span<char const> data, char const *mime_type);

} // namespace Inkscape::Colors

#endif // INKSCAPE_COLORS_DRAGNDROP

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

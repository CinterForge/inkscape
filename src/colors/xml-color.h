// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023 author
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_COLORS_XML_COLOR
#define INKSCAPE_COLORS_XML_COLOR

#include <optional>
#include <string>
#include <variant>

class SPDocument;

namespace Inkscape::XML {
struct Document;
}

namespace Inkscape::Colors {

class Color;
struct NoColor
{
};

// NoColor must go first for default constructor
using Paint = std::variant<NoColor, Color>;

XML::Document *paint_to_xml(Paint const &paint);
std::string paint_to_xml_string(Paint const &paint);

Paint xml_string_to_paint(std::string const &xmls, SPDocument *doc);
Paint xml_to_paint(XML::Document const *xml, SPDocument *doc);

} // namespace Inkscape::Colors

#endif // INKSCAPE_COLORS_XML_COLOR

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

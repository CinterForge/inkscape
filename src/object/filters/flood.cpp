// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * SVG <feFlood> implementation.
 */
/*
 * Authors:
 *   hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "flood.h"

#include <cstring>                               // for strncmp

#include "attributes.h"                          // for SPAttr

#include "display/nr-filter-flood.h"             // for FilterFlood
#include "object/filters/sp-filter-primitive.h"  // for SPFilterPrimitive
#include "object/sp-object.h"                    // for SP_OBJECT_MODIFIED_FLAG

class SPDocument;

namespace Inkscape {
class DrawingItem;
namespace Filters {
class FilterPrimitive;
} // namespace Filters
namespace XML {
class Node;
} // namespace XML
} // namespace Inkscape

void SPFeFlood::build(SPDocument *document, Inkscape::XML::Node *repr)
{
	SPFilterPrimitive::build(document, repr);

    readAttr(SPAttr::FLOOD_OPACITY);
    readAttr(SPAttr::FLOOD_COLOR);
}

void SPFeFlood::set(SPAttr key, char const *value)
{
    switch (key) {
        case SPAttr::FLOOD_COLOR: {

            flood_color = Inkscape::Colors::Color::parse(value);
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        }
        case SPAttr::FLOOD_OPACITY: {
            double n_opacity;
            if (value) {
                char *end_ptr = nullptr;
                n_opacity = g_ascii_strtod(value, &end_ptr);

                if (end_ptr && *end_ptr) {
                    g_warning("Unable to convert \"%s\" to number", value);
                    n_opacity = 1;
                }
            } else {
                n_opacity = 1;
            }

            if (n_opacity != opacity) {
                opacity = n_opacity;
                requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        default:
        	SPFilterPrimitive::set(key, value);
            break;
    }
}

std::unique_ptr<Inkscape::Filters::FilterPrimitive> SPFeFlood::build_renderer(Inkscape::DrawingItem*) const
{
    auto flood = std::make_unique<Inkscape::Filters::FilterFlood>();
    build_renderer_common(flood.get());
    flood->set_color(flood_color ? flood_color->toRGBA(opacity) : 0x0);
    return flood;
}

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

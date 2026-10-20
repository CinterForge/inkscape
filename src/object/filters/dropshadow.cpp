// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * SVG <feDropShadow> implementation.
 */
/*
 * Authors:
 *   Ryan Malloy <ryan@supported.systems>
 *
 * Copyright (C) 2025 Ryan Malloy
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "dropshadow.h"

#include <2geom/transforms.h>                    // for Translate
#include <cstring>

#include "attributes.h"                          // for SPAttr
#include "display/nr-filter-dropshadow.h"        // for FilterDropShadow (to be created)
#include "object/filters/sp-filter-primitive.h"  // for SPFilterPrimitive
#include "object/sp-object.h"                    // for SP_OBJECT_MODIFIED_FLAG
#include "util/numeric/converters.h"             // for read_number
#include "colors/color.h"                        // for Colors::Color

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

void SPFeDropShadow::build(SPDocument *document, Inkscape::XML::Node *repr)
{
    SPFilterPrimitive::build(document, repr);

    readAttr(SPAttr::DX);
    readAttr(SPAttr::DY);
    readAttr(SPAttr::STDDEVIATION);
    readAttr(SPAttr::FLOOD_COLOR);
    readAttr(SPAttr::FLOOD_OPACITY);
}

void SPFeDropShadow::set(SPAttr key, char const *value)
{
    switch(key) {
        case SPAttr::DX: {
            double read_num = value ? Inkscape::Util::read_number(value) : 2.0;
            if (read_num != dx) {
                dx = read_num;
                requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        case SPAttr::DY: {
            double read_num = value ? Inkscape::Util::read_number(value) : 2.0;
            if (read_num != dy) {
                dy = read_num;
                requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        case SPAttr::STDDEVIATION: {
            double read_num = value ? Inkscape::Util::read_number(value) : 2.0;
            if (read_num != stdDeviation) {
                stdDeviation = read_num;
                requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        case SPAttr::FLOOD_COLOR: {
            flood_color = Inkscape::Colors::Color::parse(value);
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        }
        case SPAttr::FLOOD_OPACITY: {
            double read_num = value ? Inkscape::Util::read_number(value) : 1.0;
            if (read_num != flood_opacity) {
                flood_opacity = read_num;
                requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        default:
            SPFilterPrimitive::set(key, value);
            break;
    }
}

Geom::Rect SPFeDropShadow::calculate_region(Geom::Rect const &region) const
{
    Geom::Rect expanded_region = region;

    // Expand region to accommodate offset and blur
    // The blur needs extra space proportional to stdDeviation
    double blur_expansion = 3.0 * stdDeviation; // 3-sigma rule for Gaussian blur

    // Include the offset in the expansion
    double total_expansion_x = blur_expansion + std::abs(dx);
    double total_expansion_y = blur_expansion + std::abs(dy);

    expanded_region.expandBy(total_expansion_x, total_expansion_y);

    return expanded_region;
}

std::unique_ptr<Inkscape::Filters::FilterPrimitive> SPFeDropShadow::build_renderer(Inkscape::DrawingItem*) const
{
    auto dropshadow = std::make_unique<Inkscape::Filters::FilterDropShadow>();
    build_renderer_common(dropshadow.get());

    dropshadow->set_dx(dx);
    dropshadow->set_dy(dy);
    dropshadow->set_stdDeviation(stdDeviation);
    dropshadow->set_flood_color(flood_color ? flood_color->toRGBA(flood_opacity) : 0x000000ff);
    dropshadow->set_flood_opacity(1.0);  // Opacity is already included in the color

    return std::move(dropshadow);
}
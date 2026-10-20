// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * SVG <feDisplacementMap> implementation.
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

#include "displacementmap.h"

#include "attributes.h"                          // for SPAttr
#include "slot-resolver.h"                       // for SlotResolver

#include "display/nr-filter-displacement-map.h"  // for FilterDisplacementMap
#include "object/filters/sp-filter-primitive.h"  // for SPFilterPrimitive
#include "object/sp-object.h"                    // for SP_OBJECT_MODIFIED_FLAG
#include "util/numeric/converters.h"             // for read_number
#include "util/optstr.h"                         // for assign, to_cstr
#include "xml/document.h"                        // for Document
#include "xml/node.h"                            // for Node

class SPDocument;

namespace Inkscape {
class DrawingItem;
namespace Filters {
class FilterPrimitive;
} // namespace Filters
} // namespace Inkscape

void SPFeDisplacementMap::build(SPDocument *document, Inkscape::XML::Node *repr)
{
	SPFilterPrimitive::build(document, repr);

    readAttr(SPAttr::SCALE);
    readAttr(SPAttr::IN2);
    readAttr(SPAttr::XCHANNELSELECTOR);
    readAttr(SPAttr::YCHANNELSELECTOR);
}

static FilterDisplacementMapChannelSelector read_channel_selector(char const *value)
{
    if (!value) return DISPLACEMENTMAP_CHANNEL_ALPHA;
    
    switch (value[0]) {
        case 'R':
            return DISPLACEMENTMAP_CHANNEL_RED;
            break;
        case 'G':
            return DISPLACEMENTMAP_CHANNEL_GREEN;
            break;
        case 'B':
            return DISPLACEMENTMAP_CHANNEL_BLUE;
            break;
        case 'A':
            return DISPLACEMENTMAP_CHANNEL_ALPHA;
            break;
        default:
            // error
            g_warning("Invalid attribute for Channel Selector. Valid modes are 'R', 'G', 'B' or 'A'");
            break;
    }
    
    return DISPLACEMENTMAP_CHANNEL_ALPHA; // default is Alpha Channel
}

void SPFeDisplacementMap::set(SPAttr key, char const *value)
{
    switch (key) {
        case SPAttr::XCHANNELSELECTOR: {
            auto n_selector = ::read_channel_selector(value);
            if (n_selector != xChannelSelector) {
                xChannelSelector = n_selector;
                requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        case SPAttr::YCHANNELSELECTOR: {
            auto n_selector = ::read_channel_selector(value);
            if (n_selector != yChannelSelector) {
                yChannelSelector = n_selector;
                requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        case SPAttr::SCALE: {
            double n_num = value ? Inkscape::Util::read_number(value) : 0.0;
            if (n_num != scale) {
                scale = n_num;
                requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        case SPAttr::IN2: {
            if (Inkscape::Util::assign(in2_name, value)) {
                requestModified(SP_OBJECT_MODIFIED_FLAG);
                invalidate_parent_slots();
            }
            break;
        }
        default:
        	SPFilterPrimitive::set(key, value);
            break;
    }
}

static char const *get_channelselector_name(FilterDisplacementMapChannelSelector selector)
{
    switch (selector) {
        case DISPLACEMENTMAP_CHANNEL_RED:
            return "R";
        case DISPLACEMENTMAP_CHANNEL_GREEN:
            return "G";
        case DISPLACEMENTMAP_CHANNEL_BLUE:
            return "B";
        case DISPLACEMENTMAP_CHANNEL_ALPHA:
            return "A";
        default:
            return nullptr;
    }
}

Inkscape::XML::Node *SPFeDisplacementMap::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, unsigned flags)
{
    if (!repr) {
        repr = doc->createElement("svg:feDisplacementMap");
    }

    repr->setAttributeOrRemoveIfEmpty("in2", Inkscape::Util::to_cstr(in2_name));
    repr->setAttributeSvgDouble("scale", scale);
    repr->setAttribute("xChannelSelector", get_channelselector_name(xChannelSelector));
    repr->setAttribute("yChannelSelector", get_channelselector_name(yChannelSelector));

    SPFilterPrimitive::write(doc, repr, flags);

    return repr;
}

void SPFeDisplacementMap::resolve_slots(SlotResolver &resolver)
{
    in2_slot = resolver.read(in2_name);
    SPFilterPrimitive::resolve_slots(resolver);
}

std::unique_ptr<Inkscape::Filters::FilterPrimitive> SPFeDisplacementMap::build_renderer(Inkscape::DrawingItem*) const
{
    auto displacement_map = std::make_unique<Inkscape::Filters::FilterDisplacementMap>();
    build_renderer_common(displacement_map.get());

    displacement_map->set_input(1, in2_slot);
    displacement_map->set_scale(scale);
    displacement_map->set_channel_selector(0, xChannelSelector);
    displacement_map->set_channel_selector(1, yChannelSelector);

    return displacement_map;
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

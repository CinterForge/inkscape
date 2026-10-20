// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * SVG <feBlend> implementation.
 */
/*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Niko Kiirala <niko@kiirala.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2006,2007 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "blend.h"

#include <cstring>

#include "attributes.h"                          // for SPAttr
#include "slot-resolver.h"                       // for SlotResolver

#include "display/nr-filter-blend.h"             // for FilterBlend
#include "object/filters/sp-filter-primitive.h"  // for SPFilterPrimitive
#include "object/sp-object.h"                    // for SP_OBJECT_MODIFIED_FLAG
#include "util/optstr.h"                         // for assign, to_cstr
#include "xml/document.h"                        // for Document

class SPDocument;

namespace Inkscape {
class DrawingItem;

namespace Filters {
class FilterPrimitive;
} // namespace Filters
} // namespace Inkscape

void SPFeBlend::build(SPDocument *document, Inkscape::XML::Node *repr)
{
    SPFilterPrimitive::build(document, repr);

    readAttr(SPAttr::MODE);
    readAttr(SPAttr::IN2);
}

static SPBlendMode read_mode(char const *value)
{
    if (!value) {
    	return SP_CSS_BLEND_NORMAL;
    }

    switch (value[0]) {
        case 'n':
            if (std::strcmp(value, "normal") == 0)
                return SP_CSS_BLEND_NORMAL;
            break;
        case 'm':
            if (std::strcmp(value, "multiply") == 0)
                return SP_CSS_BLEND_MULTIPLY;
            break;
        case 's':
            if (std::strcmp(value, "screen") == 0)
                return SP_CSS_BLEND_SCREEN;
            if (std::strcmp(value, "saturation") == 0)
                return SP_CSS_BLEND_SATURATION;
            break;
        case 'd':
            if (std::strcmp(value, "darken") == 0)
                return SP_CSS_BLEND_DARKEN;
            if (std::strcmp(value, "difference") == 0)
                return SP_CSS_BLEND_DIFFERENCE;
            break;
        case 'l':
            if (std::strcmp(value, "lighten") == 0)
                return SP_CSS_BLEND_LIGHTEN;
            if (std::strcmp(value, "luminosity") == 0)
                return SP_CSS_BLEND_LUMINOSITY;
            break;
        case 'o':
            if (std::strcmp(value, "overlay") == 0)
                return SP_CSS_BLEND_OVERLAY;
            break;
        case 'c':
            if (std::strcmp(value, "color-dodge") == 0)
                return SP_CSS_BLEND_COLORDODGE;
            if (std::strcmp(value, "color-burn") == 0)
                return SP_CSS_BLEND_COLORBURN;
            if (std::strcmp(value, "color") == 0)
                return SP_CSS_BLEND_COLOR;
            break;
        case 'h':
            if (std::strcmp(value, "hard-light") == 0)
                return SP_CSS_BLEND_HARDLIGHT;
            if (std::strcmp(value, "hue") == 0)
                return SP_CSS_BLEND_HUE;
            break;
        case 'e':
            if (std::strcmp(value, "exclusion") == 0)
                return SP_CSS_BLEND_EXCLUSION;
        default:
            std::cerr << "SPBlendMode: Unimplemented mode: " << value << std::endl;
            // do nothing by default
            break;
    }

    return SP_CSS_BLEND_NORMAL;
}

void SPFeBlend::set(SPAttr key, char const *value)
{
    switch (key) {
        case SPAttr::MODE: {
            auto mode = ::read_mode(value);
            if (mode != blend_mode) {
                blend_mode = mode;
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

Inkscape::XML::Node *SPFeBlend::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, unsigned flags)
{
    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = doc->createElement("svg:feBlend");
    }

    repr->setAttributeOrRemoveIfEmpty("in2", Inkscape::Util::to_cstr(in2_name));

    char const *mode;
    switch (blend_mode) {
        case SP_CSS_BLEND_NORMAL:
            mode = "normal";      break;
        case SP_CSS_BLEND_MULTIPLY:
            mode = "multiply";    break;
        case SP_CSS_BLEND_SCREEN:
            mode = "screen";      break;
        case SP_CSS_BLEND_DARKEN:
            mode = "darken";      break;
        case SP_CSS_BLEND_LIGHTEN:
            mode = "lighten";     break;
        case SP_CSS_BLEND_OVERLAY:
            mode = "overlay";     break;
        case SP_CSS_BLEND_COLORDODGE:
            mode = "color-dodge"; break;
        case SP_CSS_BLEND_COLORBURN:
            mode = "color-burn";  break;
        case SP_CSS_BLEND_HARDLIGHT:
            mode = "hard-light";  break;
        case SP_CSS_BLEND_SOFTLIGHT:
            mode = "soft-light";  break;
        case SP_CSS_BLEND_DIFFERENCE:
            mode = "difference";  break;
        case SP_CSS_BLEND_EXCLUSION:
            mode = "exclusion";   break;
        case SP_CSS_BLEND_HUE:
            mode = "hue";         break;
        case SP_CSS_BLEND_SATURATION:
            mode = "saturation";  break;
        case SP_CSS_BLEND_COLOR:
            mode = "color";       break;
        case SP_CSS_BLEND_LUMINOSITY:
            mode = "luminosity";  break;
        default:
            mode = nullptr;
    }

    repr->setAttribute("mode", mode);

    return SPFilterPrimitive::write(doc, repr, flags);
}

void SPFeBlend::resolve_slots(SlotResolver &resolver)
{
    in2_slot = resolver.read(in2_name);
    SPFilterPrimitive::resolve_slots(resolver);
}

std::unique_ptr<Inkscape::Filters::FilterPrimitive> SPFeBlend::build_renderer(Inkscape::DrawingItem*) const
{
    auto blend = std::make_unique<Inkscape::Filters::FilterBlend>();
    build_renderer_common(blend.get());

    blend->set_mode(blend_mode);
    blend->set_input(1, in2_slot);

    return blend;
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

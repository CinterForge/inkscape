// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * SVG <feMorphology> implementation.
 */
/*
 * Authors:
 *   Felipe Sanches <juca@members.fsf.org>
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "morphology.h"

#include <cstring>                               // for strcmp

#include "attributes.h"                          // for SPAttr
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

void SPFeMorphology::build(SPDocument *document, Inkscape::XML::Node *repr)
{
	SPFilterPrimitive::build(document, repr);

    readAttr(SPAttr::OPERATOR);
    readAttr(SPAttr::RADIUS);
}

static Inkscape::Filters::FilterMorphologyOperator read_operator(char const *value)
{
    if (!value) {
        return Inkscape::Filters::MORPHOLOGY_OPERATOR_ERODE; // erode is default
    }
    
    switch (value[0]) {
        case 'e':
            if (std::strcmp(value, "erode") == 0) {
            	return Inkscape::Filters::MORPHOLOGY_OPERATOR_ERODE;
            }
            break;
        case 'd':
            if (std::strcmp(value, "dilate") == 0) {
            	return Inkscape::Filters::MORPHOLOGY_OPERATOR_DILATE;
            }
            break;
    }
    
    return Inkscape::Filters::MORPHOLOGY_OPERATOR_ERODE; // erode is default
}

void SPFeMorphology::set(SPAttr key, char const *value)
{
    switch (key) {
        case SPAttr::OPERATOR: {
            auto n_op = ::read_operator(value);
            if (n_op != Operator) {
                Operator = n_op;
                requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        case SPAttr::RADIUS:
            radius.set(value);

            // From SVG spec: If <y-radius> is not provided, it defaults to <x-radius>.
            if (!radius.optNumIsSet()) {
                radius.setOptNumber(radius.getNumber());
            }

            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        default:
        	SPFilterPrimitive::set(key, value);
            break;
    }
}

std::unique_ptr<Inkscape::Filters::FilterPrimitive> SPFeMorphology::build_renderer(Inkscape::DrawingItem*) const
{
    auto morphology = std::make_unique<Inkscape::Filters::FilterMorphology>();
    build_renderer_common(morphology.get());
    
    morphology->set_operator(Operator);
    morphology->set_xradius(radius.getNumber());
    morphology->set_yradius(radius.getOptNumber());

    return morphology;
}

/**
 * Calculate the region taken up by a mophoplogy primitive
 *
 * @param region The original shape's region or previous primitive's region output.
 */
Geom::Rect SPFeMorphology::calculate_region(Geom::Rect const &region) const
{
    auto r = region;
    if (Operator == Inkscape::Filters::MORPHOLOGY_OPERATOR_DILATE) {
        if (radius.optNumIsSet()) {
            r.expandBy(radius.getNumber(), radius.getOptNumber());
        } else {
            r.expandBy(radius.getNumber());
        }
    } else if (Operator == Inkscape::Filters::MORPHOLOGY_OPERATOR_ERODE) {
        if (radius.optNumIsSet()) {
            r.expandBy(-1 * radius.getNumber(), -1 * radius.getOptNumber());
        } else {
            r.expandBy(-1 * radius.getNumber());
        }
    }
    return r;
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

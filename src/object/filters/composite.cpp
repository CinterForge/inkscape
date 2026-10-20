// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * SVG <feComposite> implementation.
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

#include "composite.h"

#include <cstring>                               // for strcmp
#include <iostream>                              // for char_traits, operator<<

#include "attributes.h"                          // for SPAttr
#include "slot-resolver.h"                       // for SlotResolver

#include "display/nr-filter-composite.h"         // for FilterComposite
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

void SPFeComposite::build(SPDocument *document, Inkscape::XML::Node *repr)
{
	SPFilterPrimitive::build(document, repr);

    readAttr(SPAttr::OPERATOR);
    readAttr(SPAttr::K1);
    readAttr(SPAttr::K2);
    readAttr(SPAttr::K3);
    readAttr(SPAttr::K4);
    readAttr(SPAttr::IN2);
}

static FeCompositeOperator read_operator(char const *value)
{
    if (!value) {
    	return COMPOSITE_DEFAULT;
    }

    if (std::strcmp(value, "over") == 0) {
    	return COMPOSITE_OVER;
    } else if (std::strcmp(value, "in") == 0) {
    	return COMPOSITE_IN;
    } else if (std::strcmp(value, "out") == 0) {
    	return COMPOSITE_OUT;
    } else if (std::strcmp(value, "atop") == 0) {
    	return COMPOSITE_ATOP;
    } else if (std::strcmp(value, "xor") == 0) {
    	return COMPOSITE_XOR;
    } else if (std::strcmp(value, "arithmetic") == 0) {
    	return COMPOSITE_ARITHMETIC;
    } else if (std::strcmp(value, "lighter") == 0) {
    	return COMPOSITE_LIGHTER;
    }

    std::cerr << "Inkscape::Filters::FilterCompositeOperator: Unimplemented operator: " << value << std::endl;
    return COMPOSITE_DEFAULT;
}

void SPFeComposite::set(SPAttr key, char const *value)
{
    switch (key) {
        case SPAttr::OPERATOR: {
            auto n_op = ::read_operator(value);
            if (n_op != composite_operator) {
                composite_operator = n_op;
                requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }

        case SPAttr::K1: {
            double n_k = value ? Inkscape::Util::read_number(value) : 0.0;
            if (n_k != k1) {
                k1 = n_k;
                if (composite_operator == COMPOSITE_ARITHMETIC)
                    requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }

        case SPAttr::K2: {
            double n_k = value ? Inkscape::Util::read_number(value) : 0.0;
            if (n_k != k2) {
                k2 = n_k;
                if (composite_operator == COMPOSITE_ARITHMETIC)
                    requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }

        case SPAttr::K3: {
            double n_k = value ? Inkscape::Util::read_number(value) : 0.0;
            if (n_k != k3) {
                k3 = n_k;
                if (composite_operator == COMPOSITE_ARITHMETIC)
                    requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }

        case SPAttr::K4: {
            double n_k = value ? Inkscape::Util::read_number(value) : 0.0;
            if (n_k != k4) {
                k4 = n_k;
                if (composite_operator == COMPOSITE_ARITHMETIC)
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

Inkscape::XML::Node *SPFeComposite::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags)
{
    if (!repr) {
        repr = doc->createElement("svg:feComposite");
    }

    repr->setAttributeOrRemoveIfEmpty("in2", Inkscape::Util::to_cstr(in2_name));

    char const *comp_op;
    switch (composite_operator) {
        case COMPOSITE_OVER:
            comp_op = "over"; break;
        case COMPOSITE_IN:
            comp_op = "in"; break;
        case COMPOSITE_OUT:
            comp_op = "out"; break;
        case COMPOSITE_ATOP:
            comp_op = "atop"; break;
        case COMPOSITE_XOR:
            comp_op = "xor"; break;
        case COMPOSITE_ARITHMETIC:
            comp_op = "arithmetic"; break;
        case COMPOSITE_LIGHTER:
            comp_op = "lighter"; break;
        default:
            comp_op = nullptr;
    }
    repr->setAttribute("operator", comp_op);

    if (composite_operator == COMPOSITE_ARITHMETIC) {
        repr->setAttributeSvgDouble("k1", k1);
        repr->setAttributeSvgDouble("k2", k2);
        repr->setAttributeSvgDouble("k3", k3);
        repr->setAttributeSvgDouble("k4", k4);
    } else {
        repr->removeAttribute("k1");
        repr->removeAttribute("k2");
        repr->removeAttribute("k3");
        repr->removeAttribute("k4");
    }

    return SPFilterPrimitive::write(doc, repr, flags);
}

void SPFeComposite::resolve_slots(SlotResolver &resolver)
{
    in2_slot = resolver.read(in2_name);
    SPFilterPrimitive::resolve_slots(resolver);
}

std::unique_ptr<Inkscape::Filters::FilterPrimitive> SPFeComposite::build_renderer(Inkscape::DrawingItem*) const
{
    auto composite = std::make_unique<Inkscape::Filters::FilterComposite>();
    build_renderer_common(composite.get());

    composite->set_operator(composite_operator);
    composite->set_input(1, in2_slot);

    if (composite_operator == COMPOSITE_ARITHMETIC) {
        composite->set_arithmetic(k1, k2, k3, k4);
    }

    return composite;
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

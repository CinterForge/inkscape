// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * SVG <funcR>, <funcG>, <funcB> and <funcA> implementations.
 */
/*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Niko Kiirala <niko@kiirala.com>
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2006, 2007, 2008 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "componenttransfer-funcnode.h"

#include <cstring>                    // for strcmp
#include "attributes.h"               // for SPAttr
#include "document.h"                 // for SPDocument
#include "util/numeric/converters.h"  // for read_number, read_vector

namespace Inkscape::XML {
class Node;
} // namespace Inkscape::XML

void SPFeFuncNode::build(SPDocument *document, Inkscape::XML::Node *repr)
{
	SPObject::build(document, repr);

    readAttr(SPAttr::TYPE);
    readAttr(SPAttr::TABLEVALUES);
    readAttr(SPAttr::SLOPE);
    readAttr(SPAttr::INTERCEPT);
    readAttr(SPAttr::AMPLITUDE);
    readAttr(SPAttr::EXPONENT);
    readAttr(SPAttr::OFFSET);

    document->addResource("fefuncnode", this);
}

void SPFeFuncNode::release()
{
    if (document) {
        document->removeResource("fefuncnode", this);
    }

    tableValues.clear();

    SPObject::release();
}

static Inkscape::Filters::FilterComponentTransferType sp_feComponenttransfer_read_type(char const *value)
{
    if (!value) {
    	return Inkscape::Filters::COMPONENTTRANSFER_TYPE_ERROR; //type attribute is REQUIRED.
    }

    switch (value[0]) {
        case 'i':
            if (!std::strcmp(value, "identity")) {
            	return Inkscape::Filters::COMPONENTTRANSFER_TYPE_IDENTITY;
            }
            break;
        case 't':
            if (!std::strcmp(value, "table")) {
            	return Inkscape::Filters::COMPONENTTRANSFER_TYPE_TABLE;
            }
            break;
        case 'd':
            if (!std::strcmp(value, "discrete")) {
            	return Inkscape::Filters::COMPONENTTRANSFER_TYPE_DISCRETE;
            }
            break;
        case 'l':
            if (!std::strcmp(value, "linear")) {
            	return Inkscape::Filters::COMPONENTTRANSFER_TYPE_LINEAR;
            }
            break;
        case 'g':
            if (!std::strcmp(value, "gamma")) {
            	return Inkscape::Filters::COMPONENTTRANSFER_TYPE_GAMMA;
            }
            break;
        default:
            break;
    }

    return Inkscape::Filters::COMPONENTTRANSFER_TYPE_ERROR; //type attribute is REQUIRED.
}

void SPFeFuncNode::set(SPAttr key, char const *value)
{
    switch (key) {
        case SPAttr::TYPE: {
            auto const new_type = sp_feComponenttransfer_read_type(value);

            if (type != new_type) {
                type = new_type;
                requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        case SPAttr::TABLEVALUES: {
            if (value) {
                tableValues = Inkscape::Util::read_vector(value);
            } else {
                tableValues.clear();
            }
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        }
        case SPAttr::SLOPE: {
            auto new_slope = value ? Inkscape::Util::read_number(value) : 1;

            if (slope != new_slope) {
                slope = new_slope;
                requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        case SPAttr::INTERCEPT: {
            auto new_intercept = value ? Inkscape::Util::read_number(value) : 0;

            if (intercept != new_intercept) {
                intercept = new_intercept;
                requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        case SPAttr::AMPLITUDE: {
            auto new_amplitude = value ? Inkscape::Util::read_number(value) : 1;

            if (amplitude != new_amplitude) {
                amplitude = new_amplitude;
                requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        case SPAttr::EXPONENT: {
            auto new_exponent = value ? Inkscape::Util::read_number(value) : 1;

            if (exponent != new_exponent) {
                exponent = new_exponent;
                requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        case SPAttr::OFFSET: {
            auto new_offset = value ? Inkscape::Util::read_number(value) : 0;

            if (offset != new_offset) {
                offset = new_offset;
                requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        default:
        	SPObject::set(key, value);
            break;
    }
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

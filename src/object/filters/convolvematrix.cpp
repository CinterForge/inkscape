// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * SVG <feConvolveMatrix> implementation.
 */
/*
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *   hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "convolvematrix.h"

#include <cmath>                                 // for floor
#include <cstring>                               // for strcmp
#include <vector>                                // for vector

#include "attributes.h"                          // for SPAttr
#include "object/filters/sp-filter-primitive.h"  // for SPFilterPrimitive
#include "object/sp-object.h"                    // for SP_OBJECT_MODIFIED_FLAG
#include "util/numeric/converters.h"             // for read_number, read_bool

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

void SPFeConvolveMatrix::build(SPDocument *document, Inkscape::XML::Node *repr)
{
	SPFilterPrimitive::build(document, repr);

    readAttr(SPAttr::ORDER);
    readAttr(SPAttr::KERNELMATRIX);
    readAttr(SPAttr::DIVISOR);
    readAttr(SPAttr::BIAS);
    readAttr(SPAttr::TARGETX);
    readAttr(SPAttr::TARGETY);
    readAttr(SPAttr::EDGEMODE);
    readAttr(SPAttr::KERNELUNITLENGTH);
    readAttr(SPAttr::PRESERVEALPHA);
}

static Inkscape::Filters::FilterConvolveMatrixEdgeMode read_edgemode(char const *value)
{
    if (!value) {
        return Inkscape::Filters::CONVOLVEMATRIX_EDGEMODE_DUPLICATE; // duplicate is default
    }
    
    switch (value[0]) {
        case 'd':
            if (std::strcmp(value, "duplicate") == 0) {
            	return Inkscape::Filters::CONVOLVEMATRIX_EDGEMODE_DUPLICATE;
            }
            break;
        case 'w':
            if (std::strcmp(value, "wrap") == 0) {
            	return Inkscape::Filters::CONVOLVEMATRIX_EDGEMODE_WRAP;
            }
            break;
        case 'n':
            if (std::strcmp(value, "none") == 0) {
            	return Inkscape::Filters::CONVOLVEMATRIX_EDGEMODE_NONE;
            }
            break;
    }
    
    return Inkscape::Filters::CONVOLVEMATRIX_EDGEMODE_DUPLICATE; //duplicate is default
}

void SPFeConvolveMatrix::set(SPAttr key, gchar const *value)
{
    switch (key) {
        case SPAttr::ORDER:
            order.set(value);
            
            // From SVG spec: If <orderY> is not provided, it defaults to <orderX>.
            if (!order.optNumIsSet()) {
                order.setOptNumber(order.getNumber());
            }
            
            if (!targetXIsSet) {
                targetX = std::floor(order.getNumber() / 2);
            }
            
            if (!targetYIsSet) {
                targetY = std::floor(order.getOptNumber() / 2);
            }
            
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::KERNELMATRIX:
            if (value) {
                kernelMatrixIsSet = true;
                kernelMatrix = Inkscape::Util::read_vector(value);
                
                if (!divisorIsSet) {
                    divisor = 0;
                    
                    for (double i : kernelMatrix) {
                        divisor += i;
                    }
                    
                    if (divisor == 0) {
                        divisor = 1;
                    }
                }
                
                requestModified(SP_OBJECT_MODIFIED_FLAG);
            } else {
                g_warning("For feConvolveMatrix you MUST pass a kernelMatrix parameter!");
            }
            break;
        case SPAttr::DIVISOR: {
            if (value) { 
                double n_num = Inkscape::Util::read_number(value);
                
                if (n_num == 0) {
                    // This should actually be an error, but given our UI it is more useful to simply set divisor to the default.
                    if (kernelMatrixIsSet) {
                        for (double i : kernelMatrix) {
                            n_num += i;
                        }
                    }
                    
                    if (n_num == 0) {
                        n_num = 1;
                    }

                    if (divisorIsSet || divisor != n_num) {
                        divisorIsSet = false;
                        divisor = n_num;
                        requestModified(SP_OBJECT_MODIFIED_FLAG);
                    }
                } else if (!divisorIsSet || divisor != n_num) {
                    divisorIsSet = true;
                    divisor = n_num;
                    requestModified(SP_OBJECT_MODIFIED_FLAG);
                }
            }
            break;
        }
        case SPAttr::BIAS: {
            double n_num = 0;
            if (value) {
                n_num = Inkscape::Util::read_number(value);
            }
            if (n_num != bias) {
                bias = n_num;
                requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        case SPAttr::TARGETX:
            if (value) {
                int n_int = Inkscape::Util::read_number(value);
                
                if (n_int < 0 || n_int > order.getNumber()) {
                    g_warning("targetX must be a value between 0 and orderX! Assuming floor(orderX/2) as default value.");
                    n_int = std::floor(order.getNumber() / 2.0);
                }
                
                targetXIsSet = true;
                
                if (n_int != targetX) {
                    targetX = n_int;
                    requestModified(SP_OBJECT_MODIFIED_FLAG);
                }
            }
            break;
        case SPAttr::TARGETY:
            if (value) {
                int n_int = Inkscape::Util::read_number(value);
                
                if (n_int < 0 || n_int > order.getOptNumber()) {
                    g_warning("targetY must be a value between 0 and orderY! Assuming floor(orderY/2) as default value.");
                    n_int = std::floor(order.getOptNumber() / 2.0);
                }
                
                targetYIsSet = true;
                
                if (n_int != targetY){
                    targetY = n_int;
                    requestModified(SP_OBJECT_MODIFIED_FLAG);
                }
            }
            break;
        case SPAttr::EDGEMODE: {
            auto n_mode = ::read_edgemode(value);
            if (n_mode != edgeMode) {
                edgeMode = n_mode;
                requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        case SPAttr::KERNELUNITLENGTH:
            kernelUnitLength.set(value);
            
            //From SVG spec: If the <dy> value is not specified, it defaults to the same value as <dx>.
            if (!kernelUnitLength.optNumIsSet()) {
                kernelUnitLength.setOptNumber(kernelUnitLength.getNumber());
            }
            
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::PRESERVEALPHA: {
            bool read_bool = Inkscape::Util::read_bool(value, false);
            if (read_bool != preserveAlpha) {
                preserveAlpha = read_bool;
                requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        default:
        	SPFilterPrimitive::set(key, value);
            break;
    }
}

std::unique_ptr<Inkscape::Filters::FilterPrimitive> SPFeConvolveMatrix::build_renderer(Inkscape::DrawingItem*) const
{
    auto convolve = std::make_unique<Inkscape::Filters::FilterConvolveMatrix>();
    build_renderer_common(convolve.get());

    convolve->set_targetX(targetX);
    convolve->set_targetY(targetY);
    convolve->set_orderX(order.getNumber());
    convolve->set_orderY(order.getOptNumber());
    convolve->set_kernelMatrix(kernelMatrix);
    convolve->set_divisor(divisor);
    convolve->set_bias(bias);
    convolve->set_preserveAlpha(preserveAlpha);

    return convolve;
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

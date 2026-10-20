// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * SVG <feColorMatrix> implementation.
 */
/*
 * Authors:
 *   Felipe Sanches <juca@members.fsf.org>
 *   hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2007 Felipe C. da S. Sanches
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "colormatrix.h"

#include <cstring>                               // for strcmp

#include "attributes.h"                          // for SPAttr
#include "object/filters/sp-filter-primitive.h"  // for SPFilterPrimitive
#include "object/sp-object.h"                    // for SP_OBJECT_MODIFIED_FLAG
#include "util/numeric/converters.h"             // for read_number, read_ve...

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

void SPFeColorMatrix::build(SPDocument *document, Inkscape::XML::Node *repr)
{
	SPFilterPrimitive::build(document, repr);

    readAttr(SPAttr::TYPE);
    readAttr(SPAttr::VALUES);
}

static Inkscape::Filters::FilterColorMatrixType read_type(char const *str)
{
    if (!str) {
    	return Inkscape::Filters::COLORMATRIX_MATRIX; //matrix is default
    }

    switch (str[0]) {
        case 'm':
            if (std::strcmp(str, "matrix") == 0) return Inkscape::Filters::COLORMATRIX_MATRIX;
            break;
        case 's':
            if (std::strcmp(str, "saturate") == 0) return Inkscape::Filters::COLORMATRIX_SATURATE;
            break;
        case 'h':
            if (std::strcmp(str, "hueRotate") == 0) return Inkscape::Filters::COLORMATRIX_HUEROTATE;
            break;
        case 'l':
            if (std::strcmp(str, "luminanceToAlpha") == 0) return Inkscape::Filters::COLORMATRIX_LUMINANCETOALPHA;
            break;
    }

    return Inkscape::Filters::COLORMATRIX_MATRIX; //matrix is default
}

void SPFeColorMatrix::set(SPAttr key, char const *str)
{
    auto set_default_value = [this] {
        switch (type) {
            case Inkscape::Filters::COLORMATRIX_MATRIX:
                values = {1, 0, 0, 0, 0,  0, 1, 0, 0, 0,  0, 0, 1, 0, 0,  0, 0, 0, 1, 0};
                break;
            case Inkscape::Filters::COLORMATRIX_SATURATE:
                // Default value for saturate is 1.0 ("values" not used).
                value = 1;
                break;
            case Inkscape::Filters::COLORMATRIX_HUEROTATE:
                value = 0;
                break;
            case Inkscape::Filters::COLORMATRIX_LUMINANCETOALPHA:
                // value, values not used.
                break;
        }
    };

    switch (key) {
        case SPAttr::TYPE: {
            auto n_type = ::read_type(str);
            if (type != n_type){
                type = n_type;
                if (!value_set) set_default_value();
                requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        case SPAttr::VALUES:
            if (str) {
                values = Inkscape::Util::read_vector(str);
                value = Inkscape::Util::read_number(str, Inkscape::Util::NO_WARNING);
                value_set = true;
            } else {
                set_default_value();
                value_set = false;
            }
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        default:
        	SPFilterPrimitive::set(key, str);
            break;
    }
}

std::unique_ptr<Inkscape::Filters::FilterPrimitive> SPFeColorMatrix::build_renderer(Inkscape::DrawingItem*) const
{
    auto colormatrix = std::make_unique<Inkscape::Filters::FilterColorMatrix>();
    build_renderer_common(colormatrix.get());

    colormatrix->set_type(type);
    colormatrix->set_value(value);
    colormatrix->set_values(values);

    return colormatrix;
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

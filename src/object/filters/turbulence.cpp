// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * SVG <feTurbulence> implementation.
 */
/*
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *   hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2007 Felipe Sanches
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "turbulence.h"

#include <cmath>                                 // for floor
#include <cstring>                               // for strcmp

#include "attributes.h"                          // for SPAttr

#include "object/filters/sp-filter-primitive.h"  // for SPFilterPrimitive
#include "object/sp-object.h"                    // for SP_OBJECT_MODIFIED_FLAG
#include "util/numeric/converters.h"             // for read_number
#include "xml/node.h"                            // for Node

class SPDocument;

namespace Inkscape {
class DrawingItem;
namespace Filters {
class FilterPrimitive;
} // namespace Filters
namespace XML {
class Document;
} // namespace XML
} // namespace Inkscape

void SPFeTurbulence::build(SPDocument *document, Inkscape::XML::Node *repr)
{
	SPFilterPrimitive::build(document, repr);

    readAttr(SPAttr::BASEFREQUENCY);
    readAttr(SPAttr::NUMOCTAVES);
    readAttr(SPAttr::SEED);
    readAttr(SPAttr::STITCHTILES);
    readAttr(SPAttr::TYPE);
}

static bool read_stitchtiles(char const *value)
{
    if (!value) {
    	return false; // 'noStitch' is default
    }

    switch (value[0]) {
        case 's':
            if (std::strcmp(value, "stitch") == 0) {
            	return true;
            }
            break;
        case 'n':
            if (std::strcmp(value, "noStitch") == 0) {
            	return false;
            }
            break;
    }

    return false; // 'noStitch' is default
}

static Inkscape::Filters::FilterTurbulenceType read_type(char const *value)
{
    if (!value) {
    	return Inkscape::Filters::TURBULENCE_TURBULENCE; // 'turbulence' is default
    }

    switch (value[0]) {
        case 'f':
            if (std::strcmp(value, "fractalNoise") == 0) {
            	return Inkscape::Filters::TURBULENCE_FRACTALNOISE;
            }
            break;
        case 't':
            if (std::strcmp(value, "turbulence") == 0) {
            	return Inkscape::Filters::TURBULENCE_TURBULENCE;
            }
            break;
    }

    return Inkscape::Filters::TURBULENCE_TURBULENCE; // 'turbulence' is default
}

void SPFeTurbulence::set(SPAttr key, char const *value)
{
    switch (key) {
        case SPAttr::BASEFREQUENCY:
            baseFrequency.set(value);

            // From SVG spec: If two <number>s are provided, the first number represents
            // a base frequency in the X direction and the second value represents a base
            // frequency in the Y direction. If one number is provided, then that value is
            // used for both X and Y.
            if (baseFrequency.optNumIsSet() == false) {
                baseFrequency.setOptNumber(baseFrequency.getNumber());
            }

            updated = false;
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::NUMOCTAVES: {
            int n_int = value ? (int)std::floor(Inkscape::Util::read_number(value)) : 1;
            if (n_int != numOctaves){
                numOctaves = n_int;
                updated = false;
                requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        case SPAttr::SEED: {
            double n_num = value ? Inkscape::Util::read_number(value) : 0;
            if (n_num != seed){
                seed = n_num;
                updated = false;
                requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        case SPAttr::STITCHTILES: {
            bool n_bool = ::read_stitchtiles(value);
            if (n_bool != stitchTiles){
                stitchTiles = n_bool;
                updated = false;
                requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        case SPAttr::TYPE: {
            auto n_type = ::read_type(value);
            if (n_type != type) {
                type = n_type;
                updated = false;
                requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        default:
        	SPFilterPrimitive::set(key, value);
            break;
    }
}

Inkscape::XML::Node *SPFeTurbulence::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, unsigned flags)
{
    // TODO: Don't just clone, but create a new repr node and write all relevant values into it.
    if (!repr) {
        repr = getRepr()->duplicate(doc);
    }

    SPFilterPrimitive::write(doc, repr, flags);

    // turbulence doesn't take input
    repr->removeAttribute("in");

    return repr;
}

std::unique_ptr<Inkscape::Filters::FilterPrimitive> SPFeTurbulence::build_renderer(Inkscape::DrawingItem*) const
{
    auto turbulence = std::make_unique<Inkscape::Filters::FilterTurbulence>();
    build_renderer_common(turbulence.get());

    turbulence->set_baseFrequency(0, baseFrequency.getNumber());
    turbulence->set_baseFrequency(1, baseFrequency.getOptNumber());
    turbulence->set_numOctaves(numOctaves);
    turbulence->set_seed(seed);
    turbulence->set_stitchTiles(stitchTiles);
    turbulence->set_type(type);
    turbulence->set_updated(updated);

    return turbulence;
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

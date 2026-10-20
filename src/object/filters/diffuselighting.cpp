// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * SVG <feDiffuseLighting> implementation.
 */
/*
 * Authors:
 *   hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Jean-Rene Reinhard <jr@komite.net>
 *   Abhishek Sharma
 *
 * Copyright (C) 2006 Hugo Rodrigues
 *               2007 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "diffuselighting.h"

#include <cstring>                               // for strncmp
#include <vector>                                // for vector

#include "attributes.h"                          // for SPAttr
#include "distantlight.h"                        // for SPFeDistantLight
#include "pointlight.h"                          // for SPFePointLight
#include "spotlight.h"                           // for SPFeSpotLight

#include "display/nr-filter-diffuselighting.h"   // for FilterDiffuseLighting
#include "display/nr-light-types.h"              // for SpotLightData, Light...
#include "object/filters/sp-filter-primitive.h"  // for SPFilterPrimitive
#include "object/sp-object.h"                    // for SP_OBJECT_MODIFIED_FLAG
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

void SPFeDiffuseLighting::build(SPDocument *document, Inkscape::XML::Node *repr)
{
	SPFilterPrimitive::build(document, repr);

    readAttr(SPAttr::SURFACESCALE);
    readAttr(SPAttr::DIFFUSECONSTANT);
    readAttr(SPAttr::KERNELUNITLENGTH);
    readAttr(SPAttr::LIGHTING_COLOR);
}

void SPFeDiffuseLighting::set(SPAttr key, char const *value)
{
    // TODO test forbidden values
    switch (key) {
        case SPAttr::SURFACESCALE: {
            char *end_ptr = nullptr;

            if (value) {
                surfaceScale = g_ascii_strtod(value, &end_ptr);

                if (end_ptr) {
                    surfaceScale_set = true;
                }
            }

            if (!value || !end_ptr) {
                surfaceScale = 1;
                surfaceScale_set = false;
            }

            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        }
        case SPAttr::DIFFUSECONSTANT: {
            char *end_ptr = nullptr;

            if (value) {
                diffuseConstant = g_ascii_strtod(value, &end_ptr);

                if (end_ptr && diffuseConstant >= 0) {
                    diffuseConstant_set = true;
                } else {
                    end_ptr = nullptr;
                    g_warning("this: diffuseConstant should be a positive number ... defaulting to 1");
                }
            } 

            if (!value || !end_ptr) {
                diffuseConstant = 1;
                diffuseConstant_set = false;
            }

            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        }
        case SPAttr::KERNELUNITLENGTH:
            // TODO kernelUnit
            // kernelUnitLength.set(value);
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::LIGHTING_COLOR: {
            lighting_color = Inkscape::Colors::Color::parse(value);
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        }
        default:
        	SPFilterPrimitive::set(key, value);
            break;
    }
}

void SPFeDiffuseLighting::modified(unsigned flags)
{
    auto const cflags = cascade_flags(flags);

    for (auto c : childList(true)) {
        if (cflags || (c->mflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            c->emitModified(cflags);
        }
        sp_object_unref(c, nullptr);
    }
}

Inkscape::XML::Node *SPFeDiffuseLighting::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, unsigned flags)
{
    // TODO: Don't just clone, but create a new repr node and write all relevant values _and children_ into it.
    if (!repr) {
        repr = getRepr()->duplicate(doc);
        //repr = doc->createElement("svg:feDiffuseLighting");
    }
    
    if (surfaceScale_set) {
        repr->setAttributeCssDouble("surfaceScale", surfaceScale);
    } else {
        repr->removeAttribute("surfaceScale");
    }

    if (diffuseConstant_set) {
        repr->setAttributeCssDouble("diffuseConstant", diffuseConstant);
    } else {
        repr->removeAttribute("diffuseConstant");
    }

    /*TODO kernelUnits */
    if (lighting_color) {
        repr->setAttributeOrRemoveIfEmpty("lighting-color", lighting_color->toString());
    }
        
    SPFilterPrimitive::write(doc, repr, flags);

    return repr;
}

void SPFeDiffuseLighting::child_added(Inkscape::XML::Node *child, Inkscape::XML::Node *ref)
{
    SPFilterPrimitive::child_added(child, ref);
    requestModified(SP_OBJECT_MODIFIED_FLAG);
}

void SPFeDiffuseLighting::remove_child(Inkscape::XML::Node *child)
{
    SPFilterPrimitive::remove_child(child);
    requestModified(SP_OBJECT_MODIFIED_FLAG);
}

void SPFeDiffuseLighting::order_changed(Inkscape::XML::Node *child, Inkscape::XML::Node *old_ref, Inkscape::XML::Node *new_ref)
{
    SPFilterPrimitive::order_changed(child, old_ref, new_ref);
    requestModified(SP_OBJECT_MODIFIED_FLAG);
}

std::unique_ptr<Inkscape::Filters::FilterPrimitive> SPFeDiffuseLighting::build_renderer(Inkscape::DrawingItem*) const
{
    auto diffuselighting = std::make_unique<Inkscape::Filters::FilterDiffuseLighting>();
    build_renderer_common(diffuselighting.get());

    diffuselighting->diffuseConstant = diffuseConstant;
    diffuselighting->surfaceScale = surfaceScale;
    diffuselighting->lighting_color = lighting_color ? lighting_color->toRGBA() : 0x0;

    // We assume there is at most one child
    diffuselighting->light_type = Inkscape::Filters::NO_LIGHT;

    if (auto l = cast<SPFeDistantLight>(firstChild())) {
        diffuselighting->light_type = Inkscape::Filters::DISTANT_LIGHT;
        diffuselighting->light.distant.azimuth = l->azimuth;
        diffuselighting->light.distant.elevation = l->elevation;
    } else if (auto l = cast<SPFePointLight>(firstChild())) {
        diffuselighting->light_type = Inkscape::Filters::POINT_LIGHT;
        diffuselighting->light.point.x = l->x;
        diffuselighting->light.point.y = l->y;
        diffuselighting->light.point.z = l->z;
    } else if (auto l = cast<SPFeSpotLight>(firstChild())) {
        diffuselighting->light_type = Inkscape::Filters::SPOT_LIGHT;
        diffuselighting->light.spot.x = l->x;
        diffuselighting->light.spot.y = l->y;
        diffuselighting->light.spot.z = l->z;
        diffuselighting->light.spot.pointsAtX = l->pointsAtX;
        diffuselighting->light.spot.pointsAtY = l->pointsAtY;
        diffuselighting->light.spot.pointsAtZ = l->pointsAtZ;
        diffuselighting->light.spot.limitingConeAngle = l->limitingConeAngle;
        diffuselighting->light.spot.specularExponent = l->specularExponent;
    }

    return diffuselighting;
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

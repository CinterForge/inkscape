// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * SVG <feSpecularLighting> implementation.
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

#include "specularlighting.h"

#include "attributes.h"                          // for SPAttr
#include "distantlight.h"                        // for SPFeDistantLight
#include "pointlight.h"                          // for SPFePointLight
#include "spotlight.h"                           // for SPFeSpotLight
#include "strneq.h"                              // for strneq

#include "display/nr-filter-specularlighting.h"  // for FilterSpecularLighting
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

void SPFeSpecularLighting::build(SPDocument *document, Inkscape::XML::Node *repr)
{
	SPFilterPrimitive::build(document, repr);

    readAttr(SPAttr::SURFACESCALE);
    readAttr(SPAttr::SPECULARCONSTANT);
    readAttr(SPAttr::SPECULAREXPONENT);
    readAttr(SPAttr::KERNELUNITLENGTH);
    readAttr(SPAttr::LIGHTING_COLOR);
}

void SPFeSpecularLighting::set(SPAttr key, char const *value)
{
    // TODO test forbidden values
    switch (key) {
        case SPAttr::SURFACESCALE: {
            char *end_ptr = nullptr;
            if (value) {
                surfaceScale = g_ascii_strtod(value, &end_ptr);
                if (end_ptr) {
                    surfaceScale_set = true;
                } else {
                    g_warning("this: surfaceScale should be a number ... defaulting to 1");
                }
            }
            // if the attribute is not set or has an unreadable value
            if (!value || !end_ptr) {
                surfaceScale = 1;
                surfaceScale_set = FALSE;
            }
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        }
        case SPAttr::SPECULARCONSTANT: {
            char *end_ptr = nullptr;
            if (value) {
                specularConstant = g_ascii_strtod(value, &end_ptr);
                if (end_ptr && specularConstant >= 0) {
                    specularConstant_set = true;
                } else {
                    end_ptr = nullptr;
                    g_warning("this: specularConstant should be a positive number ... defaulting to 1");
                }
            }
            if (!value || !end_ptr) {
                specularConstant = 1;
                specularConstant_set = FALSE;
            }
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        }
        case SPAttr::SPECULAREXPONENT: {
            char *end_ptr = nullptr;
            if (value) {
                specularExponent = g_ascii_strtod(value, &end_ptr);
                if (specularExponent >= 1 && specularExponent <= 128) {
                    specularExponent_set = true;
                } else {
                    end_ptr = nullptr;
                    g_warning("this: specularExponent should be a number in range [1, 128] ... defaulting to 1");
                }
            } 
            if (!value || !end_ptr) {
                specularExponent = 1;
                specularExponent_set = FALSE;
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

void SPFeSpecularLighting::modified(unsigned flags)
{
    auto const cflags = cascade_flags(flags);

    for (auto c : childList(true)) {
        if (cflags || (c->mflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            c->emitModified(cflags);
        }
        sp_object_unref(c, nullptr);
    }
}

Inkscape::XML::Node *SPFeSpecularLighting::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, unsigned flags)
{
    /* TODO: Don't just clone, but create a new repr node and write all
     * relevant values _and children_ into it */
    if (!repr) {
        repr = getRepr()->duplicate(doc);
        //repr = doc->createElement("svg:feSpecularLighting");
    }

    if (surfaceScale_set) {
        repr->setAttributeCssDouble("surfaceScale", surfaceScale);
    }

    if (specularConstant_set) {
        repr->setAttributeCssDouble("specularConstant", specularConstant);
    }

    if (specularExponent_set) {
        repr->setAttributeCssDouble("specularExponent", specularExponent);
    }

    // TODO kernelUnits
    
    if (lighting_color) {
        repr->setAttributeOrRemoveIfEmpty("lighting-color", lighting_color->toString());
    }
    SPFilterPrimitive::write(doc, repr, flags);

    return repr;
}

void SPFeSpecularLighting::child_added(Inkscape::XML::Node *child, Inkscape::XML::Node *ref)
{
    SPFilterPrimitive::child_added(child, ref);
    requestModified(SP_OBJECT_MODIFIED_FLAG);
}

void SPFeSpecularLighting::remove_child(Inkscape::XML::Node *child)
{
    SPFilterPrimitive::remove_child(child);
    requestModified(SP_OBJECT_MODIFIED_FLAG);
}

void SPFeSpecularLighting::order_changed(Inkscape::XML::Node *child, Inkscape::XML::Node *old_ref, Inkscape::XML::Node *new_ref)
{
    SPFilterPrimitive::order_changed(child, old_ref, new_ref);
    requestModified(SP_OBJECT_MODIFIED_FLAG);
}

std::unique_ptr<Inkscape::Filters::FilterPrimitive> SPFeSpecularLighting::build_renderer(Inkscape::DrawingItem*) const
{
    auto specularlighting = std::make_unique<Inkscape::Filters::FilterSpecularLighting>();
    build_renderer_common(specularlighting.get());

    specularlighting->specularConstant = specularConstant;
    specularlighting->specularExponent = specularExponent;
    specularlighting->surfaceScale = surfaceScale;
    specularlighting->lighting_color = lighting_color ? lighting_color->toRGBA() : 0xffffffff;

    // We assume there is at most one child
    specularlighting->light_type = Inkscape::Filters::NO_LIGHT;

    if (auto l = cast<SPFeDistantLight>(firstChild())) {
        specularlighting->light_type = Inkscape::Filters::DISTANT_LIGHT;
        specularlighting->light.distant.azimuth = l->azimuth;
        specularlighting->light.distant.elevation = l->elevation;
    } else if (auto l = cast<SPFePointLight>(firstChild())) {
        specularlighting->light_type = Inkscape::Filters::POINT_LIGHT;
        specularlighting->light.point.x = l->x;
        specularlighting->light.point.y = l->y;
        specularlighting->light.point.z = l->z;
    } else if (auto l = cast<SPFeSpotLight>(firstChild())) {
        specularlighting->light_type = Inkscape::Filters::SPOT_LIGHT;
        specularlighting->light.spot.x = l->x;
        specularlighting->light.spot.y = l->y;
        specularlighting->light.spot.z = l->z;
        specularlighting->light.spot.pointsAtX = l->pointsAtX;
        specularlighting->light.spot.pointsAtY = l->pointsAtY;
        specularlighting->light.spot.pointsAtZ = l->pointsAtZ;
        specularlighting->light.spot.limitingConeAngle = l->limitingConeAngle;
        specularlighting->light.spot.specularExponent = l->specularExponent;
    }

    return specularlighting;
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

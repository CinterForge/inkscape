// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * SVG <fespotlight> implementation.
 */
/*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Niko Kiirala <niko@kiirala.com>
 *   Jean-Rene Reinhard <jr@komite.net>
 *   Abhishek Sharma
 *
 * Copyright (C) 2006,2007 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "spotlight.h"

#include "attributes.h"  // for SPAttr
#include "document.h"    // for SPDocument
#include "xml/node.h"    // for Node

namespace Inkscape::XML {
struct Document;
} // namespace Inkscape::XML

SPFeSpotLight::SPFeSpotLight()
    : x(0)
    , x_set(false)
    , y(0)
    , y_set(false)
    , z(0)
    , z_set(false)
    , pointsAtX(0)
    , pointsAtX_set(false)
    , pointsAtY(0)
    , pointsAtY_set(false)
    , pointsAtZ(0)
    , pointsAtZ_set(false)
    , specularExponent(1)
    , specularExponent_set(false)
    , limitingConeAngle(90)
    , limitingConeAngle_set(false)
{
}

SPFeSpotLight::~SPFeSpotLight() = default;

void SPFeSpotLight::build(SPDocument *document, Inkscape::XML::Node *repr)
{
	SPObject::build(document, repr);

    readAttr(SPAttr::X);
    readAttr(SPAttr::Y);
    readAttr(SPAttr::Z);
    readAttr(SPAttr::POINTSATX);
    readAttr(SPAttr::POINTSATY);
    readAttr(SPAttr::POINTSATZ);
    readAttr(SPAttr::SPECULAREXPONENT);
    readAttr(SPAttr::LIMITINGCONEANGLE);

    document->addResource("fespotlight", this);
}

void SPFeSpotLight::release() {
    if (document) {
        document->removeResource("fespotlight", this);
    }

    SPObject::release();
}

void SPFeSpotLight::set(SPAttr key, char const *value)
{
    auto read_float = [=] (float &var, float def = 0) -> bool {
        if (value) {
            char *end_ptr;
            auto tmp = g_ascii_strtod(value, &end_ptr);
            if (end_ptr) {
                var = tmp;
                return true;
            }
        }
        var = def;
        return false;
    };

    switch (key) {
        case SPAttr::X:
            x_set = read_float(x);
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::Y:
            y_set = read_float(y);
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::Z:
            z_set = read_float(z);
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::POINTSATX:
            pointsAtX_set = read_float(pointsAtX);
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::POINTSATY:
            pointsAtY_set = read_float(pointsAtY);
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::POINTSATZ:
            pointsAtZ_set = read_float(pointsAtZ);
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::SPECULAREXPONENT:
            specularExponent_set = read_float(specularExponent, 1);
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::LIMITINGCONEANGLE:
            limitingConeAngle_set = read_float(limitingConeAngle, 90);
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        default:
            SPObject::set(key, value);
            break;
    }
}

Inkscape::XML::Node *SPFeSpotLight::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, unsigned flags) {
    if (!repr) {
        repr = getRepr()->duplicate(doc);
    }

    if (x_set)
        repr->setAttributeCssDouble("x", x);
    if (y_set)
        repr->setAttributeCssDouble("y", y);
    if (z_set)
        repr->setAttributeCssDouble("z", z);
    if (pointsAtX_set)
        repr->setAttributeCssDouble("pointsAtX", pointsAtX);
    if (pointsAtY_set)
        repr->setAttributeCssDouble("pointsAtY", pointsAtY);
    if (pointsAtZ_set)
        repr->setAttributeCssDouble("pointsAtZ", pointsAtZ);
    if (specularExponent_set)
        repr->setAttributeCssDouble("specularExponent", specularExponent);
    if (limitingConeAngle_set)
        repr->setAttributeCssDouble("limitingConeAngle", limitingConeAngle);

    SPObject::write(doc, repr, flags);

    return repr;
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

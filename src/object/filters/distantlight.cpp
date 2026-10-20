// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * SVG <fedistantlight> implementation.
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

#include "distantlight.h"

#include "attributes.h"  // for SPAttr
#include "document.h"    // for SPDocument
#include "xml/node.h"    // for Node

namespace Inkscape::XML {
struct Document;
} // namespace Inkscape::XML


SPFeDistantLight::SPFeDistantLight()
    : azimuth(0)
    , azimuth_set(false)
    , elevation(0)
    , elevation_set(false)
{
}

SPFeDistantLight::~SPFeDistantLight() = default;

void SPFeDistantLight::build(SPDocument *document, Inkscape::XML::Node *repr)
{
	SPObject::build(document, repr);

    readAttr(SPAttr::AZIMUTH);
    readAttr(SPAttr::ELEVATION);

    document->addResource("fedistantlight", this);
}

void SPFeDistantLight::release()
{
    if (document) {
        document->removeResource("fedistantlight", this);
    }

    SPObject::release();
}

void SPFeDistantLight::set(SPAttr key, char const *value)
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
        case SPAttr::AZIMUTH:
            azimuth_set = read_float(azimuth);
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::ELEVATION:
            elevation_set = read_float(elevation);
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        default:
            SPObject::set(key, value);
            break;
    }
}

Inkscape::XML::Node *SPFeDistantLight::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, unsigned flags)
{
    if (!repr) {
        repr = getRepr()->duplicate(doc);
    }

    if (azimuth_set) {
        repr->setAttributeCssDouble("azimuth", azimuth);
    }

    if (elevation_set) {
        repr->setAttributeCssDouble("elevation", elevation);
    }

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

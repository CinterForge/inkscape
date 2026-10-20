// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-linear-gradient.h"

#include "attributes.h"                   // for SPAttr
#include <2geom/rect.h>                   // for Rect
#include "style-internal.h"               // for SPIFontSize
#include "style.h"                        // for SPStyle

#include "display/drawing-paintserver.h"  // for DrawingLinearGradient, Draw...
#include "object/sp-gradient.h"           // for SPGradient
#include "object/sp-item.h"               // for SPItemCtx
#include "object/sp-object.h"             // for SP_OBJECT_MODIFIED_FLAG
#include "object/sp-paint-server-data.h"  // for SPGradientVector, SPGradientUnits
#include "xml/document.h"                 // for Document
#include "xml/node.h"                     // for Node

class SPDocument;

/*
 * Linear Gradient
 */
SPLinearGradient::SPLinearGradient() : SPGradient() {
    this->x1.unset(SVGLength::PERCENT, 0.0, 0.0);
    this->y1.unset(SVGLength::PERCENT, 0.0, 0.0);
    this->x2.unset(SVGLength::PERCENT, 1.0, 1.0);
    this->y2.unset(SVGLength::PERCENT, 0.0, 0.0);
}

SPLinearGradient::~SPLinearGradient() = default;

void SPLinearGradient::build(SPDocument *document, Inkscape::XML::Node *repr) {
    this->readAttr(SPAttr::X1);
    this->readAttr(SPAttr::Y1);
    this->readAttr(SPAttr::X2);
    this->readAttr(SPAttr::Y2);

    SPGradient::build(document, repr);
}

/**
 * Callback: set attribute.
 */
void SPLinearGradient::set(SPAttr key, const gchar* value) {
    switch (key) {
        case SPAttr::X1:
            this->x1.readOrUnset(value, SVGLength::PERCENT, 0.0, 0.0);
            this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;

        case SPAttr::Y1:
            this->y1.readOrUnset(value, SVGLength::PERCENT, 0.0, 0.0);
            this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;

        case SPAttr::X2:
            this->x2.readOrUnset(value, SVGLength::PERCENT, 1.0, 1.0);
            this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;

        case SPAttr::Y2:
            this->y2.readOrUnset(value, SVGLength::PERCENT, 0.0, 0.0);
            this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;

        default:
            SPGradient::set(key, value);
            break;
    }
}

void
SPLinearGradient::update(SPCtx *ctx, guint flags)
{
    // To do: Verify flags.
    if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {

        // Walk up the reference tree to find values.
        if (!x1._set) x1.unset(find_x1());
        if (!y1._set) y1.unset(find_y1());
        if (!x2._set) x2.unset(find_x2());
        if (!y2._set) y2.unset(find_y2());

        SPItemCtx const *ictx = reinterpret_cast<SPItemCtx const *>(ctx);

        if (getUnits() == SP_GRADIENT_UNITS_USERSPACEONUSE) {
            double w = ictx->viewport.width();
            double h = ictx->viewport.height();
            double const em = style->font_size.computed;
            double const ex = 0.5 * em;  // fixme: get x height from pango or libnrtype.

            this->x1.update(em, ex, w);
            this->y1.update(em, ex, h);
            this->x2.update(em, ex, w);
            this->y2.update(em, ex, h);
        }
    }
}

/**
 * Callback: write attributes to associated repr.
 */
Inkscape::XML::Node* SPLinearGradient::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:linearGradient");
    }

    if ((flags & SP_OBJECT_WRITE_ALL) || this->x1._set) {
        repr->setAttributeSvgDouble("x1", this->x1.computed);
    }

    if ((flags & SP_OBJECT_WRITE_ALL) || this->y1._set) {
        repr->setAttributeSvgDouble("y1", this->y1.computed);
    }

    if ((flags & SP_OBJECT_WRITE_ALL) || this->x2._set) {
        repr->setAttributeSvgDouble("x2", this->x2.computed);
    }

    if ((flags & SP_OBJECT_WRITE_ALL) || this->y2._set) {
        repr->setAttributeSvgDouble("y2", this->y2.computed);
    }

    SPGradient::write(xml_doc, repr, flags);

    return repr;
}

std::vector<double> SPLinearGradient::getGradientGeom() const
{
    return {x1.computed, y1.computed, x2.computed, y2.computed};
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SVG dimensions implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Edward Flick (EAF)
 *   Abhishek Sharma
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 1999-2005 Authors
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-dimensions.h"
#include "sp-item.h"
#include "svg/svg.h"

/**
 * Update computed x/y/width/height for "percent" units and/or from its
 * referencing clone parent.
 *
 * @param assign_to_set Set `_set` to true for x/y/width/height.
 * @param use If not NULL, then overwrite computed width and height from there.
 */
void SPDimensions::calcDimsFromParentViewport(const SPItemCtx *ictx, bool assign_to_set, //
                                              SPDimensions const *use)
{
#define ASSIGN(field) { if (assign_to_set) { field._set = true; } }

    auto const *effectivewidth = &this->width;
    auto const *effectiveheight = &this->height;

    if (use) {
        assert(!assign_to_set);

        if (use->width._set) {
            effectivewidth = &use->width;
        }

        if (use->height._set) {
            effectiveheight = &use->height;
        }
    }

    if (this->x.unit == SVGLength::PERCENT) {
        ASSIGN(x);
        this->x.computed = this->x.value * ictx->viewport.width();
    }

    if (this->y.unit == SVGLength::PERCENT) {
        ASSIGN(y);
        this->y.computed = this->y.value * ictx->viewport.height();
    }

    if (effectivewidth->unit == SVGLength::PERCENT) {
        ASSIGN(width);
        this->width.computed = effectivewidth->value * ictx->viewport.width();
    } else {
        this->width.computed = effectivewidth->computed;
    }

    if (effectiveheight->unit == SVGLength::PERCENT) {
        ASSIGN(height);
        this->height.computed = effectiveheight->value * ictx->viewport.height();
    } else {
        this->height.computed = effectiveheight->computed;
    }
}

/**
 * Write the geometric properties (x/y/width/height) to XML attributes, if they are set.
 */
void SPDimensions::writeDimensions(Inkscape::XML::Node *repr) const
{
    if (x._set) {
        repr->setAttribute("x", sp_svg_length_write_with_units(x));
    }
    if (y._set) {
        repr->setAttribute("y", sp_svg_length_write_with_units(y));
    }
    if (width._set) {
        repr->setAttribute("width", sp_svg_length_write_with_units(width));
    }
    if (height._set) {
        repr->setAttribute("height", sp_svg_length_write_with_units(height));
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

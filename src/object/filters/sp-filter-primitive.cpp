// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * Superclass for all the filter primitives
 *
 */
/*
 * Authors:
 *   Kees Cook <kees@outflux.net>
 *   Niko Kiirala <niko@kiirala.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2004-2007 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstring>

#include "sp-filter-primitive.h"
#include "attributes.h"
#include "display/nr-filter-primitive.h"
#include "style.h"
#include "slot-resolver.h"
#include "util/optstr.h"

SPFilterPrimitive::SPFilterPrimitive()
{
    // We must keep track if a value is set or not, if not set then the region defaults to 0%, 0%,
    // 100%, 100% ("x", "y", "width", "height") of the -> filter <- region. If set then
    // percentages are in terms of bounding box or viewbox, depending on value of "primitiveUnits"

    // NB: SVGLength.set takes prescaled percent values: 1 means 100%
    x.unset(SVGLength::PERCENT, 0, 0);
    y.unset(SVGLength::PERCENT, 0, 0);
    width.unset(SVGLength::PERCENT, 1, 0);
    height.unset(SVGLength::PERCENT, 1, 0);
}

SPFilterPrimitive::~SPFilterPrimitive() = default;

void SPFilterPrimitive::build(SPDocument *document, Inkscape::XML::Node *repr)
{
    readAttr(SPAttr::STYLE); // struct not derived from SPItem, we need to do this ourselves.
    readAttr(SPAttr::IN_);
    readAttr(SPAttr::RESULT);
    readAttr(SPAttr::X);
    readAttr(SPAttr::Y);
    readAttr(SPAttr::WIDTH);
    readAttr(SPAttr::HEIGHT);

    SPObject::build(document, repr);
}

void SPFilterPrimitive::release()
{
    SPObject::release();
}

void SPFilterPrimitive::set(SPAttr key, char const *value)
{
    switch (key) {
        case SPAttr::IN_:
            if (Inkscape::Util::assign(in_name, value)) {
                requestModified(SP_OBJECT_MODIFIED_FLAG);
                invalidate_parent_slots();
            }
            break;
        case SPAttr::RESULT:
            if (Inkscape::Util::assign(out_name, value)) {
                requestModified(SP_OBJECT_MODIFIED_FLAG);
                invalidate_parent_slots();
            }
            break;
        case SPAttr::X:
            x.readOrUnset(value);
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::Y:
            y.readOrUnset(value);
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::WIDTH:
            width.readOrUnset(value);
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::HEIGHT:
            height.readOrUnset(value);
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        default:
            SPObject::set(key, value);
            break;
    }
}

void SPFilterPrimitive::update(SPCtx *ctx, unsigned flags)
{
    auto ictx = static_cast<SPItemCtx const*>(ctx);

    // Do here since we know viewport (Bounding box case handled during rendering)
    if (auto parent_filter = cast<SPFilter>(parent);
        parent_filter &&
        parent_filter->primitiveUnits == SP_FILTER_UNITS_USERSPACEONUSE)
    {
        calcDimsFromParentViewport(ictx, true);
    }
}

Inkscape::XML::Node *SPFilterPrimitive::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, unsigned flags)
{
    if (!repr) {
        repr = getRepr()->duplicate(doc);
    }

    repr->setAttributeOrRemoveIfEmpty("in", Inkscape::Util::to_cstr(in_name));
    repr->setAttributeOrRemoveIfEmpty("result", Inkscape::Util::to_cstr(out_name));

    // Do we need to add x, y, width, height?
    SPObject::write(doc, repr, flags);

    return repr;
}

void SPFilterPrimitive::invalidate_parent_slots()
{
    if (auto filter = cast<SPFilter>(parent)) {
        filter->invalidate_slots();
    }
}

void SPFilterPrimitive::resolve_slots(SlotResolver &resolver)
{
    in_slot = resolver.read(in_name);
    out_slot = resolver.write(out_name);
}

// Common initialization for filter primitives
void SPFilterPrimitive::build_renderer_common(Inkscape::Filters::FilterPrimitive *primitive) const
{
    g_assert(primitive);
    
    primitive->set_input(in_slot);
    primitive->set_output(out_slot);

    /* TODO: place here code to handle input images, filter area etc. */
    // We don't know current viewport or bounding box, this is wrong approach.
    primitive->set_subregion(x, y, width, height);

    // Give renderer access to filter properties
    primitive->setStyle(style);
}

/* Calculate the region taken up by this filter, given the previous region.
 *
 * @param current_region The original shape's region or previous primitive's calculate_region output.
 */
Geom::Rect SPFilterPrimitive::calculate_region(Geom::Rect const &region) const
{
    return region; // No change.
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

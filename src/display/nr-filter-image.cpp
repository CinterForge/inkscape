// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * feImage filter primitive renderer
 *
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *   Tavmjong Bah <tavmjong@free.fr>
 *   Abhishek Sharma
 *
 * Copyright (C) 2007-2011 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "nr-filter-image.h"

#include "cairo-utils.h"
#include "enums.h"

// Drawing
#include "drawing-context.h"
#include "drawing-item.h"
#include "nr-filter-slot.h"
#include "nr-filter-units.h"
#include "nr-filter.h"

namespace Inkscape {
namespace Filters {

void FilterImage::update()
{
    if (!item) {
        return;
    }

    item->update();
}

void FilterImage::render_cairo(FilterSlot &slot) const
{
    if (!item) {
        return;
    }

    Geom::OptRect area = item->drawbox();
    if (!area) {
        return;
    }

    // Viewport is filter primitive area (in user coordinates).
    // Note: viewport calculation in non-trivial. Do not rely
    // on get_matrix_primitiveunits2pb().
    Geom::Rect vp = filter_primitive_area(slot.get_units());
    slot.set_primitive_area(_output, vp); // Needed for tiling

    double feImageX      = vp.left();
    double feImageY      = vp.top();
    double feImageWidth  = vp.width();
    double feImageHeight = vp.height();

    // feImage is suppose to use the same parameters as a normal SVG image.
    // If a width or height is set to zero, the image is not suppose to be displayed.
    // This does not seem to be what Firefox or Opera does, nor does the W3C displacement
    // filter test expect this behavior. If the width and/or height are zero, we use
    // the width and height of the object bounding box.
    Geom::Affine m = slot.get_units().get_matrix_user2filterunits().inverse();
    Geom::Point bbox_00 = Geom::Point(0,0) * m;
    Geom::Point bbox_w0 = Geom::Point(1,0) * m;
    Geom::Point bbox_0h = Geom::Point(0,1) * m;
    double bbox_width = Geom::distance(bbox_00, bbox_w0);
    double bbox_height = Geom::distance(bbox_00, bbox_0h);

    if (feImageWidth  == 0) feImageWidth  = bbox_width;
    if (feImageHeight == 0) feImageHeight = bbox_height;

    int device_scale = slot.get_device_scale();

    Geom::Rect sa = slot.get_slot_area();
    cairo_surface_t *out = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, sa.width() * device_scale, sa.height() * device_scale);
    cairo_surface_set_device_scale(out, device_scale, device_scale);

    Inkscape::DrawingContext dc(out, sa.min());
    Geom::Affine user2pb = slot.get_units().get_matrix_user2pb();
    dc.transform(user2pb); // we are now in primitive units

    Geom::IntRect render_rect = area->roundOutwards();

    // Internal image, like <use>
    if (from_element) {
        dc.translate(feImageX, feImageY);
        item->render(dc, slot.get_rendercontext(), render_rect);

        // For the moment, we'll assume that any image is in sRGB color space
        set_cairo_surface_ci(out, SP_CSS_COLOR_INTERPOLATION_SRGB);
    } else {
        // For the moment, we'll assume that any image is in sRGB color space
        // set_cairo_surface_ci(out, SP_CSS_COLOR_INTERPOLATION_SRGB);
        // This seemed like a sensible thing to do but it breaks filters-displace-01-f.svg

        // Now that we have the viewport, we must map image inside.
        // Partially copied from sp-image.cpp.

        auto image_width  = area->width();
        auto image_height = area->height();

        // Do nothing if preserveAspectRatio is "none".
        if (aspect_align != SP_ASPECT_NONE) {

            // Check aspect ratio of image vs. viewport
            double feAspect = feImageHeight / feImageWidth;
            double aspect = (double)image_height / image_width;
            bool ratio = feAspect < aspect;

            double ax, ay; // Align side
            switch (aspect_align) {
                case SP_ASPECT_XMIN_YMIN:
                    ax = 0.0;
                    ay = 0.0;
                    break;
                case SP_ASPECT_XMID_YMIN:
                    ax = 0.5;
                    ay = 0.0;
                    break;
                case SP_ASPECT_XMAX_YMIN:
                    ax = 1.0;
                    ay = 0.0;
                    break;
                case SP_ASPECT_XMIN_YMID:
                    ax = 0.0;
                    ay = 0.5;
                    break;
                case SP_ASPECT_XMID_YMID:
                    ax = 0.5;
                    ay = 0.5;
                    break;
                case SP_ASPECT_XMAX_YMID:
                    ax = 1.0;
                    ay = 0.5;
                    break;
                case SP_ASPECT_XMIN_YMAX:
                    ax = 0.0;
                    ay = 1.0;
                    break;
                case SP_ASPECT_XMID_YMAX:
                    ax = 0.5;
                    ay = 1.0;
                    break;
                case SP_ASPECT_XMAX_YMAX:
                    ax = 1.0;
                    ay = 1.0;
                    break;
                default:
                    ax = 0.0;
                    ay = 0.0;
                    break;
            }

            if (aspect_clip == SP_ASPECT_SLICE) {
                // image clipped by viewbox

                if (ratio) {
                    // clip top/bottom
                    feImageY -= ay * (feImageWidth * aspect - feImageHeight);
                    feImageHeight = feImageWidth * aspect;
                } else {
                    // clip sides
                    feImageX -= ax * (feImageHeight / aspect - feImageWidth);
                    feImageWidth = feImageHeight / aspect;
                }

            } else {
                // image fits into viewbox

                if (ratio) {
                    // fit to height
                    feImageX += ax * (feImageWidth - feImageHeight / aspect );
                    feImageWidth = feImageHeight / aspect;
                } else {
                    // fit to width
                    feImageY += ay * (feImageHeight - feImageWidth * aspect);
                    feImageHeight = feImageWidth * aspect;
                }
            }
        }

        double scaleX = feImageWidth / image_width;
        double scaleY = feImageHeight / image_height;

        dc.translate(feImageX, feImageY);
        dc.scale(scaleX, scaleY);
        item->render(dc, slot.get_rendercontext(), render_rect);
    }

    slot.set(_output, out);
    cairo_surface_destroy(out);
}

bool FilterImage::can_handle_affine(Geom::Affine const &) const
{
    return true;
}

double FilterImage::complexity(Geom::Affine const &) const
{
    // TODO: right now we cannot actually measure this in any meaningful way.
    return 1.1;
}

void FilterImage::set_align(unsigned align)
{
    aspect_align = align;
}

void FilterImage::set_clip(unsigned clip)
{
    aspect_clip = clip;
}

} // namespace Filters
} // namespace Inkscape

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

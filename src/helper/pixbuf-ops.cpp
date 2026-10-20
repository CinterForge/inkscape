// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Helpers for SPItem -> gdk_pixbuf related stuff
 *
 * Authors:
 *   John Cliff <simarilius@yahoo.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2008 John Cliff
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <2geom/transforms.h>

#include "document.h"

#include "display/cairo-utils.h"
#include "display/drawing-context.h"
#include "display/drawing.h"
#include "helper/pixbuf-ops.h"
#include "object/sp-root.h"
#include "util/scope_exit.h"
#include "util/units.h"

/**
    Generates a bitmap from given items. The bitmap is stored in RAM and not written to file.
    @param document Inkscape document.
    @param area     Export area in document units.
    @param dpi      Resolution.
    @param items    Vector of pointers to SPItems to export. Export all items if empty.
    @param opaque   Set items opacity to 1 (used by Cairo renderer for filtered objects rendered as bitmaps).
    @return The created GdkPixbuf structure or nullptr if rendering failed.
*/
Inkscape::Pixbuf *sp_generate_internal_bitmap(SPDocument *document,
                                              Geom::Rect const &area,
                                              double dpi,
                                              std::vector<SPItem const *> items,
                                              bool opaque,
                                              uint32_t const *checkerboard_color,
                                              double device_scale,
                                              std::optional<Inkscape::Antialiasing> antialias)
{
    // Geometry
    if (area.hasZeroArea()) {
        return nullptr;
    }

    Geom::Point origin = area.min();
    double scale_factor = Inkscape::Util::Quantity::convert(dpi, "px", "in");
    Geom::Affine affine = Geom::Translate(-origin) * Geom::Scale (scale_factor, scale_factor);

    int width  = std::ceil(scale_factor * area.width());
    int height = std::ceil(scale_factor * area.height());

    // Document
    document->ensureUpToDate();
    unsigned dkey = SPItem::display_key_new(1);

    // Drawing
    Inkscape::Drawing drawing; // New drawing for offscreen rendering.
    drawing.setRoot(document->getRoot()->invoke_show(drawing, dkey, SP_ITEM_SHOW_DISPLAY));
    auto invoke_hide_guard = scope_exit([&] { document->getRoot()->invoke_hide(dkey); });
    drawing.root()->setTransform(affine);
    drawing.setExact(); // Maximum quality for blurs.
    drawing.setAntialiasingOverride(antialias);

    // Hide all items we don't want, instead of showing only requested items,
    // because that would not work if the shown item references something in defs.
    if (!items.empty()) {
        document->getRoot()->invoke_hide_except(dkey, items);
    }

    auto final_area = Geom::IntRect::from_xywh(0, 0, width, height);
    drawing.update(final_area);

    if (opaque) {
        // Required by sp_asbitmap_render().
        for (auto item : items) {
            if (item->get_arenaitem(dkey)) {
                item->get_arenaitem(dkey)->setOpacity(1.0);
            }
        }
    }

    // Rendering
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);

    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        long long size = (long long)height * (long long)cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
        g_warning("sp_generate_internal_bitmap: not enough memory to create pixel buffer. Need %lld.", size);
        cairo_surface_destroy(surface);
        return nullptr;
    }

    Inkscape::DrawingContext dc(surface, Geom::Point(0, 0));

    if (checkerboard_color) {
        auto pattern = ink_cairo_pattern_create_checkerboard(*checkerboard_color);
        dc.save();
        dc.transform(Geom::Scale(device_scale));
        dc.setOperator(CAIRO_OPERATOR_SOURCE);
        dc.setSource(pattern->cobj());
        dc.paint();
        dc.restore();
    }

    // render items
    drawing.render(dc, final_area, Inkscape::DrawingItem::RENDER_BYPASS_CACHE);

    if (device_scale != 1.0) {
        cairo_surface_set_device_scale(surface, device_scale, device_scale);
    }

    return new Inkscape::Pixbuf(surface);
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

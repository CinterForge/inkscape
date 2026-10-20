// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * SVGPreview: Preview cache
 */
/*
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Bryce Harrington <brycehar@bryceharrington.org>
 *   bulia byak <buliabyak@users.sf.net>
 *
 * Copyright (C) 2001-2005 authors
 * Copyright (C) 2001 Ximian, Inc.
 * Copyright (C) 2004 John Cliff
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 *
 */

#include "display/cairo-utils.h"
#include "display/drawing-context.h"
#include "display/drawing.h"
#include "ui/cache/svg_preview_cache.h"

cairo_surface_t* render_surface(Inkscape::Drawing &drawing, double scale_factor, Geom::Rect const &dbox,
    Geom::IntPoint pixsize, double device_scale, const guint32* checkerboard_color, bool no_clip)
{
    scale_factor *= device_scale;
    pixsize.x() *= device_scale;
    pixsize.y() *= device_scale;

    Geom::IntRect ibox = (dbox * Geom::Scale(scale_factor)).roundOutwards();

    if (no_clip) {
        // check if object fits in the surface
        if (ibox.width() > pixsize.x() || ibox.height() > pixsize.y()) {
            auto sx = static_cast<double>(ibox.width()) / pixsize.x();
            auto sy = static_cast<double>(ibox.height()) / pixsize.y();
            // reduce scale
            scale_factor /= std::max(sx, sy);
            ibox = (dbox * Geom::Scale(scale_factor)).roundOutwards();
        }
    }

    drawing.root()->setTransform(Geom::Scale(scale_factor));

    drawing.update(ibox);

    /* Find visible area */
    int width = ibox.width();
    int height = ibox.height();
    int dx = pixsize.x();
    int dy = pixsize.y();
    dx = (dx - width)/2; // watch out for size, since 'unsigned'-'signed' can cause problems if the result is negative
    dy = (dy - height)/2;

    Geom::IntRect area = Geom::IntRect::from_xywh(ibox.min() - Geom::IntPoint(dx, dy), pixsize);

    /* Render */
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, pixsize.x(), pixsize.y());
    Inkscape::DrawingContext dc(s, area.min());

    if (checkerboard_color) {
        guint rgba = *checkerboard_color;
        auto pattern = ink_cairo_pattern_create_checkerboard(rgba);
        dc.save();
        dc.transform(Geom::Scale(device_scale));
        dc.setOperator(CAIRO_OPERATOR_SOURCE);
        dc.setSource(pattern->cobj());
        dc.paint();
        dc.restore();
    }

    drawing.render(dc, area, Inkscape::DrawingItem::RENDER_BYPASS_CACHE);
    cairo_surface_flush(s);

    return s;
}

GdkPixbuf* render_pixbuf(Inkscape::Drawing &drawing, double scale_factor, Geom::Rect const &dbox, unsigned psize) {
    auto s = render_surface(drawing, scale_factor, dbox, Geom::IntPoint(psize, psize), 1, nullptr, true);
    GdkPixbuf* pixbuf = ink_pixbuf_create_from_cairo_surface(s);
    return pixbuf;
}

namespace Inkscape {
namespace UI {
namespace Cache {

SvgPreview::SvgPreview()
= default;

SvgPreview::~SvgPreview()
{
    for (auto & i : _pixmap_cache)
    {
        g_object_unref(i.second);
        i.second = NULL;
    }
}

Glib::ustring SvgPreview::cache_key(gchar const *uri, gchar const *name, unsigned psize) const {
    Glib::ustring key;
    key += (uri!=nullptr)  ? uri  : "";
    key += ":";
    key += (name!=nullptr) ? name : "unknown";
    key += ":";
    key += psize;
    return key;
}

GdkPixbuf* SvgPreview::get_preview_from_cache(const Glib::ustring& key) {
    std::map<Glib::ustring, GdkPixbuf *>::iterator found = _pixmap_cache.find(key);
    if ( found != _pixmap_cache.end() ) {
        return found->second;
    }
    return nullptr;
}

void SvgPreview::set_preview_in_cache(const Glib::ustring& key, GdkPixbuf* px) {
    g_object_ref(px);
    _pixmap_cache[key] = px;
}

GdkPixbuf* SvgPreview::get_preview(const gchar* uri, const gchar* id, Inkscape::DrawingItem */*root*/,
                                   double /*scale_factor*/, unsigned int psize) {
    // First try looking up the cached preview in the cache map
    Glib::ustring key = cache_key(uri, id, psize);
    GdkPixbuf* px = get_preview_from_cache(key);

    if (px == nullptr) {
        /*
            px = render_pixbuf(root, scale_factor, dbox, psize);
            set_preview_in_cache(key, px);
        */
    }
    return px;
}

void SvgPreview::remove_preview_from_cache(const Glib::ustring& key) {
    std::map<Glib::ustring, GdkPixbuf *>::iterator found = _pixmap_cache.find(key);
    if ( found != _pixmap_cache.end() ) {
        g_object_unref(found->second);
        found->second = NULL;
        _pixmap_cache.erase(key);
    }
}


}
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

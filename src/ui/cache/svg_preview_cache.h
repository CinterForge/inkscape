// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief Preview cache
 */
/*
 * Copyright (C) 2007 Bryce W. Harrington <bryce@bryceharrington.org>
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_UI_SVG_PREVIEW_CACHE_H
#define SEEN_INKSCAPE_UI_SVG_PREVIEW_CACHE_H

#include <map>
#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glibmm/ustring.h>
#include <2geom/rect.h>
#include <2geom/int-point.h>

namespace Inkscape {

class Drawing;
class DrawingItem;

} // namespace Inkscape


cairo_surface_t* render_surface(Inkscape::Drawing &drawing, double scale_factor, const Geom::Rect& dbox,
    Geom::IntPoint pixsize, double device_scale, const guint32* checkerboard_color, bool no_clip);

GdkPixbuf* render_pixbuf(Inkscape::Drawing &drawing, double scale_factor, const Geom::Rect& dbox, unsigned psize);

namespace Inkscape {
namespace UI {
namespace Cache {

class SvgPreview {
 protected:
    std::map<Glib::ustring, GdkPixbuf*>   _pixmap_cache;

 public:
    SvgPreview();
    ~SvgPreview();

    Glib::ustring cache_key(gchar const *uri, gchar const *name, unsigned psize) const;
    GdkPixbuf*    get_preview_from_cache(const Glib::ustring& key);
    void          set_preview_in_cache(const Glib::ustring& key, GdkPixbuf* px);
    GdkPixbuf*    get_preview(const gchar* uri, const gchar* id, Inkscape::DrawingItem *root, double scale_factor, unsigned int psize);
    void          remove_preview_from_cache(const Glib::ustring& key);
};

}; // namespace Cache
}; // namespace UI
}; // namespace Inkscape



#endif // SEEN_INKSCAPE_UI_SVG_PREVIEW_CACHE_H
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



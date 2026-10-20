// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Cairo surface that remembers its origin.
 *//*
 * Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *
 * Copyright (C) 2011 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_DISPLAY_DRAWING_SURFACE_H
#define INKSCAPE_DISPLAY_DRAWING_SURFACE_H

#include <cairo.h>
#include <2geom/affine.h>
#include <2geom/rect.h>
#include <2geom/transforms.h>

extern "C" {
typedef struct _cairo cairo_t;
typedef struct _cairo_surface cairo_surface_t;
typedef struct _cairo_region cairo_region_t;
}

namespace Inkscape {
class DrawingContext;

class DrawingSurface
{
public:
    explicit DrawingSurface(Geom::IntRect const &area, int device_scale = 1);
    DrawingSurface(Geom::Rect const &logbox, Geom::IntPoint const &pixdims, int device_scale = 1);
    DrawingSurface(cairo_surface_t *surface, Geom::Point const &origin);
    virtual ~DrawingSurface();

    Geom::Rect area() const { return Geom::Rect::from_xywh(_origin, dimensions()); } ///< Get the logical extents of the surface.
    Geom::IntPoint pixels() const { return _pixels; } ///< Get the pixel dimensions of the surface
    Geom::Point dimensions() const { return _pixels / _scale.vector(); } ///< Get the logical width and weight of the surface as a point.
    Geom::Point origin() const { return _origin; }
    Geom::Scale scale() const { return _scale; }
    int device_scale() const { return _device_scale; }
    Geom::Affine drawingTransform() const { return Geom::Translate(-_origin) * _scale; } ///< Get the transformation applied to the drawing context on construction.
    void dropContents();

    cairo_surface_t *raw() { return _surface; }
    cairo_t *createRawContext();

protected:
    Geom::IntRect pixelArea() const;

    cairo_surface_t *_surface;
    Geom::Point _origin;
    Geom::Scale _scale;
    Geom::IntPoint _pixels;
    int _device_scale;
    bool _has_context;

    friend class DrawingContext;
};

class DrawingCache
    : public DrawingSurface
{
public:
    explicit DrawingCache(Geom::IntRect const &area, int device_scale = 1);
    ~DrawingCache() override;

    void markDirty(Geom::IntRect const &area = Geom::IntRect::infinite());
    void markClean(Geom::IntRect const &area = Geom::IntRect::infinite());
    void scheduleTransform(Geom::IntRect const &new_area, Geom::Affine const &trans);
    void prepare();
    void paintFromCache(DrawingContext &dc, Geom::OptIntRect &area, bool is_filter);

protected:
    cairo_region_t *_clean_region;
    Geom::IntRect _pending_area;
    Geom::Affine _pending_transform;

private:
    void _dumpCache(Geom::OptIntRect const &area);
};

} // namespace Inkscape

#endif // INKSCAPE_DISPLAY_DRAWING_SURFACE_H

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

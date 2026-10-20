// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Bitmap image belonging to an SVG drawing.
 *//*
 * Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *
 * Copyright (C) 2011 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <2geom/bezier-curve.h>

#include "drawing.h"
#include "drawing-context.h"
#include "drawing-image.h"
#include "cairo-utils.h"
#include "cairo-templates.h"

namespace Inkscape {

DrawingImage::DrawingImage(Drawing &drawing)
    : DrawingItem(drawing)
    , style_image_rendering(SP_CSS_IMAGE_RENDERING_AUTO)
    , _extend(CAIRO_EXTEND_NONE) // NONE prevents artifacts in surrounding empty space
{
}

void DrawingImage::setPixbuf(std::shared_ptr<Inkscape::Pixbuf const> pixbuf)
{
    defer([this, pixbuf = std::move(pixbuf)] () mutable {
        _pixbuf = std::move(pixbuf);
        _markForUpdate(STATE_ALL, false);
    });
}

void DrawingImage::setScale(double sx, double sy)
{
    defer([=, this] {
        _scale = Geom::Scale(sx, sy);
        _markForUpdate(STATE_ALL, false);
    });
}

void DrawingImage::setOrigin(Geom::Point const &origin)
{
    defer([=, this] {
        _origin = origin;
        _markForUpdate(STATE_ALL, false);
    });
}

void DrawingImage::setClipbox(Geom::Rect const &box)
{
    defer([=, this] {
        _clipbox = box;
        _markForUpdate(STATE_ALL, false);
    });
}

void DrawingImage::setExtend(cairo_extend_t extend)
{
    defer([=, this] {
        _extend = extend;
        _markForUpdate(STATE_ALL, false);
    });
}

Geom::Rect DrawingImage::bounds() const
{
    if (!_pixbuf) return _clipbox;

    double pw = _pixbuf->width();
    double ph = _pixbuf->height();
    double vw = pw * _scale[Geom::X];
    double vh = ph * _scale[Geom::Y];
    Geom::Point wh(vw, vh);
    Geom::Rect view(_origin, _origin+wh);
    Geom::OptRect res = _clipbox & view;
    Geom::Rect ret = res ? *res : _clipbox;

    return ret;
}

void DrawingImage::setStyle(SPStyle const *style, SPStyle const *context_style)
{
    DrawingItem::setStyle(style, context_style);

    auto image_rendering = SP_CSS_IMAGE_RENDERING_AUTO;
    if (_style) {
        image_rendering = _style->image_rendering.computed;
    }

    defer([=, this] {
        style_image_rendering = image_rendering;
    });
}

unsigned DrawingImage::_updateItem(Geom::IntRect const &, UpdateContext const &, unsigned, unsigned)
{
    // Calculate bbox
    if (_pixbuf) {
        Geom::Rect r = bounds() * _ctm;
        _bbox = r.roundOutwards();
    } else {
        _bbox = Geom::OptIntRect();
    }

    return STATE_ALL;
}

unsigned DrawingImage::_renderItem(DrawingContext &dc, RenderContext &rc, Geom::IntRect const &/*area*/, unsigned flags, DrawingItem const */*stop_at*/) const
{
    bool const outline = (flags & RENDER_OUTLINE) && !_drawing.imageOutlineMode();

    if (!outline) {
        if (!_pixbuf) return RENDER_OK;
        if (_scale.vector().x() * _scale.vector().y() == 0.0) return RENDER_OK;

        Inkscape::DrawingContext::Save save(dc);
        dc.transform(_ctm);
        dc.newPath();
        dc.rectangle(_clipbox);
        dc.clip();

        dc.translate(_origin);
        dc.scale(_scale);
        // const_cast required since Cairo needs to modify the internal refcount variable, but we do not want to give up the
        // benefits of const for the rest of our code. The underlying object is guaranteed to be non-const, so this is well-defined.
        // It is also thread-safe to modify the refcount in this way, since Cairo uses atomics internally.
        dc.setSource(const_cast<cairo_surface_t*>(_pixbuf->getSurfaceRaw()), 0, 0);
        dc.patternSetExtend(_extend);

        // See: http://www.w3.org/TR/SVG/painting.html#ImageRenderingProperty
        //      https://drafts.csswg.org/css-images-3/#the-image-rendering
        //      style.h/style.cpp, cairo-render-context.cpp
        //
        // CSS 3 defines:
        //   'optimizeSpeed' as alias for "pixelated"
        //   'optimizeQuality' as alias for "smooth"
        switch (style_image_rendering) {
            case SP_CSS_IMAGE_RENDERING_OPTIMIZESPEED:
            case SP_CSS_IMAGE_RENDERING_PIXELATED:
            // we don't have an implementation for crisp-edges, but it should *not* smooth or blur
            case SP_CSS_IMAGE_RENDERING_CRISPEDGES:
                dc.patternSetFilter( CAIRO_FILTER_NEAREST );
                break;
            case SP_CSS_IMAGE_RENDERING_AUTO:
            case SP_CSS_IMAGE_RENDERING_OPTIMIZEQUALITY:
            default:
                // In recent Cairo, BEST used Lanczos3, which is prohibitively slow
                dc.patternSetFilter( CAIRO_FILTER_GOOD );
                break;
        }

        // Handle an exceptional case where the greyscale color mode needs to be applied per-image.
        bool const greyscale_exception = (flags & RENDER_OUTLINE) && _drawing.colorMode() == ColorMode::GRAYSCALE;
        if (greyscale_exception) {
            dc.pushGroup();
        }

        dc.paint();

        if (greyscale_exception) {
            ink_cairo_surface_filter(dc.rawTarget(), dc.rawTarget(), _drawing.grayscaleMatrix());
            dc.popGroupToSource();
            dc.paint();
        }

    } else { // outline; draw a rect instead

        auto rgba = Colors::Color(_drawing.imageOutlineColor());

        {   Inkscape::DrawingContext::Save save(dc);
            dc.transform(_ctm);
            dc.newPath();

            Geom::Rect r = bounds();
            Geom::Point c00 = r.corner(0);
            Geom::Point c01 = r.corner(3);
            Geom::Point c11 = r.corner(2);
            Geom::Point c10 = r.corner(1);

            dc.moveTo(c00);
            // the box
            dc.lineTo(c10);
            dc.lineTo(c11);
            dc.lineTo(c01);
            dc.lineTo(c00);
            // the diagonals
            dc.lineTo(c11);
            dc.moveTo(c10);
            dc.lineTo(c01);
        }

        dc.setLineWidth(0.5);
        dc.setSource(rgba);
        dc.stroke();
    }
    return RENDER_OK;
}

/** Calculates the closest distance from p to the segment a1-a2*/
static double distance_to_segment(Geom::Point const &p, Geom::Point const &a1, Geom::Point const &a2)
{
    Geom::LineSegment l(a1, a2);
    Geom::Point np = l.pointAt(l.nearestTime(p));
    return Geom::distance(np, p);
}

DrawingItem *DrawingImage::_pickItem(Geom::Point const &p, double delta, Geom::OptIntRect const &area_world, unsigned flags)
{
    if (!_pixbuf) return nullptr;

    bool outline = (flags & PICK_OUTLINE) && !_drawing.imageOutlineMode();

    if (outline) {
        Geom::Rect r = bounds();
        Geom::Point pick = p * _ctm.inverse();

        // find whether any side or diagonal is within delta
        // to do so, iterate over all pairs of corners
        for (unsigned i = 0; i < 3; ++i) { // for i=3, there is nothing to do
            for (unsigned j = i+1; j < 4; ++j) {
                if (distance_to_segment(pick, r.corner(i), r.corner(j)) < delta) {
                    return this;
                }
            }
        }
        return nullptr;

    } else {
        auto pixels = _pixbuf->pixels();
        int width = _pixbuf->width();
        int height = _pixbuf->height();
        size_t rowstride = _pixbuf->rowstride();

        Geom::Point tp = p * _ctm.inverse();
        Geom::Rect r = bounds();

        if (!r.contains(tp))
            return nullptr;

        double vw = width * _scale[Geom::X];
        double vh = height * _scale[Geom::Y];
        int ix = floor((tp[Geom::X] - _origin[Geom::X]) / vw * width);
        int iy = floor((tp[Geom::Y] - _origin[Geom::Y]) / vh * height);

        if ((ix < 0) || (iy < 0) || (ix >= width) || (iy >= height))
            return nullptr;

        auto pix_ptr = pixels + iy * rowstride + ix * 4;
        // pick if the image is less than 99% transparent
        guint32 alpha = 0;
        if (_pixbuf->pixelFormat() == Inkscape::Pixbuf::PF_CAIRO) {
            guint32 px = *reinterpret_cast<guint32 const *>(pix_ptr);
            alpha = (px & 0xff000000) >> 24;
        } else if (_pixbuf->pixelFormat() == Inkscape::Pixbuf::PF_GDK) {
            alpha = pix_ptr[3];
        } else {
            throw std::runtime_error("Unrecognized pixel format");
        }
        float alpha_f = (alpha / 255.0f) * _opacity;
        return alpha_f > 0.01 ? this : nullptr;
    }
}

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

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

#ifndef INKSCAPE_DISPLAY_DRAWING_IMAGE_H
#define INKSCAPE_DISPLAY_DRAWING_IMAGE_H

#include <memory>
#include <2geom/transforms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cairo.h>

#include "display/drawing-item.h"

namespace Inkscape {
class Pixbuf;

class DrawingImage
    : public DrawingItem
{
public:
    DrawingImage(Drawing &drawing);
    int tag() const override { return tag_of<decltype(*this)>; }

    void setStyle(SPStyle const *style, SPStyle const *context_style = nullptr) override;

    void setPixbuf(std::shared_ptr<Inkscape::Pixbuf const> pb);
    void setScale(double sx, double sy);
    void setOrigin(Geom::Point const &o);
    void setClipbox(Geom::Rect const &box);
    void setExtend(cairo_extend_t extend);
    Geom::Rect bounds() const;

protected:
    ~DrawingImage() override = default;

    unsigned _updateItem(Geom::IntRect const &area, UpdateContext const &ctx, unsigned flags, unsigned reset) override;
    unsigned _renderItem(DrawingContext &dc, RenderContext &rc, Geom::IntRect const &area, unsigned flags, DrawingItem const *stop_at) const override;
    DrawingItem *_pickItem(Geom::Point const &p, double delta, Geom::OptIntRect const &area_world, unsigned flags) override;

    std::shared_ptr<Inkscape::Pixbuf const> _pixbuf;

    SPImageRendering style_image_rendering;

    // TODO: the following three should probably be merged into a new Geom::Viewbox object
    Geom::Rect _clipbox; ///< for preserveAspectRatio
    Geom::Point _origin;
    Geom::Scale _scale;
    cairo_extend_t _extend;
};

} // namespace Inkscape

#endif // INKSCAPE_DISPLAY_DRAWING_IMAGE_H

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

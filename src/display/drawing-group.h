// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Group belonging to an SVG drawing element.
 *//*
 * Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *
 * Copyright (C) 2011 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_DISPLAY_DRAWING_GROUP_H
#define INKSCAPE_DISPLAY_DRAWING_GROUP_H

#include "display/drawing-item.h"

namespace Inkscape {

class DrawingGroup
    : public DrawingItem
{
public:
    DrawingGroup(Drawing &drawing);
    int tag() const override { return tag_of<decltype(*this)>; }

    bool pickChildren() { return _pick_children; }
    void setPickChildren(bool);

    void setChildTransform(Geom::Affine const &);

protected:
    ~DrawingGroup() override = default;

    unsigned _updateItem(Geom::IntRect const &area, UpdateContext const &ctx, unsigned flags, unsigned reset) override;
    unsigned _renderItem(DrawingContext &dc, RenderContext &rc, Geom::IntRect const &area, unsigned flags, DrawingItem const *stop_at) const override;
    void _clipItem(DrawingContext &dc, RenderContext &rc, Geom::IntRect const &area) const override;
    DrawingItem *_pickItem(Geom::Point const &p, double delta, Geom::OptIntRect const &area_world, unsigned flags) override;
    bool _canClip() const override { return true; }

    std::unique_ptr<Geom::Affine> _child_transform;
};

} // namespace Inkscape

#endif // INKSCAPE_DISPLAY_DRAWING_GROUP_H

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

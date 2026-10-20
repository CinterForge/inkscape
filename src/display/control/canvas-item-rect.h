// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_CANVAS_ITEM_RECT_H
#define SEEN_CANVAS_ITEM_RECT_H

/**
 * A class to represent a control rectangle. Used for rubberband selector, page outline, etc.
 */

/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Rewrite of CtrlRect
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstdint>
#include <memory>
#include <2geom/path.h>

#include "canvas-item.h"
#include "ui/pixel-alignment.h"

namespace Inkscape {

class CanvasItemRect final : public CanvasItem
{
public:
    CanvasItemRect(CanvasItemGroup *group);
    CanvasItemRect(CanvasItemGroup *group, Geom::Rect const &rect);

    // Geometry
    void set_rect(Geom::Rect const &rect);
    void visit_page_rects(std::function<void(Geom::Rect const &)> const &) const override;

    // Selection
    bool contains(Geom::Point const &p, double tolerance = 0) override;

    // Properties
    void set_is_page(bool is_page);
    void set_fill(uint32_t color) override;
    void set_dashed(bool dash = true);
    void set_inverted(bool inverted = false);
    void set_shadow(uint32_t color, int width, bool inside = false);
    void set_pixel_alignment(RectLineAlignment alignment);
    void set_infill_shift(bool shift);

protected:
    ~CanvasItemRect() override = default;

    void _update(bool propagate) override;
    void _render(Inkscape::CanvasItemBuffer &buf) const override;

    // Geometry
    double get_shadow_size() const;
    bool is_infill_shift() const;

    Geom::Rect _rect;
    bool _is_page = false;
    bool _dashed = false;
    bool _inverted = false;
    int _shadow_width = 0;
    uint32_t _shadow_color = 0x0;
    bool _shadow_inside = false; // whether shadow is around fill or border
    bool _shift_infill = false;
    RectLineAlignment _pixel_alignment = RectLineAlignment::CenterInside;
};

} // namespace Inkscape

#endif // SEEN_CANVAS_ITEM_RECT_H

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

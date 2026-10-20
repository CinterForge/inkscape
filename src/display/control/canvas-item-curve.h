// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_CANVAS_ITEM_CURVE_H
#define SEEN_CANVAS_ITEM_CURVE_H

/**
 * A class to represent a single Bezier control curve.
 */

/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Rewrite of SPCtrlLine and SPCtrlCurve
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <memory>
#include <2geom/path.h>

#include "canvas-item.h"

namespace Inkscape {

class CanvasItemCurve final : public CanvasItem
{
public:
    CanvasItemCurve(CanvasItemGroup *group);
    CanvasItemCurve(CanvasItemGroup *group, Geom::Point const &p0, Geom::Point const &p1);
    CanvasItemCurve(CanvasItemGroup *group, Geom::Point const &p0, Geom::Point const &p1,
                                            Geom::Point const &p2, Geom::Point const &p3);

    // Geometry
    void set_coords(Geom::Point const &p0, Geom::Point const &p1);
    void set_coords(Geom::Point const &p0, Geom::Point const &p1, Geom::Point const &p2, Geom::Point const &p3);
    void set_width(int width);
    void set_bg_alpha(float alpha);
    bool is_line() const { return _curve->size() == 2; }

    double closest_distance_to(Geom::Point const &p) const;

    // Selection
    bool contains(Geom::Point const &p, double tolerance = 0) override;
 
protected:
    ~CanvasItemCurve() override = default;

    void _update(bool propagate) override;
    void _render(Inkscape::CanvasItemBuffer &buf) const override;

    // Display
    std::unique_ptr<Geom::BezierCurve> _curve;

    int _width = 1;
    int background_width = 3; // this should be an odd number so that the background appears on both the sides of the curve.
    float bg_alpha = 0.5f;
};

} // namespace Inkscape

#endif // SEEN_CANVAS_ITEM_CURVE_H

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

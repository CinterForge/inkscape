// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_CANVAS_ITEM_GUIDELINE_H
#define SEEN_CANVAS_ITEM_GUIDELINE_H

/**
 * A class to represent a control guide line.
 */

/*
 * Authors:
 *   Tavmjong Bah       - Rewrite of SPGuideLine
 *   Rafael Siejakowski - Tweaks to handle appearance
 *
 * Copyright (C) 2020-2022 the Authors.
 *
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/ustring.h>

#include <2geom/point.h>
#include <2geom/transforms.h>

#include "canvas-item.h"
#include "canvas-item-ctrl.h"
#include "canvas-item-ptr.h"

namespace Inkscape {

class CanvasItemGuideHandle;

class CanvasItemGuideLine final : public CanvasItem
{
public:
    CanvasItemGuideLine(CanvasItemGroup *group, Glib::ustring label, Geom::Point const &origin, Geom::Point const &normal);

    // Geometry
    void set_origin(Geom::Point const &origin);
    void set_normal(Geom::Point const &normal);
    double closest_distance_to(Geom::Point const &p);

    // Selection
    bool contains(Geom::Point const &p, double tolerance = 0) override;

    // Properties
    void set_visible(bool visible) override;
    void set_stroke(uint32_t color) override;
    void set_label(Glib::ustring &&label);
    void set_locked(bool locked);
    void set_inverted(bool inverted);
 
    // Getters
    CanvasItemGuideHandle *dot() const;

protected:
    ~CanvasItemGuideLine() override = default;

    void _update(bool propagate) override;
    void _render(Inkscape::CanvasItemBuffer &buf) const override;

    Geom::Point _origin;
    Geom::Point _normal = Geom::Point(0, 1);
    Glib::ustring _label;
    bool _locked = true; // Flipped in constructor to trigger init of _origin_ctrl.
    bool _inverted = false;
    CanvasItemPtr<CanvasItemGuideHandle> _origin_ctrl;

    static constexpr uint32_t CONTROL_LOCKED_COLOR = 0x00000080; // RGBA black semitranslucent
    static constexpr double LABEL_SEP = 2.0; // Distance between the label and the origin control
    static constexpr int FONT_SIZE = 8;
};

// A handle ("dot") serving as draggable origin control
class CanvasItemGuideHandle final : public CanvasItemCtrl
{
public:
    CanvasItemGuideHandle(CanvasItemGroup *group, Geom::Point const &pos, CanvasItemGuideLine *line);
    double radius() const;

protected:
    ~CanvasItemGuideHandle() override = default;

    CanvasItemGuideLine *_my_line; // The guide line we belong to

    // static data
    static constexpr double   SCALE        = 0.55; // handle size relative to an auto-smooth node
    static constexpr float MINIMUM_SIZE = 7;    // smallest handle size, must be an odd int
};

} // namespace Inkscape

#endif // SEEN_CANVAS_ITEM_GUIDELINE_H

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

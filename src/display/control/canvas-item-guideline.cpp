// SPDX-License-Identifier: GPL-2.0-or-later
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

#include <2geom/line.h>
#include <pangomm/fontdescription.h>
#include <pangomm/layout.h>

#include "canvas-item-guideline.h"
#include "canvas-item-ctrl.h"
#include "colors/utils.h"
#include "display/control/canvas-item-enums.h"

namespace Inkscape {

/**
 * Create a control guide line. Points are in document units.
 */
CanvasItemGuideLine::CanvasItemGuideLine(CanvasItemGroup *group, Glib::ustring label,
                                         Geom::Point const &origin, Geom::Point const &normal)
    : CanvasItem(group)
    , _origin(origin)
    , _normal(normal)
    , _label(std::move(label))
{
    _name = "CanvasItemGuideLine:" + _label;
    _pickable = true; // For now, everybody gets events from this class!

    // Control to move guide line.
    _origin_ctrl = make_canvasitem<CanvasItemGuideHandle>(group, _origin, this);
    _origin_ctrl->set_name("CanvasItemGuideLine:Ctrl:" + _label);
    _origin_ctrl->set_size_default();
    _origin_ctrl->set_pickable(true); // The handle will also react to dragging
    _origin_ctrl->set_preferred_size_parity(1);
    set_locked(false); // Init _origin_ctrl shape and stroke.
}

/**
 * Sets origin of guide line (place where handle is located).
 */
void CanvasItemGuideLine::set_origin(Geom::Point const &origin)
{
    if (_origin != origin) {
        _origin = origin;
        _origin_ctrl->set_position(_origin);
        request_update();
    }
}

/**
 * Sets orientation of guide line.
 */
void CanvasItemGuideLine::set_normal(Geom::Point const &normal)
{
    if (_normal != normal) {
        _normal = normal;
        request_update();
    }
}

/**
 * Sets the inverted nature of the line
 */
void CanvasItemGuideLine::set_inverted(bool inverted)
{
    if (_inverted != inverted) {
        _inverted = inverted;
        request_update();
    }
}

/**
 * Returns distance between point in canvas units and nearest point on guideLine.
 */
double CanvasItemGuideLine::closest_distance_to(Geom::Point const &p)
{
    // Maybe store guide as a Geom::Line?
    auto guide = Geom::Line::from_origin_and_vector(_origin, Geom::rot90(_normal));
    guide *= affine();
    return Geom::distance(p, guide);
}

/**
 * Returns true if point p (in canvas units) is within tolerance (canvas units) distance of guideLine (or 1 if tolerance is zero).
 */
bool CanvasItemGuideLine::contains(Geom::Point const &p, double tolerance)
{
    if (tolerance == 0) {
        tolerance = 1; // Can't pick of zero!
    }

    return closest_distance_to(p) < tolerance;
}

/**
 * Returns the pointer to the origin control (the "dot")
 */
CanvasItemGuideHandle* CanvasItemGuideLine::dot() const
{
    return _origin_ctrl.get();
}

/**
 * Update and redraw control guideLine.
 */
void CanvasItemGuideLine::_update(bool)
{
    // Required when rotating canvas
    _bounds = Geom::Rect(-Geom::infinity(), -Geom::infinity(), Geom::infinity(), Geom::infinity());

    // Queue redraw of new area (and old too).
    request_redraw();
}

/**
 * Render guideLine to screen via Cairo.
 */
void CanvasItemGuideLine::_render(Inkscape::CanvasItemBuffer &buf) const
{
    // Document to canvas
    Geom::Point const normal = _normal * affine().withoutTranslation(); // Direction only
    Geom::Point const origin = _origin * affine();

    int const line_width = 1;
    Geom::Point const aligned_origin = align_to_pixels05(origin, line_width * buf.device_scale, buf.device_scale);

    // Set up the Cairo rendering context
    auto ctx = buf.cr;
    ctx->save();
    ctx->translate(-buf.rect.left(), -buf.rect.top()); // Canvas to screen
    ctx->set_source_rgba(SP_RGBA32_R_F(_stroke), SP_RGBA32_G_F(_stroke),
                         SP_RGBA32_B_F(_stroke), SP_RGBA32_A_F(_stroke));
    ctx->set_line_width(line_width);

    if (_inverted) {
        // operator not available in cairo C++ bindings
        cairo_set_operator(ctx->cobj(), CAIRO_OPERATOR_DIFFERENCE);
    }

    if (!_label.empty()) { // Render text label
        ctx->save();
        ctx->translate(aligned_origin.x(), aligned_origin.y());

        ctx->rotate(atan2(normal.cw()) + M_PI * (_context->yaxisdown() ? 1 : 0));
        ctx->translate(LABEL_SEP, -(_origin_ctrl->radius() + 2 * FONT_SIZE)); // Offset
        ctx->move_to(0, 0);

        // Call Pango to render text with fallback fonts
        auto layout = Pango::Layout::create(ctx);
        layout->set_font_description(Pango::FontDescription(Glib::ustring::compose("Sans %1", FONT_SIZE)));
        layout->set_text(_label);
        layout->show_in_cairo_context(ctx);

        ctx->restore();
    }

    // Draw guide.
    // Special case: horizontal and vertical lines (easier calculations)

    // Don't use isHorizontal()/isVertical() as they test only exact matches.
    if (Geom::are_near(normal.y(), 0.0)) {
        // Vertical
        double const position = aligned_origin.x();
        ctx->move_to(position, buf.rect.top());
        ctx->line_to(position, buf.rect.bottom());
    } else if (Geom::are_near(normal.x(), 0.0)) {
        // Horizontal
        double position = aligned_origin.y();
        ctx->move_to(buf.rect.left(), position);
        ctx->line_to(buf.rect.right(), position);
    } else {
        // Angled
        Geom::Line line = Geom::Line::from_origin_and_vector(aligned_origin, Geom::rot90(normal));

        if (auto segment = line.clip(buf.rect)) {
            auto p1 = segment->initialPoint();
            auto p2 = segment->finalPoint();
            ctx->move_to(p1.x(), p1.y());
            ctx->line_to(p2.x(), p2.y());
        }
    }
    ctx->stroke();

    ctx->restore();
}

void CanvasItemGuideLine::set_visible(bool visible)
{
    CanvasItem::set_visible(visible);
    _origin_ctrl->set_visible(visible);
}

void CanvasItemGuideLine::set_stroke(uint32_t color)
{
    // Make sure the fill of the control is the same as the stroke
    // of the guide-line:
    _origin_ctrl->set_fill(color);
    CanvasItem::set_stroke(color);
}

void CanvasItemGuideLine::set_label(Glib::ustring &&label)
{
    defer([=, this, label = std::move(label)] () mutable {
        if (_label == label) return;
        _label = std::move(label);
        request_update();
    });
}

void CanvasItemGuideLine::set_locked(bool locked)
{
    defer([=, this] {
        if (_locked == locked) return;
        _locked = locked;
        if (_locked) {
            _origin_ctrl->set_shape(CANVAS_ITEM_CTRL_SHAPE_CROSS);
            _origin_ctrl->set_stroke(CONTROL_LOCKED_COLOR);
            _origin_ctrl->set_fill(0x00000000);   // no fill
        } else {
            _origin_ctrl->set_shape(CANVAS_ITEM_CTRL_SHAPE_CIRCLE);
            _origin_ctrl->set_stroke(0x00000000); // no stroke
            _origin_ctrl->set_fill(_stroke);
        }
    });
}

//===============================================================================================

/**
 * @brief Create a handle ("dot") along a guide line
 * @param group - the associated canvas item group
 * @param pos   - position
 * @param line  - pointer to the corresponding guide line
 */
CanvasItemGuideHandle::CanvasItemGuideHandle(CanvasItemGroup *group,
                                             Geom::Point const &pos,
                                             CanvasItemGuideLine* line)
    : CanvasItemCtrl(group, CANVAS_ITEM_CTRL_TYPE_GUIDE_HANDLE, pos)
    , _my_line(line) // Save a pointer to our guide line
{
}

/**
 * Return the radius of the handle dot
 */
double CanvasItemGuideHandle::radius() const
{
    auto width = std::round(get_width());
    return 0.5 * static_cast<double>(width); // radius is half the width
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

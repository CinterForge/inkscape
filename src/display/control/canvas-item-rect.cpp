// SPDX-License-Identifier: GPL-2.0-or-later
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

#include "canvas-item-rect.h"

#include <cairo/cairo.h>
#include <cairomm/pattern.h>

#include "display/cairo-utils.h"
#include "ui/util.h"
#include "ui/widget/canvas.h"

namespace Inkscape {

/**
 * Create an null control rect.
 */
CanvasItemRect::CanvasItemRect(CanvasItemGroup *group)
    : CanvasItem(group)
{
    _name = "CanvasItemRect:Null";
    _fill = 0;
}

/**
 * Create a control rect. Point are in document coordinates.
 */
CanvasItemRect::CanvasItemRect(CanvasItemGroup *group, Geom::Rect const &rect)
    : CanvasItem(group)
    , _rect(rect)
{
    _name = "CanvasItemRect";
    _fill = 0;
}

/**
 * Set a control rect. Points are in document coordinates.
 */
void CanvasItemRect::set_rect(Geom::Rect const &rect)
{
    defer([=, this] {
        if (_rect == rect) return;
        _rect = rect;
        request_update();
    });
}

/**
 * Run a callback for each rectangle that should be filled and painted in the background.
 */
void CanvasItemRect::visit_page_rects(std::function<void(Geom::Rect const &)> const &f) const
{
    if (_is_page && _fill != 0) {
        f(_rect);
    }
}

/**
 * Returns true if point p (in canvas units) is within tolerance (canvas units) distance of rect.
 * Non-zero tolerance not implemented! Is valid for a rotated canvas.
 */
bool CanvasItemRect::contains(Geom::Point const &p, double tolerance)
{
    if (tolerance != 0) {
        std::cerr << "CanvasItemRect::contains: Non-zero tolerance not implemented!" << std::endl;
    }

    return _rect.contains(p * affine().inverse());
}

/**
 * Update and redraw control rect.
 */
void CanvasItemRect::_update(bool)
{
    // Queue redraw of old area (erase previous content).
    request_redraw();

    // Enlarge bbox by twice shadow size (to allow for shadow on any side with a 45deg rotation).
    _bounds = _rect;
    // note: add shadow size before applying transformation, since get_shadow_size accounts for scale
    if (_shadow_width > 0 && !_dashed) {
        _bounds->expandBy(2 * get_shadow_size());
    }
    *_bounds *= affine();

    // Room for stroke and outline. Not doing the extra adjustment of 2 units
    // leads to artifacts.
    _bounds->expandBy(get_effective_outline() + 2);

    // Queue redraw of new area
    request_redraw();
}

/**
 * Render rect to screen via Cairo.
 */
void CanvasItemRect::_render(Inkscape::CanvasItemBuffer &buf) const
{
    // Are we axis aligned?
    auto const &aff = affine();
    bool const axis_aligned = (Geom::are_near(aff[1], 0) && Geom::are_near(aff[2], 0))
                           || (Geom::are_near(aff[0], 0) && Geom::are_near(aff[3], 0));

    auto total_thickness = get_effective_outline();

    // If we are and the effective outline is of odd width then snap the rectangle to the pixel grid.
    Geom::Point corners[4];
    double scale = affine().descrim();
    auto rect = _rect;
    Geom::Point expansion_dir(1 / scale, 1 / scale);
    expansion_dir *= aff.withoutTranslation();
    if (!axis_aligned &&
        (_pixel_alignment == RectLineAlignment::Outside || _pixel_alignment == RectLineAlignment::Inside)) {
        auto shift = total_thickness * 0.5 / scale;
        rect.expandBy(_pixel_alignment == RectLineAlignment::Inside ? -shift : shift);
    }
    for (int i = 0; i < 4; i++) {
        corners[i] = rect.corner(i) * aff;
    }

    if (axis_aligned) {
        auto pixel_thickness = std::round(total_thickness * buf.device_scale);
        total_thickness = pixel_thickness / buf.device_scale;
        Geom::Point unaligned_corner0 = corners[0];
        Geom::Rect transformed_rect = Geom::Rect(corners[0], corners[2]);
        auto pixel_aligned = pixel_align(transformed_rect, _pixel_alignment, pixel_thickness, buf.device_scale);

        double min_dist = transformed_rect.width() + transformed_rect.height();
        min_dist *= min_dist;
        int closest_corner = 0;
        for (int i = 0; i < 4; i++) {
            corners[i] = pixel_aligned.corner(i);
            auto dist2 = (corners[i] - unaligned_corner0).lengthSq();
            if (dist2 < min_dist) {
                closest_corner = i;
                min_dist = dist2;
            }
        }
        std::rotate(std::begin(corners), &corners[closest_corner], std::end(corners));
        // transform snapped corners back to original coordinates to ensure that shadow doesn't leak inside the frame
        auto inverse_transform = aff.inverse();
        if (!_shadow_inside) {
            rect = pixel_aligned.expandedBy(total_thickness * 0.5) * inverse_transform;
        } else if (_shift_infill) {
            rect = pixel_aligned.shrunkBy(total_thickness * 0.5) * inverse_transform;
        } else {
            rect = pixel_aligned * inverse_transform;
        }
    }

    buf.cr->save();
    buf.cr->translate(-buf.rect.left(), -buf.rect.top());

    if (_inverted) {
        cairo_set_operator(buf.cr->cobj(), CAIRO_OPERATOR_DIFFERENCE);
    }

    // Draw shadow first. Shadow extends under rectangle to reduce aliasing effects. Canvas draws page shadows in OpenGL mode.
    if (_shadow_width > 0 && !_dashed && !(_is_page && get_canvas()->get_opengl_enabled())) {
        // Flip shadow upside-down if y-axis is inverted.
        auto vflip = Geom::identity();
        if (!_context->yaxisdown()) {
            vflip = Geom::Affine(1, 0, 0, -1, 0, rect.top() + rect.bottom()); // flip rect upside down
        }

        buf.cr->save();
        buf.cr->transform(geom_to_cairo(vflip * aff));
        ink_cairo_draw_drop_shadow(buf.cr, rect, get_shadow_size(), _shadow_color, SP_RGBA32_A_F(_shadow_color));
        buf.cr->restore();
    }

    // Get the points we need transformed into window coordinates.
    buf.cr->begin_new_path();
    for (auto &corner : corners) {
        buf.cr->line_to(corner.x(), corner.y());
    }
    buf.cr->close_path();

    // Draw border.
    static std::valarray<double> const dashes = {4.0, 4.0};
    if (_dashed) {
        buf.cr->set_dash(dashes, -0.5);
    }

    // We maybe have painted the background, back to "normal" compositing

    // Do outline
    if (SP_RGBA32_A_U(_outline) > 0 && _outline_width > 0) {
        ink_cairo_set_source_color(buf.cr, Colors::Color(_outline));
        buf.cr->set_line_width(get_effective_outline());
        buf.cr->stroke_preserve();
    }

    // Do stroke
    if (SP_RGBA32_A_U(_stroke) > 0 && _stroke_width > 0) {
        ink_cairo_set_source_color(buf.cr, Colors::Color(_stroke));
        buf.cr->set_line_width(_stroke_width);
        buf.cr->stroke_preserve();
    }

    // Highlight the border by drawing it in _shadow_color.
    if (_shadow_width == 1 && _dashed) {
        buf.cr->set_dash(dashes, 3.5); // Dash offset by dash length.
        ink_cairo_set_source_color(buf.cr, Colors::Color(_shadow_color));
        buf.cr->stroke_preserve();
    }

    if (_shift_infill && axis_aligned) {
        // Shift infill region so that it is inside the outline and can't be seen behind semitransparent stroke.
        // Not shifting while rotated as that tends to create ugly gap which looks worse than infill tinting
        // semi transparent stroke.
        auto shift = expansion_dir * total_thickness * 0.5;
        buf.cr->begin_new_path();
        for (auto &corner : corners) {
            corner += shift;
            buf.cr->line_to(corner.x(), corner.y());
            shift = shift.cw();
        }
    }

    // Draw fill pattern
    if (_fill_pattern && !buf.outline_pass) {
        buf.cr->set_source(_fill_pattern);
        buf.cr->fill_preserve();
    }

    // Draw fill
    if (SP_RGBA32_A_U(_fill) > 0 && !buf.outline_pass) {
        ink_cairo_set_source_color(buf.cr, Colors::Color(_fill));
        buf.cr->fill_preserve();
    }

    buf.cr->begin_new_path(); // Clear path or get weird artifacts.

    // Uncomment to show bounds
    // Geom::Rect bounds = _bounds;
    // bounds.expandBy(-1);
    // bounds -= buf.rect.min();
    // buf.cr->set_source_rgba(1.0, 0.0, _shadow_width / 3.0, 1.0);
    // buf.cr->rectangle(bounds.min().x(), bounds.min().y(), bounds.width(), bounds.height());
    // buf.cr->stroke();

    buf.cr->restore();
}

void CanvasItemRect::set_is_page(bool is_page)
{
    defer([=, this] {
        if (_is_page == is_page) return;
        _is_page = is_page;
        request_redraw();
    });
}

void CanvasItemRect::set_fill(uint32_t fill)
{
    defer([=, this] {
        if (fill != _fill && _is_page) {
            get_canvas()->set_page(fill);
        }
        _fill = fill;
        request_redraw();
    });
}

void CanvasItemRect::set_dashed(bool dashed)
{
    defer([=, this] {
        if (_dashed == dashed) return;
        _dashed = dashed;
        request_redraw();
    });
}

void CanvasItemRect::set_inverted(bool inverted)
{
    defer([=, this] {
        if (_inverted == inverted) return;
        _inverted = inverted;
        request_redraw();
    });
}

void CanvasItemRect::set_shadow(uint32_t color, int width, bool inside)
{
    defer([=, this] {
        if (_shadow_color == color && _shadow_width == width) {
            return;
        }
        _shadow_color = color;
        _shadow_width = width;
        _shadow_inside = inside;
        request_redraw();
        if (_is_page) {
            get_canvas()->set_shadow(_shadow_width > 0 ? color : 0x0, _shadow_width);
        }
    });
}

void CanvasItemRect::set_pixel_alignment(RectLineAlignment alignment)
{
    defer([=, this] {
        if (_pixel_alignment == alignment) {
            return;
        }
        _pixel_alignment = alignment;
        request_redraw();
    });
}

void CanvasItemRect::set_infill_shift(bool shift)
{
    defer([=, this] {
        if (_shift_infill == shift) {
            return;
        }
        _shift_infill = shift;
        request_redraw();
    });
}

double CanvasItemRect::get_shadow_size() const
{
    // gradient drop shadow needs much more room than solid one, so inflating the size;
    // fudge factor of 6 used to make sizes baked in svg documents work as steps:
    // typical value of 2 will work out to 12 pixels which is a narrow shadow (b/c of exponential fall of)
    auto size = _shadow_width * 6;
    if (size < 0) {
        size = 0;
    } else if (size > 120) {
        // arbitrarily selected max size, so Cairo gradient doesn't blow up if document has bogus shadow values
        size = 120;
    }
    auto scale = affine().descrim();

    // calculate space for gradient shadow; if divided by 'scale' it would be zoom independent (fixed in size);
    // if 'scale' is not used, drop shadow will be getting smaller with document zoom;
    // here hybrid approach is used: "unscaling" with square root of scale allows shadows to diminish
    // more slowly at small zoom levels (so it's still perceptible) and grow more slowly at high mag (where it doesn't matter, b/c it's typically off-screen)
    return size / (scale > 0 ? sqrt(scale) : 1);
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

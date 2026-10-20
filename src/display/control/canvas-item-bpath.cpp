// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * A class to represent a Bezier path.
 */

/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Rewrite of SPCanvasBPath
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "canvas-item-bpath.h"
#include <cairomm/matrix.h>
#include <cstdint>

#include "path/path-curve.h"
#include "display/cairo-utils.h"
#include "helper/geom.h" // bounds_exact_transformed()

namespace Inkscape {

/**
 * Create a null control bpath.
 */
CanvasItemBpath::CanvasItemBpath(CanvasItemGroup *group)
    : CanvasItem(group)
{
    _name = "CanvasItemBpath:Null";
    _pickable = true; // For now, everyone gets events from this class!
}

/**
 * Create a control bpath. Path is in document coordinates.
 */
CanvasItemBpath::CanvasItemBpath(CanvasItemGroup *group, Geom::PathVector path, bool phantom_line)
    : CanvasItem(group)
    , _path(std::move(path))
    , _phantom_line(phantom_line)
{
    _name = "CanvasItemBpath";
    _pickable = true; // For now, everyone gets events from this class!
    request_update(); // Render immediately or temporary bpaths won't show.
}

/**
 * Set a control bpath. Path is in document coordinates.
 */
void CanvasItemBpath::set_bpath(Geom::PathVector path, bool phantom_line)
{
    defer([=, this, path = std::move(path)] () mutable {
        _path = std::move(path);
        _phantom_line = phantom_line;
        request_update();
    });
}

/**
 * Set the fill color and fill rule.
 */
void CanvasItemBpath::set_fill(uint32_t fill, SPWindRule fill_rule)
{
    defer([=, this] {
        if (_fill == fill && _fill_rule == fill_rule) return;
        _fill = fill;
        _fill_rule = fill_rule;
        request_redraw();
    });
}

void CanvasItemBpath::set_dashes(std::vector<double> &&dashes)
{
    defer([this, dashes = std::move(dashes)] () mutable {
        _dashes = std::move(dashes);
    });
}

/**
 * Returns distance between point in canvas units and nearest point on bpath.
 */
double CanvasItemBpath::closest_distance_to(Geom::Point const &p) const
{
    double d = Geom::infinity();

    // Convert p to document coordinates (quicker than converting path to canvas units).
    Geom::Point p_doc = p * affine().inverse();
    _path.nearestTime(p_doc, &d);
    d *= affine().descrim(); // Uniform scaling and rotation only.

    return d;
}

/**
 * Returns true if point p (in canvas units) is within tolerance (canvas units) distance of bpath.
 */
bool CanvasItemBpath::contains(Geom::Point const &p, double tolerance)
{
    if (tolerance == 0) {
        tolerance = 1; // Need a minimum tolerance value or always returns false.
    }

    // Check for 'inside' a filled bpath if a fill is being used.
    if ((_fill & 0xff) != 0) {
        Geom::Point p_doc = p * affine().inverse();
        if (_path.winding(p_doc) % 2 != 0) {
            return true;
        }
    }

    // Otherwise see how close we are to the outside line.
    return closest_distance_to(p) < tolerance;
}

/**
 * Update and redraw control bpath.
 */
void CanvasItemBpath::_update(bool)
{
    // Queue redraw of old area (erase previous content).
    request_redraw();

    if (_path.empty()) {
        _bounds = {};
        return;
    }

    // Room for stroke and outline.
    // CanvasItemBpath doesn't seem to require the extra adjustment of 2 units to avoid artifacts,
    // but it is done for consistency with CanvasItemRect.
    _bounds = expandedBy(bounds_exact_transformed(_path, affine()), get_effective_outline() / 2 + 2);

    // Queue redraw of new area
    request_redraw();
}

/**
 * Render bpath to screen via Cairo.
 */
void CanvasItemBpath::_render(Inkscape::CanvasItemBuffer &buf) const
{
    bool do_fill   = (_fill   & 0xff) != 0; // Not invisible.
    bool do_stroke = (_stroke & 0xff) != 0; // Not invisible.

    if (!do_fill && !do_stroke) {
        // Both fill and stroke invisible.
        return;
    }

    buf.cr->save();

    // Setup path
    buf.cr->set_tolerance(0.5);
    buf.cr->begin_new_path();

    feed_pathvector_to_cairo(buf.cr->cobj(), _path, affine(), buf.rect,
                             /* optimize_stroke */ !(do_fill || _fill_pattern), get_effective_outline());

    // Do fill
    if (do_fill) {
        ink_cairo_set_source_color(buf.cr, Colors::Color(_fill));
        buf.cr->set_fill_rule(_fill_rule == SP_WIND_RULE_EVENODD ?
                               Cairo::Context::FillRule::EVEN_ODD : Cairo::Context::FillRule::WINDING);
        buf.cr->fill_preserve();
    }

    // Do fill pattern
    if (_fill_pattern) {
        buf.cr->save();
        buf.cr->translate(-buf.rect.min().x(), -buf.rect.min().y());
        buf.cr->set_source(_fill_pattern);
        buf.cr->fill_preserve();
        buf.cr->restore();
    }

    // Do outline
    if (SP_RGBA32_A_U(_outline) > 0 && _outline_width > 0) {
        ink_cairo_set_source_color(buf.cr, Colors::Color(_outline));
        buf.cr->set_line_width(get_effective_outline());
        buf.cr->stroke_preserve();
    }

    // Do stroke
    if (do_stroke && _stroke_width > 0) {

        if (!_dashes.empty()) {
            buf.cr->set_dash(_dashes, 0.0); // 0.0 is offset
        }

        if (_phantom_line) {
            buf.cr->set_source_rgba(1.0, 1.0, 1.0, 0.25);
            buf.cr->set_line_width(2.0);
            buf.cr->stroke_preserve();
        }

        ink_cairo_set_source_color(buf.cr, Colors::Color(_stroke));
        buf.cr->set_line_width(_stroke_width);
        buf.cr->stroke();

    } else {
        buf.cr->begin_new_path(); // Clears path
    }

    buf.cr->restore();
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

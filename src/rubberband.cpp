// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Rubberbanding selector.
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "rubberband.h"

#include <cairomm/pattern.h>
#include <cstdint>
#include <unordered_map>
#include <utility>

#include <2geom/path.h>
#include "desktop.h"
#include "display/cairo-utils.h"
#include "display/control/canvas-item-bpath.h"
#include "display/control/canvas-item-enums.h"
#include "display/control/canvas-item-rect.h"
#include "display/control/ctrl-handle-manager.h"
#include "display/control/ctrl-handle-styling.h"
#include "preferences.h"
#include "ui/widget/canvas.h" // autoscroll

Inkscape::Rubberband *Inkscape::Rubberband::_instance = nullptr;

Inkscape::Rubberband::Rubberband(SPDesktop *dt)
    : _desktop(dt)
{}

void Inkscape::Rubberband::delete_canvas_items()
{
    _rect.reset();
    _touchpath.reset();
}

Geom::Path Inkscape::Rubberband::getPath() const
{
    g_assert(_started);
    if (_mode == Rubberband::Mode::TOUCHPATH) {
        return _path * _desktop->w2d();
    }
    return Geom::Path(*getRectangle());
}

std::vector<Geom::Point> Inkscape::Rubberband::getPoints() const
{
    return _path.nodes();
}

void Inkscape::Rubberband::start(SPDesktop *d, Geom::Point const &p, bool tolerance)
{
    _desktop = d;

    _start = p;
    _started = true;
    _moved = false;

    auto prefs = Inkscape::Preferences::get();
    _tolerance = tolerance ? prefs->getIntLimited("/options/dragtolerance/value", 0, 0, 100) : 0.0;

    _touchpath_curve.start(p);

    _path = Geom::Path(_desktop->d2w(p));

    delete_canvas_items();
}

void Inkscape::Rubberband::stop()
{
    _started = false;
    _moved = false;

    _mode = default_mode;
    _handle = default_handle;
    _deselect_handle = default_deselect_handle;

    _touchpath_curve.clear();
    _path.clear();

    delete_canvas_items();
}

static std::unordered_map<uint32_t, Cairo::RefPtr<Cairo::Pattern>> _pattern_cache;

static Cairo::RefPtr<Cairo::Pattern> get_cached_pattern(uint32_t color)
{
    auto &pat = _pattern_cache[color];
    if (!pat) {
        pat = ink_cairo_pattern_create_slanting_stripes(color);
    }
    return pat;
}

void Inkscape::Rubberband::move(Geom::Point const &p)
{
    if (!_started) 
        return;

    if (!_moved) {
        if (Geom::are_near(_start, p, _tolerance/_desktop->current_zoom()))
            return;
    }

    _end = p;
    _moved = true;
    _desktop->getCanvas()->enable_autoscroll();
    _touchpath_curve.appendNew<Geom::LineSegment>(p);

    Geom::Point next = _desktop->d2w(p);
    // we want the points to be at most 0.5 screen pixels apart,
    // so that we don't lose anything small;
    // if they are farther apart, we interpolate more points
    auto prev = _path.finalPoint();
    if (Geom::L2(next-prev) > 0.5) {
        int subdiv = 2 * (int) round(Geom::L2(next-prev) + 0.5);
        for (int i = 1; i <= subdiv; i ++) {
            _path.appendNew<Geom::LineSegment>(prev + ((double)i/subdiv) * (next - prev));
        }
    } else {
        _path.appendNew<Geom::LineSegment>(next);
    }

    if (_touchpath) _touchpath->set_visible(false);
    if (_rect) _rect->set_visible(false);

    auto const &css = Handles::Manager::get().getCss()->style_map;
    auto const &style = css.at(Handles::TypeState{.type = _handle});
    auto const &invert_style = css.at(Handles::TypeState{.type = _invert_handle});
    auto const &deselect_style = css.at(Handles::TypeState{.type = _deselect_handle});

    // the initial values are for Operation::ADD
    auto fill_color = style.getFill();
    auto stroke_color = style.getStroke();

    // operation to add or remove objects
    switch (_operation) {
        case Inkscape::Rubberband::Operation::INVERT:
            fill_color = invert_style.getFill();
            stroke_color = invert_style.getStroke();
            break;
        case Inkscape::Rubberband::Operation::REMOVE:
            fill_color = deselect_style.getFill();
            stroke_color = deselect_style.getStroke();
            break;
        default:
            break;
    }

    switch (_mode) {
        case Inkscape::Rubberband::Mode::RECT:
            if (!_rect) {
                _rect = make_canvasitem<CanvasItemRect>(_desktop->getCanvasControls());
                _rect->set_stroke_width(style.stroke_width());
                _rect->set_outline(style.getOutline());
                _rect->set_outline_width(style.outline_width());
                _rect->set_shadow(0xffffffff, 0); // Not a shadow
            }
            _rect->set_rect(Geom::Rect(_start, _end));
            _rect->set_fill(fill_color);
            _rect->set_stroke(stroke_color);
            _rect->set_visible(true);
            break;
        case Inkscape::Rubberband::Mode::TOUCHRECT:
            if (!_rect) {
                _rect = make_canvasitem<CanvasItemRect>(_desktop->getCanvasControls());
                _rect->set_stroke_width(style.stroke_width());
                _rect->set_outline(style.getOutline());
                _rect->set_outline_width(style.outline_width());
                _rect->set_shadow(0xffffffff, 0); // Not a shadow
            }
            _rect->set_rect(Geom::Rect(_start, _end));
            _rect->set_fill_pattern(get_cached_pattern(fill_color));
            _rect->set_stroke(stroke_color);
            _rect->set_visible(true);
            break;
        case Inkscape::Rubberband::Mode::TOUCHPATH:
            if (!_touchpath) {
                _touchpath = make_canvasitem<CanvasItemBpath>(_desktop->getCanvasControls()); // Should be sketch?
                _touchpath->set_stroke_width(style.stroke_width());
                _touchpath->set_outline(style.getOutline());
                _touchpath->set_outline_width(style.outline_width());
            }
            _touchpath->set_bpath(_touchpath_curve);
            _touchpath->set_fill(fill_color, SP_WIND_RULE_EVENODD);
            _touchpath->set_stroke(stroke_color);
            _touchpath->set_visible(true);
            break;
        default:
            break;
    }
}

/**
 * @return Rectangle in desktop coordinates
 */
Geom::OptRect Inkscape::Rubberband::getRectangle() const
{
    if (!_started) {
        return Geom::OptRect();
    }

    return Geom::Rect(_start, _end);
}

Inkscape::Rubberband *Inkscape::Rubberband::get(SPDesktop *desktop)
{
    if (!_instance) {
        _instance = new Inkscape::Rubberband(desktop);
    }

    return _instance;
}

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

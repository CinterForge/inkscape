//   SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * HSLuv color wheel widget, based on the web implementation at
 * https://www.hsluv.org
 *//*
 * Authors:
 *   Tavmjong Bah
 *   Massinissa Derriche <massinissa.derriche@gmail.com>
 *
 * Copyright (C) 2018, 2021 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "ui/widget/ink-color-wheel.h"

#include <gdkmm/general.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/eventcontrollerkey.h>
#include <gtkmm/eventcontrollermotion.h>
#include <gtkmm/gestureclick.h>

#include "colors/spaces/hsluv.h"
#include "ui/controller.h"
#include "ui/util.h"
#include "ui/widget/generic/bin.h"
#include "util/drawing-utils.h"
#include "util/theme-utils.h"
#include "ink-color-wheel.h"

using namespace Inkscape::Colors;
using Inkscape::Colors::Space::Luv;
using Inkscape::Colors::Space::Type;

namespace Inkscape::UI::Widget {

// Sizes in pixels
constexpr static int const SIZE = 400;
constexpr static int const OUTER_CIRCLE_RADIUS = 190;
constexpr static double MAX_HUE = 360.0;
constexpr static double MAX_SATURATION = 100.0;
constexpr static double MAX_LIGHTNESS = 100.0;
constexpr static double MIN_HUE = 0.0;
constexpr static double MIN_SATURATION = 0.0;
constexpr static double MIN_LIGHTNESS = 0.0;
constexpr static double OUTER_CIRCLE_DASH_SIZE = 10.0;
constexpr static double VERTEX_EPSILON = 0.01;
constexpr static double marker_radius = 4.0;
constexpr static double focus_line_width = 1.0;
constexpr static double focus_padding = 3.0;
static auto const focus_dash = std::vector{1.5};
constexpr double ring_width = 0.15;

/** Represents a vertex of the Luv color polygon (intersection of bounding lines). */
struct Intersection final
{
    Intersection();

    Intersection(int line_1, int line_2, Geom::Point &&intersection_point, Geom::Angle start_angle)
        : line1{line_1}
        , line2{line_2}
        , point{intersection_point}
        , polar_angle{point}
        , relative_angle{polar_angle - start_angle}
    {
    }

    int line1 = 0; ///< Index of the first of the intersecting lines.
    int line2 = 0; ///< Index of the second of the intersecting lines.
    Geom::Point point; ///< The geometric position of the intersection.
    Geom::Angle polar_angle = 0.0; ///< Polar angle of the point (in radians).
    /** Angle relative to the polar angle of the point at which the boundary of the polygon
     *  passes the origin at the minimum distance (i.e., where an expanding origin-centered
     *  circle inside the polygon starts touching an edge of the polygon.)
     */
    Geom::Angle relative_angle = 0.0;
};

static double lerp(double v0, double v1, double t0, double t1, double t);
static ColorPoint lerp(ColorPoint const &v0, ColorPoint const &v1, double t0, double t1, double t);
static double luminance(Color const &color);
static Geom::Point to_pixel_coordinate(Geom::Point const &point, double scale, double resize);
static Geom::Point from_pixel_coordinate(Geom::Point const &point, double scale, double resize);
static std::vector<Geom::Point> to_pixel_coordinate(std::vector<Geom::Point> const &points, double scale,
                                                    double resize);
static void draw_vertical_padding(ColorPoint p0, ColorPoint p1, int padding, bool pad_upwards, guint32 *buffer,
                                  int height, int stride);

/* Base Color Wheel */

ColorWheelBase::ColorWheelBase(Type type, std::vector<double> initial_color)
    : Gtk::AspectFrame(0.5, 0.5, 1.0, false)
    , _bin{Gtk::make_managed<UI::Widget::Bin>()}
    , _values{type, std::move(initial_color)}
    , _drawing_area{Gtk::make_managed<Gtk::DrawingArea>()} {
    construct();
}

void ColorWheelBase::construct() {
    set_name("ColorWheel");
    add_css_class("flat");

    _drawing_area->set_focusable(true);
    _drawing_area->set_expand(true);
    _bin->connectAfterResize(sigc::mem_fun(*this, &ColorWheelBase::on_drawing_area_size));
    _drawing_area->set_draw_func(sigc::mem_fun(*this, &ColorWheelBase::on_drawing_area_draw ));
    _drawing_area->property_has_focus().signal_changed().connect([this]{ _drawing_area->queue_draw(); });
    _bin->set_child(_drawing_area);
    set_child(*_bin);

    auto const click = Gtk::GestureClick::create();
    click->set_button(0); // any
    click->signal_pressed().connect(Controller::use_state([this](auto &&...args) { return on_click_pressed(args...); }, *click));
    click->signal_released().connect(Controller::use_state([this](auto &, auto &&...args) { return on_click_released(args...); }, *click));
    _drawing_area->add_controller(click);

    auto const motion = Gtk::EventControllerMotion::create();
    motion->signal_motion().connect([this, &motion = *motion](auto &&...args) { return _on_motion(motion, args...); });
    _drawing_area->add_controller(motion);

    auto const key = Gtk::EventControllerKey::create();
    key->signal_key_pressed().connect(sigc::mem_fun(*this, &ColorWheelBase::on_key_pressed), true);
    key->signal_key_released().connect(sigc::mem_fun(*this, &ColorWheelBase::on_key_released));
    _drawing_area->add_controller(key);
}

ColorWheelBase::ColorWheelBase(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder, Type type, std::vector<double> initial_color):
    Gtk::AspectFrame(cobject),
    _bin(Gtk::make_managed<Bin>()),
    _values(type, initial_color),
    _drawing_area{Gtk::make_managed<Gtk::DrawingArea>()} {

    construct();
}

void ColorWheelBase::_on_motion(Gtk::EventControllerMotion const &motion, double x, double y)
{
    if (_adjusting) {
        auto state = motion.get_current_event_state();
        if (!Controller::has_flag(state, Gdk::ModifierType::BUTTON1_MASK)) {
            // lost button release event
            on_click_released(0, x, y);
            return;
        }
    }

    on_motion(motion, x, y);
}

sigc::connection ColorWheelBase::connect_color_changed(sigc::slot<void ()> slot)
{
    return _signal_color_changed.connect(std::move(slot));
}

void ColorWheelBase::color_changed()
{
    _signal_color_changed.emit();
    _drawing_area->queue_draw();
}

void ColorWheelBase::queue_drawing_area_draw()
{
    _drawing_area->queue_draw();
}

Gtk::Allocation ColorWheelBase::get_drawing_area_allocation() const
{
    return _drawing_area->get_allocation();
}

bool ColorWheelBase::drawing_area_has_focus() const
{
    return _drawing_area->has_focus();
}

void ColorWheelBase::focus_drawing_area()
{
    _drawing_area->grab_focus();
}

void ColorWheelBase::on_key_released(unsigned keyval, unsigned /*keycode*/, Gdk::ModifierType state)
{
    switch (keyval) {
        case GDK_KEY_Up:
        case GDK_KEY_KP_Up:
        case GDK_KEY_Down:
        case GDK_KEY_KP_Down:
        case GDK_KEY_Left:
        case GDK_KEY_KP_Left:
        case GDK_KEY_Right:
        case GDK_KEY_KP_Right:
            _adjusting = false;
    }
}

/* HSL Color Wheel */

bool ColorWheelHSL::setColor(Color const &color,
                           bool const /*overrideHue*/, bool const emit)
{
    if (_values.set(color, true)) {
        _triangle_corners.reset();
        _marker_point.reset();
        if (emit)
            color_changed();
        else
            queue_drawing_area_draw();
        return true;
    }
    return false;
}

void ColorWheelHSL::update_ring_source()
{
    if (_radii && _source_ring) return;

    auto const [width, height] = *_cache_size;
    auto const cx = width  / 2.0;
    auto const cy = height / 2.0;

    auto const stride = Cairo::ImageSurface::format_stride_for_width(Cairo::Surface::Format::RGB24, width);
    _source_ring.reset();
    _buffer_ring.resize(height * stride / 4);

    auto const &[r_min, r_max] = get_radii();
    double r2_max = (r_max+2) * (r_max+2); // Must expand a bit to avoid edge effects.
    double r2_min = (r_min-2) * (r_min-2); // Must shrink a bit to avoid edge effects.

    for (int i = 0; i < height; ++i) {
        auto p = _buffer_ring.data() + i * width;
        double dy = (cy - i);

        for (int j = 0; j < width; ++j) {
            double dx = (j - cx);
            double r2 = dx * dx + dy * dy;
            if (r2 < r2_min || r2 > r2_max) {
                *p++ = 0; // Save calculation time.
            } else {
                double angle = atan2 (dy, dx);
                if (angle < 0.0) {
                    angle += 2.0 * M_PI;
                }
                double hue = angle/(2.0 * M_PI);
                *p++ = Color(Type::HSV, {hue, 1.0, 1.0}).toARGB();
            }
        }
    }

    auto const data = reinterpret_cast<unsigned char *>(_buffer_ring.data());
    _source_ring = Cairo::ImageSurface::create(data,
                                               Cairo::Surface::Format::RGB24,
                                               width, height, stride);
}

ColorWheelHSL::TriangleCorners
ColorWheelHSL::update_triangle_source()
{
    bool const source_is_stale = !_triangle_corners.has_value();

    // Reorder so we paint from top down.
    auto ps = get_triangle_corners();
    std::sort(ps.begin(), ps.end(), [](auto const &l, auto const &r){ return l.y < r.y; });
    auto const &[p0, p1, p2] = ps;

    if (_source_triangle && !source_is_stale) return {p0, p1, p2};

    /* The triangle is painted by first finding color points on the
     * edges of the triangle at the same y value via linearly
     * interpolating between corner values, and then interpolating along
     * x between the those edge points. The interpolation is in sRGB
     * space which leads to a complicated mapping between x/y and
     * saturation/value. This was probably done to remove the need to
     * convert between HSV and RGB for each pixel.
     * Black corner: v = 0, s = 1
     * White corner: v = 1, s = 0
     * Color corner; v = 1, s = 1
     */
    constexpr int padding = 3; // Avoid edge artifacts.

    _source_triangle.reset();
    auto const [width, height] = *_cache_size;
    auto const stride = Cairo::ImageSurface::format_stride_for_width(Cairo::Surface::Format::RGB24, width);
    _buffer_triangle.resize(height * stride / 4);

    for (int y = 0; y < height; ++y) {
        if (p0.y <= y + padding && y - padding < p2.y) {
            // Get values on side at position y.
            ColorPoint side0;
            double y_inter = std::clamp(static_cast<double>(y), p0.y, p2.y);
            if (y < p1.y) {
                side0 = lerp(p0, p1, p0.y, p1.y, y_inter);
            } else {
                side0 = lerp(p1, p2, p1.y, p2.y, y_inter);
            }

            ColorPoint side1 = lerp(p0, p2, p0.y, p2.y, y_inter);

            // side0 should be on left
            if (side0.x > side1.x) {
                std::swap (side0, side1);
            }

            int const x_start = std::max(0, static_cast<int>(side0.x));
            int const x_end   = std::min(static_cast<int>(side1.x), width);

            auto p = _buffer_triangle.data() + y * (stride / 4);
            int x = 0;
            for (; x <= x_start; ++x) {
                *p++ = side0.color.toARGB();
            }
            for (; x < x_end; ++x) {
                *p++ = lerp(side0, side1, side0.x, side1.x, x).color.toARGB();
            }
            for (; x < width; ++x) {
                *p++ = side1.color.toARGB();
            }
        }
    }

    // add vertical padding to each side separately

    ColorPoint temp_point = lerp(p0, p1, p0.x, p1.x, (p0.x + p1.x) / 2.0);
    bool pad_upwards = _is_in_triangle(temp_point.x, temp_point.y + 1);
    draw_vertical_padding(p0, p1, padding, pad_upwards, _buffer_triangle.data(), height, stride / 4);

    temp_point = lerp(p0, p2, p0.x, p2.x, (p0.x + p2.x) / 2.0);
    pad_upwards = _is_in_triangle(temp_point.x, temp_point.y + 1);
    draw_vertical_padding(p0, p2, padding, pad_upwards, _buffer_triangle.data(), height, stride / 4);

    temp_point = lerp(p1, p2, p1.x, p2.x, (p1.x + p2.x) / 2.0);
    pad_upwards = _is_in_triangle(temp_point.x, temp_point.y + 1);
    draw_vertical_padding(p1, p2, padding, pad_upwards, _buffer_triangle.data(), height, stride / 4);

    auto const data = reinterpret_cast<unsigned char *>(_buffer_triangle.data());
    _source_triangle = Cairo::ImageSurface::create(data,
                                                   Cairo::Surface::Format::RGB24,
                                                   width, height, stride);

    return {p0, p1, p2};
}

void ColorWheelHSL::on_drawing_area_size(int width, int height, int baseline)
{
    auto const size = Geom::IntPoint{width, height};
    if (size == _cache_size) return;
    _cache_size = size;
    _radii.reset();
    _source_ring.reset();
}

std::array<Geom::Point, 3> find_triangle_points(double width, double height, double radius, double angle) {
    double cx = width  / 2.0;
    double cy = height / 2.0;
    auto add2 = 2.0 * M_PI / 3.0;
    auto angle2 = angle  + add2;
    auto angle4 = angle2 + add2;
    auto x0 = cx + std::cos(angle ) * radius;
    auto y0 = cy - std::sin(angle ) * radius;
    auto x1 = cx + std::cos(angle2) * radius;
    auto y1 = cy - std::sin(angle2) * radius;
    auto x2 = cx + std::cos(angle4) * radius;
    auto y2 = cy - std::sin(angle4) * radius;
    return { Geom::Point{x0, y0}, {x1, y1}, {x2, y2} };
}

void ColorWheelHSL::on_drawing_area_draw(Cairo::RefPtr<Cairo::Context> const &cr, int, int)
{
    auto const width = _cache_size->x();
    auto const height = _cache_size->y();
    auto const cx = width  / 2.0;
    auto const cy = height / 2.0;
    const auto angle = _values[0] * M_PI * 2;

    cr->set_antialias(Cairo::ANTIALIAS_SUBPIXEL);

    // Update caches
    update_ring_source();
    auto const &[p0, p1, p2] = update_triangle_source();
    auto const r_min = get_radii()[0];
    auto const r_max = get_radii()[1];

    // Paint with ring surface, clipping to ring.
    cr->save();
    cr->set_source(_source_ring, 0, 0);
    cr->set_line_width (r_max - r_min);
    cr->begin_new_path();
    cr->arc(cx, cy, (r_max + r_min) / 2.0, 0, 2.0 * M_PI);
    cr->stroke();
    bool dark = Util::is_current_theme_dark(*this);
    auto radius = r_max;
    auto area = Geom::Rect(cx, cy, cx, cy).expandedBy(radius);
    Util::draw_standard_border(cr, area, dark, radius, get_scale_factor(), true);
    radius = r_min + 0.5;
    auto small_area = Geom::Rect(cx, cy, cx, cy).expandedBy(radius);
    Util::draw_standard_border(cr, small_area, dark, radius, get_scale_factor(), true, false);
    cr->restore();
    // Paint marker on ring
    auto r = (r_min + r_max) / 2;
    auto ring_pos = Geom::Point(cx + cos(angle) * r, cy - sin(angle) * r);
    Util::draw_point_indicator(cr, ring_pos, marker_radius * 2);

    // Paint with triangle surface, clipping to triangle.
    cr->save();
    cr->set_source(_source_triangle, 0, 0);
    cr->move_to(p0.x, p0.y);
    cr->line_to(p1.x, p1.y);
    cr->line_to(p2.x, p2.y);
    cr->close_path();
    cr->fill();
    auto border_color = Util::get_standard_border_color(dark);
    auto scale = get_scale_factor();
    Util::draw_border_shape(cr, Geom::Rect(0, 0, width, height), border_color, scale, [=](auto& ctx, auto&, int step) {
        auto [p1, p2, p3] = find_triangle_points(width * scale, height * scale, r_min * scale - step, angle);
        ctx->move_to(p1.x(), p1.y());
        ctx->line_to(p2.x(), p2.y());
        ctx->line_to(p3.x(), p3.y());
        ctx->close_path();
    });
    cr->restore();

    // Draw marker
    auto mp = get_marker_point();
    Util::draw_point_indicator(cr, mp, marker_radius * 2);
    double a = luminance(getColor()) < 0.5 ? 1.0 : 0.0;

    // Draw focus ring around one of color indicators
    if (drawing_area_has_focus()) {
        // The focus_dash width & alpha(foreground_color) are from GTK3 Adwaita.
        cr->set_dash(focus_dash, 0);
        cr->set_line_width(1.0);
        cr->begin_new_path();
        if (_focus_on_ring) {
            auto c = getColor();
            c.set(1, 1.0);
            c.set(2, 1.0);
            a = luminance(c) < 0.5 ? 1.0 : 0.0;
            mp = ring_pos;
        }
        cr->set_source_rgb(a, a, a);
        cr->arc(mp.x(), mp.y(), marker_radius + focus_padding, 0, 2 * M_PI);
        cr->stroke();
    }
}

std::optional<bool> ColorWheelHSL::focus(Gtk::DirectionType const direction)
{
    // Any focus change must update focus indicators (add or remove).
    queue_drawing_area_draw();

    // In forward direction, focus passes from no focus to ring focus to triangle
    // focus to no focus.
    if (!drawing_area_has_focus()) {
        _focus_on_ring = (direction == Gtk::DirectionType::TAB_FORWARD);
        focus_drawing_area();
        return true;
    }

    // Already have focus
    bool keep_focus = true;

    switch (direction) {
        case Gtk::DirectionType::TAB_BACKWARD:
            if (!_focus_on_ring) {
                _focus_on_ring = true;
            } else {
                keep_focus = false;
            }
            break;

        case Gtk::DirectionType::TAB_FORWARD:
            if (_focus_on_ring) {
                _focus_on_ring = false;
            } else {
                keep_focus = false;
            }
    }

    return keep_focus;
}

bool ColorWheelHSL::_set_from_xy(double const x, double const y)
{
    auto const [width, height] = *_cache_size;
    double const cx = width/2.0;
    double const cy = height/2.0;

    double const r = std::min(cx, cy) * (1 - ring_width);

    // We calculate RGB value under the cursor by rotating the cursor
    // and triangle by the hue value and looking at position in the
    // now right pointing triangle.
    double angle = _values[0]  * 2 * M_PI;
    double sin = std::sin(angle);
    double cos = std::cos(angle);
    double xp =  ((x - cx) * cos - (y - cy) * sin) / r;
    double yp =  ((x - cx) * sin + (y - cy) * cos) / r;

    double xt = lerp(0.0, 1.0, -0.5, 1.0, xp);
    xt = std::clamp(xt, 0.0, 1.0);

    double dy = (1-xt) * std::cos(M_PI / 6.0);
    double yt = lerp(0.0, 1.0, -dy, dy, yp);
    yt = std::clamp(yt, 0.0, 1.0);

    ColorPoint c0(0, 0, Color(Type::RGB, {yt, yt, yt}));      // Grey point along base.
    ColorPoint c1(0, 0, Color(Type::HSV, {_values[0], 1, 1})); // Hue point at apex
    ColorPoint c = lerp(c0, c1, 0, 1, xt);
    c.color.setOpacity(_values.getOpacity()); // Remember opacity
    return setColor(c.color, false); // Don't override previous hue.
}

bool ColorWheelHSL::set_from_xy_delta(double const dx, double const dy)
{
    auto [mx, my] = get_marker_point();
    mx += dx;
    my += dy;
    return _set_from_xy(mx, my);
}

bool ColorWheelHSL::_is_in_ring(double x, double y)
{
    auto const [width, height] = *_cache_size;
    auto const cx = width  / 2.0;
    auto const cy = height / 2.0;

    auto const &[r_min, r_max] = get_radii();
    double r2_max = r_max * r_max;
    double r2_min = r_min * r_min;

    double dx = x - cx;
    double dy = y - cy;
    double r2 = dx * dx + dy * dy;

    return (r2_min < r2 && r2 < r2_max);
}

bool ColorWheelHSL::_is_in_triangle(double x, double y)
{
    auto const &[p0, p1, p2] = get_triangle_corners();
    auto const &[x0, y0] = p0.get_xy();
    auto const &[x1, y1] = p1.get_xy();
    auto const &[x2, y2] = p2.get_xy();

    double det = (x2 - x1) * (y0 - y1) - (y2 - y1) * (x0 - x1);
    double s = ((x - x1) * (y0 - y1) - (y - y1) * (x0 - x1)) / det;
    if (s < 0.0) return false;

    double t = ((x2 - x1) * (y - y1) - (y2 - y1) * (x - x1)) / det;
    return (t >= 0.0 && s + t <= 1.0);
}

void ColorWheelHSL::_update_ring_color(double x, double y)
{
    auto const [width, height] = *_cache_size;
    double cx = width / 2.0;
    double cy = height / 2.0;

    double angle = -atan2(y - cy, x - cx);
    if (angle < 0) {
        angle += 2.0 * M_PI;
    }
    angle /= 2.0 * M_PI;

    if (_values.set(0, angle)) {
        _triangle_corners.reset();
        color_changed();
    }
}

Gtk::EventSequenceState ColorWheelHSL::on_click_pressed(Gtk::GestureClick const &controller,
                                                        int /*n_press*/, double x, double y)
{
    if (_is_in_ring(x, y) ) {
        _adjusting = true;
        _mode = DragMode::HUE;
        focus_drawing_area();
        _focus_on_ring = true;
        _update_ring_color(x, y);
        return Gtk::EventSequenceState::CLAIMED;
    } else if (_is_in_triangle(x, y)) {
        _adjusting = true;
        _mode = DragMode::SATURATION_VALUE;
        focus_drawing_area();
        _focus_on_ring = false;
        _set_from_xy(x, y);
        return Gtk::EventSequenceState::CLAIMED;
    }

    return Gtk::EventSequenceState::NONE;
}

Gtk::EventSequenceState ColorWheelHSL::on_click_released(int /*n_press*/, double /*x*/, double /*y*/)
{
    _mode = DragMode::NONE;
    _adjusting = false;
    return Gtk::EventSequenceState::CLAIMED;
}

void ColorWheelHSL::on_motion(Gtk::EventControllerMotion const &motion, double x, double y)
{
    if (!_adjusting) return;

    if (_mode == DragMode::HUE) {
        _update_ring_color(x, y);
    } else if (_mode == DragMode::SATURATION_VALUE) {
        _set_from_xy(x, y);
    }
}

bool ColorWheelHSL::on_key_pressed(unsigned keyval, unsigned /*keycode*/, Gdk::ModifierType state)
{
    static constexpr double delta_hue = 2.0 / MAX_HUE;
    auto dx = 0.0, dy = 0.0;

    switch (keyval) {
        case GDK_KEY_Up:
        case GDK_KEY_KP_Up:
            dy = -1.0;
            break;

        case GDK_KEY_Down:
        case GDK_KEY_KP_Down:
            dy = +1.0;
            break;

        case GDK_KEY_Left:
        case GDK_KEY_KP_Left:
            dx = -1.0;
            break;

        case GDK_KEY_Right:
        case GDK_KEY_KP_Right:
            dx = +1.0;
            break;
    }

    if (dx == 0.0 && dy == 0.0) return false;

    bool changed = false;
    if (_focus_on_ring) {
        changed = _values.set(0, _values[0] - ((dx != 0 ? dx : dy) * delta_hue));
    } else {
        changed = set_from_xy_delta(dx, dy);
    }

    _values.normalize();

    if (changed) {
        _triangle_corners.reset();
        color_changed();
    }

    return changed;
}

ColorWheelHSL::MinMax const &ColorWheelHSL::get_radii()
{
    if (_radii) return *_radii;

    // Force calc others, too.
    _triangle_corners.reset();

    _radii.emplace();
    auto &[r_min, r_max] = *_radii;
    auto const [width, height] = *_cache_size;
    r_max = std::round(std::min(width, height) / 2.0 - focus_line_width);
    r_min = std::round(r_max * (1.0 - ring_width));
    return *_radii;
}

std::array<ColorPoint, 3> const &ColorWheelHSL::get_triangle_corners()
{
    if (_triangle_corners) return *_triangle_corners;

    auto const [width, height] = *_cache_size;
    double const cx = width  / 2.0;
    double const cy = height / 2.0;

    auto const &[r_min, r_max] = get_radii();
    double angle = _values[0] * 2.0 * M_PI;
    auto const add2 = 2.0 * M_PI / 3.0;
    auto const angle2 = angle  + add2;
    auto const angle4 = angle2 + add2;

    // Force calc this too
    _marker_point.reset();

    _triangle_corners.emplace();
    auto &[p0, p1, p2] = *_triangle_corners;
    auto const x0 = cx + std::cos(angle ) * r_min;
    auto const y0 = cy - std::sin(angle ) * r_min;
    auto const x1 = cx + std::cos(angle2) * r_min;
    auto const y1 = cy - std::sin(angle2) * r_min;
    auto const x2 = cx + std::cos(angle4) * r_min;
    auto const y2 = cy - std::sin(angle4) * r_min;
    p0 = {x0, y0, Color(Type::HSV, {_values[0], 1.0, 1.0})};
    p1 = {x1, y1, Color(Type::HSV, {_values[0], 1.0, 0.0})};
    p2 = {x2, y2, Color(Type::HSV, {_values[0], 0.0, 1.0})};
    return *_triangle_corners;
}

Geom::Point const &ColorWheelHSL::get_marker_point()
{
    if (_marker_point) return *_marker_point;

    auto const &[p0, p1, p2] = get_triangle_corners();
    auto const &[x0, y0] = p0.get_xy();
    auto const &[x1, y1] = p1.get_xy();
    auto const &[x2, y2] = p2.get_xy();

    _marker_point.emplace();
    auto &[mx, my] = *_marker_point;
    auto const v1v2 = _values[1] * _values[2];
    mx = x1 + (x2 - x1) * _values[2] + (x0 - x2) * v1v2;
    my = y1 + (y2 - y1) * _values[2] + (y0 - y2) * v1v2;
    return *_marker_point;
}

ColorWheelHSL::ColorWheelHSL()
    : Glib::ObjectBase{"ColorWheelHSL"}
    , WidgetVfuncsClassInit{}
    // All the calculations are based on HSV, not HSL
    , ColorWheelBase(Type::HSV, {0, 0, 0, 1})
{}

ColorWheelHSL::ColorWheelHSL(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder) :
    ColorWheelBase(cobject, builder, Type::HSV, {0, 0, 0, 1}) {}

/* HSLuv Color Wheel */

ColorWheelHSLuv::ColorWheelHSLuv()
    : ColorWheelBase(Type::HSLUV, {0, 1, 0.5, 1})
{
    _picker_geometry = std::make_unique<PickerGeometry>();
}

ColorWheelHSLuv::ColorWheelHSLuv(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder) :
    ColorWheelBase(cobject, builder, Type::HSLUV, {0, 1, 0.5, 1}) {

    _picker_geometry = std::make_unique<PickerGeometry>();
}

bool ColorWheelHSLuv::setColor(Color const &color,
                             bool /*overrideHue*/, bool const emit)
{
    if (_values.set(color, true)) {
        assert(_values.getSpace()->getType() == Type::HSLUV);
        updateGeometry();
        _scale = OUTER_CIRCLE_RADIUS / _picker_geometry->outer_circle_radius;
        _updatePolygon();
        if (emit)
            color_changed();
        else
            queue_drawing_area_draw();
        return true;
    }
    return false;
}

/* Multi-marker Color Wheel */

bool MultiMarkerWheel::setColor(Color const &, bool, bool)
{
    // Doesn't make sense to set the colour for a multi-colour wheel.
    return false;
}

/**
 * it takes a vector of colors then clears the current _values_vector and _markers_points
 * and resets the color wheel then repopulates it with the new colors
 * then emit the color changed signal 
 */
void MultiMarkerWheel::setColors(std::vector<Colors::Color> colors)
{
    _values_vector = std::move(colors);
    for (auto &col : _values_vector) {
        col.convert(Colors::Space::Type::HSV);
    }
    _source_wheel.reset();
    _markers_points.clear();
    _markers_points.resize(_values_vector.size());
    _active_index = _values_vector.empty() ? -1 : 0;
    color_changed();
}

/**
 * takes cairo context , color value and index of the color
 * get the center of the colorwheel by dividing width and height by 2
 * get the center of the marker by passing the index to get_marker_point function
 * then choose the color of the marker (black or white) based on its luminance
 * then start start drawing with cairo , if the index == hover index just make the radius bigger
 * then if the marker has foucs add focus dash to it 
 */
void MultiMarkerWheel::_draw_marker(Cairo::RefPtr<Cairo::Context> const &cr, Colors::Color const &value, int index)
{
    auto const [width, height] = *_cache_size;
    auto const cx = width / 2.0;
    auto const cy = height / 2.0;
    auto const &[mx, my] = get_marker_point(index);

   _draw_line_to_marker(cr,mx,my,cx,cy,value,index);

    auto color_on_wheel = Color(Type::HSV, {value[0], 1.0, 1.0});
    double a = luminance(color_on_wheel) < 0.5 ? 1.0 : 0.0;
    if (index == _active_index) {
        cr->set_source_rgb(0.2588, 0.5216, 0.9255);
    } else {
        cr->set_source_rgb(a, a, a);
    }
    cr->set_dash(std::valarray<double>(), 0);
    cr->begin_new_path();
    if (index == _hover_index) {
        cr->arc(mx, my, marker_radius+2, 0, 2 * M_PI);
    } else {
        cr->arc(mx, my, marker_radius, 0, 2 * M_PI);
    }
    cr->stroke();

    // Draw focus
    if (drawing_area_has_focus()) {
        // The focus_dash width & alpha(foreground_color) are from GTK3 Adwaita.
        if (index == _active_index) {
            cr->set_dash(focus_dash, 0);
            cr->set_line_width(1.0);
            cr->set_source_rgb(1 - a, 1 - a, 1 - a);
            cr->begin_new_path();
            cr->arc(mx, my, marker_radius + focus_padding, 0, 2 * M_PI);
        }

        cr->stroke();
    }
}

/**
 * try to get marker index from the input position (x,y)
 * by getting the distance between the marker center and the point (x,y)
 * if it is less than marker_radius + marker_click_tolerance it means that 
 * the point is inside the marker area then return its index
 * if not found return -1
 */
int MultiMarkerWheel::_get_marker_index(Geom::Point const &p)
{
    for (int i = 0; i < _values_vector.size(); i++) {
        auto m = get_marker_point(i);
        if (Geom::distance(p, m) <= marker_radius + marker_click_tolerance) {
            return i;
        }
    }
    return -1;
}

/**
 * If hue lock is enabled, this function calculates how far each marker's hue
 * is from the active marker's hue.
 *
 * Because hue is a circle (0.0 and 1.0 are the same color), the raw difference
 * can sometimes look too big (e.g. 0.9 - 0.1 = 0.8), even though the real
 * distance around the circle is much smaller (0.2).
 *
 * To fix this, the delta is adjusted:
 *   - If delta > 0.5  → subtract 1.0
 *   - If delta < -0.5 → add 1.0
 *
 * This makes sure the difference is always the shortest distance on the color wheel,
 * inside the range [-0.5, +0.5].
 */
void MultiMarkerWheel::_update_hue_lock_positions()
{
    if (!_hue_lock) {
        return;
    }

    std::vector<double> delta_angles;
    auto active_hue = _values_vector[_active_index][0];
    for (int i = 0; i < _values_vector.size(); i++) {
        if (i == _active_index) {
            delta_angles.push_back(0.0);
            continue;
        }
        auto delta_hue = _values_vector[i][0] - active_hue;
        if (delta_hue > 0.5) delta_hue -= 1.0;
        if (delta_hue < -0.5) delta_hue += 1.0;
        delta_angles.push_back(delta_hue);
    }

    _relative_hue_angles = delta_angles;
}

/**
 * function to draw line to the begining of the marker by calculating the distance from the wheel center
 * to marker center
 * normalize the differences between centers by dividing them by the length of the line
 * to be a unit vector between [-1,1]
 * then calculate the end points ty,tx by subtracting the marker radius multiplied by the direction of the vector 
 * from the marker center , calculate the luminance of the line
 * move the cairo context tot the center of the colorwheel by converting the polar coordinates to cartisain ones
 * then draw the line to point (tx,ty)
 */
void MultiMarkerWheel::_draw_line_to_marker(Cairo::RefPtr<Cairo::Context> const &cr, double mx, double my, double cx,
                                           double cy, Colors::Color const &value, int index)
{
    auto const &[r_min, r_max] = get_radii(); 
    auto color_on_wheel = Color(Type::HSV, {value[0], 1.0, 1.0});
    double dy = my-cy;
    double dx = mx-cx;
    double len = std::sqrt(dx*dx+dy*dy); 
    if (len > 1e-5) {
        dx /= len;
        dy /= len;
    }
    double mr = index == _hover_index ? marker_radius+2 : marker_radius; // bigger radius for on hover effect
    double tx = mx - dx * mr;
    double ty = my - dy * mr;
    double l = luminance(color_on_wheel) < 0.5 ? 1.0 : 0.0;
    cr->save();
    cr->set_source_rgb(l, l, l);
    cr->move_to(cx + cos(value[0] * M_PI * 2.0) * r_min,
                cy - sin(value[0] * M_PI * 2.0) * r_min); // x = r*cos(angel) , y = r*sin(angel)
    // adding cx and subtracting cy to start from wheel center not the origin (0,0)
    cr->line_to(tx,ty);
    if (index != _active_index && !_hue_lock) {
        cr->set_dash(focus_dash, 0);
        cr->set_line_width(1.0);
    } else if (!_hue_lock) {
        auto const dash = std::vector{3.0}; // wider dashes for focused line 
        cr->set_dash(dash,0);
        cr->set_line_width(2.0);
    } else {
        cr->set_dash(std::valarray<double>(), 0);
        if (index == _active_index) {
            cr->set_line_width(3.0);
        }
    }
    cr->stroke();
    cr->restore();
}

/**
 * draw the colorwheel pixel by pixel
 */
void MultiMarkerWheel::update_wheel_source()
{
    if (_radii && _source_wheel) {
        return;
    }

    auto const [width, height] = *_cache_size;
    auto const cx = width / 2.0;
    auto const cy = height / 2.0;

    auto const stride = Cairo::ImageSurface::format_stride_for_width(Cairo::Surface::Format::RGB24, width);
    _source_wheel.reset();
    _buffer_wheel.resize(height * stride / 4);

    auto const &[r_min, r_max] = get_radii();
    double r2_max = (r_max + 2) * (r_max + 2); // Must expand a bit to avoid edge effects.
    double r2_min = (r_min - 2) * (r_min - 2); // Must shrink a bit to avoid edge effects.

    for (int i = 0; i < height; ++i) {
        auto p = _buffer_wheel.data() + i * width;
        double dy = (cy - i);
        for (int j = 0; j < width; ++j) {
            double dx = (j - cx);
            double r2 = dx * dx + dy * dy;
            if (r2 < r2_min || r2 > r2_max) {
                *p++ = 0; // Save calculation time.
            } else {
                double angle = atan2(dy, dx);
                if (angle < 0.0) {
                    angle += 2.0 * M_PI;
                }
                double hue = angle / (2.0 * M_PI);

                double saturation = sqrt(r2)/r_max;
                saturation = std::clamp(saturation,0.0,1.0);
                // double value = 1.0 - ((dy+(height/2.0))/height);
                // value = std::clamp(value,0.0,1.0);

                *p++ = Color(Type::HSV, {hue, saturation,lightness}).toARGB();
            }
        }
    }

    auto const data = reinterpret_cast<unsigned char *>(_buffer_wheel.data());
    _source_wheel = Cairo::ImageSurface::create(data, Cairo::Surface::Format::RGB24, width, height, stride);
}

/**
 * takes index of the requested changed color and the new color
 * change it in the _values_vector and reset its marker and emit color changed signal to update the widget
 * and return true if succeeded
 * used to sync wheel's colors if the color chnaged from the colorlist
 */
bool MultiMarkerWheel::changeColor(int index, Colors::Color const &color)
{
    if (index < 0 || index >= _values_vector.size()) {
        return false;
    }

    if (_values_vector[index].set(color, true)) {
        _markers_points[index].reset();
        color_changed();
        return true;
    }

    return false;
}

/**
 * set lightness for all colors in the wheel when hue lock is on
 * if it is off just changed lightness for the active color
 */
void MultiMarkerWheel::setLightness(double value) 
{
    lightness = value / 100.0;
    _source_wheel.reset();
    if (_hue_lock) {
        for (size_t i = 0; i < _values_vector.size(); i++) {
            _values_vector[i].set(2, lightness);
            if (i < _markers_points.size()) {
                _markers_points[i].reset();
            }
        }
        color_changed();
    } else {
        int index = getActiveIndex();
        if (index > -1) {
            _values_vector[index].set(2, lightness);
            _markers_points[index].reset();
            color_changed();
        }
    }
}

/**
 * set saturation for all colors in the wheel when hue lock is on
 * if it is off just change saturation for the active color
 */
void MultiMarkerWheel::setSaturation(double value) 
{
    saturation = value / 100.0;
    if (_hue_lock) {
        for (size_t i = 0; i < _values_vector.size(); i++) {
            _values_vector[i].set(1, saturation);
            if (i < _markers_points.size()) {
                _markers_points[i].reset();
            }
        }
        color_changed();
    } else {
        int index = getActiveIndex();
        if (index >- 1) {
            _values_vector[index].set(1, saturation);
            _markers_points[index].reset();
            color_changed();
        }
    }
}

void MultiMarkerWheel::on_drawing_area_size(int width, int height, int baseline)
{
    auto const size = Geom::IntPoint{width, height};
    if (size == _cache_size) {
        return;
    }
    _cache_size = size;
    _radii.reset();
    _source_wheel.reset();
}

/**
 * main function for drawing the whole wheel and markers and lines
 */
void MultiMarkerWheel::on_drawing_area_draw(Cairo::RefPtr<Cairo::Context> const &cr, int, int)
{

    auto const [width, height] = *_cache_size;
    auto const cx = width / 2.0;
    auto const cy = height / 2.0;

    cr->set_antialias(Cairo::ANTIALIAS_SUBPIXEL);

    // Update caches
    update_wheel_source();
    auto const &[r_min, r_max] = get_radii();

    // Paint with ring surface, clipping to ring.
    cr->save();
    cr->set_source(_source_wheel, 0, 0);
    cr->set_line_width(r_max - r_min);
    cr->begin_new_path();
    cr->arc(cx, cy, (r_max + r_min) / 2.0, 0, 2.0 * M_PI);
    cr->stroke();
    cr->restore();
    // Paint line to markers and markers
    if (_markers_points.size() != _values_vector.size()) {
        _markers_points.resize(_values_vector.size());
    }

    for (int i = 0; i < _values_vector.size(); i++) {
        _draw_marker(cr, _values_vector[i], i);
    }
}

std::optional<bool> MultiMarkerWheel::focus(Gtk::DirectionType const direction)
{
    // Any focus change must update focus indicators (add or remove).
    queue_drawing_area_draw();

    // In forward direction, focus passes from no focus to ring focus to triangle
    // focus to no focus.
    if (!drawing_area_has_focus()) {
        _focus_on_wheel = (direction == Gtk::DirectionType::TAB_FORWARD);
        focus_drawing_area();
        return true;
    }

    // Already have focus
    bool keep_focus = true;

    switch (direction) {
        case Gtk::DirectionType::TAB_BACKWARD:
            if (!_focus_on_wheel) {
                _focus_on_wheel = true;
            } else {
                keep_focus = false;
            }
            break;

        case Gtk::DirectionType::TAB_FORWARD:
            if (_focus_on_wheel) {
                _focus_on_wheel = false;
            } else {
                keep_focus = false;
            }
    }

    return keep_focus;
}

/**
 * checks whether the point is inside the wheel or not
 * by checking if the distance is less than the wheel radius
 */
bool MultiMarkerWheel::_is_in_wheel(double x, double y)
{
    // std::cout<<"x: "<<x<<" y: "<<y<<std::endl;
    auto const [width, height] = *_cache_size;
    // std::cout<<"w: "<<width<<" h: "<<height<<std::endl;
    auto const cx = width / 2.0;
    auto const cy = height / 2.0;

    auto const &[r_min, r_max] = get_radii();
    double r2_max = r_max * r_max;

    double dx = x - cx;
    double dy = y - cy;
    double r2 = dx * dx + dy * dy;

    return r2 < r2_max;
}

/**
 * used to update colors when markers pressed or moves by
 * calculating the angle of the line from the point to the wheel center to calculate the new hue value
 * if angle less than 0 normalize it then flip it to follow the mouse movement by subtracting it from 1
 * calculate the distance from the point to the center of the wheel to get the saturation value
 * then set them in the _values_vector[index] reset its old marker and emit color changed signal
 */
void MultiMarkerWheel::_update_wheel_color(double x, double y, int index)
{
    auto const [width, height] = *_cache_size;
    double cx = width / 2.0;
    double cy = height / 2.0;

    double angle = std::atan2(y - cy, x - cx);
    if (angle < 0) {
        angle += 2.0 * M_PI;
    }
    angle = 1.0 - angle / (2.0 * M_PI);
    double dx = x-cx;
    double dy = y-cy;
    double distance = std::sqrt(dx*dx+dy*dy);
    auto const &[r_min, r_max] = get_radii();
    double saturation = distance/r_max;
    saturation = std::clamp(saturation,0.0,1.0);

    bool changed = false;

    if (_values_vector[index].set(0, angle)) {
        changed = true;
    }

    if (_values_vector[index].set(1, saturation)) {
        changed = true;
    }

    if (_values_vector[index].set(2, lightness)) {
        changed = true;
    }

    if (changed) {
        _markers_points[index].reset();
        color_changed();
    }
}

/**
 * signal handler that checks if clicked point is inside the wheel
 * then search for the marker pressed to make it active
 * check for hue lock by calling _update_hue_lock_positions()
 * updates the clicked color to new pressed point
 */
Gtk::EventSequenceState MultiMarkerWheel::on_click_pressed(Gtk::GestureClick const &controller, int /*n_press*/,
                                                           double x, double y)
{
    if (_is_in_wheel(x, y)) {
        _adjusting = true;
        _mode = DragMode::HUE;
        focus_drawing_area();
        _focus_on_wheel = true;
        int index = _get_marker_index({x, y});
        if (index >= 0) {
            _active_index = index;
        }
        _update_hue_lock_positions();
        if (_active_index >= 0 && _active_index < _values_vector.size()) {
            _update_wheel_color(x, y, _active_index);
        }
        return Gtk::EventSequenceState::CLAIMED;
    }

    return Gtk::EventSequenceState::NONE;
}

Gtk::EventSequenceState MultiMarkerWheel::on_click_released(int /*n_press*/, double /*x*/, double /*y*/)
{
    _mode = DragMode::NONE;
    _adjusting = false;
    return Gtk::EventSequenceState::CLAIMED;
}

/**
 * if not adusting a marker
 * it detects if the point is on or near some marker gets its index and emits _signal_color_hovered
 * so the marker gets redrawed with a bigger radius and call for any action related to hover 
 * (e.g. highlighting objects that has the hovered marker color)
 * -1 _hover_index to cancel the hover effect when start moving the marker
 * also checks for the _hue_lock to change reset of the markers accordingly if it is on
 */
void MultiMarkerWheel::on_motion(Gtk::EventControllerMotion const &motion, double x, double y)
{
    if (!_adjusting) {
        int hover_index = _get_marker_index({x, y});
        _signal_color_hovered.emit();
        if (_hover_index != hover_index) {
            _hover_index = hover_index;
            if (hover_index >= 0 && hover_index < _values_vector.size()) {
                queue_drawing_area_draw();
            }
        }
        return;
    }
    auto state = motion.get_current_event_state();
    if (!Controller::has_flag(state, Gdk::ModifierType::BUTTON1_MASK)) {
        // lost button release event
        _mode = DragMode::NONE;
        _adjusting = false;
        return;
    }

    if (_mode == DragMode::HUE || _mode == DragMode::SATURATION_VALUE) {
        _hover_index = -1;
        _signal_color_hovered.emit();
        if (_active_index >= 0 && _active_index < _values_vector.size()) {
            _update_wheel_color(x, y, _active_index);
        }
        if (_hue_lock && !_relative_hue_angles.empty()) {
            bool changed = false;
            double hue = _values_vector[_active_index][0];
            for (int i = 0; i < _values_vector.size(); i++) {
                if (i != _active_index) {
                    double new_hue = hue + _relative_hue_angles[i];
                    new_hue = fmod(new_hue + 1.0, 1.0);
                    if (_values_vector[i].set(0, new_hue)) {
                        _markers_points[i].reset();
                        changed = true;
                    }
                }
            }
            if (changed) {
                color_changed();
            }
        }
    }
}

/**
 * signal handler function that handels keyboard adjustments to the wheel on hue and saturation values
 * same as the one in ColorWheelHSL too
 */
bool MultiMarkerWheel::on_key_pressed(unsigned keyval, unsigned /*keycode*/, Gdk::ModifierType state)
{
    static constexpr double delta_hue = 2.0 / MAX_HUE;
    static constexpr double delta_sat = 2.0 / MAX_SATURATION;
    auto dx = 0.0, dy = 0.0;

    switch (keyval) {
        case GDK_KEY_Up:
        case GDK_KEY_KP_Up:
            dy = -1.0;
            break;

        case GDK_KEY_Down:
        case GDK_KEY_KP_Down:
            dy = +1.0;
            break;

        case GDK_KEY_Left:
        case GDK_KEY_KP_Left:
            dx = -1.0;
            break;

        case GDK_KEY_Right:
        case GDK_KEY_KP_Right:
            dx = +1.0;
    }

    if (dx == 0.0 && dy == 0.0) {
        return false;
    }

    bool changed = false;
    if (_focus_on_wheel) {
        changed = _values_vector[_active_index].set(0, _values_vector[_active_index][0] - ((dx != 0 ? dx : dy) * delta_hue));
        changed = _values_vector[_active_index].set(1, _values_vector[_active_index][1] - ((dy != 0 ? dy : dx) * delta_sat));
    }
   
    _values_vector[_active_index].normalize();

    if (changed) {
        _markers_points[_active_index].reset();
        color_changed();
    }

    return changed;
}

/**
 * that is the same as get radii in the ColorWheelHSL i didn't change any thing even
 * though it has only one radius (r_max = r_min) now as a whole circle not a ring but i dare not change a working function
 * (and i am lazy too)
 */
MultiMarkerWheel::MinMax const &MultiMarkerWheel::get_radii()
{
    if (_radii) {
        return *_radii;
    }

    _radii.emplace();
    auto &[r_min, r_max] = *_radii;
    auto const [width, height] = *_cache_size;
    r_max = std::min(width, height) / 2.0 - 2 * (focus_line_width + focus_padding);
    r_min = r_max * (1.0 - _wheel_width);
    return *_radii;
}

/**
 * if the marker isn't in _markers_points it calculates the marker position
 * by the hue angle and saturation as the distance from the center to the desired color
 */
Geom::Point MultiMarkerWheel::get_marker_point(int index)
{
    if (index < 0 || index >= _values_vector.size()) {
        return {};
    }

    if (index >= _markers_points.size()) {
        _markers_points.resize(_values_vector.size());
    }

    if (_markers_points[index]) {
        return *_markers_points[index];
    }

    auto const [width, height] = *_cache_size;
    auto const cx = width / 2.0;
    auto const cy = height / 2.0;
    auto const&[r_min,r_max] = get_radii();
    double hue = _values_vector[index][0];
    double saturation = _values_vector[index][1];
    double angle = (1.0 - hue) * 2 * M_PI;
    _markers_points[index].emplace();
    auto &[mx, my] = *_markers_points[index];
    mx = cx + r_max * saturation * cos(angle); // polar cooordinates to cartesian coordinates calculation
    my = cy + r_max * saturation * sin(angle);
    return *_markers_points[index];
}

MultiMarkerWheel::MultiMarkerWheel()
    : Glib::ObjectBase{"MultiMarkerWheel"}
    , WidgetVfuncsClassInit{} // All the calculations are based on HSV, not HSL
    , ColorWheelBase(Type::HSV, {0.5, 0.2, 0.7, 1}) // redundant values nothing important 
{}

MultiMarkerWheel::MultiMarkerWheel(BaseObjectType *cobject, Glib::RefPtr<Gtk::Builder> const &builder)
    : ColorWheelBase(cobject, builder, Type::HSV, {0.5, 0.2, 0.7, 1}) // redundant values nothing important 
{}

/**
 * Update the PickerGeometry structure owned by the instance.
 */
void ColorWheelHSLuv::updateGeometry()
{
    // Separate from the extremes to avoid overlapping intersections
    double lightness = std::clamp((_values[2] * 100) + 0.01, 0.1, 99.9);

    // Find the lines bounding the gamut polygon
    auto const lines = Space::HSLuv::get_bounds(lightness);

    // Find the line closest to origin
    Geom::Line const *closest_line = nullptr;
    double closest_distance = -1;

    for (auto const &line : lines) {
        double d = Geom::distance(Geom::Point(0, 0), line);
        if (closest_distance < 0 || d < closest_distance) {
            closest_distance = d;
            closest_line = &line;
        }
    }

    g_assert(closest_line);
    auto const nearest_time = closest_line->nearestTime(Geom::Point(0, 0));
    Geom::Angle start_angle{closest_line->pointAt(nearest_time)};

    constexpr auto num_lines = 6;
    constexpr auto max_intersections = num_lines * (num_lines - 1) / 2;
    std::vector<Intersection> intersections;
    intersections.reserve(max_intersections);

    for (int i = 0; i < num_lines - 1; i++) {
        for (int j = i + 1; j < num_lines; j++) {
            auto xings = lines[i].intersect(lines[j]);
            if (xings.empty()) {
                continue;
            }
            intersections.emplace_back(i, j, xings.front().point(), start_angle);
        }
    }

    std::sort(intersections.begin(), intersections.end(), [](Intersection const &lhs, Intersection const &rhs) {
        return lhs.relative_angle.radians0() >= rhs.relative_angle.radians0();
    });

    // Find the relevant vertices of the polygon, in the counter-clockwise order.
    std::vector<Geom::Point> ordered_vertices;
    ordered_vertices.reserve(intersections.size());
    double circumradius = 0.0;
    unsigned current_index = closest_line - &lines[0];

    for (auto const &intersection : intersections) {
        if (intersection.line1 == current_index) {
            current_index = intersection.line2;
        } else if (intersection.line2 == current_index) {
            current_index = intersection.line1;
        } else {
            continue;
        }
        ordered_vertices.emplace_back(intersection.point);
        circumradius = std::max(circumradius, intersection.point.length());
    }

    _picker_geometry->vertices = std::move(ordered_vertices);
    _picker_geometry->outer_circle_radius = circumradius;
    _picker_geometry->inner_circle_radius = closest_distance;
}

static Geom::IntPoint _getMargin(Gtk::Allocation const &allocation)
{
    int const width = allocation.get_width();
    int const height = allocation.get_height();

    return {std::max(0, (width - height) / 2),
            std::max(0, (height - width) / 2)};
}

inline static Geom::IntPoint _getAllocationDimensions(Gtk::Allocation const &allocation)
{
    return {allocation.get_width(), allocation.get_height()};
}

inline static int _getAllocationSize(Gtk::Allocation const &allocation)
{
    return std::min(allocation.get_width(), allocation.get_height());
}

/// Detect whether we're at the top or bottom vertex of the color space.
bool ColorWheelHSLuv::_vertex() const
{
    return _values[2] < VERTEX_EPSILON || _values[2] > 1.0 - VERTEX_EPSILON;
}

void ColorWheelHSLuv::on_drawing_area_draw(::Cairo::RefPtr<::Cairo::Context> const &cr, int, int)
{
    auto const &allocation = get_drawing_area_allocation();
    auto dimensions = _getAllocationDimensions(allocation);
    auto center = (0.5 * (Geom::Point)dimensions).floor();

    auto size = _getAllocationSize(allocation);
    double const resize = size / static_cast<double>(SIZE);

    auto const margin = _getMargin(allocation);
    auto polygon_vertices_px = to_pixel_coordinate(_picker_geometry->vertices, _scale, resize);
    for (auto &point : polygon_vertices_px) {
        point += margin;
    }

    bool const is_vertex = _vertex();
    cr->set_antialias(Cairo::ANTIALIAS_SUBPIXEL);

    if (size > _square_size && !polygon_vertices_px.empty()) {
        if (_cache_size != dimensions) {
            _updatePolygon();
        }
        if (!is_vertex) {
            // Paint with surface, clipping to polygon
            cr->save();
            cr->set_source(_surface_polygon, 0, 0);
            auto it = polygon_vertices_px.begin();
            cr->move_to((*it)[Geom::X], (*it)[Geom::Y]);
            for (++it; it != polygon_vertices_px.end(); ++it) {
                cr->line_to((*it)[Geom::X], (*it)[Geom::Y]);
            }
            cr->close_path();
            cr->fill();
            cr->restore();
        }
    }

    // Draw foreground

    // Outer circle
    std::vector<double> dashes{OUTER_CIRCLE_DASH_SIZE};
    cr->set_line_width(1);
    // White dashes
    cr->set_source_rgb(1.0, 1.0, 1.0);
    cr->set_dash(dashes, 0.0);
    cr->begin_new_path();
    cr->arc(center[Geom::X], center[Geom::Y], _scale * resize * _picker_geometry->outer_circle_radius, 0, 2 * M_PI);
    cr->stroke();
    // Black dashes
    cr->set_source_rgb(0.0, 0.0, 0.0);
    cr->set_dash(dashes, OUTER_CIRCLE_DASH_SIZE);
    cr->begin_new_path();
    cr->arc(center[Geom::X], center[Geom::Y], _scale * resize * _picker_geometry->outer_circle_radius, 0, 2 * M_PI);
    cr->stroke();
    cr->unset_dash();

    // Contrast
    auto [gray, alpha] = get_contrasting_color(perceptual_lightness(_values[2]));
    cr->set_source_rgba(gray, gray, gray, alpha);

    // Draw inscribed circle
    double const inner_stroke_width = 2.0;
    double inner_radius = is_vertex ? 0.01 : _picker_geometry->inner_circle_radius;
    cr->set_line_width(inner_stroke_width);
    cr->begin_new_path();
    cr->arc(center[Geom::X], center[Geom::Y], _scale * resize * inner_radius, 0, 2 * M_PI);
    cr->stroke();

    // Center
    cr->begin_new_path();
    cr->arc(center[Geom::X], center[Geom::Y], 2, 0, 2 * M_PI);
    cr->fill();

    // Draw marker
    auto luv = Luv::toCoordinates(_values.converted(Type::LUV)->getValues());
    auto mp = to_pixel_coordinate({luv[1], luv[2]}, _scale, resize) + margin;

    cr->set_line_width(inner_stroke_width);
    cr->begin_new_path();
    cr->arc(mp[Geom::X], mp[Geom::Y], marker_radius, 0, 2 * M_PI);
    cr->stroke();

    // Focus
    if (drawing_area_has_focus()) {
        cr->set_dash(focus_dash, 0);
        cr->set_line_width(focus_line_width);
        cr->set_source_rgb(1 - gray, 1 - gray, 1 - gray);
        cr->begin_new_path();
        cr->arc(mp[Geom::X], mp[Geom::Y], marker_radius + focus_padding, 0, 2 * M_PI);
        cr->stroke();
    }
}

bool ColorWheelHSLuv::_set_from_xy(double const x, double const y)
{
    auto const allocation = get_drawing_area_allocation();
    int const width = allocation.get_width();
    int const height = allocation.get_height();

    double const resize = std::min(width, height) / static_cast<double>(SIZE);
    auto const p = from_pixel_coordinate(Geom::Point(x, y) - _getMargin(allocation), _scale, resize);

    if (_values.set(Color(Type::LUV, Luv::fromCoordinates({_values[2] * 100, p[Geom::X], p[Geom::Y]})), true)) {
        color_changed();
        return true;
    }
    return false;
}

void ColorWheelHSLuv::_updatePolygon()
{
    auto const allocation = get_drawing_area_allocation();
    auto allocation_size = _getAllocationDimensions(allocation);
    int const size = std::min(allocation_size[Geom::X], allocation_size[Geom::Y]);

    // Update square size
    _square_size = std::max(1, static_cast<int>(size / 50));
    if (size < _square_size) {
        return;
    }

    _cache_size = allocation_size;

    double const resize = size / static_cast<double>(SIZE);

    auto const margin = _getMargin(allocation);
    auto polygon_vertices_px = to_pixel_coordinate(_picker_geometry->vertices, _scale, resize);

    // Find the bounding rectangle containing all points (adjusted by the margin).
    Geom::Rect bounding_rect;
    for (auto const &point : polygon_vertices_px) {
        bounding_rect.expandTo(point + margin);
    }
    bounding_rect *= Geom::Scale(1.0 / _square_size);

    // Round to integer pixel coords
    auto const bounding_max = bounding_rect.max().ceil();
    auto const bounding_min = bounding_rect.min().floor();

    int const stride = Cairo::ImageSurface::format_stride_for_width(Cairo::Surface::Format::RGB24, _cache_size.x());

    _surface_polygon.reset();
    _buffer_polygon.resize(_cache_size.y() * stride / 4);
    std::vector<guint32> buffer_line(stride / 4);

    auto const square_center = Geom::IntPoint(_square_size / 2, _square_size / 2);
    std::vector<double> color_vals = {_values[2] * 100, 0, 0};

    // Set the color of each pixel/square
    for (int y = bounding_min[Geom::Y]; y < bounding_max[Geom::Y]; y++) {
        for (int x = bounding_min[Geom::X]; x < bounding_max[Geom::X]; x++) {
            auto pos = Geom::IntPoint(x * _square_size, y * _square_size);
            auto point = from_pixel_coordinate(pos + square_center - margin, _scale, resize);
            color_vals[1] = point[Geom::X];
            color_vals[2] = point[Geom::Y];

            auto color = Color(Type::LUV, Luv::fromCoordinates(color_vals));
            guint32 *p = buffer_line.data() + (x * _square_size);
            for (int i = 0; i < _square_size; i++) {
                p[i] = color.toARGB();
            }
        }

        // Copy the line buffer to the surface buffer
        int const scaled_y = y * _square_size;
        for (int i = 0; i < _square_size; i++) {
            guint32 *t = _buffer_polygon.data() + (scaled_y + i) * (stride / 4);
            std::memcpy(t, buffer_line.data(), stride);
        }
    }

    _surface_polygon = ::Cairo::ImageSurface::create(reinterpret_cast<unsigned char *>(_buffer_polygon.data()),
                                                     Cairo::Surface::Format::RGB24, _cache_size.x(), _cache_size.y(), stride);
}

Gtk::EventSequenceState ColorWheelHSLuv::on_click_pressed(Gtk::GestureClick const &,
                                                          int /*n_press*/, double x, double y)
{
    auto const event_pt = Geom::Point(x, y);
    auto const allocation = get_drawing_area_allocation();
    int const size = _getAllocationSize(allocation);
    auto const region = Geom::IntRect::from_xywh(_getMargin(allocation), {size, size});

    if (region.contains(event_pt.round())) {
        _adjusting = true;
        focus_drawing_area();
        _setFromPoint(event_pt);
        return Gtk::EventSequenceState::CLAIMED;
    }

    return Gtk::EventSequenceState::NONE;
}

Gtk::EventSequenceState ColorWheelHSLuv::on_click_released(int /*n_press*/, double /*x*/, double /*y*/)
{
    _adjusting = false;
    return Gtk::EventSequenceState::CLAIMED;
}

void ColorWheelHSLuv::on_motion(Gtk::EventControllerMotion const &/*motion*/,
                                double x, double y)
{
    if (_adjusting) {
        _set_from_xy(x, y);
    }
}

bool ColorWheelHSLuv::on_key_pressed(unsigned keyval, unsigned /*keycode*/, Gdk::ModifierType state)
{
    bool consumed = false;

    // Get current point
    auto luv = *_values.converted(Type::LUV);

    double const marker_move = 1.0 / _scale;

    switch (keyval) {
        case GDK_KEY_Up:
        case GDK_KEY_KP_Up:
            luv.set(2, luv[2] + marker_move);
            consumed = true;
            break;
        case GDK_KEY_Down:
        case GDK_KEY_KP_Down:
            luv.set(2, luv[2] - marker_move);
            consumed = true;
            break;
        case GDK_KEY_Left:
        case GDK_KEY_KP_Left:
            luv.set(1, luv[1] - marker_move);
            consumed = true;
            break;
        case GDK_KEY_Right:
        case GDK_KEY_KP_Right:
            luv.set(1, luv[1] + marker_move);
            consumed = true;
    }

    if (!consumed) return false;

    _adjusting = true;

    if (_values.set(luv, true))
        color_changed();

    return true;
}

/* ColorPoint */
ColorPoint::ColorPoint()
    : x(0)
    , y(0)
    , color(0x0)
{}

ColorPoint::ColorPoint(double x, double y, Color c)
    : x(x)
    , y(y)
    , color(std::move(c))
{}

ColorPoint::ColorPoint(double x, double y, guint c)
    : x(x)
    , y(y)
    , color(c)
{}

static double lerp(double v0, double v1, double t0, double t1, double t)
{
    double const s = (t0 != t1) ? (t - t0) / (t1 - t0) : 0.0;
    return Geom::lerp(s, v0, v1);
}

static ColorPoint lerp(ColorPoint const &v0, ColorPoint const &v1, double t0, double t1,
        double t)
{
    double x = lerp(v0.x, v1.x, t0, t1, t);
    double y = lerp(v0.y, v1.y, t0, t1, t);

    auto r0 = *v0.color.converted(Type::RGB);
    auto r1 = *v1.color.converted(Type::RGB);
    double r = lerp(r0[0], r1[0], t0, t1, t);
    double g = lerp(r0[1], r1[1], t0, t1, t);
    double b = lerp(r0[2], r1[2], t0, t1, t);

    return ColorPoint(x, y, Color(Type::RGB, {r, g, b}));
}

// N.B. We also have Color:get_perceptual_lightness(), but that uses different weightings..!
double luminance(Color const &color)
{
    auto c = *color.converted(Type::RGB);
    return (c[0] * 0.2125 + c[1] * 0.7154 + c[2] * 0.0721);
}

/**
 * Convert a point of the gamut color polygon (Luv) to pixel coordinates.
 *
 * @param point The point in Luv coordinates.
 * @param scale Zoom amount to fit polygon to outer circle.
 * @param resize Zoom amount to fit wheel in widget.
 */
static Geom::Point to_pixel_coordinate(Geom::Point const &point, double scale, double resize)
{
    return Geom::Point(
        point[Geom::X] * scale * resize + (SIZE * resize / 2.0),
        (SIZE * resize / 2.0) - point[Geom::Y] * scale * resize
    );
}

/**
 * Convert a point in pixels on the widget to Luv coordinates.
 *
 * @param point The point in pixel coordinates.
 * @param scale Zoom amount to fit polygon to outer circle.
 * @param resize Zoom amount to fit wheel in widget.
 */
static Geom::Point from_pixel_coordinate(Geom::Point const &point, double scale, double resize)
{
    return Geom::Point(
        (point[Geom::X] - (SIZE * resize / 2.0)) / (scale * resize),
        ((SIZE * resize / 2.0) - point[Geom::Y]) / (scale * resize)
    );
}

/**
 * @overload
 * @param point A vector of points in Luv coordinates.
 * @param scale Zoom amount to fit polygon to outer circle.
 * @param resize Zoom amount to fit wheel in widget.
 */
static std::vector<Geom::Point> to_pixel_coordinate(std::vector<Geom::Point> const &points,
                                                    double scale, double resize)
{
    std::vector<Geom::Point> result;

    for (auto const &p : points) {
        result.emplace_back(to_pixel_coordinate(p, scale, resize));
    }

    return result;
}

/**
  * Paints padding for an edge of the triangle,
  * using the (vertically) closest point.
  *
  * @param p0 A corner of the triangle. Not the same corner as p1
  * @param p1 A corner of the triangle. Not the same corner as p0
  * @param padding The height of the padding
  * @param pad_upwards True if padding is above the line
  * @param buffer Array that the triangle is painted to
  * @param height Height of buffer
  * @param stride Stride of buffer
*/

void draw_vertical_padding(ColorPoint p0, ColorPoint p1, int padding, bool pad_upwards,
        guint32 *buffer, int height, int stride)
{
    // skip if horizontal padding is more accurate, e.g. if the edge is vertical
    double gradient = (p1.y - p0.y) / (p1.x - p0.x);
    if (std::abs(gradient) > 1.0) {
        return;
    }

    double min_y = std::min(p0.y, p1.y);
    double max_y = std::max(p0.y, p1.y);

    double min_x = std::min(p0.x, p1.x);
    double max_x = std::max(p0.x, p1.x);

    // go through every point on the line
    for (int y = min_y; y <= max_y; ++y) {
        double start_x = lerp(p0, p1, p0.y, p1.y, std::clamp(static_cast<double>(y), min_y,
                    max_y)).x;
        double end_x = lerp(p0, p1, p0.y, p1.y, std::clamp(static_cast<double>(y) + 1, min_y,
                    max_y)).x;
        if (start_x > end_x) {
            std::swap(start_x, end_x);
        }

        guint32 *p = buffer + y * stride;
        p += static_cast<int>(start_x);
        for (int x = start_x; x <= end_x; ++x) {
            // get the color at this point on the line
            ColorPoint point = lerp(p0, p1, p0.x, p1.x, std::clamp(static_cast<double>(x),
                        min_x, max_x));
            // paint the padding vertically above or below this point
            for (int offset = 0; offset <= padding; ++offset) {
                if (pad_upwards && (point.y - offset) >= 0) {
                    *(p - (offset * stride)) = point.color.toARGB();
                } else if (!pad_upwards && (point.y + offset) < height) {
                    *(p + (offset * stride)) = point.color.toARGB();
                }
            }
            ++p;
        }
    }
}

} // namespace Inkscape::UI::Widget

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim:filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8: textwidth=99:

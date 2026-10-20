// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * A slider with colored background - implementation.
 *//*
 * Authors:
 *   see git history
 *
 * Copyright (C) 2018-2024 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "color-slider.h"

#include <gdkmm/frameclock.h>
#include <gdkmm/general.h>
#include <gtkmm/eventcontrollermotion.h>
#include <gtkmm/gestureclick.h>
#include <gtkmm/gesturedrag.h>
#include <sigc++/functors/mem_fun.h>
#include <utility>

#include "colors/color-set.h"
#include "colors/spaces/gamut.h"
#include "colors/manager.h"
#include "colors/spaces/components.h"
#include "colors/spaces/enum.h"
#include "ui/controller.h"
#include "ui/util.h"
#include "util/drawing-utils.h"
#include "util/theme-utils.h"

constexpr int THUMB_SPACE = 16;
constexpr int TRACK_HEIGHT = 8;
constexpr int THUMB_SIZE = TRACK_HEIGHT + 2;
constexpr int RING_THICKNESS = 2;
constexpr int CHECKERBOARD_TILE = TRACK_HEIGHT / 2;
constexpr uint32_t ERR_DARK = 0xff00ff00;    // Green
constexpr uint32_t ERR_LIGHT = 0xffff00ff;   // Magenta

namespace Inkscape::UI::Widget {

ColorSlider::ColorSlider(std::shared_ptr<Colors::ColorSet> colors, Colors::Space::Component component) :
    _colors(std::move(colors)),
    _component(std::move(component)) {

    construct();
}

ColorSlider::ColorSlider(
    BaseObjectType *cobject,
    Glib::RefPtr<Gtk::Builder> const &builder,
    std::shared_ptr<Colors::ColorSet> colors,
    Colors::Space::Component component)
    : Gtk::DrawingArea(cobject)
    , _colors(std::move(colors))
    , _component(std::move(component)) {

    construct();
}

ColorSlider::~ColorSlider() {
    if (_tick_callback) {
        remove_tick_callback(_tick_callback);
    }
}

void ColorSlider::construct() {
    set_name("ColorSlider");

    _ring_size = THUMB_SIZE;
    _ring_thickness = RING_THICKNESS;
    set_draw_func(sigc::mem_fun(*this, &ColorSlider::draw_func));

    auto const click = Gtk::GestureClick::create();
    click->set_button(1); // left
    click->signal_pressed().connect([this, &click = *click](auto &&...args) { on_click_pressed(click, args...); });
    add_controller(click);

    // slider thumb animation logic
    auto connect_tick_callback = [this] {
        if (_tick_callback) return;

        _tick_callback = add_tick_callback([this] (const Glib::RefPtr<Gdk::FrameClock>& clock) {
            auto timings = clock->get_current_timings();
            auto ft = timings->get_frame_time();
            double dt; // time delta
            if (_last_time) {
                dt = ft - _last_time;
            }
            else {
                dt = timings->get_refresh_interval();
                if (dt <= 0) dt = 1e6 / 60; // pick some default interval for 60 frames/second
            }
            _last_time = ft;
            dt /= 1'000'000; // microseconds to seconds
            // on mouse hover grow by 12 px/s, otherwise shrink by 6 px/s:
            auto change = dt * (_hover ? 12 : 6); // calc distance in pixels

            // vary ring thickness to show the user that they are hovering over the slider
            auto size = std::clamp(_ring_size + (_hover ? -change : change), THUMB_SIZE - 1.0, THUMB_SIZE + 0.0);
            auto thickness = std::clamp(_ring_thickness + (_hover ? change : -change), RING_THICKNESS + 0.0, RING_THICKNESS + 1.0);
            queue_draw();

            if (size != _ring_size || thickness != _ring_thickness) {
                _ring_size = size;
                _ring_thickness = thickness;
                return true; // continue animation
            }
            else {
                _tick_callback = 0;
                _last_time = 0;
                return false; // stop the callback
            }
        });
    };
    auto const motion = Gtk::EventControllerMotion::create();
    motion->signal_motion().connect([this, &motion = *motion](auto &&...args) { on_motion(motion, args...); });
    motion->signal_enter().connect([this, connect_tick_callback](auto&& ...) {
        _hover = true;
        connect_tick_callback();
    });
    motion->signal_leave().connect([this, connect_tick_callback](auto&& ...) {
        _hover = false;
        connect_tick_callback();
    });
    add_controller(motion);

    _drag = Gtk::GestureDrag::create();
    _drag->set_button(1); // left
    _drag->signal_begin().connect([this](auto){ _dragging = true; });
    _drag->signal_update().connect([this](auto seq){ on_drag(seq); });
    _drag->signal_end().connect([this](auto){ _dragging = false; });
    _drag->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    add_controller(_drag);

    _changed_connection = _colors->signal_changed.connect([this]() {
        queue_draw();
    });
}

static Geom::OptIntRect get_active_area(int full_width, int full_height) {
    int width = full_width - THUMB_SPACE;
    if (width <= 0) return {};

    int left = THUMB_SPACE / 2;
    int top = 0;
    return Geom::IntRect::from_xywh(left, top, width, full_height);
}

static double get_value_at(Gtk::Widget const &self, double const x, double const y)
{
    auto area = get_active_area(self.get_width(), self.get_height());
    if (!area) return 0.0;
    return CLAMP((x - area->left()) / area->width(), 0.0, 1.0);
}

void ColorSlider::on_click_pressed(Gtk::GestureClick const &click,
                                   int /*n_press*/, double const x, double const y)
{
    update_component(x, y, click.get_current_event_state());
}

void ColorSlider::on_motion(Gtk::EventControllerMotion const &motion, double x, double y)
{
    auto const state = motion.get_current_event_state();
    if (Controller::has_flag(state, Gdk::ModifierType::BUTTON1_MASK)) {
        // only update color if user is dragging the slider;
        // don't rely on any click/release events, as release event might be lost leading to unintended updates
        update_component(x, y, state);
    }
    _dragging = false;
}

void ColorSlider::update_component(double x, double y, Gdk::ModifierType const state)
{
    // auto const constrained = Controller::has_flag(state, Gdk::ModifierType::CONTROL_MASK);
    // XXX We don't know how to deal with constraints yet.
    if (_colors->isValid(_component) && _colors->setAll(_component, get_value_at(*this, x, y))) {
        signal_value_changed.emit();
    }
}

void ColorSlider::on_drag(Gdk::EventSequence* sequence) {
    if (!_drag->get_current_button() || !_drag->is_active()) {
        _dragging = false;
        return;
    }

    // only update color if user is dragging the slider
    if (_dragging) {
        double x = 0.0;
        double y = 0.0;
        _drag->get_start_point(x, y);
        double dx = 0.0;
        double dy = 0.0;
        _drag->get_offset(dx, dy);
        auto state = _drag->get_current_event_state();
        update_component(x + dx, y + dy, state);
    }
}

/**
 * Generate a checkerboard pattern with the given colors.
 *
 * @arg dark - The RGBA dark color
 * @arg light - The RGBA light color
 * @arg scale - The scale factor of the cairo surface
 * @arg buffer - The memory to populate with this pattern
 *
 * @returns A Gdk::Pixbuf of the buffer memory.
 */
Glib::RefPtr<Gdk::Pixbuf> _make_checkerboard(uint32_t dark, uint32_t light, unsigned scale, std::vector<uint32_t> &buffer)
{
    // A pattern of 2x2 blocks is enough for REPEAT mode to do the rest, this way we never need to recalculate the checkerboard
    static auto block = CHECKERBOARD_TILE * scale;
    static auto pattern = block * 2;

    buffer = std::vector<uint32_t>(pattern * pattern);
    for (auto y = 0; y < pattern; y++) {
        for (auto x = 0; x < pattern; x++) {
            buffer[(y * pattern) + x] = ((x / block) & 1) != ((y / block) & 1) ? dark : light;
        }
    }
    return Gdk::Pixbuf::create_from_data((guint8*)buffer.data(), Gdk::Colorspace::RGB, true, 8, pattern, pattern, pattern * 4);
}

static void draw_slider_thumb(const Cairo::RefPtr<Cairo::Context>& ctx, const Geom::Point& location, double size, double thickness, const Gdk::RGBA& fill, const Gdk::RGBA& stroke) {
    auto center = location.round(); //todo - verify pix grid fit + Geom::Point(0.5, 0.5);
    auto radius = size / 2;

    ctx->arc(center.x(), center.y(), radius, 0, 2 * M_PI);
    ctx->set_source_rgba(stroke.get_red(), stroke.get_green(), stroke.get_blue(), stroke.get_alpha());
    ctx->set_line_width(thickness + 2.0);
    ctx->stroke();
    // ring
    ctx->arc(center.x(), center.y(), radius, 0, 2 * M_PI);
    ctx->set_source_rgba(fill.get_red(), fill.get_green(), fill.get_blue(), fill.get_alpha());
    ctx->set_line_width(thickness);
    ctx->stroke();
}

void ColorSlider::draw_func(Cairo::RefPtr<Cairo::Context> const &cr,
                            int const full_width, int const full_height)
{
    auto maybe_area = get_active_area(full_width, full_height);
    if (!maybe_area) return;

    auto area = *maybe_area;

    auto border = area;
    // stretch the track horizontally to align its rounded ends with a center of the thumb
    // when it's in a leftmost/rightmost position
    border.expandBy(4, 0);
    // shrink the track to the desired height
    border.shrinkBy(0, (area.height() - TRACK_HEIGHT) / 2);
    border.setBottom(border.top() + TRACK_HEIGHT);
    // rounded ends
    auto radius = TRACK_HEIGHT / 2.0;
    Util::rounded_rectangle(cr, border, radius);

    auto const scale = get_scale_factor();
    auto width = border.width() * scale;
    auto left = border.left() * scale;
    auto top = border.top() * scale;
    bool const is_alpha = _component.id == "alpha";

    // changing scale to draw pixmap at display resolution
    cr->save();
    cr->scale(1.0 / scale, 1.0 / scale);

    // Color set is empty, this is not allowed, show warning colors
    if (_colors->isEmpty()) {
        static std::vector<uint32_t> err_buffer;
        static Glib::RefPtr<Gdk::Pixbuf> error = _make_checkerboard(ERR_DARK, ERR_LIGHT, scale, err_buffer);

        Gdk::Cairo::set_source_pixbuf(cr, error, left, top);
        cr->get_source()->set_extend(Cairo::Pattern::Extend::REPEAT);
        cr->fill();

        // Don't try and paint any color (there isn't any)
        cr->restore();
        return;
    }

    // The alpha background is a checkerboard pattern of light and dark pixels
    if (is_alpha) {
        std::vector<uint32_t> bg_buffer;
        auto [col1, col2] = Util::get_checkerboard_colors(*this, true);
        Glib::RefPtr<Gdk::Pixbuf> background = _make_checkerboard(col1, col2, scale, bg_buffer);

        // Paint the alpha background
        Gdk::Cairo::set_source_pixbuf(cr, background, left, top);
        cr->get_source()->set_extend(Cairo::Pattern::Extend::REPEAT);
        cr->fill_preserve();
    }

    // Draw row of colored pixels here
    auto paint_color = _colors->getAverage();

    if (!is_alpha) {
        // Remove alpha channel from paint
        paint_color.enableOpacity(false);
    }

    // When the widget is wider, we want a new color gradient buffer
    if (!_gradient || _gr_buffer.size() < static_cast<size_t>(width)) {
        _gr_buffer.resize(width);
        _gradient = Gdk::Pixbuf::create_from_data((guint8*)&_gr_buffer.front(), Gdk::Colorspace::RGB, true, 8, width, 1, width * 4);
    }

    double lim = width > 1 ? width - 1.0 : 1.0;
    auto space_rgb = Colors::Manager::get().find(Colors::Space::Type::RGB);
    for (int x = 0; x < width; x++) {
        paint_color.set(_component.index, x / lim);
        auto c = Colors::to_gamut_css(paint_color, space_rgb);
        _gr_buffer[x] = c.toABGR();// paint_color.toABGR();
    }

    Gdk::Cairo::set_source_pixbuf(cr, _gradient, left, top);
    cr->get_source()->set_extend(Cairo::Pattern::Extend::REPEAT);
    cr->fill();
    cr->restore();

    bool dark_theme = Util::is_current_theme_dark(*this);
    Util::draw_standard_border(cr, border, dark_theme, radius, scale);

    // draw slider thumb
    if (_colors->isValid(_component)) {
        auto ring = get_color();
        auto dark = get_luminance(ring) < 0.5;
        float x = dark ? 1.0f : 0.0f;
        float alpha = dark ? 0.40f : 0.25f;
        auto stroke = Gdk::RGBA(x, x, x, alpha);

        double value = std::clamp(_colors->getAverage(_component), 0.0, 1.0);
        if (std::isfinite(value)) {
            draw_slider_thumb(cr, Geom::Point(area.left() + value * area.width(), area.midpoint().y()), _ring_size, _ring_thickness, ring, stroke);
        }
    }
}

double ColorSlider::getScaled() const
{
    if (_colors->isEmpty())
        return 0.0;
    return _colors->getAverage(_component) * _component.scale;
}

void ColorSlider::setScaled(double value)
{
    if (!_colors->isValid(_component)) {
        g_message("ColorSlider - cannot set color channel, it is not valid.");
        return;
    }
    // setAll replaces every color with the same value, setAverage moves them all by the same amount.
    _colors->setAll(_component, value / _component.scale);
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

// SPDX-License-Identifier: GPL-2.0-or-later

#include "color-plate.h"
#include <gtkmm/eventcontrollermotion.h>
#include <gtkmm/gestureclick.h>

#include "ui/controller.h"
#include "util/drawing-utils.h"
#include "util/theme-utils.h"

namespace Inkscape::UI::Widget {

using namespace Colors;

void circle(const Cairo::RefPtr<Cairo::Context>& ctx, const Geom::Point& center, double radius) {
    ctx->arc(center.x(), center.y(), radius, 0, 2 * M_PI);
}

static void draw_color_plate(const Cairo::RefPtr<Cairo::Context>& ctx, const Geom::Rect& area, double radius, const Cairo::RefPtr<Cairo::ImageSurface>& preview, bool circular) {
    if (area.width() <= 0 || area.height() <= 0) return;

    ctx->save();
    if (circular) {
        //TODO: on low-res display - align midpoint to whole pixels
        circle(ctx, area.midpoint(), area.minExtent() / 2);
    }
    else {
        Util::rounded_rectangle(ctx, area, radius);
    }
    ctx->clip();

    Geom::Point scale;
    auto offset = area.min();
    if (circular) {
        // Note: circular color preview needs to be larger than the requested area to make sure
        // that there are no miscolored pixels visible after a clip path is applied.
        // Note 2: comment out clip() path above to verify that the circle is centered with respect to the border

        auto size = std::min(area.width(), area.height());
        // uniform scaling of the circular preview; it is square, so just use width;
        // subtract pixels - it's a border for clipping
        auto s = size / (preview->get_width() - 2);
        scale = Geom::Point(s, s);
        offset += {-1.0 * s, -1.0 * s}; // move outline to hide extra border pixels
        // center the preview
        auto d = area.width() - area.height();
        if (d > 0) {
            // center horizontally
            offset += {d / 2, 0};
        }
        else if (d < 0) {
            // in the middle
            offset += {0, -d / 2};
        }
    }
    else {
        // rectangular color preview; stretch it to cover "area"
        // subtract pixels - it's a border for clipping
        auto scale_x = area.width() / (preview->get_width() - 2);
        auto scale_y = area.height() / (preview->get_height() - 2);
        scale = Geom::Point(scale_x, scale_y);
        offset -= scale;
    }

    ctx->scale(scale.x(), scale.y());
    ctx->set_source(preview, offset.x() / scale.x(), offset.y() / scale.y());
    ctx->paint();

    ctx->restore();
}

static Geom::Point get_color_coordinates(double val1, double val2, bool circular) {
    val1 = std::clamp(val1, 0.0, 1.0);
    val2 = std::clamp(val2, 0.0, 1.0);

    if (circular) {
        // point in a circle
        // val1 is an angle (0..1 -> -2pi..2pi), while val2 is a distance
        auto angle = (val1 * 2 * M_PI) - M_PI;
        auto x = sin(angle) * val2;
        auto y = cos(angle) * val2;
        return {x, y};
    }
    else {
        // point in a rectangle
        return {val1, 1 - val2};
    }
}

static void set_color_helper(Color& color, int channel1, int channel2, double x, double y, bool disc) {
    if (disc) {
        auto dist = std::hypot(x, y);
        auto angle = (atan2(x, y) + M_PI) / (2 * M_PI); // angle in 0..1 range
        color.set(channel1, angle);
        color.set(channel2, dist);
    }
    else {
        // rectangle
        color.set(channel1, x);
        color.set(channel2, 1 - y);
    }
}

static Cairo::RefPtr<Cairo::ImageSurface> create_color_preview(int size, const std::function<void (std::vector<std::uint32_t>&, int width)>& draw) {
    auto fmt = Cairo::ImageSurface::Format::ARGB32;
    auto stride = Cairo::ImageSurface::format_stride_for_width(fmt, size);
    auto width = stride / sizeof(std::uint32_t);
    std::vector<std::uint32_t> data(size * width, 0x00000000);

    draw(data, width);

    void* buffer = data.data();
    auto src = Cairo::ImageSurface::create(static_cast<unsigned char*>(buffer), fmt, size, size, stride);
    auto dest = Cairo::ImageSurface::create(fmt, size, size);
    auto ctx = Cairo::Context::create(dest);
    ctx->set_source(src, 0, 0);
    ctx->paint();
    return dest;
}

// rectangular color picker
static Cairo::RefPtr<Cairo::ImageSurface> create_color_plate(unsigned int resolution, const Color& base, int channel1, int channel2) {
    const double limit = resolution;
    const int size = resolution + 1;

    return create_color_preview(size, [=](auto& data, auto width) {
        auto color = base;
        color.addOpacity();
        int row = 0;
        //TODO: add duplicated border pixels
        for (int iy = 0; iy <= limit; ++iy, ++row) {
            auto y = iy / limit;
            color.set(channel2, 1 - y);
            auto index = static_cast<size_t>(row * width);
            for (int ix = 0; ix <= limit; ++ix) {
                auto x = ix / limit;
                color.set(channel1, x);
                data[index++] = color.toARGB();
            }
        }
        //todo: compilation error on linux:
        // assert(index <= data.size());
    });
}

static Cairo::RefPtr<Cairo::ImageSurface> create_color_wheel(unsigned int resolution, const Color& base, int channel1, int channel2) {
    const int radius = resolution / 2;
    const double limit = radius;
    const int size = radius * 2 + 1;
    Color color = base;

    return create_color_preview(size, [&](auto& data, auto width) {
        // extra pixels at the borderline (that's the +1/radius), so clipping doesn't expose anything "unpainted"
        double rsqr = std::pow(1.0 + 1.0/radius, 2);
        int row = 0;
        for (int iy = -radius; iy <= radius; ++iy, ++row) {
            int index = row * width;
            auto y = iy / limit;
            auto sy = y * y;
            for (int ix = -radius; ix <= radius; ++ix, ++index) {
                auto x = ix / limit;
                auto sx = x * x;
                // transparent pixels outside the circle
                if (sx + sy > rsqr) continue;

                set_color_helper(color, channel1, channel2, x, y, true);
                data[index] = color.toARGB();
            }
        }
    });
}

static Geom::Point screen_to_local(const Geom::Rect& active, Geom::Point point, bool circular, bool* inside = nullptr) {
    if (inside) {
        *inside = active.contains(point);
    }
    // normalize point
    point = active.clamp(point);
    point = (point - active.min()) / active.dimensions();

    if (circular) {
        // restrict point to a circle
        auto min = active.minExtent();
        auto scale = Geom::Point(min, min) / active.dimensions();
        // coords in -1..1 range:
        auto c = (point * 2 - Geom::Point(1, 1)) / scale;
        auto dist = L2(c);
        if (dist > 1) {
            c /= dist;
            if (inside) {
                *inside = false;
            }
        }
        point = c;
    }

    return point;
}

static Geom::Point local_to_screen(const Geom::Rect& active, Geom::Point point, bool circular) {
    if (circular) {
        auto min = active.minExtent();
        auto scale = Geom::Point(min, min) / active.dimensions();
        point = (point * scale + Geom::Point(1, 1)) / 2;
    }

    return active.min() + point * active.dimensions();
}


ColorPlate::ColorPlate() {
    set_name("ColorPlate");
    set_disc(_disc); // add right CSS class

    set_draw_func([this](const Cairo::RefPtr<Cairo::Context>& ctx, int /*width*/, int /*height*/){
        draw_plate(ctx);
    });

    auto const motion = Gtk::EventControllerMotion::create();
    motion->set_propagation_phase(Gtk::PropagationPhase::TARGET);
    motion->signal_motion().connect([this, &motion = *motion](auto &&...args) { on_motion(motion, args...); });
    add_controller(motion);

    auto const click = Gtk::GestureClick::create();
    click->set_button(1); // left
    click->signal_pressed().connect(Controller::use_state([this](auto &, int, double x, double y) {
        // verify click location
        if (auto area = get_active_area()) {
            bool inside = false;
            auto down = screen_to_local(*area, Geom::Point(x, y), _disc, &inside);
            if (inside) {
                _down = down;
                _drag = true;
                queue_draw();
                fire_color_changed();
                return Gtk::EventSequenceState::CLAIMED;
            }
        }
        _down = {};
        _drag = false;
        return Gtk::EventSequenceState::NONE;
    }, *click));
    add_controller(click);
}

void ColorPlate::draw_plate(const Cairo::RefPtr<Cairo::Context>& ctx) {
    auto maybe_area = get_area();
    if (!maybe_area) return;

    auto area = *maybe_area;

    if (!_plate) {
        // color preview resolution in discrete color steps;
        // in-betweens will be interpolated in sRGB as the preview image gets stretched;
        // this number should be kept small for faster interactive color plate refresh
        constexpr int resolution = 64; // this number impacts performance big time!
        _plate = _disc ?
            create_color_wheel(resolution, _base_color, _channel1, _channel2) :
            create_color_plate(resolution, _base_color, _channel1, _channel2);
    }
    draw_color_plate(ctx, area, _radius, _plate, _disc);
    bool dark = Util::is_current_theme_dark(*this);
    Util::draw_standard_border(ctx, area, dark, _radius, get_scale_factor(), _disc);

    if (auto maybe = get_active_area(); _down && maybe) {
        auto pt = local_to_screen(*maybe, *_down, _disc);
        Util::draw_point_indicator(ctx, pt);
    }
}

void ColorPlate::set_base_color(Color color, int fixed_channel, int var_channel1, int var_channel2) {
    color.setOpacity(1);
    if (_base_color != color) {
        // optimization: rebuild the plate only if "fixed" channel value has changed, necessitating new rendering
        if (_base_color.getSpace() != color.getSpace() || fabs(_fixed_channel_val - color[fixed_channel]) > 0.005 ||
            _channel1 != var_channel1 || _channel2 != var_channel2) {
            _plate.reset();
            _fixed_channel_val = color[fixed_channel];
            _channel1 = var_channel1;
            _channel2 = var_channel2;
            queue_draw();
        }
        _base_color = std::move(color);
    }
}

Geom::OptRect ColorPlate::get_area() const {
    auto alloc = get_allocation();
    auto min = 2 * _padding;
    if (alloc.get_width() <= min || alloc.get_height() <= min) return {};

    return Geom::Rect(0, 0, alloc.get_width(), alloc.get_height()).shrunkBy(_padding);
}

Geom::OptRect ColorPlate::get_active_area() const {
    auto area = get_area();
    if (!area || area->minExtent() < 1) return {};

    return area->shrunkBy(1, 1);
}

Color ColorPlate::get_color_at(const Geom::Point& point) const {
    auto color = _base_color;
    set_color_helper(color, _channel1, _channel2, point.x(), point.y(), _disc);
    return color;
}

void ColorPlate::fire_color_changed() {
    if (_down.has_value()) {
        auto color = get_color_at(*_down);
        _signal_color_changed.emit(color);
    }
}

void ColorPlate::on_motion(Gtk::EventControllerMotion const &motion, double x, double y) {
    if (!_drag) return;

    if (x == 0 && y == 0) {
        // this value is legit, but the motion controller also reports it when
        // the mouse button leaves the popover, leading to an unexpected jump;
        // skip it (as of gtk 4.15.7)
        return;
    }

    auto state = motion.get_current_event_state();
    auto drag = Controller::has_flag(state, Gdk::ModifierType::BUTTON1_MASK);
    if (!drag) return;

    // drag move
    if (auto area = get_active_area()) {
        _down = screen_to_local(*area, Geom::Point(x, y), _disc);
        queue_draw();
        fire_color_changed();
    }
}

void ColorPlate::set_disc(bool disc) {
    _disc = disc;
    if (disc) {
        remove_css_class("rectangular");
        add_css_class("circular");
    }
    else {
        remove_css_class("circular");
        add_css_class("rectangular");
    }
    queue_draw();
}

bool ColorPlate::is_disc() const {
    return _disc;
}

void ColorPlate::set_padding(int pad) {
    if (pad >= 0 && _padding != pad) {
        _padding = pad;
        queue_draw();
    }
}

void ColorPlate::move_indicator_to(const Colors::Color& color) {
    // find 'color' on the plate and move the indicator to it
    auto point = get_color_coordinates(color[_channel1], color[_channel2], _disc);
    if (_down && _down == point) return;

    _down = point;
    queue_draw();
}

sigc::signal<void(const Color&)>& ColorPlate::signal_color_changed() {
    return _signal_color_changed;
}

} // namespace

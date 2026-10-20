//   SPDX-License-Identifier: GPL-2.0-or-later
//

#include <chrono>

#include "color-plate.h"
#include "oklab-color-wheel.h"
#include "colors/spaces/base.h"

namespace Inkscape::UI::Widget {

using namespace Colors::Space;

constexpr bool TEST_TIMING = false;

class FastColorWheel : public ColorPlate, public ColorWheel {
public:
    FastColorWheel(Type source, Type plate, int fixed_channel, int var_channel1, int var_channel2, bool disc) {
        _source = source;
        _plate = plate;
        _fixed_channel = fixed_channel;
        _var_channel1 = var_channel1;
        _var_channel2 = var_channel2;
        set_disc(disc);
    }

    void set_color(const Colors::Color& color) override {
        auto copy = color.converted(_plate);
        auto dest = copy.value_or(Colors::Color{_plate, {0,0,0}});
        set_base_color(dest, _fixed_channel, _var_channel1, _var_channel2);
        // move color indicator to correct spot on the disc
        move_indicator_to(dest);
    }

    Widget& get_widget() override { return *this; }

    sigc::connection connect_color_changed(sigc::slot<void(const Colors::Color&)> cb) override {
        return signal_color_changed().connect([this, cb](auto& c) {
            auto color = c.converted(_source);
            if (color) cb(*color); else g_warning("Color conversion from type %d to type %d failed.", int(_plate), int(_source));
        });
    }

    void redraw(const Cairo::RefPtr<Cairo::Context>& ctx) override { draw_plate(ctx); }

private:
    Type _source;
    Type _plate;
    int _fixed_channel;
    int _var_channel1;
    int _var_channel2;
};

static ColorWheel* create_plate(Type source, Type plate, bool disc) {
    if (disc) {
        auto value = 2; // if value changes, color wheel needs to be rebuilt
        auto hue = 0;   // vary hue with angle (while painting the disc)
        auto sat = 1;   // vary saturation with distance from the center of the disc (while painting the disc)
        return new FastColorWheel(source, plate, value, hue, sat, disc);
    }
    else {
        auto hue = 0;   // hue is fixed; it's a single hue rectangular plate
        auto sat = 1;
        auto value = 2;
        return new FastColorWheel(source, plate, hue, sat, value, disc);
    }
}

static std::pair<ColorWheel*, bool> create_color_wheel_helper(Type type, bool create_widget, bool disc) {
    bool can_create = true;
    ColorWheel* wheel = nullptr;

    switch (type) {
    case Type::HSL:
        if (create_widget) wheel = disc ? new ColorWheelHSL() : create_plate(type, Type::HSV, disc);
        break;

    case Type::HSLUV:
        if (create_widget) wheel = disc ? new ColorWheelHSLuv() : create_plate(type, Type::HSV, disc);
        break;

    case Type::OKHSL:
        if (create_widget) {
            wheel = create_plate(type, Type::OKHSV, disc);
        }
        break;

    case Type::OKLCH:
        if (create_widget) {
            wheel = create_plate(type, Type::OKHSV, disc);
        }
        break;

    case Type::HSV:
        if (create_widget) {
            wheel = disc ? new ColorWheelHSL() : create_plate(type, Type::HSV, disc);
        }
        break;

    case Type::RGB:
        if (create_widget) {
            wheel = create_plate(type, Type::HSV, disc);
        }
        break;

    case Type::CMYK:
        if (create_widget) {
            wheel = create_plate(type, Type::HSV, disc);
        }
        break;

    default:
        can_create = false;
        break;
    }

    // Speed test - use this test to evaluate how quickly we can rebuild a color wheel of given Space::Type
    if (TEST_TIMING && create_widget) {
        // test
        ColorWheel* w1 = new FastColorWheel(Type::OKHSL, Type::OKHSL, 0, 1, 2, true);
        ColorWheel* w2 = new OKWheel();

        auto s = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, 1024, 1024);
        auto ctx = Cairo::Context::create(s);
        for (auto w : {w1, w2}) {
            w->get_widget().size_allocate(Gtk::Allocation(0,0,500,500), 0);
            // w->get_widget().set_size_request(500, 500);
            auto old_time =  std::chrono::high_resolution_clock::now();
            Colors::Color color(Type::OKHSL, {0.5, 0.5, 0.5});
            for (int i = 0; i < 100; ++i) {
                color.set(0, i / 100.0);
                w->set_color(color);
                w->redraw(ctx);
            }
            auto current_time =  std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - old_time);
            g_message("render time for test wheel: %d ms", static_cast<int>(elapsed.count()));
        }

        delete w1;
        delete w2;
    }

    return std::make_pair(wheel, can_create);
}

ColorWheel* create_managed_color_wheel(Type type, bool disc) {
    auto [wheel, _] = create_color_wheel_helper(type, true, disc);
    if (wheel) {
        wheel->get_widget().set_manage();
    }
    return wheel;
}

bool can_create_color_wheel(Type type) {
    auto [_, ok] = create_color_wheel_helper(type, false, true);
    return ok;
}

} // Inkscape

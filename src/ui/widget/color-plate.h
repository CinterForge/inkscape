// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef COLOR_SELECTOR_H
#define COLOR_SELECTOR_H

#include <gtkmm/drawingarea.h>
#include <2geom/point.h>

#include "colors/color.h"

namespace Gtk {
class EventControllerMotion;
}

namespace Inkscape::UI::Widget {

class ColorPlate : public Gtk::DrawingArea {
public:
    ColorPlate();

    // set color to use for creating plates and tell which channel is fixed and which channels to vary doing so
    void set_base_color(Colors::Color color, int fixed_channel, int var_channel1, int var_channel2);
    // should we show a disc color selector (true) or a rectangular one (false)?
    void set_disc(bool disc);
    bool is_disc() const;
    // extra space around the widget reserved for the current color indicator
    void set_padding(int pad);
    // move the on-plate indicator point to matching color
    void move_indicator_to(const Colors::Color& color);

    sigc::signal<void (const Colors::Color&)>& signal_color_changed();

protected:
    void draw_plate(const Cairo::RefPtr<Cairo::Context>& ctx);

private:
    void on_motion(Gtk::EventControllerMotion const &motion, double x, double y);
    Geom::OptRect get_area() const;
    Geom::OptRect get_active_area() const;
    Colors::Color get_color_at(const Geom::Point& point) const;
    void fire_color_changed();

    int _padding = 4;
    double _radius = 4.0;
    bool _disc = true;
    Cairo::RefPtr<Cairo::ImageSurface> _plate;
    std::optional<Geom::Point> _down;
    Colors::Color _base_color{Colors::Space::Type::RGB, {0,0,0}};
    // to optimize plate rebuilding, remember one color channel it was created with
    double _fixed_channel_val = -1.0;
    int _channel1 = 1;
    int _channel2 = 2;
    sigc::signal<void (const Colors::Color&)> _signal_color_changed;
    bool _drag = false;
};

} // namespace

#endif

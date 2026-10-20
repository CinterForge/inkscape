// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef COLOR_WHEEL_H
#define COLOR_WHEEL_H

namespace Inkscape::Colors {
class Color;
}

namespace Inkscape::UI::Widget {

class ColorWheel {
public:
    virtual ~ColorWheel() = default;

    // set current color
    virtual void set_color(const Colors::Color& color) = 0;

    // signal color changed
    virtual sigc::connection connect_color_changed(sigc::slot<void (const Colors::Color&)>) = 0;

    // get widget base
    virtual Gtk::Widget& get_widget() = 0;

    // redraw for testing refresh speed
    virtual void redraw(const Cairo::RefPtr<Cairo::Context>& ctx) = 0;
};

}

#endif //COLOR_WHEEL_H

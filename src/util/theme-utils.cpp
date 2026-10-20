// SPDX-License-Identifier: GPL-2.0-or-later

#include "theme-utils.h"
#include "colors/color.h"
#include "colors/utils.h"
#include "ui/util.h"

namespace Inkscape::Util {

bool is_current_theme_dark(Gtk::Widget& widget) {
    auto style = widget.get_style_context();
    Gdk::RGBA bgnd;
    bool found = style->lookup_color("theme_bg_color", bgnd);
    // use theme background color as a proxy for dark theme; this method is fast,
    // which is important if we call it frequently (like in a drawing function)
    auto dark_theme = found && get_luminance(bgnd) <= 0.5;
    return dark_theme;
}

std::tuple<std::uint32_t, std::uint32_t> get_checkerboard_colors(Gtk::Widget& widget, bool argb) {
    auto dark = is_current_theme_dark(widget);
    Colors::Color a(dark ? 0x606060ff : 0xe0e0e0ff, true);
    auto b = make_contrasted_color(a, 1.8);
    return argb ?
        std::make_tuple(a.toARGB(), b.toARGB()) :
        std::make_tuple(a.toRGBA(), b.toRGBA());
}

}

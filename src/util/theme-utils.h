// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef THEME_UTILS_H
#define THEME_UTILS_H

#include <cstdint>
#include <gtkmm/widget.h>

namespace Inkscape::Util {

// check background color to see if we are using a dark theme
bool is_current_theme_dark(Gtk::Widget& widget);

// checkerboard ARGB colors (background for semi-transparent drawing); dark-theme aware
std::tuple<std::uint32_t, std::uint32_t> get_checkerboard_colors(Gtk::Widget& widget, bool argb);

}

#endif //THEME_UTILS_H

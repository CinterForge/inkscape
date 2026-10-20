// SPDX-License-Identifier: GPL-2.0-or-later

#include <cairomm/context.h>
#include <cairomm/refptr.h>
#include <gdkmm/rgba.h>
#include <gtkmm/stylecontext.h>

#include <2geom/rect.h>

namespace Inkscape::Util {

// create a rectangular path with rounded corners
Geom::Rect rounded_rectangle(const Cairo::RefPtr<Cairo::Context>& ctx, const Geom::Rect& rect, double radius);

// draw a shaded border around given area using draw_path function
void draw_border_shape(const Cairo::RefPtr<Cairo::Context>& ctx, Geom::Rect rect, const Gdk::RGBA& color, int device_scale, std::function<void (const Cairo::RefPtr<Cairo::Context>&, Geom::Rect&, int)> draw_path);

// get appropriate border color for the dark / light UI theme
Gdk::RGBA get_standard_border_color(bool dark_theme);

// draw a border that stands out in a bright and dark theme
void draw_standard_border(const Cairo::RefPtr<Cairo::Context>& ctx, Geom::Rect rect, bool dark_theme, double radius, int device_scale, bool circular = false, bool inwards = true);

// find theme background color; it may not be defined on some themes
std::optional<Gdk::RGBA> lookup_background_color(Glib::RefPtr<Gtk::StyleContext>& style);

// find theme foreground color; it may not be defined on some themes
std::optional<Gdk::RGBA> lookup_foreground_color(Glib::RefPtr<Gtk::StyleContext>& style);

// find theme foreground selection color
std::optional<Gdk::RGBA> lookup_selected_foreground_color(Glib::RefPtr<Gtk::StyleContext>& style);

// find theme background selection color
std::optional<Gdk::RGBA> lookup_selected_background_color(Glib::RefPtr<Gtk::StyleContext>& style);

// find theme border color
std::optional<Gdk::RGBA> lookup_border_color(Glib::RefPtr<Gtk::StyleContext>& style);

// draw a circular marker indicating the selected point in a color wheel or similar location
void draw_point_indicator(const Cairo::RefPtr<Cairo::Context>& ctx, const Geom::Point& point, double size = 8.0);

} // namespace Inkscape::Util

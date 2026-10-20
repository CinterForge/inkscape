// SPDX-License-Identifier: GPL-2.0-or-later

#include "drawing-utils.h"

namespace Inkscape::Util {

Geom::Rect rounded_rectangle(const Cairo::RefPtr<Cairo::Context>& ctx, const Geom::Rect& rect, double radius) {
    auto x = rect.left();
    auto y = rect.top();
    auto width = rect.width();
    auto height = rect.height();
    if (radius > 0) {
        ctx->arc(x + width - radius, y + radius, radius, -M_PI_2, 0);
        ctx->arc(x + width - radius, y + height - radius, radius, 0, M_PI_2);
        ctx->arc(x + radius, y + height - radius, radius, M_PI_2, M_PI);
        ctx->arc(x + radius, y + radius, radius, M_PI, 3 * M_PI_2);
        ctx->close_path();
    }
    else {
        ctx->move_to(x, y);
        ctx->line_to(x + width, y);
        ctx->line_to(x + width, y + height);
        ctx->line_to(x, y + height);
        ctx->close_path();
    }
    return rect.shrunkBy(1);
}

void circle(const Cairo::RefPtr<Cairo::Context>& ctx, const Geom::Point& center, double radius) {
    ctx->arc(center.x(), center.y(), radius, 0, 2 * M_PI);
}

void draw_border_shape(const Cairo::RefPtr<Cairo::Context>& ctx, Geom::Rect rect, const Gdk::RGBA& color, int device_scale, std::function<void (const Cairo::RefPtr<Cairo::Context>&, Geom::Rect&, int)> draw_path) {

    if (rect.width() < 1 || rect.height() < 1) return;

    // there's one pixel overhang, so eliminate that:
    auto pix = 1.0 / device_scale;
    rect = Geom::Rect::from_xywh(rect.min().x(), rect.min().y(), rect.width() - pix, rect.height() - pix);

    ctx->save();
    // operate on physical pixels
    ctx->scale(1.0 / device_scale, 1.0 / device_scale);
    // align 1.0 wide stroke to pixel grid
    ctx->translate(0.5, 0.5);
    ctx->set_line_width(1.0);
    // radius *= device_scale;
    // shadow depth
    const int steps = 3 * device_scale;
    auto alpha = color.get_alpha();
    ctx->set_operator(Cairo::Context::Operator::OVER);
    // rect in physical pixels
    rect = Geom::Rect(rect.min() * device_scale, rect.max() * device_scale);
    for (int i = 0; i < steps; ++i) {
        draw_path(ctx, rect, i);
        ctx->set_source_rgba(color.get_red(), color.get_green(), color.get_blue(), alpha);
        ctx->stroke();
        alpha *= 0.5;
    }
    ctx->restore();
}

// draw relief around the given rect to stop colors inside blend with a background outside
void draw_border(const Cairo::RefPtr<Cairo::Context>& ctx, Geom::Rect start_rect, double radius, const Gdk::RGBA& color, int device_scale, bool circular, bool inwards) {

    radius *= device_scale;

    draw_border_shape(ctx, start_rect, color, device_scale, [&](const Cairo::RefPtr<Cairo::Context>&, Geom::Rect& rect, int) {
        if (circular) {
            circle(ctx, rect.midpoint(), rect.minExtent() / 2);
        }
        else {
            rounded_rectangle(ctx, rect, inwards ? radius-- : radius++);
        }
        rect.expandBy(inwards ? -1 : 1);
    });
}

Gdk::RGBA get_standard_border_color(bool dark_theme) {
    return dark_theme ? Gdk::RGBA(1, 1, 1, 0.25) : Gdk::RGBA(0, 0, 0, 0.25);
}

void draw_standard_border(const Cairo::RefPtr<Cairo::Context>& ctx, Geom::Rect rect, bool dark_theme, double radius, int device_scale, bool circular, bool inwards) {
    auto color = get_standard_border_color(dark_theme);
    draw_border(ctx, rect, radius, color, device_scale, circular, inwards);
}

// draw a circle around given point to show currently selected color
void draw_point_indicator(const Cairo::RefPtr<Cairo::Context>& ctx, const Geom::Point& point, double size) {
    ctx->save();

    auto pt = point;
    ctx->set_line_width(1.0);
    circle(ctx, pt, (size - 2) / 2);
    ctx->set_source_rgb(1, 1, 1);
    ctx->stroke();
    circle(ctx, pt, size / 2);
    ctx->set_source_rgb(0, 0, 0);
    ctx->stroke();

    ctx->restore();
}

static std::optional<Gdk::RGBA> lookup_theme_color(Glib::RefPtr<Gtk::StyleContext>& style, const Glib::ustring& name) {
    Gdk::RGBA color;
    if (style && style->lookup_color(name, color)) {
        return color;
    }
    return {};
}

std::optional<Gdk::RGBA> lookup_background_color(Glib::RefPtr<Gtk::StyleContext>& style) {
    return lookup_theme_color(style, "theme_bg_color");
}

std::optional<Gdk::RGBA> lookup_foreground_color(Glib::RefPtr<Gtk::StyleContext>& style) {
    return lookup_theme_color(style, "theme_fg_color");
}

std::optional<Gdk::RGBA> lookup_selected_foreground_color(Glib::RefPtr<Gtk::StyleContext>& style) {
    return lookup_theme_color(style, "theme_selected_fg_color");
}

std::optional<Gdk::RGBA> lookup_selected_background_color(Glib::RefPtr<Gtk::StyleContext>& style) {
    return lookup_theme_color(style, "theme_selected_bg_color");
}

std::optional<Gdk::RGBA> lookup_border_color(Glib::RefPtr<Gtk::StyleContext>& style) {
    return lookup_theme_color(style, "borders");
}

}

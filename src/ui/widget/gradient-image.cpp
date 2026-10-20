// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * A simple gradient preview
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "gradient-image.h"

#include <gdkmm/pixbuf.h>

#include "display/cairo-utils.h"
#include "object/sp-gradient.h"
#include "object/sp-stop.h"

static void sp_gradient_draw(SPGradient *gr, int width, int height, cairo_t *ct)
{
    if (!gr) {
        return;
    }

    auto check = ink_cairo_pattern_create_checkerboard();
    cairo_set_source(ct, check->cobj());
    cairo_paint(ct);

    auto p = gr->create_preview_pattern(width);
    if (!p) {
        return;
    }
    cairo_set_source(ct, p);
    cairo_paint(ct);
    cairo_pattern_destroy(p);
}

namespace Inkscape::UI::Widget {

GradientImage::GradientImage(SPGradient *gradient)
{
    set_name("GradientImage");
    set_draw_func(sigc::mem_fun(*this, &GradientImage::draw_func));
    set_gradient(gradient);
}

void GradientImage::draw_func(Cairo::RefPtr<Cairo::Context> const &cr, int width, int height)
{
    sp_gradient_draw(_gradient, width, height, cr->cobj());
}

void GradientImage::set_gradient(SPGradient *gradient)
{
    if (_gradient == gradient) {
        return;
    }

    if (_gradient) {
        _release_connection.disconnect();
        _modified_connection.disconnect();
    }

    _gradient = gradient;

    if (gradient) {
        _release_connection = gradient->connectRelease([this] (auto) {
            set_gradient(nullptr);
        });
        _modified_connection = gradient->connectModified([this] (auto, auto) {
            queue_draw();
        });
    }

    queue_draw();
}

} // namespace Inkscape::UI::Widget

Glib::RefPtr<Gdk::Pixbuf> sp_gradient_to_pixbuf(SPGradient *gr, int width, int height)
{
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t *ct = cairo_create(s);
    sp_gradient_draw(gr, width, height, ct);
    cairo_destroy(ct);
    cairo_surface_flush(s);

    // no need to free s - the call below takes ownership
    return Glib::wrap(ink_pixbuf_create_from_cairo_surface(s));
}

Cairo::RefPtr<Cairo::ImageSurface> sp_gradient_to_surface(SPGradient* gr, int width, int height) {
    auto surface = Cairo::ImageSurface::create(Cairo::ImageSurface::Format::ARGB32, width, height);
    auto ctx = Cairo::Context::create(surface);
    sp_gradient_draw(gr, width, height, ctx->cobj());
    surface->flush();

    return surface;
}

Cairo::RefPtr<Cairo::ImageSurface> sp_gradstop_to_surface(SPStop *stop, int width, int height)
{
    auto surface = Cairo::ImageSurface::create(Cairo::ImageSurface::Format::ARGB32, width, height);
    auto ctx = Cairo::Context::create(surface);
    cairo_t *ct = ctx->cobj();

    /* Checkerboard background */
    auto check = ink_cairo_pattern_create_checkerboard();
    cairo_rectangle(ct, 0, 0, width, height);
    cairo_set_source(ct, check->cobj());
    cairo_fill_preserve(ct);

    if (stop) {
        /* Alpha area */
        cairo_rectangle(ct, 0, 0, width/2, height);
        ink_cairo_set_source_color(ct, stop->getColor());
        cairo_fill(ct);

        /* Solid area */
        auto no_alpha = stop->getColor();
        no_alpha.enableOpacity(false);
        cairo_rectangle(ct, width/2, 0, width, height);
        ink_cairo_set_source_color(ct, no_alpha);
        cairo_fill(ct);
    }

    surface->flush();

    return surface;
}

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :

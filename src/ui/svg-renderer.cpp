// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * SVG to Pixbuf renderer
 *
 * Author:
 *   Michael Kowalski
 *
 * Copyright (C) 2020-2021 Michael Kowalski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "svg-renderer.h"

#include <giomm/file.h>
#include <gdkmm/pixbuf.h>

#include "document.h"
#include "display/cairo-utils.h"
#include "helper/pixbuf-ops.h"
#include "io/file.h"
#include "util/units.h"

namespace Inkscape {

template <typename T>
static auto ensure_nonnull(T &&t, char const *message)
{
    if (!t) {
        throw std::runtime_error{message};
    }
    return std::forward<T>(t);
}

svg_renderer::svg_renderer(SPDocument &document)
    : _document{document}
    , _root{*ensure_nonnull(_document.getRoot(), "Cannot find root element in svg document")}
{}

svg_renderer::svg_renderer(char const *path)
    : _optional_storage{ensure_nonnull(ink_file_open(Gio::File::create_for_path(path)).first, "Cannot load svg document")}
    , _document{*_optional_storage}
    , _root{*ensure_nonnull(_document.getRoot(), "Cannot find root element in svg document")}
{}

svg_renderer::~svg_renderer() = default;

size_t svg_renderer::set_style(const Glib::ustring& selector, const char* name, const Glib::ustring& value) {
    auto objects = _document.getObjectsBySelector(selector);
    for (auto el : objects) {
        if (SPCSSAttr* css = sp_repr_css_attr(el->getRepr(), "style")) {
            sp_repr_css_set_property(css, name, value.c_str());
            el->changeCSS(css, "style");
            sp_repr_css_attr_unref(css);
        }
    }
    return objects.size();
}

double svg_renderer::get_width_px() const {
    return _document.getWidth().value("px");
}

double svg_renderer::get_height_px() const {
    return _document.getHeight().value("px");
}

Inkscape::Pixbuf* svg_renderer::do_render(double device_scale) {
    auto dpi = 96 * device_scale * _scale;
    auto area = *_document.preferredBounds();

    auto checkerboard_ptr = _checkerboard ? &*_checkerboard : nullptr;
    return sp_generate_internal_bitmap(&_document, area, dpi, {}, false, checkerboard_ptr, device_scale);
}

Glib::RefPtr<Gdk::Pixbuf> svg_renderer::render(double scale) {
    auto pixbuf = do_render(scale);
    if (!pixbuf) return {};

    // ref it
    auto raw = Glib::wrap(pixbuf->getPixbufRaw(), true);
    delete pixbuf;
    return raw;
}

Cairo::RefPtr<Cairo::ImageSurface> svg_renderer::render_surface(double scale) {
    auto pixbuf = do_render(scale);
    if (!pixbuf) return Cairo::RefPtr<Cairo::ImageSurface>();

    // ref it by saying that we have no reference
    auto surface = Cairo::RefPtr<Cairo::ImageSurface>(new Cairo::ImageSurface(pixbuf->getSurfaceRaw(), false));
    delete pixbuf;
    return surface;
}

void svg_renderer::set_scale(double scale) {
    if (scale > 0) {
        _scale = scale;
    }
}

} // namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim:filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99:

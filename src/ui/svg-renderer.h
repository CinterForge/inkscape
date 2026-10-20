// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SEEN_SVG_RENDERER_H
#define SEEN_SVG_RENDERER_H

#include <cairomm/surface.h>
#include <glibmm/refptr.h>
#include <cstdint>
#include <optional>

class SPDocument;
class SPRoot;

namespace Cairo {
class Surface;
} // namespace Cairo

namespace Glib {
class ustring;
} // namespace Glib

namespace Gdk {
class Pixbuf;
} // namespace Gdk

namespace Inkscape {

class Pixbuf;

class svg_renderer
{
public:
    // load SVG document from file (abs path)
    explicit svg_renderer(char const *path);

    // pass in document to render
    explicit svg_renderer(SPDocument &document);

    ~svg_renderer();

    // set inline style on selected elements; return number of elements modified
    size_t set_style(const Glib::ustring& selector, const char* name, const Glib::ustring& value);

    // render document at given scale
    Glib::RefPtr<Gdk::Pixbuf> render(double scale);
    Cairo::RefPtr<Cairo::ImageSurface> render_surface(double scale);

    // if set, draw checkerboard pattern before image
    void set_checkerboard_color(uint32_t rgba) { _checkerboard = rgba; }

    // set requested scale, by default it is 1.0
    void set_scale(double scale);

    // get document size
    double get_width_px() const;
    double get_height_px() const;

private:
    Pixbuf *do_render(double device_scale);

    std::unique_ptr<SPDocument> const _optional_storage;
    SPDocument &_document;
    SPRoot &_root;

    std::optional<uint32_t> _checkerboard;
    double _scale = 1.0;
};

} // namespace Inkscape

#endif // SEEN_SVG_RENDERER_H

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

// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef INKSCAPE_UI_WIDGET_CANVAS_TEXTURE_H
#define INKSCAPE_UI_WIDGET_CANVAS_TEXTURE_H

#include <2geom/point.h>
#include <epoxy/gl.h>

namespace Inkscape::UI::Widget {

class Texture
{
public:
    // Create null texture owning no resources.
    Texture() = default;

    // Allocate a blank texture of a given size. The texture is bound to GL_TEXTURE_2D.
    Texture(Geom::IntPoint const &size);

    // Wrap an existing texture.
    Texture(GLuint id, Geom::IntPoint const &size) : _id(id), _size(size) {}

    // Boilerplate constructors/operators
    Texture(Texture &&other) noexcept { _movefrom(other); }
    Texture &operator=(Texture &&other) noexcept { _reset(); _movefrom(other); return *this; }
    ~Texture() { _reset(); }

    // Observers
    GLuint id() const { return _id; }
    Geom::IntPoint const &size() const { return _size; }
    explicit operator bool() const { return _id; }

    // Methods
    void clear() { _reset(); _id = 0; }
    void invalidate();

private:
    GLuint _id = 0;
    Geom::IntPoint _size;

    void _reset() noexcept { if (_id) glDeleteTextures(1, &_id); }
    void _movefrom(Texture &other) noexcept { _id = other._id; _size = other._size; other._id = 0; }
};

} // namespace Inkscape::UI::Widget

#endif // INKSCAPE_UI_WIDGET_CANVAS_TEXTURE_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

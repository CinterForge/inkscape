// SPDX-License-Identifier: GPL-2.0-or-later

#include "texture.h"

namespace Inkscape::UI::Widget {

static bool have_gltexstorage()
{
    static bool const result = epoxy_gl_version() >= 42 || epoxy_has_gl_extension("GL_ARB_texture_storage");
    return result;
}

static bool have_glinvalidateteximage()
{
    static bool const result = epoxy_gl_version() >= 43 || epoxy_has_gl_extension("ARB_invalidate_subdata");
    return result;
}

Texture::Texture(Geom::IntPoint const &size)
    : _size(size)
{
    glGenTextures(1, &_id);
    glBindTexture(GL_TEXTURE_2D, _id);

    // Common flags for all textures used at the moment.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    if (have_gltexstorage()) {
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, size.x(), size.y());
    } else {
        // Note: This fallback path is always chosen on the Mac due to Apple's crippling of OpenGL.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.x(), size.y(), 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
    }
}

void Texture::invalidate()
{
    if (have_glinvalidateteximage()) {
        glInvalidateTexImage(_id, 0);
    }
}

} // namespace Inkscape::UI::Widget

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

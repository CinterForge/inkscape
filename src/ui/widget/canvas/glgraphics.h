// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * OpenGL display backend.
 * Copyright (C) 2022 PBS <pbs3141@gmail.com>
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_WIDGET_CANVAS_GLGRAPHICS_H
#define INKSCAPE_UI_WIDGET_CANVAS_GLGRAPHICS_H

#include <memory>
#include <mutex>
#include <boost/noncopyable.hpp>
#include <epoxy/gl.h>

#include "graphics.h"
#include "texturecache.h"

namespace Inkscape::UI::Widget {

class Stores;
class Prefs;
class PixelStreamer;

template <GLuint type>
struct Shader : boost::noncopyable
{
    GLuint id{};
    Shader(char const *src) { id = glCreateShader(type); glShaderSource(id, 1, &src, nullptr); glCompileShader(id); }
    ~Shader() { glDeleteShader(id); }
};
using GShader = Shader<GL_GEOMETRY_SHADER>;
using VShader = Shader<GL_VERTEX_SHADER>;
using FShader = Shader<GL_FRAGMENT_SHADER>;

struct Program : boost::noncopyable
{
    GLuint id = 0;
    void create(VShader const &v,                   FShader const &f) { id = glCreateProgram(); glAttachShader(id, v.id);                           glAttachShader(id, f.id); glLinkProgram(id); }
    void create(VShader const &v, const GShader &g, FShader const &f) { id = glCreateProgram(); glAttachShader(id, v.id); glAttachShader(id, g.id); glAttachShader(id, f.id); glLinkProgram(id); }
    auto loc(char const *str) const { return glGetUniformLocation(id, str); }
    ~Program() { glDeleteProgram(id); }
};

class VAO
{
public:
    GLuint vao = 0;
    GLuint vbuf{};

    VAO() = default;
    VAO(GLuint vao, GLuint vbuf) : vao(vao), vbuf(vbuf) {}
    VAO(VAO &&other) noexcept { movefrom(other); }
    VAO &operator=(VAO &&other) noexcept { reset(); movefrom(other); return *this; }
    ~VAO() { reset(); }

private:
    void reset() noexcept { if (vao) { glDeleteVertexArrays(1, &vao); glDeleteBuffers(1, &vbuf); } }
    void movefrom(VAO &other) noexcept { vao = other.vao; vbuf = other.vbuf; other.vao = 0; }
};

struct GLFragment
{
    Texture texture;
    Texture outline_texture;
};

class GLGraphics : public Graphics
{
public:
    GLGraphics(Prefs const &prefs, Stores const &stores, PageInfo const &pi);
    ~GLGraphics() override;

    void set_scale_factor(int scale) override { scale_factor = scale; }
    void set_outlines_enabled(bool) override;
    void set_background_in_stores(bool enabled) override { background_in_stores = enabled; }
    void set_colours(uint32_t p, uint32_t d, uint32_t b) override { page = p; desk = d; shadow_color = b; }
    void set_shadow_size(float size) override { shadow_size = size; }

    void recreate_store(Geom::IntPoint const &dimensions) override;
    void shift_store(Fragment const &dest) override;
    void swap_stores() override;
    void fast_snapshot_combine() override;
    void snapshot_combine(Fragment const &dest) override;
    void invalidate_snapshot() override;

    bool is_opengl() const override { return true; }
    void invalidated_glstate() override { state = State::None; }

    Cairo::RefPtr<Cairo::ImageSurface> request_tile_surface(Geom::IntRect const &rect, bool nogl) override;
    void draw_tile(Fragment const &fragment, Cairo::RefPtr<Cairo::ImageSurface> surface, Cairo::RefPtr<Cairo::ImageSurface> outline_surface) override;
    void junk_tile_surface(Cairo::RefPtr<Cairo::ImageSurface> surface) override;

    void paint_widget(Fragment const &view, PaintArgs const &args, Cairo::RefPtr<Cairo::Context> const &cr) override;

private:
    // Drawn content.
    GLFragment store, snapshot;

    // OpenGL objects.
    VAO rect; // Rectangle vertex data.
    Program checker, shadow, texcopy, texcopydouble, outlineoverlay, xray, outlineoverlayxray; // Shaders
    GLuint fbo{}; // Framebuffer object for rendering to stores.

    // Pixel streamer and texture cache for uploading pixel data to GPU.
    std::unique_ptr<PixelStreamer> pixelstreamer;
    std::unique_ptr<TextureCache> texturecache;
    std::mutex ps_mutex;

    // For preventing unnecessary pipeline recreation.
    enum class State { None, Widget, Stores, Tiles };
    State state = State::None;
    void setup_stores_pipeline();
    void setup_tiles_pipeline();
    void setup_widget_pipeline(Fragment const &view);

    // For caching frequently-used uniforms.
    GLuint mat_loc{}, trans_loc{}, subrect_loc{}, tex_loc{}, texoutline_loc{};

    // Dependency objects in canvas.
    Prefs const &prefs;
    Stores const &stores;
    PageInfo const &pi;

    // Backend-agnostic state.
    int scale_factor = 1;
    bool outlines_enabled = false;
    bool background_in_stores = false;
    uint32_t page{}, desk{}, shadow_color{};
    float shadow_size = 0;
};

} // namespace Inkscape::UI::Widget

#endif // INKSCAPE_UI_WIDGET_CANVAS_GLGRAPHICS_H

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

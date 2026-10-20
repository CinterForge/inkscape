// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef EXTENSION_INTERNAL_CAIRO_RENDER_CONTEXT_H_SEEN
#define EXTENSION_INTERNAL_CAIRO_RENDER_CONTEXT_H_SEEN

/** \file
 * Declaration of CairoRenderContext, a class used for rendering with Cairo.
 */
/*
 * Authors:
 *     Miklos Erdelyi <erdelyim@gmail.com>
 *
 * Copyright (C) 2006 Miklos Erdelyi
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "extension/extension.h"
#include <set>
#include <string>

#include <2geom/forward.h>
#include <2geom/affine.h>

#include "style-internal.h" // SPIEnum

#include <cairo.h>

class SPClipPath;
class SPMask;

typedef struct _PangoFont PangoFont;
typedef struct _PangoLayout PangoLayout;

namespace Inkscape {
class Pixbuf;

namespace Extension::Internal {

class CairoRenderer;
class CairoRenderContext;
struct CairoRenderState;

// Holds info for rendering a glyph
struct CairoGlyphInfo {
    unsigned long index;
    double x;
    double y;
};

struct CairoRenderState {
    unsigned merge_opacity        : 1 = true;   ///< whether fill/stroke opacity can be mul'd with item opacity
    unsigned need_layer           : 1 = false;  ///< whether object is masked, clipped, and/or has a non-zero opacity
    unsigned has_overflow         : 1 = false;
    unsigned parent_has_userspace : 1 = false;  ///< whether the parent's ctm should be applied
    unsigned has_filtereffect     : 1 = false;

    float opacity = 1.0;
    Geom::Affine item_transform;  ///< this item's item->transform, for correct clipping

    SPClipPath *clip_path = nullptr;
    SPMask* mask = nullptr;

    Geom::Affine transform;  /// the current transform matrix
};

class CairoRenderContext {
    friend class CairoRenderer;
public:
    // Constructor is private: only a CairoRenderer can create a new context.
    ~CairoRenderContext();

    CairoRenderContext(CairoRenderContext const &other) = delete; // We hold a FILE handle
    CairoRenderContext(CairoRenderContext &&other);

    CairoRenderContext &operator=(CairoRenderContext const &other) = delete;
    CairoRenderContext &operator=(CairoRenderContext &&other);

    /* Rendering methods */
    enum CairoPaintOrder {
        STROKE_OVER_FILL,
        FILL_OVER_STROKE,
        FILL_ONLY,
        STROKE_ONLY
    };

    enum CairoRenderMode {
        RENDER_MODE_NORMAL,
        RENDER_MODE_CLIP
    };

    enum CairoClipMode {
        CLIP_MODE_PATH,
        CLIP_MODE_MASK
    };

    CairoRenderContext createSimilar(double width, double height) const;
    bool finish(bool finish_surface = true);
    bool finishPage();
    bool nextPage(double width, double height, char const *label);

    CairoRenderer *getRenderer() const { return _renderer; }

    bool setImageTarget(cairo_format_t format);
    bool setPdfTarget(gchar const *utf8_fn);
    bool setPsTarget(gchar const *utf8_fn);
    /** Set the cairo_surface_t from an external source */
    bool setSurfaceTarget(cairo_surface_t *surface, bool is_vector, cairo_matrix_t *ctm=nullptr);

    /// Extract metadata from the document and store it in the context.
    void setMetadata(SPDocument const &document);

    void setPSLevel(unsigned int level);
    void setEPS(bool eps) { _eps = eps; }
    void setPDFLevel(unsigned int level);
    void setTextToPath(bool texttopath) { _is_texttopath = texttopath; }
    void setOmitText(bool omittext) { _is_omittext = omittext; }
    void setFilterToBitmap(bool filtertobitmap) { _is_filtertobitmap = filtertobitmap; }
    bool getFilterToBitmap() { return _is_filtertobitmap; }
    void setBitmapResolution(unsigned resolution) { _bitmapresolution = resolution; }
    unsigned getBitmapResolution() { return _bitmapresolution; }

    /** Creates the cairo_surface_t for the context with the
    given width, height and with the currently set target
    surface type. Also sets supported metadata on the surface. */
    bool setupSurface(double width, double height);

    cairo_surface_t *getSurface();

    /** Saves the contents of the context to a PNG file. */
    bool saveAsPng(const char *file_name);

    /** On targets supporting multiple pages, sends subsequent rendering to a new page*/
    void newPage();

    /* Render/clip mode setting/query */
    void setRenderMode(CairoRenderMode mode);
    CairoRenderMode getRenderMode() const { return _render_mode; }
    void setClipMode(CairoClipMode mode);
    CairoClipMode getClipMode() const { return _clip_mode; }

    void addPathVector(Geom::PathVector const &pv);
    void setPathVector(Geom::PathVector const &pv);

    void pushLayer();
    void popLayer(cairo_operator_t composite = CAIRO_OPERATOR_CLEAR);

    void tagBegin(const char* link);
    void tagEnd();
    void destBegin(const char* link);
    void destEnd();

    /* Graphics state manipulation */
    void pushState();
    void popState();
    const CairoRenderState *getCurrentState() const { return &_state_stack.back(); }
    const CairoRenderState *getParentState() const;
    void setStateForStyle(SPStyle const *style);
    void setStateForItem(SPItem const *item);
    void setStateNeedsLayer(bool state_needs_layer) { _state_stack.back().need_layer = state_needs_layer; }
    void setStateMergeOpacity(bool state_merge_opacity) { _state_stack.back().merge_opacity = state_merge_opacity; }

    void transform(Geom::Affine const &transform);
    void setTransform(Geom::Affine const &transform);
    void setItemTransform(Geom::Affine const &transform);
    Geom::Affine getTransform() const;
    Geom::Affine getItemTransform() const;
    Geom::Affine getParentTransform() const;

    /* Clipping methods */
    void addClipPath(Geom::PathVector const &pv, SPIEnum<SPWindRule> const *fill_rule);
    void addClippingRect(double x, double y, double width, double height);

    bool renderPathVector(Geom::PathVector const &pathv, SPStyle const *style, Geom::OptRect const &pbox, CairoPaintOrder order = STROKE_OVER_FILL);
    bool renderImage(Inkscape::Pixbuf const *pb,
                     Geom::Affine const &image_transform, SPStyle const *style);
    bool renderGlyphtext(PangoFont *font, Geom::Affine const &font_matrix,
                         std::vector<CairoGlyphInfo> const &glyphtext, SPStyle const *style,
                         bool second_pass = false);

    /* More general rendering methods will have to be added (like fill, stroke) */

private:
    CairoRenderContext(CairoRenderer *renderer);
    enum class OmitTextPageState {
        EMPTY,
        GRAPHIC_ON_TOP,
        NEW_PAGE_ON_GRAPHIC
    };

    float _width = 0.0;
    float _height = 0.0;
    unsigned _dpi = 72;
    unsigned int _pdf_level = 1;
    unsigned int _ps_level = 1;
    unsigned _bitmapresolution = 72;

    bool _is_valid          : 1 = false;
    bool _eps               : 1 = false;
    bool _is_texttopath     : 1 = false;
    bool _is_omittext       : 1 = false;
    bool _is_show_page      : 1 = false;
    bool _is_filtertobitmap : 1 = false;
    // If both ps and pdf are false, then we are printing.
    bool _is_pdf : 1 = false;
    bool _is_ps  : 1 = false;

    unsigned int _clip_rule : 8;
    unsigned int _clip_winding_failed : 1;
    unsigned int _vector_based_target : 1 = false;
    OmitTextPageState _omittext_state = OmitTextPageState::EMPTY;

    FILE *_stream = nullptr;

    cairo_t *_cr = nullptr; // Cairo context
    cairo_surface_t *_surface = nullptr;
    cairo_surface_type_t _target = CAIRO_SURFACE_TYPE_IMAGE;
    cairo_format_t _target_format = CAIRO_FORMAT_ARGB32;

    PangoLayout *_layout = nullptr;
    std::vector<CairoRenderState> _state_stack;
    CairoRenderer *_renderer;

    CairoRenderMode _render_mode = RENDER_MODE_NORMAL;
    CairoClipMode _clip_mode = CLIP_MODE_MASK;

    // Metadata to set on the cairo surface (if the surface supports it)
    struct CairoRenderContextMetadata {
        Glib::ustring title;
        Glib::ustring author;
        Glib::ustring subject;
        Glib::ustring keywords;
        Glib::ustring copyright;
        Glib::ustring creator;
        Glib::ustring cdate; // currently unused
        Glib::ustring mdate; // currently unused
    } _metadata;

    cairo_pattern_t *_createPatternForPaintServer(SPPaintServer const *const paintserver,
                                                  Geom::OptRect const &pbox, float alpha);
    cairo_pattern_t *_createPatternPainter(SPPaintServer const *const paintserver, Geom::OptRect const &pbox);
    cairo_pattern_t *_createHatchPainter(SPPaintServer const *const paintserver, Geom::OptRect const &pbox);

    unsigned int _showGlyphs(cairo_t *cr, PangoFont *font, std::vector<CairoGlyphInfo> const &glyphtext, bool is_stroke);

    bool _finishSurfaceSetup(cairo_surface_t *surface, cairo_matrix_t *ctm = nullptr);
    void _setSurfaceMetadata(cairo_surface_t *surface);

    void _setFillStyle(SPStyle const *style, Geom::OptRect const &pbox);
    void _setStrokeStyle(SPStyle const *style, Geom::OptRect const &pbox);
    float _mergedOpacity(float source_opacity) const;

    void _concatTransform(cairo_t *cr, double xx, double yx, double xy, double yy, double x0, double y0);
    void _concatTransform(cairo_t *cr, Geom::Affine const &transform);

    void _prepareRenderGraphic();
    void _prepareRenderText();

    void _freeResources();

    template <cairo_surface_type_t type>
    bool _setVectorTarget(gchar const *utf8_fn);

    std::map<gpointer, cairo_font_face_t *> _font_table;
    static void font_data_free(gpointer data);

    CairoRenderState *_addState() { return &_state_stack.emplace_back(); }
};

}  // namespace Extension::Internal
}  // namespace Inkscape

#endif  // !EXTENSION_INTERNAL_CAIRO_RENDER_CONTEXT_H_SEEN

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

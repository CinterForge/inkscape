// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef EXTENSION_INTERNAL_CAIRO_RENDERER_H_SEEN
#define EXTENSION_INTERNAL_CAIRO_RENDERER_H_SEEN

/** \file
 * Declaration of CairoRenderer, a class used for rendering via a CairoRenderContext.
 */
/*
 * Authors:
 * 	   Miklos Erdelyi <erdelyim@gmail.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2006 Miklos Erdelyi
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "extension/extension.h"
#include <set>
#include <string>

//#include "libnrtype/font-instance.h"
#include <cairo.h>

class SPItem;
class SPClipPath;
class SPMask;
class SPHatchPath;
class SPPage;

namespace Inkscape {
namespace Extension {
namespace Internal {

class CairoRenderer;
class CairoRenderContext;

class CairoRenderer {
public:
    CairoRenderer();
    ~CairoRenderer();

    CairoRenderContext createContext();

    void applyClipPath(CairoRenderContext *ctx, SPClipPath const *cp);
    void applyMask(CairoRenderContext *ctx, SPMask const *mask);

    /** Initializes the CairoRenderContext according to the specified
    SPDocument. A set*Target function can only be called on the context
    before setupDocument. */
    bool setupDocument(CairoRenderContext *ctx, SPDocument *doc, SPItem const *base = nullptr);


    /** Traverses the object tree and invokes the render methods. */
    void renderItem(CairoRenderContext *ctx, SPItem const *item, SPItem const *origin = nullptr, SPPage const *page = nullptr);
    void renderHatchPath(CairoRenderContext *ctx, SPHatchPath const &hatchPath, unsigned key);
    bool renderPages(CairoRenderContext *ctx, SPDocument *doc, bool stretch_to_fit);
    bool renderPage(CairoRenderContext *ctx, SPDocument *doc, SPPage const *page, bool stretch_to_fit);

private:
    /** Decide whether the given item should be rendered as a bitmap. */
    static bool _shouldRasterize(CairoRenderContext *ctx, SPItem const *item);

    /** Render a single item in a fully set up context. */
    static void _doRender(SPItem const *item, CairoRenderContext *ctx, SPItem const *origin = nullptr,
                          SPPage const *page = nullptr);

};

// FIXME: this should be a static method of CairoRenderer
void calculatePreserveAspectRatio(unsigned int aspect_align, unsigned int aspect_clip, double vp_width,
                                  double vp_height, double *x, double *y, double *width, double *height);

}  /* namespace Internal */
}  /* namespace Extension */
}  /* namespace Inkscape */

#endif /* !EXTENSION_INTERNAL_CAIRO_RENDERER_H_SEEN */

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

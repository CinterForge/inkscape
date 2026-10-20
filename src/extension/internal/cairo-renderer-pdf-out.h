// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * A quick hack to use the Cairo renderer to write out a file.  This
 * then makes 'save as...' PDF.
 *
 * Authors:
 *   Ted Gould <ted@gould.cx>
 *   Ulf Erikson <ulferikson@users.sf.net>
 *
 * Copyright (C) 2004-2006 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef EXTENSION_INTERNAL_CAIRO_RENDERER_PDF_OUT_H
#define EXTENSION_INTERNAL_CAIRO_RENDERER_PDF_OUT_H

#include "extension/implementation/implementation.h"

namespace Inkscape {
namespace Extension {
namespace Internal {

class CairoRendererPdfOutput : public Inkscape::Extension::Implementation::Implementation {

public:
    bool check(Inkscape::Extension::Extension *module) override;
    void save(Inkscape::Extension::Output *mod,
              SPDocument *doc,
              gchar const *filename) override;
    static void init();
};

struct PDFOptions {
    bool text_to_path      : 1; ///< Convert text to paths?
    bool text_to_latex     : 1; ///< Put text in a LaTeX document?
    bool rasterize_filters : 1; ///< Rasterize filter effects?
    bool drawing_only      : 1; ///< Set page size to drawing + margin instead of document page.
    bool stretch_to_fit    : 1; ///< Compensate for Cairo's page size rounding to integers (in pt)?
};

} } }  /* namespace Inkscape, Extension, Internal */

#endif /* !EXTENSION_INTERNAL_CAIRO_RENDERER_PDF_OUT_H */

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

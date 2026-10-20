// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Build PDF text elements
 *
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2024 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef EXTENSION_INTERNAL_PDFOUTPUT_BUILD_TEXT_H
#define EXTENSION_INTERNAL_PDFOUTPUT_BUILD_TEXT_H

#include "build-document.h"
#include "libnrtype/Layout-TNG.h"

namespace Inkscape::Extension::Internal::PdfBuilder {

class TextContext
{
public:
    TextContext(Document &doc, capypdf::DrawContext &ctx, bool soft_mask);

    bool set_text_style(std::shared_ptr<FontInstance> const &font, SPStyle *style);
    void set_paint_style(StyleMap const &map, SPStyle const *style, SPStyle const *context_style);
    void set_text_mode(CapyPDF_Text_Mode mode);
    void render_text(Text::Layout const &layout, Text::Layout::Span const &span);
    void finalize();

protected:
    std::optional<double> get_softmask(double opacity) const;

private:
    Document &_doc;
    capypdf::DrawContext &_ctx;
    capypdf::Text _tx;
    bool _soft_mask;

    // Text style memory
    std::string last_font;
    double last_letter_spacing = 0;
    double last_ca = 1.0;
    double last_CA = 1.0;
    CapyPDF_Text_Mode last_text_mode = CAPY_TEXT_FILL;
};

} // namespace Inkscape::Extension::Internal::PdfBuilder

#endif /* !EXTENSION_INTERNAL_PDFOUTPUT_BUILD_TEXT_H */

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

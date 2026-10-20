// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SVG <hkern> and <vkern> elements implementation
 *
 * Authors:
 *    Felipe C. da S. Sanches <juca@members.fsf.org>
 *
 * Copyright (C) 2008 Felipe C. da S. Sanches
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_SP_GLYPH_KERNING_H
#define SEEN_SP_GLYPH_KERNING_H

#include "sp-object.h"
#include "unicoderange.h"

class GlyphNames {
public: 
    GlyphNames(char const* value);
    ~GlyphNames();
    bool contains(char const* name);
private:
    char* names;
};

class SPGlyphKerning : public SPObject {
public:
    SPGlyphKerning();
    ~SPGlyphKerning() override = default;
    int tag() const override { return tag_of<decltype(*this)>; }

    // FIXME encapsulation
    UnicodeRange* u1;
    GlyphNames* g1;
    UnicodeRange* u2;
    GlyphNames* g2;
    double k;

protected:
    void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
    void release() override;
    void set(SPAttr key, char const* value) override;
    void update(SPCtx* ctx, unsigned int flags) override;
    Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, unsigned int flags) override;
};

class SPHkern final : public SPGlyphKerning {
    ~SPHkern() override = default;
    int tag() const override { return tag_of<decltype(*this)>; }
};

class SPVkern final : public SPGlyphKerning {
    ~SPVkern() override = default;
    int tag() const override { return tag_of<decltype(*this)>; }
};

#endif // !SEEN_SP_GLYPH_KERNING_H

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

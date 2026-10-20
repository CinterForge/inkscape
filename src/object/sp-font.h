// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SP_FONT_H_SEEN
#define SP_FONT_H_SEEN

/*
 * SVG <font> element implementation
 *
 * Authors:
 *    Felipe C. da S. Sanches <juca@members.fsf.org>
 *
 * Copyright (C) 2008 Felipe C. da S. Sanches
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-object.h"
class SPGlyph;

class SPFont final : public SPObject {
public:
    SPFont();
    ~SPFont() override;
    int tag() const override { return tag_of<decltype(*this)>; }

    double horiz_origin_x;
    double horiz_origin_y;
    double horiz_adv_x;
    double vert_origin_x;
    double vert_origin_y;
    double vert_adv_y;

    // add new glyph to the font with optional name and given unicode string (code point, or code points for the glyph)
    SPGlyph* create_new_glyph(const char* name, const char* unicode);

    // sort glyphs in the font by "unicode" attribute (code points)
    void sort_glyphs();

protected:
    void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
    void release() override;

    void child_added(Inkscape::XML::Node* child, Inkscape::XML::Node* ref) override;
    void remove_child(Inkscape::XML::Node* child) override;

    void set(SPAttr key, char const* value) override;

    void update(SPCtx* ctx, unsigned int flags) override;

    Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, unsigned int flags) override;

private:
    bool _block = false;
};

#endif //#ifndef SP_FONT_H_SEEN

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

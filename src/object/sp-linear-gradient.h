// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SP_LINEAR_GRADIENT_H
#define SP_LINEAR_GRADIENT_H

/** \file
 * SPLinearGradient: SVG <lineargradient> implementation
 */

#include "sp-gradient.h"
#include "svg/svg-length.h"
#include <2geom/line.h>

/** Linear gradient. */
class SPLinearGradient final
    : public SPGradient
{
public:
    SPLinearGradient();
    ~SPLinearGradient() override;
    int tag() const override { return tag_of<decltype(*this)>; }

    SVGLength x1;
    SVGLength y1;
    SVGLength x2;
    SVGLength y2;

    // Walk up the reference tree to find values.
    SVGLength find_x1() const { return find_attr_length(this, &SPLinearGradient::x1); }
    SVGLength find_y1() const { return find_attr_length(this, &SPLinearGradient::y1); }
    SVGLength find_x2() const { return find_attr_length(this, &SPLinearGradient::x2); }
    SVGLength find_y2() const { return find_attr_length(this, &SPLinearGradient::y2); }

    PaintServerType getPaintType() const override { return PaintServerType::LINEAR_GRADIENT; }
    std::vector<double> getGradientGeom() const override;

    Geom::Line getLine() const {
        return Geom::Line({x1.computed, y1.computed}, {x2.computed, y2.computed});
    }

protected:
    void build(SPDocument *document, Inkscape::XML::Node *repr) override;
    void set(SPAttr key, char const *value) override;
    void update(SPCtx *ctx, guint flags) override;
    Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;
};

#endif /* !SP_LINEAR_GRADIENT_H */

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

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief SVG color matrix filter effect
 *//*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SP_FECOLORMATRIX_H_SEEN
#define SP_FECOLORMATRIX_H_SEEN

#include <vector>
#include "sp-filter-primitive.h"
#include "display/nr-filter-colormatrix.h"

class SPFeColorMatrix final
    : public SPFilterPrimitive
{
public:
    int tag() const override { return tag_of<decltype(*this)>; }

    Inkscape::Filters::FilterColorMatrixType get_type() const { return type; }
    std::vector<double> const &get_values() const { return values; }
    double get_value() const { return value; }

private:
    Inkscape::Filters::FilterColorMatrixType type = Inkscape::Filters::COLORMATRIX_MATRIX;
    double value = 0.0;
    std::vector<double> values;
    bool value_set = false;

protected:
	void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
    void set(SPAttr key, char const *value) override;

    std::unique_ptr<Inkscape::Filters::FilterPrimitive> build_renderer(Inkscape::DrawingItem *item) const override;
};

#endif // SP_FECOLORMATRIX_H_SEEN

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

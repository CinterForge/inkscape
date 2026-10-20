// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief SVG matrix convolution filter effect
 */
/*
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SP_FECONVOLVEMATRIX_H_SEEN
#define SP_FECONVOLVEMATRIX_H_SEEN

#include <vector>
#include "sp-filter-primitive.h"
#include "number-opt-number.h"
#include "display/nr-filter-convolve-matrix.h"

class SPFeConvolveMatrix final
    : public SPFilterPrimitive
{
public:
    int tag() const override { return tag_of<decltype(*this)>; }

    NumberOptNumber get_order() const { return order; }
    std::vector<double> const &get_kernel_matrix() const { return kernelMatrix; }

private:
    double bias = 0.0;
    Inkscape::Filters::FilterConvolveMatrixEdgeMode edgeMode = Inkscape::Filters::CONVOLVEMATRIX_EDGEMODE_DUPLICATE;
    bool preserveAlpha = false;

    double divisor = 0.0;
    int targetX = 1;
    int targetY = 1;
    std::vector<double> kernelMatrix;

    bool divisorIsSet = false;
    bool targetXIsSet = false;
    bool targetYIsSet = false;
    bool kernelMatrixIsSet = false;

    NumberOptNumber order = NumberOptNumber(3, 3);
    NumberOptNumber kernelUnitLength;

protected:
    void build(SPDocument *doc, Inkscape::XML::Node *repr) override;
    void set(SPAttr key, char const *value) override;

    std::unique_ptr<Inkscape::Filters::FilterPrimitive> build_renderer(Inkscape::DrawingItem *item) const override;
};

#endif // SP_FECONVOLVEMATRIX_H_SEEN

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

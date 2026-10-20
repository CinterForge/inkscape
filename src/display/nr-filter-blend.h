// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_NR_FILTER_BLEND_H
#define SEEN_NR_FILTER_BLEND_H

/*
 * SVG feBlend renderer
 *
 * "This filter composites two objects together using commonly used
 * imaging software blending modes. It performs a pixel-wise combination
 * of two input images."
 * http://www.w3.org/TR/SVG11/filters.html#feBlend
 *
 * Authors:
 *   Niko Kiirala <niko@kiirala.com>
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include <set>
#include "style-enums.h"
#include "display/nr-filter-primitive.h"

namespace Inkscape {
namespace Filters {

class FilterBlend : public FilterPrimitive
{
public:
    FilterBlend();
    ~FilterBlend() override;

    void render_cairo(FilterSlot &slot) const override;
    bool can_handle_affine(Geom::Affine const &) const override;
    double complexity(Geom::Affine const &ctm) const override;
    bool uses_background() const override;

    void set_input(int slot) override;
    void set_input(int input, int slot) override;
    void set_mode(SPBlendMode mode);

    Glib::ustring name() const override { return Glib::ustring("Blend"); }

private:
    static const std::set<SPBlendMode> _valid_modes;
    SPBlendMode _blend_mode;
    int _input2;
};


} // namespace Filters
} // namespace Inkscape

#endif // SEEN_NR_FILTER_BLEND_H
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

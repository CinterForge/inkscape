// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * SVG <gaussianBlur> implementation.
 */
/*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Niko Kiirala <niko@kiirala.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2006,2007 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "gaussian-blur.h"
#include "attributes.h"                          // for SPAttr
#include "display/nr-filter-gaussian.h"          // for FilterGaussian
#include "object/filters/sp-filter-primitive.h"  // for SPFilterPrimitive
#include "object/sp-object.h"                    // for SP_OBJECT_MODIFIED_FLAG
#include "util/numeric/converters.h"             // for format_number
#include "xml/node.h"                            // for Node

class SPDocument;

namespace Inkscape {
class DrawingItem;
namespace Filters {
class FilterPrimitive;
} // namespace Filters
} // namespace Inkscape

void SPGaussianBlur::build(SPDocument *document, Inkscape::XML::Node *repr)
{
    SPFilterPrimitive::build(document, repr);
    readAttr(SPAttr::STDDEVIATION);
}

void SPGaussianBlur::set(SPAttr key, char const *value)
{
    switch (key) {
        case SPAttr::STDDEVIATION:
            stdDeviation.set(value);
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        default:
        	SPFilterPrimitive::set(key, value);
            break;
    }
}

std::unique_ptr<Inkscape::Filters::FilterPrimitive> SPGaussianBlur::build_renderer(Inkscape::DrawingItem*) const
{
    auto blur = std::make_unique<Inkscape::Filters::FilterGaussian>();
    build_renderer_common(blur.get());

    float num = stdDeviation.getNumber();

    if (num >= 0.0) {
        float optnum = stdDeviation.getOptNumber();
        if (optnum >= 0.0) {
            blur->set_deviation(num, optnum);
        } else {
            blur->set_deviation(num);
        }
    }

    return blur;
}

void SPGaussianBlur::set_deviation(const NumberOptNumber &stdDeviation)
{
    double num = stdDeviation.getNumber();
    std::string arg = Inkscape::Util::format_number(num);

    double optnum = stdDeviation.getOptNumber();
    if (optnum != num && optnum != -1) {
        arg += " " + Inkscape::Util::format_number(optnum);
    }
    getRepr()->setAttribute("stdDeviation", arg);
}

/** Calculate the region taken up by gaussian blur
 *
 * @param region The original shape's region or previous primitive's region output.
 */
Geom::Rect SPGaussianBlur::calculate_region(Geom::Rect const &region) const
{
    double x = stdDeviation.getNumber();
    double y = stdDeviation.getOptNumber();
    if (y == -1.0) {
        y = x;
    }
    // If not within the default 10% margin (see
    // http://www.w3.org/TR/SVG11/filters.html#FilterEffectsRegion), specify margins
    // The 2.4 is an empirical coefficient: at that distance the cutoff is practically invisible
    // (the opacity at 2.4 * radius is about 3e-3)
    auto r = region;
    r.expandBy(2.4 * x, 2.4 * y);
    return r;
}

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

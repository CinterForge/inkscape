// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SVG filters rendering
 *
 * Author:
 *   Niko Kiirala <niko@kiirala.com>
 *   Tavmjong Bah <tavmjong@free.fr> (primitive subregion)
 *
 * Copyright (C) 2006 Niko Kiirala
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "display/nr-filter-primitive.h"

#include "style.h"

#include "display/nr-filter-slot.h"   // for FilterSlot
#include "display/nr-filter-types.h"  // for FilterSlotType
#include "display/nr-filter-units.h"  // for FilterUnits
#include "object/sp-filter-units.h"   // for SPFilterUnits
#include "svg/svg-length.h"           // for SVGLength

#include "colors/spaces/base.h"

namespace Inkscape {
namespace Filters {

FilterPrimitive::FilterPrimitive()
{
    _input = NR_FILTER_SLOT_NOT_SET;
    _output = NR_FILTER_SLOT_NOT_SET;

    // Primitive subregion, should default to the union of all subregions of referenced nodes
    // (i.e. other filter primitives except feTile).  If no referenced nodes, defaults to filter
    // region expressed in percent. At the moment, we do not check referenced nodes.

    // We must keep track if a value is set or not, if not set then the region defaults to 0%, 0%,
    // 100%, 100% ("x", "y", "width", "height") of the -> filter <- region. If set, then
    // percentages are in terms of bounding box or viewbox, depending on value of "primitiveUnits".

    // NB: SVGLength.set takes prescaled percent values: 1 means 100%
    _subregion_x.unset(SVGLength::PERCENT, 0, 0);
    _subregion_y.unset(SVGLength::PERCENT, 0, 0);
    _subregion_width.unset(SVGLength::PERCENT, 1, 0);
    _subregion_height.unset(SVGLength::PERCENT, 1, 0);

    color_interpolation = SP_CSS_COLOR_INTERPOLATION_AUTO;
}

FilterPrimitive::~FilterPrimitive() = default;

void FilterPrimitive::render_cairo(FilterSlot &slot) const
{
    // passthrough
    cairo_surface_t *in = slot.getcairo(_input);
    slot.set(_output, in);
}

void FilterPrimitive::set_input(int slot)
{
    set_input(0, slot);
}

void FilterPrimitive::set_input(int input, int slot)
{
    if (input == 0) _input = slot;
}

void FilterPrimitive::set_output(int slot)
{
    if (slot >= 0) _output = slot;
}

// We need to copy reference even if unset as we need to know if
// someone has unset a value.
void FilterPrimitive::set_x(SVGLength const &length)
{
    _subregion_x = length;
}

void FilterPrimitive::set_y(SVGLength const &length)
{
    _subregion_y = length;
}

void FilterPrimitive::set_width(SVGLength const &length)
{
    _subregion_width = length;
}

void FilterPrimitive::set_height(SVGLength const &length)
{
    _subregion_height = length;
}

void FilterPrimitive::set_subregion(SVGLength const &x, SVGLength const &y,
                                    SVGLength const &width, SVGLength const &height)
{
    _subregion_x = x;
    _subregion_y = y;
    _subregion_width = width;
    _subregion_height = height;
}

Geom::Rect FilterPrimitive::filter_primitive_area(FilterUnits const &units) const
{
    Geom::OptRect const fa_opt = units.get_filter_area();
    if (!fa_opt) {
        std::cerr << "FilterPrimitive::filter_primitive_area: filter area undefined." << std::endl;
        return Geom::Rect::from_xywh(0, 0, 0, 0);
    }
    Geom::Rect fa = *fa_opt;

    // x, y, width, and height are independently defined (i.e. one can be defined, by default, to
    // the filter area (via default value ) while another is defined relative to the bounding
    // box). It is better to keep track of them separately and then compose the Rect at the end.
    double x = 0;
    double y = 0;
    double width = 0;
    double height = 0;

    // If subregion not set, by special case use filter region.
    if (!_subregion_x._set)      x = fa.left();
    if (!_subregion_y._set)      y = fa.top();
    if (!_subregion_width._set)  width  = fa.width();
    if (!_subregion_height._set) height = fa.height();

    if (units.get_primitive_units() == SP_FILTER_UNITS_OBJECTBOUNDINGBOX) {

        Geom::OptRect const bb_opt = units.get_item_bbox();
        if (!bb_opt) {
            std::cerr << "FilterPrimitive::filter_primitive_area: bounding box undefined and 'primitiveUnits' is 'objectBoundingBox'." << std::endl;
            return Geom::Rect();
        }
        Geom::Rect bb = *bb_opt;

        // Update computed values for ex, em, %.
        // For %, assumes primitive unit is objectBoundingBox.
        // TODO: fetch somehow the object ex and em lengths; 12, 6 are just dummy values.
        auto const len_x = bb.width();
        auto const len_y = bb.height();

        auto compute = [] (SVGLength length, double scale) {
            length.update(12, 6, scale);
            return length.computed;
        };

        auto const subregion_x_computed = compute(_subregion_x, len_x);
        auto const subregion_y_computed = compute(_subregion_y, len_y);
        auto const subregion_w_computed = compute(_subregion_width, len_x);
        auto const subregion_h_computed = compute(_subregion_height, len_y);

        // Values are in terms of fraction of bounding box.
        if (_subregion_x._set      && _subregion_x.unit      != SVGLength::PERCENT)      x = bb.left() + bb.width() * _subregion_x.value;
        if (_subregion_y._set      && _subregion_y.unit      != SVGLength::PERCENT)      y = bb.top() + bb.height() * _subregion_y.value;
        if (_subregion_width._set  && _subregion_width.unit  != SVGLength::PERCENT)  width = bb.width()  * _subregion_width.value;
        if (_subregion_height._set && _subregion_height.unit != SVGLength::PERCENT) height = bb.height() * _subregion_height.value;
        // Values are in terms of percent                                                         
        if (_subregion_x._set      && _subregion_x.unit      == SVGLength::PERCENT)      x = bb.left() + subregion_x_computed;
        if (_subregion_y._set      && _subregion_y.unit      == SVGLength::PERCENT)      y = bb.top() + subregion_y_computed;
        if (_subregion_width._set  && _subregion_width.unit  == SVGLength::PERCENT)  width = subregion_w_computed;
        if (_subregion_height._set && _subregion_height.unit == SVGLength::PERCENT) height = subregion_h_computed;
    } else {
        // Values are in terms of user space coordinates or percent of viewport (already calculated in sp-filter-primitive.cpp).
        if (_subregion_x._set     )      x = _subregion_x.computed;
        if (_subregion_y._set     )      y = _subregion_y.computed;
        if (_subregion_width._set )  width = _subregion_width.computed;
        if (_subregion_height._set) height = _subregion_height.computed;
    }

    return Geom::Rect::from_xywh(x, y, width, height);
}

void FilterPrimitive::setStyle(SPStyle const *style)
{
    // Default filter interpolation is linearRGB, no AUTO beyond this point
    color_interpolation = SP_CSS_COLOR_INTERPOLATION_LINEARRGB;
    if (style) {
        if (auto space = style->color_interpolation_filters.getInterpolationSpace()) {
            switch (space->getType()) {
                case Colors::Space::Type::RGB:
                    color_interpolation = SP_CSS_COLOR_INTERPOLATION_SRGB;
                    break;
                case Colors::Space::Type::linearRGB:
                    color_interpolation = SP_CSS_COLOR_INTERPOLATION_LINEARRGB;
                    break;
                default:
                    std::cerr << "Can't use color interpolation space: '" << space->getName().c_str() << "'\n";
            }
        }
    }
}

} // namespace Filters
} // namespace Inkscape

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

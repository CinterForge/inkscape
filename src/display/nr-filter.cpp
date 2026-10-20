// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SVG filters rendering
 *
 * Author:
 *   Niko Kiirala <niko@kiirala.com>
 *
 * Copyright (C) 2006-2008 Niko Kiirala
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "display/nr-filter.h"

#include <cmath>
#include <cstring>
#include <string>
#include <cairo.h>
#include <glib.h>
#include <2geom/affine.h>
#include <2geom/rect.h>

#include "display/cairo-utils.h"
#include "display/drawing-context.h"
#include "display/drawing-item.h"
#include "display/drawing-surface.h"
#include "display/drawing.h"
#include "display/nr-filter-blend.h"
#include "display/nr-filter-colormatrix.h"
#include "display/nr-filter-component-transfer.h"
#include "display/nr-filter-composite.h"
#include "display/nr-filter-convolve-matrix.h"
#include "display/nr-filter-diffuselighting.h"
#include "display/nr-filter-displacement-map.h"
#include "display/nr-filter-dropshadow.h"
#include "display/nr-filter-flood.h"
#include "display/nr-filter-gaussian.h"
#include "display/nr-filter-image.h"
#include "display/nr-filter-merge.h"
#include "display/nr-filter-morphology.h"
#include "display/nr-filter-offset.h"
#include "display/nr-filter-primitive.h"
#include "display/nr-filter-slot.h"
#include "display/nr-filter-specularlighting.h"
#include "display/nr-filter-tile.h"
#include "display/nr-filter-turbulence.h"
#include "display/nr-filter-types.h"
#include "display/nr-filter-units.h"
#include "svg/svg-length.h"
//#include "sp-filter-units.h"

namespace Inkscape {
namespace Filters {

using Geom::X;
using Geom::Y;

Filter::Filter()
{
    _common_init();
}

Filter::Filter(int n)
{
    if (n > 0) primitives.reserve(n);
    _common_init();
}

void Filter::_common_init()
{
    _slot_count = 1;
    // Having "not set" here as value means the output of last filter
    // primitive will be used as output of this filter
    _output_slot = NR_FILTER_SLOT_NOT_SET;

    // These are the default values for filter region,
    // as specified in SVG standard
    // NB: SVGLength.set takes prescaled percent values: -.10 means -10%
    _region_x.set(SVGLength::PERCENT, -.10, 0);
    _region_y.set(SVGLength::PERCENT, -.10, 0);
    _region_width.set(SVGLength::PERCENT, 1.20, 0);
    _region_height.set(SVGLength::PERCENT, 1.20, 0);

    // Filter resolution, negative value here stands for "automatic"
    _x_pixels = -1.0;
    _y_pixels = -1.0;

    _filter_units = SP_FILTER_UNITS_OBJECTBOUNDINGBOX;
    _primitive_units = SP_FILTER_UNITS_USERSPACEONUSE;
}

void Filter::update()
{
    for (auto &p : primitives) {
        p->update();
    }
}

int Filter::render(Inkscape::DrawingItem const *item, DrawingContext &graphic, DrawingSurface const *bg, RenderContext &rc) const
{
    // std::cout << "Filter::render() for: " << const_cast<Inkscape::DrawingItem *>(item)->name() << std::endl;
    // std::cout << "  graphic drawing_scale: " << graphic.surface()->device_scale() << std::endl;

    if (primitives.empty()) {
        // when no primitives are defined, clear source graphic
        graphic.setSource(0,0,0,0);
        graphic.setOperator(CAIRO_OPERATOR_SOURCE);
        graphic.paint();
        graphic.setOperator(CAIRO_OPERATOR_OVER);
        return 1;
    }
    FilterQuality filterquality = (FilterQuality)item->drawing().filterQuality();
    int blurquality = item->drawing().blurQuality();

    Geom::Affine trans = item->ctm();

    Geom::OptRect filter_area = filter_effect_area(item->itemBounds());
    if (!filter_area) return 1;

    FilterUnits units(_filter_units, _primitive_units);
    units.set_ctm(trans);
    units.set_item_bbox(item->itemBounds());
    units.set_filter_area(*filter_area);

    auto resolution = _filter_resolution(*filter_area, trans, filterquality);
    if (!(resolution.first > 0 && resolution.second > 0)) {
        // zero resolution - clear source graphic and return
        graphic.setSource(0,0,0,0);
        graphic.setOperator(CAIRO_OPERATOR_SOURCE);
        graphic.paint();
        graphic.setOperator(CAIRO_OPERATOR_OVER);
        return 1;
    }

    units.set_resolution(resolution.first, resolution.second);
    if (_x_pixels > 0) {
        units.set_automatic_resolution(false);
    }
    else {
        units.set_automatic_resolution(true);
    }

    units.set_paraller(false);
    Geom::Affine pbtrans = units.get_matrix_display2pb();
    for (auto &i : primitives) {
        if (!i->can_handle_affine(pbtrans)) {
            units.set_paraller(true);
            break;
        }
    }

    auto slot = FilterSlot(graphic, units, rc, blurquality);
    if (bg) {
        slot.set(NR_FILTER_BACKGROUNDIMAGE, const_cast<DrawingSurface *>(bg)->raw());
        slot.set_background_area(bg->area().roundOutwards());
    }

    for (auto &i : primitives) {
        i->render_cairo(slot);
    }

    Geom::Point origin = graphic.targetLogicalBounds().min();
    cairo_surface_t *result = slot.get_result(_output_slot);

    // Assume for the moment that we paint the filter in sRGB
    set_cairo_surface_ci(result, SP_CSS_COLOR_INTERPOLATION_SRGB);

    graphic.setSource(result, origin[Geom::X], origin[Geom::Y]);
    graphic.setOperator(CAIRO_OPERATOR_SOURCE);
    graphic.paint();
    graphic.setOperator(CAIRO_OPERATOR_OVER);
    cairo_surface_destroy(result);

    return 0;
}

void Filter::add_primitive(std::unique_ptr<FilterPrimitive> primitive)
{
    primitives.emplace_back(std::move(primitive));
}

void Filter::set_filter_units(SPFilterUnits unit)
{
    _filter_units = unit;
}

void Filter::set_primitive_units(SPFilterUnits unit)
{
    _primitive_units = unit;
}

void Filter::area_enlarge(Geom::IntRect &bbox, Inkscape::DrawingItem const *item) const
{
    for (auto const &i : primitives) {
        if (i) i->area_enlarge(bbox, item->ctm());
    }

/*
  TODO: something. See images at the bottom of filters.svg with medium-low
  filtering quality.

    FilterQuality const filterquality = ...

    if (_x_pixels <= 0 && (filterquality == FILTER_QUALITY_BEST ||
                           filterquality == FILTER_QUALITY_BETTER)) {
        return;
    }

    Geom::Rect item_bbox;
    Geom::OptRect maybe_bbox = item->itemBounds();
    if (maybe_bbox.empty()) {
        // Code below needs a bounding box
        return;
    }
    item_bbox = *maybe_bbox;

    std::pair<double,double> res_low
        = _filter_resolution(item_bbox, item->ctm(), filterquality);
    //std::pair<double,double> res_full
    //    = _filter_resolution(item_bbox, item->ctm(), FILTER_QUALITY_BEST);
    double pixels_per_block = fmax(item_bbox.width() / res_low.first,
                                   item_bbox.height() / res_low.second);
    bbox.x0 -= (int)pixels_per_block;
    bbox.x1 += (int)pixels_per_block;
    bbox.y0 -= (int)pixels_per_block;
    bbox.y1 += (int)pixels_per_block;
*/
}

Geom::OptRect Filter::filter_effect_area(Geom::OptRect const &bbox) const
{
    Geom::Point minp, maxp;

    if (_filter_units == SP_FILTER_UNITS_OBJECTBOUNDINGBOX) {
        double len_x = bbox ? bbox->width() : 0;
        double len_y = bbox ? bbox->height() : 0;
        /* TODO: fetch somehow the object ex and em lengths */

        // Update for em, ex, and % values
        auto compute = [] (SVGLength length, double scale) {
            length.update(12, 6, scale);
            return length.computed;
        };
        auto const region_x_computed = compute(_region_x, len_x);
        auto const region_y_computed = compute(_region_y, len_y);
        auto const region_w_computed = compute(_region_width, len_x);
        auto const region_h_computed = compute(_region_height, len_y);;

        if (!bbox) return Geom::OptRect();

        if (_region_x.unit == SVGLength::PERCENT) {
            minp[X] = bbox->left() + region_x_computed;
        } else {
            minp[X] = bbox->left() + region_x_computed * len_x;
        }
        if (_region_width.unit == SVGLength::PERCENT) {
            maxp[X] = minp[X] + region_w_computed;
        } else {
            maxp[X] = minp[X] + region_w_computed * len_x;
        }

        if (_region_y.unit == SVGLength::PERCENT) {
            minp[Y] = bbox->top() + region_y_computed;
        } else {
            minp[Y] = bbox->top() + region_y_computed * len_y;
        }
        if (_region_height.unit == SVGLength::PERCENT) {
            maxp[Y] = minp[Y] + region_h_computed;
        } else {
            maxp[Y] = minp[Y] + region_h_computed * len_y;
        }
    } else if (_filter_units == SP_FILTER_UNITS_USERSPACEONUSE) {
        // Region already set in sp-filter.cpp
        minp[X] = _region_x.computed;
        maxp[X] = minp[X] + _region_width.computed;
        minp[Y] = _region_y.computed;
        maxp[Y] = minp[Y] + _region_height.computed;
    } else {
        g_warning("Error in Inkscape::Filters::Filter::filter_effect_area: unrecognized value of _filter_units");
    }

    Geom::OptRect area(minp, maxp);
    // std::cout << "Filter::filter_effect_area: area: " << *area << std::endl;
    return area;
}

double Filter::complexity(Geom::Affine const &ctm) const
{
    double factor = 1.0;
    for (auto &i : primitives) {
        if (i) {
            double f = i->complexity(ctm);
            factor += f - 1.0;
        }
    }
    return factor;
}

bool Filter::uses_background() const
{
    for (auto &i : primitives) {
        if (i && i->uses_background()) {
            return true;
        }
    }
    return false;
}

void Filter::clear_primitives()
{
    primitives.clear();
}

void Filter::set_x(SVGLength const &length)
{
  if (length._set)
      _region_x = length;
}

void Filter::set_y(SVGLength const &length)
{
  if (length._set)
      _region_y = length;
}

void Filter::set_width(SVGLength const &length)
{
  if (length._set)
      _region_width = length;
}

void Filter::set_height(SVGLength const &length)
{
  if (length._set)
      _region_height = length;
}

void Filter::set_resolution(double pixels)
{
    if (pixels > 0) {
        _x_pixels = pixels;
        _y_pixels = pixels;
    }
}

void Filter::set_resolution(double x_pixels, double y_pixels)
{
    if (x_pixels >= 0 && y_pixels >= 0) {
        _x_pixels = x_pixels;
        _y_pixels = y_pixels;
    }
}

void Filter::reset_resolution()
{
    _x_pixels = -1;
    _y_pixels = -1;
}

int Filter::_resolution_limit(FilterQuality quality)
{
    switch (quality) {
        case FILTER_QUALITY_WORST:
            return 32;
        case FILTER_QUALITY_WORSE:
            return 64;
        case FILTER_QUALITY_NORMAL:
            return 256;
        case FILTER_QUALITY_BETTER:
            return 1024;
        case FILTER_QUALITY_BEST:
        default:
            return -1;
    }
}

std::pair<double, double> Filter::_filter_resolution(Geom::Rect const &area, Geom::Affine const &trans, FilterQuality filterquality) const
{
    std::pair<double, double> resolution;
    if (_x_pixels > 0) {
        double y_len;
        if (_y_pixels > 0) {
            y_len = _y_pixels;
        } else {
            y_len = (_x_pixels * (area.max()[Y] - area.min()[Y])) / (area.max()[X] - area.min()[X]);
        }
        resolution.first = _x_pixels;
        resolution.second = y_len;
    } else {
        auto origo = area.min() * trans;
        auto max_i = Geom::Point(area.max()[X], area.min()[Y]) * trans;
        auto max_j = Geom::Point(area.min()[X], area.max()[Y]) * trans;
        double i_len = (origo - max_i).length();
        double j_len = (origo - max_j).length();
        int limit = _resolution_limit(filterquality);
        if (limit > 0 && (i_len > limit || j_len > limit)) {
            double aspect_ratio = i_len / j_len;
            if (i_len > j_len) {
                i_len = limit;
                j_len = i_len / aspect_ratio;
            }
            else {
                j_len = limit;
                i_len = j_len * aspect_ratio;
            }
        }
        resolution.first = i_len;
        resolution.second = j_len;
    }
    return resolution;
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

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * A container class for filter slots. Allows for simple getting and
 * setting images in filter slots without having to bother with
 * table indexes and such.
 *
 * Author:
 *   Niko Kiirala <niko@kiirala.com>
 *
 * Copyright (C) 2006,2007 Niko Kiirala
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cassert>
#include <cstring>

#include <2geom/transforms.h>
#include "cairo-utils.h"
#include "drawing-context.h"
#include "drawing-surface.h"
#include "nr-filter-types.h"
#include "nr-filter-gaussian.h"
#include "nr-filter-slot.h"
#include "nr-filter-units.h"

namespace Inkscape {
namespace Filters {

FilterSlot::FilterSlot(DrawingContext &graphic, FilterUnits const &units, RenderContext &rc, int blurquality)
    : _source_graphic(graphic.rawTarget())
    , _source_graphic_area(graphic.targetLogicalBounds().roundOutwards()) // fixme
    , _units(units)
    , _last_out(NR_FILTER_SOURCEGRAPHIC)
    , _blurquality(blurquality)
    , rc(rc)
    , device_scale(graphic.surface()->device_scale())
{
    using Geom::X;
    using Geom::Y;

    // compute slot bbox
    Geom::Affine trans = _units.get_matrix_display2pb();
    Geom::Rect bbox_trans = graphic.targetLogicalBounds() * trans;
    Geom::Point min = bbox_trans.min();
    _slot_x = min[X];
    _slot_y = min[Y];

    if (trans.isTranslation()) {
        _slot_w = _source_graphic_area.width();
        _slot_h = _source_graphic_area.height();
    } else {
        _slot_w = std::ceil(bbox_trans.width());
        _slot_h = std::ceil(bbox_trans.height());
    }
}

FilterSlot::~FilterSlot()
{
    for (auto &_slot : _slots) {
        cairo_surface_destroy(_slot.second);
    }
}

cairo_surface_t *FilterSlot::getcairo(int slot_nr)
{
    if (slot_nr == NR_FILTER_SLOT_NOT_SET)
        slot_nr = _last_out;

    SlotMap::iterator s = _slots.find(slot_nr);

    /* If we didn't have the specified image, but we could create it
     * from the other information we have, let's do that */
    if (s == _slots.end()
        && (slot_nr == NR_FILTER_SOURCEGRAPHIC
            || slot_nr == NR_FILTER_SOURCEALPHA
            || slot_nr == NR_FILTER_BACKGROUNDIMAGE
            || slot_nr == NR_FILTER_BACKGROUNDALPHA
            || slot_nr == NR_FILTER_FILLPAINT
            || slot_nr == NR_FILTER_STROKEPAINT))
    {
        switch (slot_nr) {
            case NR_FILTER_SOURCEGRAPHIC: {
                cairo_surface_t *tr = _get_transformed_source_graphic();
                // Assume all source graphics are sRGB
                set_cairo_surface_ci( tr, SP_CSS_COLOR_INTERPOLATION_SRGB );
                _set_internal(NR_FILTER_SOURCEGRAPHIC, tr);
                cairo_surface_destroy(tr);
            } break;
            case NR_FILTER_BACKGROUNDIMAGE: {
                cairo_surface_t *bg = _get_transformed_background();
                // Assume all backgrounds are sRGB
                set_cairo_surface_ci( bg, SP_CSS_COLOR_INTERPOLATION_SRGB );
                _set_internal(NR_FILTER_BACKGROUNDIMAGE, bg);
                cairo_surface_destroy(bg);
            } break;
            case NR_FILTER_SOURCEALPHA: {
                cairo_surface_t *src = getcairo(NR_FILTER_SOURCEGRAPHIC);
                cairo_surface_t *alpha = ink_cairo_extract_alpha(src);
                _set_internal(NR_FILTER_SOURCEALPHA, alpha);
                cairo_surface_destroy(alpha);
            } break;
            case NR_FILTER_BACKGROUNDALPHA: {
                cairo_surface_t *src = getcairo(NR_FILTER_BACKGROUNDIMAGE);
                cairo_surface_t *ba = ink_cairo_extract_alpha(src);
                _set_internal(NR_FILTER_BACKGROUNDALPHA, ba);
                cairo_surface_destroy(ba);
            } break;
            case NR_FILTER_FILLPAINT: //TODO
            case NR_FILTER_STROKEPAINT: //TODO
            default:
                break;
        }
        s = _slots.find(slot_nr);
    }

    if (s == _slots.end()) {
        // create empty surface
        cairo_surface_t *empty = cairo_surface_create_similar(
            _source_graphic, cairo_surface_get_content(_source_graphic),
            _slot_w, _slot_h);
        _set_internal(slot_nr, empty);
        cairo_surface_destroy(empty);
        s = _slots.find(slot_nr);
    }

    if (s->second && cairo_surface_status(s->second) == CAIRO_STATUS_NO_MEMORY) {
        // Simulate Cairomm behaviour by throwing std::bad_alloc.
        throw std::bad_alloc();
    }

    return s->second;
}

cairo_surface_t *FilterSlot::_get_transformed_source_graphic() const
{
    Geom::Affine trans = _units.get_matrix_display2pb();

    if (trans.isTranslation()) {
        cairo_surface_reference(_source_graphic);
        return _source_graphic;
    }

    cairo_surface_t *tsg = cairo_surface_create_similar(
        _source_graphic, cairo_surface_get_content(_source_graphic),
        _slot_w, _slot_h);
    cairo_t *tsg_ct = cairo_create(tsg);

    cairo_translate(tsg_ct, -_slot_x, -_slot_y);
    ink_cairo_transform(tsg_ct, trans);
    cairo_translate(tsg_ct, _source_graphic_area.left(), _source_graphic_area.top());
    cairo_set_source_surface(tsg_ct, _source_graphic, 0, 0);
    cairo_set_operator(tsg_ct, CAIRO_OPERATOR_SOURCE);
    cairo_paint(tsg_ct);
    cairo_destroy(tsg_ct);

    return tsg;
}

cairo_surface_t *FilterSlot::_get_transformed_background() const
{
    Geom::Affine trans = _units.get_matrix_display2pb();

    cairo_surface_t *tbg = nullptr;

    auto bg_iter = _slots.find(NR_FILTER_BACKGROUNDIMAGE);
    if (bg_iter != _slots.end()) {
        auto bg = bg_iter->second;

        tbg = cairo_surface_create_similar(
            bg, cairo_surface_get_content(bg),
            _slot_w, _slot_h);
        cairo_t *tbg_ct = cairo_create(tbg);

        cairo_translate(tbg_ct, -_slot_x, -_slot_y);
        ink_cairo_transform(tbg_ct, trans);
        cairo_translate(tbg_ct, _background_area.left(), _background_area.top());
        cairo_set_source_surface(tbg_ct, bg, 0, 0);
        cairo_set_operator(tbg_ct, CAIRO_OPERATOR_SOURCE);
        cairo_paint(tbg_ct);
        cairo_destroy(tbg_ct);
    } else {
        tbg = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, _slot_w * device_scale, _slot_h * device_scale);
    }

    return tbg;
}

cairo_surface_t *FilterSlot::get_result(int res)
{
    cairo_surface_t *result = getcairo(res);

    Geom::Affine trans = _units.get_matrix_pb2display();
    if (trans.isIdentity()) {
        cairo_surface_reference(result);
        return result;
    }

    cairo_surface_t *r = cairo_surface_create_similar(_source_graphic,
        cairo_surface_get_content(_source_graphic),
        _source_graphic_area.width(),
        _source_graphic_area.height());
    copy_cairo_surface_ci( result, r );
    cairo_t *r_ct = cairo_create(r);

    cairo_translate(r_ct, -_source_graphic_area.left(), -_source_graphic_area.top());
    ink_cairo_transform(r_ct, trans);
    cairo_translate(r_ct, _slot_x, _slot_y);
    cairo_set_source_surface(r_ct, result, 0, 0);
    cairo_set_operator(r_ct, CAIRO_OPERATOR_SOURCE);
    cairo_paint(r_ct);
    cairo_destroy(r_ct);

    return r;
}

void FilterSlot::_set_internal(int slot_nr, cairo_surface_t *surface)
{
    // destroy after referencing
    // this way assigning a surface to a slot it already occupies will not cause errors
    cairo_surface_reference(surface);

    SlotMap::iterator s = _slots.find(slot_nr);
    if (s != _slots.end()) {
        cairo_surface_destroy(s->second);
    }

    _slots[slot_nr] = surface;
}

void FilterSlot::set(int slot_nr, cairo_surface_t *surface)
{
    g_return_if_fail(surface != nullptr);

    if (slot_nr == NR_FILTER_SLOT_NOT_SET)
        slot_nr = NR_FILTER_UNNAMED_SLOT;

    _set_internal(slot_nr, surface);
    _last_out = slot_nr;
}

void FilterSlot::set_primitive_area(int slot_nr, Geom::Rect &area)
{
    if (slot_nr == NR_FILTER_SLOT_NOT_SET)
        slot_nr = NR_FILTER_UNNAMED_SLOT;

    _primitiveAreas[slot_nr] = area;
}

Geom::Rect FilterSlot::get_primitive_area(int slot_nr) const
{
    if (slot_nr == NR_FILTER_SLOT_NOT_SET)
        slot_nr = _last_out;

    auto s = _primitiveAreas.find(slot_nr);

    if (s == _primitiveAreas.end()) {
        return *_units.get_filter_area();
    }
    return s->second;
}

Geom::Rect FilterSlot::get_slot_area() const
{
    return Geom::Rect::from_xywh(_slot_x, _slot_y, _slot_w, _slot_h);
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

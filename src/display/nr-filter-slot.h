// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_NR_FILTER_SLOT_H
#define SEEN_NR_FILTER_SLOT_H

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

#include <map>
#include "nr-filter-types.h"
#include "nr-filter-units.h"

extern "C" {
typedef struct _cairo cairo_t;
typedef struct _cairo_surface cairo_surface_t;
}

namespace Inkscape {
class DrawingContext;
class DrawingItem;
class DrawingSurface;
class RenderContext;

namespace Filters {

class FilterSlot final
{
public:
    /** Creates a new FilterSlot object. */
    FilterSlot(DrawingContext &graphic, FilterUnits const &units, RenderContext &rc, int blurquality);

    /** Destroys the FilterSlot object and all its contents */
    ~FilterSlot();

    /** Returns the pixblock in specified slot.
     * Parameter 'slot' may be either an positive integer or one of
     * pre-defined filter slot types: NR_FILTER_SLOT_NOT_SET,
     * NR_FILTER_SOURCEGRAPHIC, NR_FILTER_SOURCEALPHA,
     * NR_FILTER_BACKGROUNDIMAGE, NR_FILTER_BACKGROUNDALPHA,
     * NR_FILTER_FILLPAINT, NR_FILTER_SOURCEPAINT.
     */
    cairo_surface_t *getcairo(int slot);

    /** Sets or re-sets the pixblock associated with given slot.
     * If there was a pixblock already assigned with this slot,
     * that pixblock is destroyed.
     */
    void set(int slot, cairo_surface_t *s);
    void set_background_area(Geom::IntRect const &area) { _background_area = area; }

    cairo_surface_t *get_result(int slot_nr);

    void set_primitive_area(int slot, Geom::Rect &area);
    Geom::Rect get_primitive_area(int slot) const;
    
    /** Returns the number of slots in use. */
    int get_slot_count() const { return _slots.size(); }

    /** Gets the gaussian filtering quality. Affects used interpolation methods */
    int get_blurquality() const { return _blurquality; }

    /** Gets the device scale; for high DPI monitors. */
    int get_device_scale() const { return device_scale; }

    FilterUnits const &get_units() const { return _units; }
    Geom::Rect get_slot_area() const;

    RenderContext &get_rendercontext() const { return rc; }

private:
    using SlotMap = std::map<int, cairo_surface_t *>;
    SlotMap _slots;

    // We need to keep track of the primitive area as this is needed in feTile
    using PrimitiveAreaMap = std::map<int, Geom::Rect>;
    PrimitiveAreaMap _primitiveAreas;

    int _slot_w, _slot_h;
    double _slot_x, _slot_y;
    cairo_surface_t *_source_graphic;
    Geom::IntRect _source_graphic_area;
    Geom::IntRect _background_area; // remove me
    FilterUnits const &_units;
    int _last_out;
    int _blurquality;
    int device_scale;
    RenderContext &rc;

    cairo_surface_t *_get_transformed_source_graphic() const;
    cairo_surface_t *_get_transformed_background() const;
    cairo_surface_t *_get_fill_paint() const;
    cairo_surface_t *_get_stroke_paint() const;

    void _set_internal(int slot, cairo_surface_t *s);
};

} // namespace Filters
} // namespace Inkscape

#endif // SEEN_NR_FILTER_SLOT_H
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

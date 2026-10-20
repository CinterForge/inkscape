// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_NR_FILTER_GAUSSIAN_H
#define SEEN_NR_FILTER_GAUSSIAN_H

/*
 * Gaussian blur renderer
 *
 * Authors:
 *   Niko Kiirala <niko@kiirala.com>
 *   bulia byak
 *   Jasper van de Gronde <th.v.d.gronde@hccnet.nl>
 *
 * Copyright (C) 2006 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <2geom/forward.h>
#include "display/nr-filter-primitive.h"

enum
{
    BLUR_QUALITY_BEST = 2,
    BLUR_QUALITY_BETTER = 1,
    BLUR_QUALITY_NORMAL = 0,
    BLUR_QUALITY_WORSE = -1,
    BLUR_QUALITY_WORST = -2
};

namespace Inkscape {
class dispatch_pool;

namespace Filters {

class FilterGaussian : public FilterPrimitive
{
public:
    FilterGaussian();
    ~FilterGaussian() override;

    void render_cairo(FilterSlot &slot) const override;
    void area_enlarge(Geom::IntRect &area, Geom::Affine const &m) const override;
    bool can_handle_affine(Geom::Affine const &m) const override;
    double complexity(Geom::Affine const &ctm) const override;

    /**
     * Set the standard deviation value for gaussian blur. Deviation along
     * both axis is set to the provided value.
     * Negative value, NaN and infinity are considered an error and no
     * changes to filter state are made. If not set, default value of zero
     * is used, which means the filter results in transparent black image.
     */
    void set_deviation(double deviation);

    /**
     * Set the standard deviation value for gaussian blur. First parameter
     * sets the deviation alogn x-axis, second along y-axis.
     * Negative value, NaN and infinity are considered an error and no
     * changes to filter state are made. If not set, default value of zero
     * is used, which means the filter results in transparent black image.
     */
    void set_deviation(double x, double y);

    Glib::ustring name() const override { return Glib::ustring("Gaussian Blur"); }

private:
    double _deviation_x;
    double _deviation_y;
};

// High-level blur function for reuse by other filter primitives (e.g. feDropShadow)
typedef struct _cairo_surface cairo_surface_t;

/**
 * Apply Gaussian blur to a cairo surface in-place.
 * Automatically selects optimal algorithm (IIR for σ > 3, FIR otherwise)
 * and handles threading via dispatch_pool.
 *
 * @param surface Surface to blur (modified in-place)
 * @param stdDeviation Blur radius (standard deviation)
 */
void blur_surface(cairo_surface_t *surface, double stdDeviation);

} // namespace Filters
} // namespace Inkscape

#endif // SEEN_NR_FILTER_GAUSSIAN_H
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

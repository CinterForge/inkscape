// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * feFlood filter primitive renderer
 *
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *   Tavmjong Bah <tavmjong@free.fr> (use primitive filter region)
 *
 * Copyright (C) 2007, 2011 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"  // only include where actually required!
#endif

#include "display/cairo-utils.h"
#include "display/nr-filter-flood.h"
#include "display/nr-filter-slot.h"

namespace Inkscape {
namespace Filters {

FilterFlood::FilterFlood() = default;

FilterFlood::~FilterFlood() = default;

void FilterFlood::render_cairo(FilterSlot &slot) const
{
    cairo_surface_t *input = slot.getcairo(_input);

    double r = SP_RGBA32_R_F(color);
    double g = SP_RGBA32_G_F(color);
    double b = SP_RGBA32_B_F(color);
    double a = SP_RGBA32_A_F(color);

    cairo_surface_t *out = ink_cairo_surface_create_same_size(input, CAIRO_CONTENT_COLOR_ALPHA);

    // Flood color is always defined in terms of sRGB, preconvert to linearRGB
    // if color_interpolation_filters set to linearRGB (for efficiency assuming
    // next filter primitive has same value of cif).
    if (color_interpolation == SP_CSS_COLOR_INTERPOLATION_LINEARRGB) {
        r = srgb_to_linear(r);
        g = srgb_to_linear(g);
        b = srgb_to_linear(b);
    }
    set_cairo_surface_ci(out, color_interpolation);

    // Get filter primitive area in user units
    Geom::Rect fp = filter_primitive_area(slot.get_units());

    // Convert to Cairo units
    Geom::Rect fp_cairo = fp * slot.get_units().get_matrix_user2pb();

    // Get area in slot (tile to fill)
    Geom::Rect sa = slot.get_slot_area();

    // Get overlap
    Geom::OptRect optoverlap = intersect(fp_cairo, sa);
    if (optoverlap) {
        Geom::Rect overlap = *optoverlap;

        auto d = fp_cairo.min() - sa.min();
        if (d.x() < 0.0) d.x() = 0.0;
        if (d.y() < 0.0) d.y() = 0.0;

        cairo_t *ct = cairo_create(out);
        cairo_set_source_rgba(ct, r, g, b, a);
        cairo_set_operator(ct, CAIRO_OPERATOR_SOURCE);
        cairo_rectangle(ct, d.x(), d.y(), overlap.width(), overlap.height());
        cairo_fill(ct);
        cairo_destroy(ct);
    }

    slot.set(_output, out);
    cairo_surface_destroy(out);
}

bool FilterFlood::can_handle_affine(Geom::Affine const &) const
{
    // flood is a per-pixel primitive and is immutable under transformations
    return true;
}

void FilterFlood::set_color(guint32 c)
{
    color = c;
}

double FilterFlood::complexity(Geom::Affine const &) const
{
    // flood is actually less expensive than normal rendering,
    // but when flood is processed, the object has already been rendered
    return 1.0;
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

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * feDropShadow filter primitive renderer
 *
 * Authors:
 *   Ryan Malloy <ryan@supported.systems>
 *
 * Copyright (C) 2025 Ryan Malloy
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <algorithm>
#include <cmath>
#include <cairo.h>

#include "display/cairo-utils.h"
#include "display/nr-filter-dropshadow.h"
#include "display/nr-filter-gaussian.h"
#include "display/nr-filter-slot.h"
#include "display/nr-filter-units.h"
#include "util/scope_exit.h"

namespace Inkscape {
namespace Filters {

using Geom::X;
using Geom::Y;

FilterDropShadow::FilterDropShadow() = default;

FilterDropShadow::~FilterDropShadow() = default;

void FilterDropShadow::render_cairo(FilterSlot &slot) const
{
    cairo_surface_t *in = slot.getcairo(_input);
    if (!in || cairo_surface_status(in) != CAIRO_STATUS_SUCCESS) {
        return;
    }

    set_cairo_surface_ci(in, color_interpolation);

    cairo_surface_t *out = ink_cairo_surface_create_identical(in);
    if (!out || cairo_surface_status(out) != CAIRO_STATUS_SUCCESS) {
        return;
    }
    auto out_cleanup = scope_exit([&] { cairo_surface_destroy(out); });

    copy_cairo_surface_ci(in, out);
    set_cairo_surface_ci(out, color_interpolation);
    cairo_t *ct = cairo_create(out);
    if (!ct || cairo_status(ct) != CAIRO_STATUS_SUCCESS) {
        return;
    }
    auto ct_cleanup = scope_exit([&] { cairo_destroy(ct); });

    Geom::Rect vp = filter_primitive_area(slot.get_units());
    slot.set_primitive_area(_output, vp);

    Geom::Affine p2pb = slot.get_units().get_matrix_primitiveunits2pb();
    double offset_x = dx * p2pb.expansionX();
    double offset_y = dy * p2pb.expansionY();

    // Step 1: Create shadow surface
    cairo_surface_t *shadow = ink_cairo_surface_create_identical(in);
    if (!shadow || cairo_surface_status(shadow) != CAIRO_STATUS_SUCCESS) {
        return;
    }
    auto shadow_cleanup = scope_exit([&] { cairo_surface_destroy(shadow); });

    copy_cairo_surface_ci(in, shadow);
    set_cairo_surface_ci(shadow, color_interpolation);
    cairo_t *shadow_ct = cairo_create(shadow);
    if (!shadow_ct || cairo_status(shadow_ct) != CAIRO_STATUS_SUCCESS) {
        return;
    }
    auto shadow_ct_cleanup = scope_exit([&] { cairo_destroy(shadow_ct); });

    // Step 2: Copy alpha and fill with shadow color
    cairo_set_source_surface(shadow_ct, in, 0, 0);
    cairo_paint(shadow_ct);

    if (cairo_status(shadow_ct) != CAIRO_STATUS_SUCCESS) {
        return;
    }

    double red = SP_RGBA32_R_F(flood_color);
    double green = SP_RGBA32_G_F(flood_color);
    double blue = SP_RGBA32_B_F(flood_color);
    double alpha = SP_RGBA32_A_F(flood_color) * flood_opacity;
    alpha = std::max(0.0, std::min(1.0, alpha));

    cairo_set_source_rgba(shadow_ct, red, green, blue, alpha);
    cairo_set_operator(shadow_ct, CAIRO_OPERATOR_IN);
    cairo_paint(shadow_ct);

    if (cairo_status(shadow_ct) != CAIRO_STATUS_SUCCESS) {
        return;
    }

    // Step 3: Apply Gaussian blur
    blur_surface(shadow, stdDeviation);

    // Step 4: Composite shadow with offset
    cairo_set_operator(ct, CAIRO_OPERATOR_CLEAR);
    cairo_paint(ct);

    cairo_set_operator(ct, CAIRO_OPERATOR_OVER);
    cairo_set_source_surface(ct, shadow, offset_x, offset_y);
    cairo_paint(ct);

    if (cairo_status(ct) != CAIRO_STATUS_SUCCESS) {
        return;
    }

    // Step 5: Composite original on top
    cairo_set_source_surface(ct, in, 0, 0);
    cairo_paint(ct);

    if (cairo_status(ct) != CAIRO_STATUS_SUCCESS) {
        return;
    }

    slot.set(_output, out);
}

bool FilterDropShadow::can_handle_affine(Geom::Affine const &) const
{
    return true;
}

void FilterDropShadow::set_dx(double amount)
{
    dx = amount;
}

void FilterDropShadow::set_dy(double amount)
{
    dy = amount;
}

void FilterDropShadow::set_stdDeviation(double deviation)
{
    stdDeviation = std::max(0.0, deviation);
}

void FilterDropShadow::set_flood_color(guint32 color)
{
    flood_color = color;
}

void FilterDropShadow::set_flood_opacity(double opacity)
{
    flood_opacity = std::max(0.0, std::min(1.0, opacity));
}

void FilterDropShadow::area_enlarge(Geom::IntRect &area, Geom::Affine const &trans) const
{
    Geom::Point offset(dx, dy);
    offset *= trans;
    offset[X] -= trans[4];
    offset[Y] -= trans[5];

    double blur_expansion = (stdDeviation > 0.1) ? 3.0 * stdDeviation : 0.0;
    Geom::Point blur_expand(blur_expansion, blur_expansion);
    if (blur_expansion > 0.0) {
        blur_expand *= trans;
        blur_expand[X] -= trans[4];
        blur_expand[Y] -= trans[5];
    }

    double x0 = area.left();
    double y0 = area.top();
    double x1 = area.right();
    double y1 = area.bottom();

    if (offset[X] > 0) {
        x1 += offset[X];
    } else {
        x0 += offset[X];
    }
    if (offset[Y] > 0) {
        y1 += offset[Y];
    } else {
        y0 += offset[Y];
    }

    x0 -= std::abs(blur_expand[X]);
    y0 -= std::abs(blur_expand[Y]);
    x1 += std::abs(blur_expand[X]);
    y1 += std::abs(blur_expand[Y]);

    area = Geom::IntRect::from_xywh(static_cast<int>(std::floor(x0)), static_cast<int>(std::floor(y0)),
                                    static_cast<int>(std::ceil(x1 - x0)), static_cast<int>(std::ceil(y1 - y0)));
}

double FilterDropShadow::complexity(Geom::Affine const &ctm) const
{
    if (stdDeviation <= 0.1) {
        return 1.0;
    } else {
        return 2.0 + stdDeviation * 0.5;
    }
}

} // namespace Filters
} // namespace Inkscape

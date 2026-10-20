// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_NR_FILTER_DROPSHADOW_H
#define SEEN_NR_FILTER_DROPSHADOW_H

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

#include <cairo.h>

#include "colors/color.h"
#include "display/nr-filter-primitive.h"
#include "display/nr-filter-slot.h"
#include "display/nr-filter-units.h"

namespace Inkscape {
namespace Filters {

class FilterDropShadow : public FilterPrimitive
{
public:
    FilterDropShadow();
    ~FilterDropShadow() override;

    void render_cairo(FilterSlot &slot) const override;
    void area_enlarge(Geom::IntRect &area, Geom::Affine const &trans) const override;
    bool can_handle_affine(Geom::Affine const &) const override;
    double complexity(Geom::Affine const &ctm) const override;

    void set_dx(double amount);
    void set_dy(double amount);
    void set_stdDeviation(double deviation);
    void set_flood_color(guint32 color);
    void set_flood_opacity(double opacity);

    Glib::ustring name() const override { return Glib::ustring("DropShadow"); }

private:
    // Default values from SVG 2.0 spec section 15.25 (Filter Effects)
    // https://www.w3.org/TR/filter-effects-1/#feDropShadowElement
    static constexpr double DEFAULT_DX = 2.0;                  // Default horizontal offset
    static constexpr double DEFAULT_DY = 2.0;                  // Default vertical offset
    static constexpr double DEFAULT_STD_DEVIATION = 2.0;       // Default blur radius
    static constexpr guint32 DEFAULT_FLOOD_COLOR = 0x000000ff; // Opaque black (RGBA)
    static constexpr double DEFAULT_FLOOD_OPACITY = 1.0;       // Fully opaque

    double dx = DEFAULT_DX;
    double dy = DEFAULT_DY;
    double stdDeviation = DEFAULT_STD_DEVIATION;
    guint32 flood_color = DEFAULT_FLOOD_COLOR;
    double flood_opacity = DEFAULT_FLOOD_OPACITY;
};

} // namespace Filters
} // namespace Inkscape

#endif // SEEN_NR_FILTER_DROPSHADOW_H
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
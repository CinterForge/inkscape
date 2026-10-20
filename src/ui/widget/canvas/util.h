// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_WIDGET_CANVAS_UTIL_H
#define INKSCAPE_UI_WIDGET_CANVAS_UTIL_H

#include <cairomm/context.h>
#include <cairomm/refptr.h>
#include <cairomm/region.h>

#include "colors/color.h"

namespace Inkscape {
namespace UI {
namespace Widget {

// Cairo additions

/**
 * Turn a Cairo region into a path on a given Cairo context.
 */
void region_to_path(Cairo::RefPtr<Cairo::Context> const &cr, Cairo::RefPtr<Cairo::Region> const &reg);

/**
 * Shrink a region by d/2 in all directions, while also translating it by (d/2 + t, d/2 + t).
 */
Cairo::RefPtr<Cairo::Region> shrink_region(Cairo::RefPtr<Cairo::Region> const &reg, int d, int t = 0);

inline auto unioned(Cairo::RefPtr<Cairo::Region> a, Cairo::RefPtr<Cairo::Region> const &b)
{
    a->do_union(b);
    return a;
}

// Colour operations

inline auto rgb_to_array(uint32_t rgb)
{
    return std::array{SP_RGBA32_R_U(rgb) / 255.0f, SP_RGBA32_G_U(rgb) / 255.0f, SP_RGBA32_B_U(rgb) / 255.0f};
}

inline auto rgba_to_array(uint32_t rgba)
{
    return std::array{SP_RGBA32_R_U(rgba) / 255.0f, SP_RGBA32_G_U(rgba) / 255.0f, SP_RGBA32_B_U(rgba) / 255.0f, SP_RGBA32_A_U(rgba) / 255.0f};
}

inline auto premultiplied(std::array<float, 4> arr)
{
    arr[0] *= arr[3];
    arr[1] *= arr[3];
    arr[2] *= arr[3];
    return arr;
}

Colors::Color checkerboard_darken(Colors::Color color);

inline std::array<float, 3> checkerboard_darken(uint32_t rgba)
{
    auto const color = checkerboard_darken(Colors::Color{rgba});
    return {(float)color[0], (float)color[1], (float)color[2]};
}

} // namespace Widget
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_WIDGET_CANVAS_UTIL_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

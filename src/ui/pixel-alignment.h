// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef INKSCAPE_PIXEL_ALIGNMENT_H
#define INKSCAPE_PIXEL_ALIGNMENT_H

#include <2geom/rect.h>

namespace Inkscape {
enum class RectLineAlignment
{
    None,
    Outside,
    Inside,
    CenterInside,
    CenterOutside,
};

/** Align vertical/horizontal lines to pixel grid
 *
 * @param x desired center position of line in logical units
 * @param physical_thickness thickness in screen pixels
 * @param scaling_factor pixel scaling factor used by logical units
 * @return pixel aligned center line.
 */
double pixel_align_line(double x, int physical_thickness, int scaling_factor);
Geom::Point pixel_align_line(Geom::Point p, int physical_thickness, int scaling_factor);

Geom::Interval pixel_align(Geom::Interval interval, RectLineAlignment alignment, int physical_thickness,
                           int scaling_factor);
Geom::Rect pixel_align(Geom::Rect rect, RectLineAlignment alignment, int physical_thickness, int scaling_factor);
} // namespace Inkscape

#endif // INKSCAPE_PIXEL_ALIGNMENT_H

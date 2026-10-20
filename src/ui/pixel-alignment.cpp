// SPDX-License-Identifier: GPL-2.0-or-later
#include "pixel-alignment.h"

double Inkscape::pixel_align_line(double x, int line_width, int scaling_factor)
{
    // not using round to avoid directionality based on position relative to 0

    double half = 0.5 * line_width;
    double left = x * scaling_factor - half;
    // using floor(x+0.5) instead of round to avoid directionality based on position relative to 0
    // +1/64 -> for the purpose of more consistent result in common cases
    // Drawings operating on pixel grid will often have x values that are close to integer (0.9999999 or 1.00001)
    // Such line should be consistently placed on the same side of preferred position.
    return (std::floor(left + 0.5 + (line_width % 2 ? +(1 / 64.0) : 0)) + half) / scaling_factor;
}
Geom::Point Inkscape::pixel_align_line(Geom::Point p, int physical_thickness, int scaling_factor)
{
    return Geom::Point(pixel_align_line(p.x(), physical_thickness, scaling_factor),
                       pixel_align_line(p.y(), physical_thickness, scaling_factor));
}

Geom::Interval Inkscape::pixel_align(Geom::Interval interval, Inkscape::RectLineAlignment alignment,
                                     int physical_thickness, int scaling_factor)
{
    if (alignment == Inkscape::RectLineAlignment::None || scaling_factor <= 0) {
        return interval;
    }
    interval *= scaling_factor;
    double x0 = interval.min();
    double x1 = interval.max();
    x0 = std::floor(x0 + 0.5);
    x1 = std::floor(x1 + 0.5);
    interval.setEnds(x0, x1);
    double expand = 0;
    switch (alignment) {
        case RectLineAlignment::Inside:
            expand = -(0.5 * physical_thickness);
            break;
        case RectLineAlignment::Outside:
            expand = +(0.5 * physical_thickness);
            break;
        case RectLineAlignment::CenterInside:
            expand = -((physical_thickness % 2) ? 0.5 : 0);
            break;
        case RectLineAlignment::CenterOutside:
            expand = +((physical_thickness % 2) ? 0.5 : 0);
            break;
        case RectLineAlignment::None:
            break; // should not be possible
    }
    interval.expandBy(expand);
    interval *= 1.0 / scaling_factor;
    return interval;
}

Geom::Rect Inkscape::pixel_align(Geom::Rect rect, RectLineAlignment alignment, int physical_thickness,
                                 int scaling_factor)
{
    if (alignment == RectLineAlignment::None || scaling_factor <= 0) {
        return rect;
    }

    rect[Geom::Dim2::X] = pixel_align(rect[Geom::Dim2::X], alignment, physical_thickness, scaling_factor);
    rect[Geom::Dim2::Y] = pixel_align(rect[Geom::Dim2::Y], alignment, physical_thickness, scaling_factor);
    return rect;
}
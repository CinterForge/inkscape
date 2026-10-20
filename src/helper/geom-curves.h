// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_HELPER_GEOM_CURVES_H
#define INKSCAPE_HELPER_GEOM_CURVES_H

/**
 * @file
 * Specific curve type functions for Inkscape, not provided by lib2geom.
 */
/*
 * Author:
 *   Johan Engelen <goejendaagh@zonnet.nl>
 *
 * Copyright (C) 2008-2009 Johan Engelen
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <2geom/line.h>
#include <2geom/bezier-curve.h>

/// \todo un-inline this function
inline bool is_straight_curve(Geom::BezierCurve const &c)
{
    // the curve can be a quad/cubic bezier, but could still be a perfect straight line
    // if the control points are exactly on the line connecting the initial and final points.
    auto const line = Geom::Line{c.initialPoint(), c.finalPoint()};
    for (int i = 1; i < c.order(); i++) {
        if (!Geom::are_near(c[i], line)) {
            return false;
        }
    }
    return true;
}

inline bool is_straight_curve(Geom::Curve const &c)
{
    if (dynamic_cast<Geom::LineSegment const *>(&c)) {
        return true;
    } else if (auto bezier = dynamic_cast<Geom::BezierCurve const *>(&c)) {
        return is_straight_curve(*bezier);
    } else {
        return false;
    }
}

#endif // INKSCAPE_HELPER_GEOM_CURVES_H

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

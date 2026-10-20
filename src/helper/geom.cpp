// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Specific geometry functions for Inkscape, not provided my lib2geom.
 *
 * Author:
 *   Johan Engelen <goejendaagh@zonnet.nl>
 *
 * Copyright (C) 2008 Johan Engelen
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <algorithm>
#include <array>
#include <cmath>
#include "helper/geom.h"
#include "helper/geom-curves.h"
#include <glib.h>
#include <2geom/curves.h>
#include <2geom/sbasis-to-bezier.h>
#include <2geom/path-intersection.h>
#include <2geom/convex-hull.h>

using Geom::X;
using Geom::Y;

// This gap defines an extra translation on points to avoid zeros in some calculations that really
// affect spiro maths. See https://gitlab.com/inkscape/inkscape/-/work_items/5658 for details.
static Geom::Translate handle_cubic_gap(0.01, 0.01);

//#################################################################################
// BOUNDING BOX CALCULATIONS

/* Fast bbox calculation */
/* Thanks to Nathan Hurst for suggesting it */
static void
cubic_bbox (Geom::Coord x000, Geom::Coord y000, Geom::Coord x001, Geom::Coord y001, Geom::Coord x011, Geom::Coord y011, Geom::Coord x111, Geom::Coord y111, Geom::Rect &bbox)
{
    Geom::Coord a, b, c, D;

    bbox[0].expandTo(x111);
    bbox[1].expandTo(y111);

    // It already contains (x000,y000) and (x111,y111)
    // All points of the Bezier lie in the convex hull of (x000,y000), (x001,y001), (x011,y011) and (x111,y111)
    // So, if it also contains (x001,y001) and (x011,y011) we don't have to compute anything else!
    // Note that we compute it for the X and Y range separately to make it easier to use them below
    bool containsXrange = bbox[0].contains(x001) && bbox[0].contains(x011);
    bool containsYrange = bbox[1].contains(y001) && bbox[1].contains(y011);

    /*
     * xttt = s * (s * (s * x000 + t * x001) + t * (s * x001 + t * x011)) + t * (s * (s * x001 + t * x011) + t * (s * x011 + t * x111))
     * xttt = s * (s2 * x000 + s * t * x001 + t * s * x001 + t2 * x011) + t * (s2 * x001 + s * t * x011 + t * s * x011 + t2 * x111)
     * xttt = s * (s2 * x000 + 2 * st * x001 + t2 * x011) + t * (s2 * x001 + 2 * st * x011 + t2 * x111)
     * xttt = s3 * x000 + 2 * s2t * x001 + st2 * x011 + s2t * x001 + 2st2 * x011 + t3 * x111
     * xttt = s3 * x000 + 3s2t * x001 + 3st2 * x011 + t3 * x111
     * xttt = s3 * x000 + (1 - s) 3s2 * x001 + (1 - s) * (1 - s) * 3s * x011 + (1 - s) * (1 - s) * (1 - s) * x111
     * xttt = s3 * x000 + (3s2 - 3s3) * x001 + (3s - 6s2 + 3s3) * x011 + (1 - 2s + s2 - s + 2s2 - s3) * x111
     * xttt = (x000 - 3 * x001 + 3 * x011 -     x111) * s3 +
     *        (       3 * x001 - 6 * x011 + 3 * x111) * s2 +
     *        (                  3 * x011 - 3 * x111) * s  +
     *        (                                 x111)
     * xttt' = (3 * x000 - 9 * x001 +  9 * x011 - 3 * x111) * s2 +
     *         (           6 * x001 - 12 * x011 + 6 * x111) * s  +
     *         (                       3 * x011 - 3 * x111)
     */

    if (!containsXrange) {
        a = 3 * x000 - 9 * x001 + 9 * x011 - 3 * x111;
        b = 6 * x001 - 12 * x011 + 6 * x111;
        c = 3 * x011 - 3 * x111;

        /*
        * s = (-b +/- sqrt (b * b - 4 * a * c)) / 2 * a;
        */
        if (fabs (a) < Geom::EPSILON) {
            /* s = -c / b */
            if (fabs (b) > Geom::EPSILON) {
                double s;
                s = -c / b;
                if ((s > 0.0) && (s < 1.0)) {
                    double t = 1.0 - s;
                    double xttt = s * s * s * x000 + 3 * s * s * t * x001 + 3 * s * t * t * x011 + t * t * t * x111;
                    bbox[0].expandTo(xttt);
                }
            }
        } else {
            /* s = (-b +/- sqrt (b * b - 4 * a * c)) / 2 * a; */
            D = b * b - 4 * a * c;
            if (D >= 0.0) {
                Geom::Coord d, s, t, xttt;
                /* Have solution */
                d = sqrt (D);
                s = (-b + d) / (2 * a);
                if ((s > 0.0) && (s < 1.0)) {
                    t = 1.0 - s;
                    xttt = s * s * s * x000 + 3 * s * s * t * x001 + 3 * s * t * t * x011 + t * t * t * x111;
                    bbox[0].expandTo(xttt);
                }
                s = (-b - d) / (2 * a);
                if ((s > 0.0) && (s < 1.0)) {
                    t = 1.0 - s;
                    xttt = s * s * s * x000 + 3 * s * s * t * x001 + 3 * s * t * t * x011 + t * t * t * x111;
                    bbox[0].expandTo(xttt);
                }
            }
        }
    }

    if (!containsYrange) {
        a = 3 * y000 - 9 * y001 + 9 * y011 - 3 * y111;
        b = 6 * y001 - 12 * y011 + 6 * y111;
        c = 3 * y011 - 3 * y111;

        if (fabs (a) < Geom::EPSILON) {
            /* s = -c / b */
            if (fabs (b) > Geom::EPSILON) {
                double s;
                s = -c / b;
                if ((s > 0.0) && (s < 1.0)) {
                    double t = 1.0 - s;
                    double yttt = s * s * s * y000 + 3 * s * s * t * y001 + 3 * s * t * t * y011 + t * t * t * y111;
                    bbox[1].expandTo(yttt);
                }
            }
        } else {
            /* s = (-b +/- sqrt (b * b - 4 * a * c)) / 2 * a; */
            D = b * b - 4 * a * c;
            if (D >= 0.0) {
                Geom::Coord d, s, t, yttt;
                /* Have solution */
                d = sqrt (D);
                s = (-b + d) / (2 * a);
                if ((s > 0.0) && (s < 1.0)) {
                    t = 1.0 - s;
                    yttt = s * s * s * y000 + 3 * s * s * t * y001 + 3 * s * t * t * y011 + t * t * t * y111;
                    bbox[1].expandTo(yttt);
                }
                s = (-b - d) / (2 * a);
                if ((s > 0.0) && (s < 1.0)) {
                    t = 1.0 - s;
                    yttt = s * s * s * y000 + 3 * s * s * t * y001 + 3 * s * t * t * y011 + t * t * t * y111;
                    bbox[1].expandTo(yttt);
                }
            }
        }
    }
}

Geom::OptRect
bounds_fast_transformed(Geom::PathVector const & pv, Geom::Affine const & t)
{
    return bounds_exact_transformed(pv, t); //use this as it is faster for now! :)
//    return Geom::bounds_fast(pv * t);
}

Geom::OptRect bounds_exact_transformed(Geom::PathVector const &pv, Geom::Affine const &t)
{
    if (pv.empty()) {
        return {};
    }

    auto const initial = pv.initialPoint() * t;

    // Obtain non-empty initial bbox to avoid having to deal with OptRect.
    auto bbox = Geom::Rect(initial, initial);

    for (auto const &path : pv) {
        bbox.expandTo(path.initialPoint() * t);

        // Don't loop including closing segment, since that segment can never increase the bbox.
        for (auto curve = path.begin(); curve != path.end_open(); ++curve) {
            curve->expandToTransformed(bbox, t);
        }
    }

    return bbox;
}

bool pathv_similar(Geom::PathVector const &apv, Geom::PathVector const &bpv, double precision)
{
    if (apv == bpv) {
        return true;
    }
    size_t totala = apv.curveCount();
    if (totala != bpv.curveCount()) {
        return false;
    }
    for (size_t i = 0; i < totala; i++) {
        for (auto f : { 0.2, 0.4, 0.0 }) {
            if (!Geom::are_near(apv.pointAt(i + f), bpv.pointAt(i + f), precision)) {
                return false;
            }
        }
    }
    return true;
}

/*
* Checks if the path describes a simple rectangle, returning true and populating rect if it does.
*/
Geom::OptRect check_simple_rect(Geom::PathVector const &pathv, double precision)
{
    // "Simple rectangle" is a single path with 4 line segments.
    if (pathv.size() != 1) {
        return {};
    }

    auto const &path = pathv.front();
    // might be a better way to check if all the curves are line segments
    if (!path.closed() || path.size() != 4 ||
        std::any_of(path.begin(), path.end(), [](auto const &curve) { return !curve.isLineSegment(); }))
    {
        return {};
    }

    auto const p0 = path.initialPoint();
    auto start = p0;
    int prev_change = 0;
    for (auto const &curve : path) {
        auto end = curve.finalPoint();
        // define a change in X as 1, change in Y as -1. Both is 0
        int const change = !Geom::are_near(start.x(), end.x(), precision) - !Geom::are_near(start.y(), end.y(), precision);
        if (change == 0 || change == prev_change) {
            // Changing in both x and y, or continuing the same direction as the previous segment.
            return {};
        }
        start = end;
        prev_change = change;
    }

    // if we've made it this far, we can assume that the final point of the second segment is the opposite corner
    auto const p1 = path[1].finalPoint();
    return Geom::Rect(p0, p1);
}

static void
geom_line_wind_distance (Geom::Coord x0, Geom::Coord y0, Geom::Coord x1, Geom::Coord y1, Geom::Point const &pt, int *wind, Geom::Coord *best)
{
    Geom::Coord Ax, Ay, Bx, By, Dx, Dy, s;
    Geom::Coord dist2;

    /* Find distance */
    Ax = x0;
    Ay = y0;
    Bx = x1;
    By = y1;
    Dx = x1 - x0;
    Dy = y1 - y0;
    const Geom::Coord Px = pt[X];
    const Geom::Coord Py = pt[Y];

    if (best) {
        s = ((Px - Ax) * Dx + (Py - Ay) * Dy) / (Dx * Dx + Dy * Dy);
        if (std::isnan(s) || s <= 0.0) { // is nan if Dx and Dy are zero ("A point" == "B point")
            dist2 = (Px - Ax) * (Px - Ax) + (Py - Ay) * (Py - Ay);
        } else if (s >= 1.0) {
            dist2 = (Px - Bx) * (Px - Bx) + (Py - By) * (Py - By);
        } else {
            Geom::Coord Qx, Qy;
            Qx = Ax + s * Dx;
            Qy = Ay + s * Dy;
            dist2 = (Px - Qx) * (Px - Qx) + (Py - Qy) * (Py - Qy);
        }

        if (dist2 < (*best * *best)) *best = sqrt (dist2);
    }

    if (wind) {
        /* Find wind */
        if ((Ax >= Px) && (Bx >= Px)) return;
        if ((Ay >= Py) && (By >= Py)) return;
        if ((Ay < Py) && (By < Py)) return;
        if (Ay == By) return;
        /* Ctach upper y bound */
        if (Ay == Py) {
            if (Ax < Px) *wind -= 1;
            return;
        } else if (By == Py) {
            if (Bx < Px) *wind += 1;
            return;
        } else {
            Geom::Coord Qx;
            /* Have to calculate intersection */
            Qx = Ax + Dx * (Py - Ay) / Dy;
            if (Qx < Px) {
                *wind += (Dy > 0.0) ? 1 : -1;
            }
        }
    }
}

static void
geom_cubic_bbox_wind_distance (Geom::Coord x000, Geom::Coord y000,
                 Geom::Coord x001, Geom::Coord y001,
                 Geom::Coord x011, Geom::Coord y011,
                 Geom::Coord x111, Geom::Coord y111,
                 Geom::Point const &pt,
                 Geom::Rect *bbox, int *wind, Geom::Coord *best,
                 Geom::Coord tolerance)
{
    Geom::Coord x0, y0, x1, y1, len2;
    int needdist, needwind;

    const Geom::Coord Px = pt[X];
    const Geom::Coord Py = pt[Y];

    needdist = 0;
    needwind = 0;

    if (bbox) cubic_bbox (x000, y000, x001, y001, x011, y011, x111, y111, *bbox);

    x0 = std::min (x000, x001);
    x0 = std::min (x0, x011);
    x0 = std::min (x0, x111);
    y0 = std::min (y000, y001);
    y0 = std::min (y0, y011);
    y0 = std::min (y0, y111);
    x1 = std::max (x000, x001);
    x1 = std::max (x1, x011);
    x1 = std::max (x1, x111);
    y1 = std::max (y000, y001);
    y1 = std::max (y1, y011);
    y1 = std::max (y1, y111);

    if (best) {
        /* Quickly adjust to endpoints */
        len2 = (x000 - Px) * (x000 - Px) + (y000 - Py) * (y000 - Py);
        if (len2 < (*best * *best)) *best = (Geom::Coord) sqrt (len2);
        len2 = (x111 - Px) * (x111 - Px) + (y111 - Py) * (y111 - Py);
        if (len2 < (*best * *best)) *best = (Geom::Coord) sqrt (len2);

        if (((x0 - Px) < *best) && ((y0 - Py) < *best) && ((Px - x1) < *best) && ((Py - y1) < *best)) {
            /* Point is inside sloppy bbox */
            /* Now we have to decide, whether subdivide */
            /* fixme: (Lauris) */
            if (((y1 - y0) > 5.0) || ((x1 - x0) > 5.0)) {
                needdist = 1;
            }
        }
    }
    if (!needdist && wind) {
        if ((y1 >= Py) && (y0 < Py) && (x0 < Px)) {
            /* Possible intersection at the left */
            /* Now we have to decide, whether subdivide */
            /* fixme: (Lauris) */
            if (((y1 - y0) > 5.0) || ((x1 - x0) > 5.0)) {
                needwind = 1;
            }
        }
    }

    if (needdist || needwind) {
        Geom::Coord x00t, x0tt, xttt, x1tt, x11t, x01t;
        Geom::Coord y00t, y0tt, yttt, y1tt, y11t, y01t;
        Geom::Coord s, t;

        t = 0.5;
        s = 1 - t;

        x00t = s * x000 + t * x001;
        x01t = s * x001 + t * x011;
        x11t = s * x011 + t * x111;
        x0tt = s * x00t + t * x01t;
        x1tt = s * x01t + t * x11t;
        xttt = s * x0tt + t * x1tt;

        y00t = s * y000 + t * y001;
        y01t = s * y001 + t * y011;
        y11t = s * y011 + t * y111;
        y0tt = s * y00t + t * y01t;
        y1tt = s * y01t + t * y11t;
        yttt = s * y0tt + t * y1tt;

        geom_cubic_bbox_wind_distance (x000, y000, x00t, y00t, x0tt, y0tt, xttt, yttt, pt, nullptr, wind, best, tolerance);
        geom_cubic_bbox_wind_distance (xttt, yttt, x1tt, y1tt, x11t, y11t, x111, y111, pt, nullptr, wind, best, tolerance);
    } else {
        geom_line_wind_distance (x000, y000, x111, y111, pt, wind, best);
    }
}

static void
geom_curve_bbox_wind_distance(Geom::Curve const & c, Geom::Affine const &m,
                 Geom::Point const &pt,
                 Geom::Rect *bbox, int *wind, Geom::Coord *dist,
                 Geom::Coord tolerance, Geom::Rect const *viewbox,
                 Geom::Point &p0) // pass p0 through as it represents the last endpoint added (the finalPoint of last curve)
{
    unsigned order = 0;
    if (Geom::BezierCurve const* b = dynamic_cast<Geom::BezierCurve const*>(&c)) {
        order = b->order();
    }
    if (order == 1) {
        Geom::Point pe = c.finalPoint() * m;
        if (bbox) {
            bbox->expandTo(pe);
        }
        if (dist || wind) {
            if (wind) { // we need to pick fill, so do what we're told
                geom_line_wind_distance (p0[X], p0[Y], pe[X], pe[Y], pt, wind, dist);
            } else { // only stroke is being picked; skip this segment if it's totally outside the viewbox
                Geom::Rect swept(p0, pe);
                if (!viewbox || swept.intersects(*viewbox))
                    geom_line_wind_distance (p0[X], p0[Y], pe[X], pe[Y], pt, wind, dist);
            }
        }
        p0 = pe;
    }
    else if (order == 3) {
        Geom::CubicBezier const& cubic_bezier = static_cast<Geom::CubicBezier const&>(c);
        Geom::Point p1 = cubic_bezier[1] * m;
        Geom::Point p2 = cubic_bezier[2] * m;
        Geom::Point p3 = cubic_bezier[3] * m;

        // get approximate bbox from handles (convex hull property of beziers):
        Geom::Rect swept(p0, p3);
        swept.expandTo(p1);
        swept.expandTo(p2);

        if (!viewbox || swept.intersects(*viewbox)) { // we see this segment, so do full processing
            geom_cubic_bbox_wind_distance ( p0[X], p0[Y],
                                            p1[X], p1[Y],
                                            p2[X], p2[Y],
                                            p3[X], p3[Y],
                                            pt,
                                            bbox, wind, dist, tolerance);
        } else {
            if (wind) { // if we need fill, we can just pretend it's a straight line
                geom_line_wind_distance (p0[X], p0[Y], p3[X], p3[Y], pt, wind, dist);
            } else { // otherwise, skip it completely
            }
        }
        p0 = p3;
    } else { 
        //this case handles sbasis as well as all other curve types
        try {
            Geom::Path sbasis_path = Geom::cubicbezierpath_from_sbasis(c.toSBasis(), 0.1);
            //recurse to convert the new path resulting from the sbasis to svgd
            for (const auto & iter : sbasis_path) {
                geom_curve_bbox_wind_distance(iter, m, pt, bbox, wind, dist, tolerance, viewbox, p0);
            }
        } catch (const Geom::Exception &e) {
            // Curve isFinite failed.
            g_warning("Error parsing curve: %s", e.what());
        }
    }
}

bool 
pointInTriangle(Geom::Point const &p, Geom::Point const &p1, Geom::Point const &p2, Geom::Point const &p3)
{
    //http://totologic.blogspot.com.es/2014/01/accurate-point-in-triangle-test.html
    using Geom::X;
    using Geom::Y;
    double denominator = (p1[X]*(p2[Y] - p3[Y]) + p1[Y]*(p3[X] - p2[X]) + p2[X]*p3[Y] - p2[Y]*p3[X]);
    double t1 = (p[X]*(p3[Y] - p1[Y]) + p[Y]*(p1[X] - p3[X]) - p1[X]*p3[Y] + p1[Y]*p3[X]) / denominator;
    double t2 = (p[X]*(p2[Y] - p1[Y]) + p[Y]*(p1[X] - p2[X]) - p1[X]*p2[Y] + p1[Y]*p2[X]) / -denominator;
    double s = t1 + t2;

    return 0 <= t1 && t1 <= 1 && 0 <= t2 && t2 <= 1 && s <= 1;
}


/* Calculates...
   and returns ... in *wind and the distance to ... in *dist.
   Returns bounding box in *bbox if bbox!=NULL.
 */
void
pathv_matrix_point_bbox_wind_distance (Geom::PathVector const & pathv, Geom::Affine const &m, Geom::Point const &pt,
                         Geom::Rect *bbox, int *wind, Geom::Coord *dist,
                         Geom::Coord tolerance, Geom::Rect const *viewbox)
{
    if (pathv.empty()) {
        if (wind) *wind = 0;
        if (dist) *dist = Geom::infinity();
        return;
    }

    // remember last point of last curve
    Geom::Point p0(0,0);

    // remembering the start of subpath
    Geom::Point p_start(0,0);
    bool start_set = false;

    for (const auto & it : pathv) {

        if (start_set) { // this is a new subpath
            if (wind && (p0 != p_start)) // for correct fill picking, each subpath must be closed
                geom_line_wind_distance (p0[X], p0[Y], p_start[X], p_start[Y], pt, wind, dist);
        }
        p0 = it.initialPoint() * m;
        p_start = p0;
        start_set = true;
        if (bbox) {
            bbox->expandTo(p0);
        }

        // loop including closing segment if path is closed
        for (Geom::Path::const_iterator cit = it.begin(); cit != it.end_default(); ++cit) {
            geom_curve_bbox_wind_distance(*cit, m, pt, bbox, wind, dist, tolerance, viewbox, p0);
        }
    }

    if (start_set) { 
        if (wind && (p0 != p_start)) // for correct picking, each subpath must be closed
            geom_line_wind_distance (p0[X], p0[Y], p_start[X], p_start[Y], pt, wind, dist);
    }
}

//#################################################################################

/**
 * An exact check for whether the two pathvectors intersect or overlap, including the case of
 * a line crossing through a solid shape.
 */
bool pathvs_have_nonempty_overlap(Geom::PathVector const &a, Geom::PathVector const &b)
{
    // Fast negative check using bounds.
    auto intersected_bounds = a.boundsFast() & b.boundsFast();
    if (!intersected_bounds) {
        return false;
    }

    // Slightly slower positive check using vertex containment.
    for (auto &node : b.nodes()) {
        if (a.winding(node)) {
            return true;
        }
    }
   for (auto &node : a.nodes()) {
        if (b.winding(node)) {
            return true;
        }
    }

    // The winding method may not detect nodeless Bézier arcs in one pathvector glancing
    // the edge of the other pathvector. We must deal with this possibility by also checking for
    // intersections of boundaries.
    auto crossings = Geom::SimpleCrosser().crossings(a, b);
    if (crossings.empty()) {
        return false;
    }
    auto is_empty = [](Geom::Crossings const &xings) -> bool { return xings.empty(); };
    if (!std::all_of(crossings.begin(), crossings.end(), is_empty)) { // An intersection has been found
        return true;
    }
    return false;
}

/**
 * Checks whether the filled region defined by pathvector @a a and fill rule @a fill_rule
 * completely contains pathvector @a b.
 */
bool pathv_fully_contains(Geom::PathVector const &a, Geom::PathVector const &b, FillRule fill_rule, double precision)
{
    // Fast-path the case where a is a rectangle.
    auto const b_bounds = b.boundsExact();
    if (auto const a_rect = check_simple_rect(a, precision)) {
        return a_rect->contains(b_bounds);
    }

    // At minimum, bbox of a must contain bbox of b
    if (!a.boundsExact().contains(b_bounds)) {
        return false;
    }

    // There must be no intersections of edges.
    // Fixme: This condition is too strict.
    if (!a.intersect(b, precision).empty()) {
        return false;
    }

    // Check winding number of first node of each subpath.
    // Fixme: This condition is also too strict.
    for (auto const &path : b) {
        if (!is_point_inside(fill_rule, a.winding(path.initialPoint()))) {
            return false;
        }
    }

    // BBox is fully contained, no intersections, passed the winding test, the path must be contained
    return true;
}

/*
 * Converts all segments in all paths to Geom::LineSegment or Geom::HLineSegment or
 * Geom::VLineSegment or Geom::CubicBezier.
 */
Geom::PathVector
pathv_to_linear_and_cubic_beziers( Geom::PathVector const &pathv )
{
    Geom::PathVector output;

    for (const auto & pit : pathv) {
        output.push_back( Geom::Path() );
        output.back().setStitching(true);
        output.back().start( pit.initialPoint() );

        for (Geom::Path::const_iterator cit = pit.begin(); cit != pit.end_open(); ++cit) {
            if (is_straight_curve(*cit)) {
                Geom::LineSegment l(cit->initialPoint(), cit->finalPoint());
                output.back().append(l);
            } else {
                Geom::BezierCurve const *curve = dynamic_cast<Geom::BezierCurve const *>(&*cit);
                if (curve && curve->order() == 3) {
                    Geom::CubicBezier b((*curve)[0], (*curve)[1], (*curve)[2], (*curve)[3]);
                    output.back().append(b);
                } else {
                    // convert all other curve types to cubicbeziers
                    try {
                        Geom::Path cubicbezier_path = Geom::cubicbezierpath_from_sbasis(cit->toSBasis(), 0.1);
                        cubicbezier_path.close(false);
                        output.back().append(cubicbezier_path);
                    } catch (const Geom::Exception &e) {
                        // Curve isFinite failed.
                        g_warning("Error parsing curve: %s", e.what());
                        break;
                    }
                }
            }
        }
        
        output.back().close( pit.closed() );
    }
    
    return output;
}

/*
 * Converts all segments in all paths to Geom::LineSegment.  There is an intermediate
 * stage where some may be converted to beziers.  maxdisp is the maximum displacement from
 * the line segment to the bezier curve; ** maxdisp is not used at this moment **.
 *
 * This is NOT a terribly fast method, but it should give a solution close to the one with the
 * fewest points.
 */
Geom::PathVector
pathv_to_linear( Geom::PathVector const &pathv, double /*maxdisp*/)
{
    Geom::PathVector output;
    Geom::PathVector tmppath = pathv_to_linear_and_cubic_beziers(pathv);
    
    // Now all path segments are either already lines, or they are beziers.

    for (const auto & pit : tmppath) {
        output.push_back( Geom::Path() );
        output.back().start( pit.initialPoint() );
        output.back().close( pit.closed() );

        for (Geom::Path::const_iterator cit = pit.begin(); cit != pit.end_open(); ++cit) {
            if (is_straight_curve(*cit)) {
                Geom::LineSegment ls(cit->initialPoint(), cit->finalPoint());
                output.back().append(ls);
            } 
            else { /* all others must be Bezier curves */
                Geom::BezierCurve const *curve = dynamic_cast<Geom::BezierCurve const *>(&*cit);
                std::vector<Geom::Point> bzrpoints = curve->controlPoints();
                Geom::Point A = bzrpoints[0];
                Geom::Point B = bzrpoints[1];
                Geom::Point C = bzrpoints[2];
                Geom::Point D = bzrpoints[3];
                std::vector<Geom::Point> pointlist;
                pointlist.push_back(A);
                recursive_bezier4(
                   A[X], A[Y], 
                   B[X], B[Y], 
                   C[X], C[Y], 
                   D[X], D[Y],
                   pointlist, 
                   0);
                pointlist.push_back(D);
                Geom::Point r1 = pointlist[0];
                for (unsigned int i=1; i<pointlist.size();i++){
                   Geom::Point prev_r1 = r1;
                   r1 = pointlist[i];
                   Geom::LineSegment ls(prev_r1, r1);
                   output.back().append(ls);
                }
                pointlist.clear();
           }
        }
    }
    
    return output;
}

/*
 * Converts all segments in all paths to Geom Cubic bezier.
 * This is used in lattice2 LPE, maybe is better move the function to the effect
 * But maybe could be usable by others, so i put here.
 * The straight curve part is needed as is for the effect to work appropriately
 */
Geom::PathVector
pathv_to_cubicbezier( Geom::PathVector const &pathv, bool nolines)
{
    Geom::PathVector output;
    for (const auto & pit : pathv) {
        if (pit.empty()) {
            continue;
        }
        output.push_back( Geom::Path() );
        output.back().start( pit.initialPoint() );
        output.back().close( pit.closed() );
        bool end_open = false;
        if (pit.closed()) {
            auto const &closingline = pit.back_closed();
            if (!are_near(closingline.initialPoint(), closingline.finalPoint())) {
                end_open = true;
            }
        }
        Geom::Path pitCubic = (Geom::Path)pit;
        if(end_open && pit.closed()){
            pitCubic.close(false);
            pitCubic.appendNew<Geom::LineSegment>( pitCubic.initialPoint() );
            pitCubic.close(true);
        }
        for (Geom::Path::iterator cit = pitCubic.begin(); cit != pitCubic.end_open(); ++cit) {
            Geom::BezierCurve const *curve = dynamic_cast<Geom::BezierCurve const *>(&*cit);
            // is_straight curves dont work for bspline
            if (nolines && is_straight_curve(*cit)) {
                Geom::CubicBezier b(cit->initialPoint(), cit->pointAt(0.3334) * handle_cubic_gap,
                                    cit->finalPoint(), cit->finalPoint());
                output.back().append(b);
            } else if (!curve || curve->order() != 3) {
                // convert all other curve types to cubicbeziers
                Geom::Path cubicbezier_path = Geom::cubicbezierpath_from_sbasis(cit->toSBasis(), 0.1);
                output.back().append(cubicbezier_path);
            } else if (Geom::are_near((*curve)[0],(*curve)[1]) && Geom::are_near((*curve)[2],(*curve)[3])){
                Geom::LineSegment ls(cit->initialPoint(), cit->finalPoint());
                output.back().append(ls);
            } else {
                Geom::CubicBezier b((*curve)[0], (*curve)[1], (*curve)[2], (*curve)[3]);
                output.back().append(b);
            }
        }
    }

    return output;
}

//Study move to 2Geom

/*
 * The following functions are used (mostly) by LPE's. Their main feature is that they exclude
 * almost degenerate closing curves (lib2geom will only remove exactly zero length closing curves
 * but Inkscape will remove closing curves with length less than Geom::EPSILON when editing). You
 * can get almost zero length closing curves due to rounding errors, especially with relative
 * coordinates.
 *
 * Empty paths ("M 0,0", M 0,0 z") are always ignored.
 */
size_t
count_pathvector_nodes(Geom::PathVector const &pathv) {
    size_t tot = 0;
    for (auto const &subpath : pathv) {
        tot += count_path_nodes(subpath);
    }
    return tot;
}

size_t
count_pathvector_curves(Geom::PathVector const &pathv) {
    size_t tot = 0;
    for (auto const &subpath : pathv) {
        tot += count_path_curves(subpath);
    }
    return tot;
}

/*
 * For closed paths, the number of nodes is exactly the same as the number of curves.
 * For open paths, there is one more node that number of curves.
 */
size_t count_path_nodes(Geom::Path const &path)
{
    size_t tot = count_path_curves(path);
    if (!path.closed()) {
        tot++;
    }
    return tot;
}

size_t count_path_curves(Geom::Path const &path)
{
    if (path.empty()) {
        std::cerr << "count_path_curves: path is empty! " << path << std::endl;
        return 0;
    }

    size_t tot = path.size();
    if (path.closed()) {
        auto const &closingline = path.closingSegment(); // Always a Geom::LineSegment.
        if (!path.closingSegment().isDegenerate() &&  // Degenerate segment already not counted.
            are_near(closingline.initialPoint(), closingline.finalPoint())) {
            std::cerr << "count_path_curves: found almost degenerate path! "
                      << path << std::endl;
            tot -= 1;
        }
    }
    return tot;
}

// The next routine is modified from curv4_div::recursive_bezier from file agg_curves.cpp
//----------------------------------------------------------------------------
// Anti-Grain Geometry (AGG) - Version 2.5
// A high quality rendering engine for C++
// Copyright (C) 2002-2006 Maxim Shemanarev
// Contact: mcseem@antigrain.com
//          mcseemagg@yahoo.com
//          http://antigrain.com
// 
// AGG is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// AGG is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with AGG; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
// MA 02110-1301, USA.
//----------------------------------------------------------------------------
void
recursive_bezier4(const double x1, const double y1, 
                  const double x2, const double y2, 
                  const double x3, const double y3, 
                  const double x4, const double y4,
                  std::vector<Geom::Point> &m_points,
                  int level)
    {
        // some of these should be parameters, but do it this way for now.
        const double curve_collinearity_epsilon              = 1e-30;
        const double curve_angle_tolerance_epsilon           = 0.01;
        double       m_cusp_limit                            = 0.0;
        double       m_angle_tolerance                       = 0.0;
        double       m_approximation_scale                   = 1.0;
        double       m_distance_tolerance_square = 0.5 / m_approximation_scale;
        m_distance_tolerance_square *= m_distance_tolerance_square;
        enum curve_recursion_limit_e { curve_recursion_limit = 32 };
#define calc_sq_distance(A,B,C,D) ((A-C)*(A-C) + (B-D)*(B-D))

        if(level > curve_recursion_limit) 
        {
            return;
        }


        // Calculate all the mid-points of the line segments
        //----------------------
        double x12   = (x1 + x2) / 2;
        double y12   = (y1 + y2) / 2;
        double x23   = (x2 + x3) / 2;
        double y23   = (y2 + y3) / 2;
        double x34   = (x3 + x4) / 2;
        double y34   = (y3 + y4) / 2;
        double x123  = (x12 + x23) / 2;
        double y123  = (y12 + y23) / 2;
        double x234  = (x23 + x34) / 2;
        double y234  = (y23 + y34) / 2;
        double x1234 = (x123 + x234) / 2;
        double y1234 = (y123 + y234) / 2;


        // Try to approximate the full cubic curve by a single straight line
        //------------------
        double dx = x4-x1;
        double dy = y4-y1;

        double d2 = fabs(((x2 - x4) * dy - (y2 - y4) * dx));
        double d3 = fabs(((x3 - x4) * dy - (y3 - y4) * dx));
        double da1, da2, k;

        switch((int(d2 > curve_collinearity_epsilon) << 1) +
                int(d3 > curve_collinearity_epsilon))
        {
        case 0:
            // All collinear OR p1==p4
            //----------------------
            k = dx*dx + dy*dy;
            if(k == 0)
            {
                d2 = calc_sq_distance(x1, y1, x2, y2);
                d3 = calc_sq_distance(x4, y4, x3, y3);
            }
            else
            {
                k   = 1 / k;
                da1 = x2 - x1;
                da2 = y2 - y1;
                d2  = k * (da1*dx + da2*dy);
                da1 = x3 - x1;
                da2 = y3 - y1;
                d3  = k * (da1*dx + da2*dy);
                if(d2 > 0 && d2 < 1 && d3 > 0 && d3 < 1)
                {
                    // Simple collinear case, 1---2---3---4
                    // We can leave just two endpoints
                    return;
                }
                     if(d2 <= 0) d2 = calc_sq_distance(x2, y2, x1, y1);
                else if(d2 >= 1) d2 = calc_sq_distance(x2, y2, x4, y4);
                else             d2 = calc_sq_distance(x2, y2, x1 + d2*dx, y1 + d2*dy);

                     if(d3 <= 0) d3 = calc_sq_distance(x3, y3, x1, y1);
                else if(d3 >= 1) d3 = calc_sq_distance(x3, y3, x4, y4);
                else             d3 = calc_sq_distance(x3, y3, x1 + d3*dx, y1 + d3*dy);
            }
            if(d2 > d3)
            {
                if(d2 < m_distance_tolerance_square)
                {
                    m_points.emplace_back(x2, y2);
                    return;
                }
            }
            else
            {
                if(d3 < m_distance_tolerance_square)
                {
                    m_points.emplace_back(x3, y3);
                    return;
                }
            }
            break;

        case 1:
            // p1,p2,p4 are collinear, p3 is significant
            //----------------------
            if(d3 * d3 <= m_distance_tolerance_square * (dx*dx + dy*dy))
            {
                if(m_angle_tolerance < curve_angle_tolerance_epsilon)
                {
                    m_points.emplace_back(x23, y23);
                    return;
                }

                // Angle Condition
                //----------------------
                da1 = fabs(atan2(y4 - y3, x4 - x3) - atan2(y3 - y2, x3 - x2));
                if(da1 >= M_PI) da1 = 2*M_PI - da1;

                if(da1 < m_angle_tolerance)
                {
                    m_points.emplace_back(x2, y2);
                    m_points.emplace_back(x3, y3);
                    return;
                }

                if(m_cusp_limit != 0.0)
                {
                    if(da1 > m_cusp_limit)
                    {
                        m_points.emplace_back(x3, y3);
                        return;
                    }
                }
            }
            break;

        case 2:
            // p1,p3,p4 are collinear, p2 is significant
            //----------------------
            if(d2 * d2 <= m_distance_tolerance_square * (dx*dx + dy*dy))
            {
                if(m_angle_tolerance < curve_angle_tolerance_epsilon)
                {
                    m_points.emplace_back(x23, y23);
                    return;
                }

                // Angle Condition
                //----------------------
                da1 = fabs(atan2(y3 - y2, x3 - x2) - atan2(y2 - y1, x2 - x1));
                if(da1 >= M_PI) da1 = 2*M_PI - da1;

                if(da1 < m_angle_tolerance)
                {
                    m_points.emplace_back(x2, y2);
                    m_points.emplace_back(x3, y3);
                    return;
                }

                if(m_cusp_limit != 0.0)
                {
                    if(da1 > m_cusp_limit)
                    {
                        m_points.emplace_back(x2, y2);
                        return;
                    }
                }
            }
            break;

        case 3: 
            // Regular case
            //-----------------
            if((d2 + d3)*(d2 + d3) <= m_distance_tolerance_square * (dx*dx + dy*dy))
            {
                // If the curvature doesn't exceed the distance_tolerance value
                // we tend to finish subdivisions.
                //----------------------
                if(m_angle_tolerance < curve_angle_tolerance_epsilon)
                {
                    m_points.emplace_back(x23, y23);
                    return;
                }

                // Angle & Cusp Condition
                //----------------------
                k   = atan2(y3 - y2, x3 - x2);
                da1 = fabs(k - atan2(y2 - y1, x2 - x1));
                da2 = fabs(atan2(y4 - y3, x4 - x3) - k);
                if(da1 >= M_PI) da1 = 2*M_PI - da1;
                if(da2 >= M_PI) da2 = 2*M_PI - da2;

                if(da1 + da2 < m_angle_tolerance)
                {
                    // Finally we can stop the recursion
                    //----------------------
                    m_points.emplace_back(x23, y23);
                    return;
                }

                if(m_cusp_limit != 0.0)
                {
                    if(da1 > m_cusp_limit)
                    {
                        m_points.emplace_back(x2, y2);
                        return;
                    }

                    if(da2 > m_cusp_limit)
                    {
                        m_points.emplace_back(x3, y3);
                        return;
                    }
                }
            }
            break;
        }

        // Continue subdivision
        //----------------------
        recursive_bezier4(x1, y1, x12, y12, x123, y123, x1234, y1234, m_points, level + 1); 
        recursive_bezier4(x1234, y1234, x234, y234, x34, y34, x4, y4, m_points, level + 1); 
}

/**
 * Returns whether an affine transformation is approximately a dihedral transformation, i.e.
 * it maps the axis-aligned unit square centred at the origin to itself.
 */
bool approx_dihedral(Geom::Affine const &affine, double eps)
{
    // Ensure translation is zero.
    if (std::abs(affine[4]) > eps || std::abs(affine[5]) > eps) return false;

    // Ensure matrix has integer components.
    std::array<int, 4> arr;
    for (int i = 0; i < 4; i++) {
        arr[i] = std::round(affine[i]);
        if (std::abs(affine[i] - arr[i]) > eps) return false;
        arr[i] = std::abs(arr[i]);
    }

    // Ensure rounded matrix is correct.
    return arr == std::array {1, 0, 0, 1 } || arr == std::array{ 0, 1, 1, 0 };
}

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

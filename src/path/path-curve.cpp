// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Johan Engelen
 *
 * Copyright (C) 2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2002 Lauris Kaplinski
 * Copyright (C) 2008 Johan Engelen
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "path/path-curve.h"

#include <glib.h> // g_error
#include <2geom/sbasis-geometric.h>
#include <2geom/sbasis-to-bezier.h>
#include <2geom/point.h>

Geom::Path rect_to_open_path(Geom::Rect const &rect)
{
    auto path = Geom::Path{rect.corner(0)};

    for (int i = 3; i >= 0; --i) {
        path.appendNew<Geom::LineSegment>(rect.corner(i));
    }

    // When _constrained_ snapping to a path, the 2geom::SimpleCrosser will be invoked which doesn't consider the closing segment.
    // of a path. Consequently, in case we want to snap to for example the page border, we must provide all four sides of the
    // rectangle explicitly

    return path;
}

void closepath_current(Geom::Path &path)
{
    if (path.size() > 0 && dynamic_cast<Geom::LineSegment const *>(&path.back_open())) {
        path.erase_last();
    } else {
        path.setFinal(path.initialPoint());
    }
    path.close();
}

bool is_closed(Geom::PathVector const &pathv)
{
    if (pathv.empty()) {
        return false;
    }

    for (auto const &path : pathv) {
        if (!path.closed()) {
            return false;
        }
    }

    return true;
}

Geom::Curve const *get_last_segment(Geom::PathVector const &pathv)
{
    if (pathv.empty() || pathv.back().empty()) {
        return nullptr;
    }
    return &pathv.back().back_default();
}

Geom::Curve const *get_first_segment(Geom::PathVector const &pathv)
{
    if (pathv.empty() || pathv.front().empty()) {
        return nullptr;
    }
    return &pathv.front().front();
}

void pathvector_append(Geom::PathVector &to, Geom::PathVector const &pathv, bool use_lineto)
{
    if (pathv.empty()) {
        return;
    }

    if (use_lineto) {
        auto it = pathv.begin();
        if (!to.empty()) {
            Geom::Path &lastpath = to.back();
            lastpath.appendNew<Geom::LineSegment>(it->initialPoint());
            lastpath.append(*it);
        } else {
            to.push_back(*it);
        }

        for (++it; it != pathv.end(); ++it) {
            to.push_back(*it);
        }
    } else {
        for (auto const &it : pathv) {
            to.push_back(it);
        }
    }
}

bool pathvector_append_continuous(Geom::PathVector &to, Geom::PathVector const &pathv, double tolerance)
{
    if (is_closed(to) || is_closed(pathv)) {
        return false;
    }

    if (pathv.empty()) {
        return true;
    }

    if (to.empty()) {
        to = pathv;
        return true;
    }

    if (Geom::LInfty(to.finalPoint() - pathv.initialPoint()) <= tolerance) {
        // c1's first subpath can be appended to this curve's last subpath
        Geom::PathVector::const_iterator path_it = pathv.begin();
        Geom::Path &lastpath = to.back();

        Geom::Path newfirstpath(*path_it);
        newfirstpath.setInitial(lastpath.finalPoint());
        lastpath.append(newfirstpath);

        for (++path_it; path_it != pathv.end(); ++path_it) {
            to.push_back(*path_it);
        }

    } else {
        pathvector_append(to, pathv, true);
    }

    return true;
}

void backspace(Geom::PathVector &pathv)
{
    if (pathv.empty()) {
        return;
    }
    backspace(pathv.back());
}

void backspace(Geom::Path &path)
{
    if (!path.empty()) {
        path.erase_last();
        path.close(false);
    }
}

/**
 * TODO: add comments about what this method does and what assumptions are made and requirements are put on SPCurve
 (2:08:18 AM) Johan: basically, i convert the path to pw<d2>
(2:08:27 AM) Johan: then i calculate an offset path
(2:08:29 AM) Johan: to move the knots
(2:08:36 AM) Johan: then i add it
(2:08:40 AM) Johan: then convert back to path
If I remember correctly, this moves the firstpoint to new_p0, and the lastpoint to new_p1, and moves all nodes in between according to their arclength (interpolates the movement amount)
 */
void stretch_endpoints(Geom::PathVector &pathv, Geom::Point const &new_p0, Geom::Point const &new_p1)
{
    if (pathv.empty()) {
        return;
    }

    auto const offset0 = new_p0 - pathv.initialPoint();
    auto const offset1 = new_p1 - pathv.finalPoint();

    Geom::Piecewise<Geom::D2<Geom::SBasis>> pwd2 = pathv.front().toPwSb();
    Geom::Piecewise<Geom::SBasis> arclength = Geom::arcLengthSb(pwd2);
    if (arclength.lastValue() <= 0) {
        g_error("stretch_endpoints - arclength <= 0");
    }
    arclength *= 1./arclength.lastValue();
    auto const A = offset0;
    auto const B = offset1;
    Geom::Piecewise<Geom::SBasis> offsetx = (arclength*-1.+1)*A[0] + arclength*B[0];
    Geom::Piecewise<Geom::SBasis> offsety = (arclength*-1.+1)*A[1] + arclength*B[1];
    Geom::Piecewise<Geom::D2<Geom::SBasis>> offsetpath = Geom::sectionize(Geom::D2<Geom::Piecewise<Geom::SBasis>>(offsetx, offsety));
    pwd2 += offsetpath;
    pathv = Geom::path_from_piecewise(pwd2, 0.001);
}

void move_endpoints(Geom::PathVector &pathv, Geom::Point const &new_p0, Geom::Point const &new_p1)
{
    if (!pathv.empty()) {
        move_endpoints(pathv.front(), new_p0, new_p1);
    }
}

void move_endpoints(Geom::Path &path, Geom::Point const &new_p0, Geom::Point const &new_p1)
{
    path.setInitial(new_p0);
    path.setFinal(new_p1);
}

size_t node_count(Geom::PathVector const &pathv)
{
    size_t nr = 0;

    for (auto const &path : pathv) {
        // if the path does not have any segments, it is a naked moveto,
        // and therefore any path has at least one valid node
        size_t psize = std::max<size_t>(1, path.size_closed());
        nr += psize;
        if (path.closed() && path.size_closed() > 0) {
            auto const &closingline = path.back_closed();
            // the closing line segment is always of type
            // Geom::LineSegment.
            if (Geom::are_near(closingline.initialPoint(), closingline.finalPoint())) {
                // closingline.isDegenerate() did not work, because it only checks for
                // *exact* zero length, which goes wrong for relative coordinates and
                // rounding errors...
                // the closing line segment has zero-length. So stop before that one!
                nr -= 1;
            }
        }
    }

    return nr;
}

void last_point_additive_move(Geom::PathVector &pathv, Geom::Point const &p)
{
    if (pathv.empty()) {
        return;
    }

    pathv.back().setFinal(pathv.back().finalPoint() + p);

    // Move handle as well when the last segment is a cubic bezier segment:
    // TODO: what to do for quadratic beziers?
    if (auto const lastcube = dynamic_cast<Geom::CubicBezier const *>(&pathv.back().back())) {
        auto newcube = Geom::CubicBezier{*lastcube};
        newcube.setPoint(2, newcube[2] + p);
        pathv.back().replace(--pathv.back().end(), newcube);
    }
}

Geom::Path path_from_curve(std::unique_ptr<Geom::Curve> curve)
{
    auto path = Geom::Path{curve->initialPoint()};
    path.append(curve.release());
    return path;
}

Geom::Path path_from_curve(Geom::Curve const &curve)
{
    return path_from_curve(std::unique_ptr<Geom::Curve>(curve.duplicate()));
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:

// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_HELPER_GEOM_H
#define INKSCAPE_HELPER_GEOM_H

/**
 * @file
 * Specific geometry functions for Inkscape, not provided my lib2geom.
 */
/*
 * Author:
 *   Johan Engelen <goejendaagh@zonnet.nl>
 *
 * Copyright (C) 2008 Johan Engelen
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <vector>
#include <2geom/forward.h>
#include <2geom/rect.h>
#include <2geom/affine.h>
#include "livarot/LivarotDefs.h"
#include "mathfns.h"

Geom::OptRect bounds_fast_transformed(Geom::PathVector const & pv, Geom::Affine const & t);
Geom::OptRect bounds_exact_transformed(Geom::PathVector const & pv, Geom::Affine const & t);

void pathv_matrix_point_bbox_wind_distance ( Geom::PathVector const & pathv, Geom::Affine const &m, Geom::Point const &pt,
                                             Geom::Rect *bbox, int *wind, Geom::Coord *dist,
                                             Geom::Coord tolerance, Geom::Rect const *viewbox);

bool pathvs_have_nonempty_overlap(Geom::PathVector const &a, Geom::PathVector const &b);
bool pathv_fully_contains(Geom::PathVector const &a, Geom::PathVector const &b, FillRule fill_rule, double precision = Geom::EPSILON);

// Functions used mostly by LPE's
size_t count_pathvector_nodes(Geom::PathVector const &pathv);
size_t count_path_nodes(Geom::Path const &path);
size_t count_pathvector_curves(Geom::PathVector const &pathv);
size_t count_path_curves(Geom::Path const &path);

bool pointInTriangle(Geom::Point const &p, Geom::Point const &p1, Geom::Point const &p2, Geom::Point const &p3);
Geom::PathVector pathv_to_linear_and_cubic_beziers( Geom::PathVector const &pathv );
Geom::PathVector pathv_to_linear( Geom::PathVector const &pathv, double maxdisp );
Geom::PathVector pathv_to_cubicbezier( Geom::PathVector const &pathv, bool nolines);
bool pathv_similar(Geom::PathVector const &apv, Geom::PathVector const &bpv, double precission = 0.001);
Geom::OptRect check_simple_rect(Geom::PathVector const &pathv, double precision = Geom::EPSILON);
void recursive_bezier4(const double x1, const double y1, const double x2, const double y2, 
                       const double x3, const double y3, const double x4, const double y4,
                       std::vector<Geom::Point> &pointlist,
                       int level);
bool approx_dihedral(Geom::Affine const &affine, double eps = 0.0001);

/// Returns signed area of triangle given by points; may be negative.
inline Geom::Coord triangle_area(Geom::Point const &p1, Geom::Point const &p2, Geom::Point const &p3)
{
    using Geom::X;
    using Geom::Y;
    return p1[X] * p2[Y] + p1[Y] * p3[X] + p2[X] * p3[Y] - p2[Y] * p3[X] - p1[Y] * p2[X] - p1[X] * p3[Y];
}

inline auto round_down(Geom::IntPoint const &a, Geom::IntPoint const &b)
{
    using namespace Inkscape::Util;
    return Geom::IntPoint(round_down(a.x(), b.x()), round_down(a.y(), b.y()));
}

inline auto expandedBy(Geom::IntRect rect, int amount)
{
    rect.expandBy(amount);
    return rect;
}

inline auto expandedBy(Geom::Rect rect, double amount)
{
    rect.expandBy(amount);
    return rect;
}

inline Geom::OptRect expandedBy(Geom::OptRect const &rect, double amount)
{
    if (!rect) {
        return {};
    } else {
        return expandedBy(*rect, amount);
    }
}

inline auto operator/(double a, Geom::Point const &b)
{
    return Geom::Point(a / b.x(), a / b.y());
}

inline auto absolute(Geom::Point const &a)
{
    return Geom::Point(std::abs(a.x()), std::abs(a.y()));
}

inline auto min(Geom::IntPoint const &a)
{
    return std::min(a.x(), a.y());
}

inline auto min(Geom::Point const &a)
{
    return std::min(a.x(), a.y());
}

inline auto max(Geom::IntPoint const &a)
{
    return std::max(a.x(), a.y());
}

inline auto max(Geom::Point const &a)
{
    return std::max(a.x(), a.y());
}

/// Get the bounding box of a collection of points.
template <typename... Args>
auto bounds_of(Geom::Point const &pt, Args const &... args)
{
    if constexpr (sizeof...(args) == 0) {
        return Geom::Rect(pt, pt);
    } else {
        auto rect = bounds_of(args...);
        rect.expandTo(pt);
        return rect;
    }
}

inline auto floor(Geom::Rect const &rect)
{
    return Geom::Rect(rect.min().floor(), rect.max().floor());
}

inline auto roundedOutwards(Geom::OptRect const &rect)
{
    return rect ? rect->roundOutwards() : Geom::OptIntRect();
}

/**
 * Compute the maximum factor by which @a affine can increase a vector's length.
 */
inline double max_expansion(Geom::Affine const &affine)
{
    auto const t = (Geom::sqr(affine[0]) + Geom::sqr(affine[1]) + Geom::sqr(affine[2]) + Geom::sqr(affine[3])) / 2;
    auto const d = std::abs(affine.det());
    return std::sqrt(t + std::sqrt(std::max(t - d, 0.0) * (t + d)));
}

#endif // INKSCAPE_HELPER_GEOM_H

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

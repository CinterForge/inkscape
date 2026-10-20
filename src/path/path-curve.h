// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2002 Lauris Kaplinski
 * Copyright (C) 2008 Johan Engelen
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_DISPLAY_CURVE_H
#define SEEN_DISPLAY_CURVE_H

#include <cstddef>
#include <optional>
#include <2geom/pathvector.h>

/// Whether all subpaths are closed. Returns false if the curve is empty.
bool is_closed(Geom::PathVector const &pathv);

/**
 * returns the number of nodes in a path, used for statusbar text when selecting an spcurve.
 * Sum of nodes in all the paths. When a path is closed, and its closing line segment is of zero-length,
 * this function will not count the closing knot double (so basically ignores the closing line segment when it has zero length)
 */
size_t node_count(Geom::PathVector const &pathv);

/**
 * Return last pathsegment (possibly the closing path segment) of the last path in PathVector or null.
 * If the last path is empty (contains only a moveto), the function returns null.
 */
Geom::Curve const *get_last_segment(Geom::PathVector const &pathv);
Geom::Curve const *get_last_segment(Geom::Path const &) = delete;

/**
 * Return first pathsegment in PathVector or NULL.
 * equal in functionality to SPCurve::first_bpath()
 */
Geom::Curve const *get_first_segment(Geom::PathVector const &pathv);
Geom::Curve const *get_first_segment(Geom::Path const &) = delete;

/// ?
void stretch_endpoints(Geom::PathVector &pathv, Geom::Point const &new_p0, Geom::Point const &new_p1);

/// Sets start of first path to new_p0, and end of first path to new_p1.
void move_endpoints(Geom::PathVector &pathv, Geom::Point const &new_p0, Geom::Point const &new_p1);
void move_endpoints(Geom::Path &pathv, Geom::Point const &new_p0, Geom::Point const &new_p1);

/// Add p to the last point (and last handle if present) of the last path.
void last_point_additive_move(Geom::PathVector &pathv, Geom::Point const &p);

/**
 * Append \a pathv to \a to.
 * If \a use_lineto is false, simply add all paths in \a pathv to \a to;
 * if \a use_lineto is true, combine \a to's last path and \a pathv's first path and add the rest of the paths in \a pathv to \a to.
 */
void pathvector_append(Geom::PathVector &to, Geom::PathVector const &pathv, bool use_lineto = false);

/**
 * Append \a pathv to \a to with possible fusing of close endpoints. If the end of @a to and the start of @a pathv are within tolerance distance,
 * then the startpoint of @a pathv is moved to the end of @a to and the first subpath of @a pathv is appended to the last subpath of @a to.
 * When one of the curves is empty, this curves path becomes the non-empty path.
 *
 * @param tolerance Tolerance for endpoint fusion (applied to x and y separately)
 * @return false if one of the curves is closed, true otherwise.
 */
bool pathvector_append_continuous(Geom::PathVector &to, Geom::PathVector const &pathv, double tolerance = 0.0625);

/**
 * Construct an open path from a rectangle. That is, with the fourth side represented by a genuine line segment,
 * rather than the closing segment.
 */
Geom::Path rect_to_open_path(Geom::Rect const &rect);

/**
 * Close path by setting the end point to the start point instead of adding a new lineto.
 * Used for freehand drawing when the user draws back to the start point.
 */
void closepath_current(Geom::Path &path);

/// Remove last segment of curve.
void backspace(Geom::PathVector &pathv);
void backspace(Geom::Path &path);

/// Create a std::optional<T> from a (generalised) pointer to T.
template <typename T>
auto ptr_to_opt(T const &p)
{
    return p ? std::make_optional(*p) : std::nullopt;
}

/// Construct an open Geom::Path from Geom::Curve. Fixme: Should be in 2geom.
Geom::Path path_from_curve(std::unique_ptr<Geom::Curve> curve);
Geom::Path path_from_curve(Geom::Curve const &curve);

#endif // SEEN_DISPLAY_CURVE_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :

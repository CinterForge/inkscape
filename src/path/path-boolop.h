// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Boolean operations.
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef PATH_BOOLOP_H
#define PATH_BOOLOP_H

#include <vector>

#include <2geom/forward.h>
#include <2geom/pathvector.h>

#include "livarot/LivarotDefs.h" // FillRule, BooleanOp
#include "livarot/Path.h"

constexpr auto RELATIVE_THRESHOLD = 0.08;

void distribute_intersection_times(std::vector<Geom::PathVectorTime> &dst1,
                                   std::vector<Geom::PathVectorTime> &dst2,
                                   std::vector<Geom::PathVectorIntersection> const &intersections);
bool is_line(Path const &path);
void sort_and_clean_intersection_times(std::vector<Geom::PathVectorTime> &vec);

/// Flatten a pathvector according to the given fill rule.
Geom::PathVector flattened(Geom::PathVector const &pathv, FillRule fill_rule);
void flatten(Geom::PathVector &pathv, FillRule fill_rule);

/// Cut a pathvector along a collection of lines into several smaller pathvectors.
std::vector<Geom::PathVector> pathvector_cut(Geom::PathVector const &pathv, Geom::PathVector const &lines);

/// Perform a boolean operation on two pathvectors.
Geom::PathVector sp_pathvector_boolop(Geom::PathVector const &pathva, Geom::PathVector const &pathvb, BooleanOp bop, FillRule fra, FillRule frb);

#endif // PATH_BOOLOP_H

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

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Grid Snapper for Rectangular and Axonometric Grids
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2022 Authors
 * Copyright (C) Johan Engelen 2006-2007 <johan@shouraizou.nl>
 * Copyright (C) Lauris Kaplinski 2000
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_GRID_SNAPPER_H_
#define SEEN_GRID_SNAPPER_H_

#include "line-snapper.h"

class SPGrid;

namespace Inkscape {

/**
 * Snapper class for grids
 */
class GridSnapper : public LineSnapper {
public:
    GridSnapper(SPGrid const *grid, SnapManager *sm, Geom::Coord const d);
    bool ThisSnapperMightSnap() const override;

    Geom::Coord getSnapperTolerance() const override; //returns the tolerance of the snapper in screen pixels (i.e. independent of zoom)
    bool getSnapperAlwaysSnap(SnapSourceType const &source) const override; //if true, then the snapper will always snap, regardless of its tolerance

protected:
    LineList _getSnapLines(Geom::Point const &p) const override;
    void _addSnappedLine(IntermSnapResults &isr, Geom::Point const &snapped_point, Geom::Coord const &snapped_distance, SnapSourceType const &source, long source_num, Geom::Point const &normal_to_line, const Geom::Point &point_on_line) const override;
    void _addSnappedPoint(IntermSnapResults &isr, Geom::Point const &snapped_point, Geom::Coord const &snapped_distance, SnapSourceType const &source, long source_num, bool constrained_snap) const override;
    void _addSnappedLinePerpendicularly(IntermSnapResults &isr, Geom::Point const &snapped_point, Geom::Coord const &snapped_distance, SnapSourceType const &source, long source_num, bool constrained_snap) const override;

private:
    SPGrid const *_grid;

    LineList getSnapLinesAxonom(Geom::Point const &p) const;

    LineList get_snap_lines(const Geom::Point& p, int limit) const;
};

} // namespace Inkscape

#endif // SEEN_GRID_SNAPPER_H_

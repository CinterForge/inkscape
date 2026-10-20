// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_OBJECT_SNAPPER_H
#define SEEN_OBJECT_SNAPPER_H
/*
 * Authors:
 *   Carl Hetherington <inkscape@carlh.net>
 *   Diederik van Lierop <mail@diedenrezi.nl>
 *
 * Copyright (C) 2005 - 2011 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <memory>
#include "snapper.h"
#include "snap-candidate.h"

class SPDesktop;
class SPNamedView;
class SPObject;
class SPPath;
class SPDesktop;

namespace Inkscape
{

/**
 * Snapping things to objects.
 */
class ObjectSnapper : public Snapper
{

public:
    ObjectSnapper(SnapManager *sm, Geom::Coord const d);
    ~ObjectSnapper() override;

    /**
     * @return true if this Snapper will snap at least one kind of point.
     */
    bool ThisSnapperMightSnap() const override;

    /**
     * @return Snap tolerance (desktop coordinates); depends on current zoom so that it's always the same in screen pixels.
     */
    Geom::Coord getSnapperTolerance() const override; //returns the tolerance of the snapper in screen pixels (i.e. independent of zoom)

    bool getSnapperAlwaysSnap(SnapSourceType const &source) const override; //if true, then the snapper will always snap, regardless of its tolerance

    void freeSnap(IntermSnapResults &isr,
                  Inkscape::SnapCandidatePoint const &p,
                  Geom::OptRect const &bbox_to_snap,
                  std::vector<SPObject const *> const *it,
                  std::vector<SnapCandidatePoint> *unselected_nodes) const override;

    void constrainedSnap(IntermSnapResults &isr,
                  Inkscape::SnapCandidatePoint const &p,
                  Geom::OptRect const &bbox_to_snap,
                  SnapConstraint const &c,
                  std::vector<SPObject const *> const *it,
                  std::vector<SnapCandidatePoint> *unselected_nodes) const override;

private:
    std::unique_ptr<std::vector<SnapCandidatePoint>> _points_to_snap_to;
    std::unique_ptr<std::vector<SnapCandidatePath >> _paths_to_snap_to;

    void _snapNodes(IntermSnapResults &isr,
                      Inkscape::SnapCandidatePoint const &p, // in desktop coordinates
                      std::vector<SnapCandidatePoint> *unselected_nodes,
                      SnapConstraint const &c = SnapConstraint(),
                      Geom::Point const &p_proj_on_constraint = Geom::Point()) const;

    void _snapTranslatingGuide(IntermSnapResults &isr,
                     Geom::Point const &p,
                     Geom::Point const &guide_normal) const;

    void _collectNodes(Inkscape::SnapSourceType const &t,
                  bool const &first_point) const;

    void _snapPaths(IntermSnapResults &isr,
                      Inkscape::SnapCandidatePoint const &p, // in desktop coordinates
                      std::vector<Inkscape::SnapCandidatePoint> *unselected_nodes, // in desktop coordinates
                      SPPath const *selected_path) const;

    void _snapPathsConstrained(IntermSnapResults &isr,
                 Inkscape::SnapCandidatePoint const &p, // in desktop coordinates
                 SnapConstraint const &c,
                 Geom::Point const &p_proj_on_constraint,
				 std::vector<SnapCandidatePoint> *unselected_nodes,
				 SPPath const *selected_path) const;

    void _snapPathsTangPerp(bool snap_tang,
                            bool snap_perp,
                            IntermSnapResults &isr,
                            SnapCandidatePoint const &p,
                            Geom::Curve const *curve,
                            SPDesktop const *dt) const;

    bool isUnselectedNode(Geom::Point const &point, std::vector<Inkscape::SnapCandidatePoint> const *unselected_nodes) const;

    /**
     * Returns index of first NR_END bpath in array.
     */
    void _collectPaths(Geom::Point p,
                      Inkscape::SnapSourceType const source_type,
                      bool const &first_point) const;

    void _clear_paths() const;
    Geom::PathVector _getBorderPathv() const;
    Geom::PathVector _getPathvFromRect(Geom::Rect const rect) const;
    bool _allowSourceToSnapToTarget(SnapSourceType source, SnapTargetType target, bool strict_snapping) const;

}; // end of ObjectSnapper class

void getBBoxPoints(Geom::OptRect const bbox, std::vector<SnapCandidatePoint> *points, bool const isTarget, bool const corners, bool const edges, bool const midpoint);
void getBBoxPoints(Geom::OptRect const bbox, std::vector<SnapCandidatePoint> *points, bool const isTarget,
                   Inkscape::SnapSourceType corners, Inkscape::SnapTargetType cornert,
                   Inkscape::SnapSourceType edges, Inkscape::SnapTargetType edget,
                   Inkscape::SnapSourceType midpoints, Inkscape::SnapTargetType midpointt);

} // end of namespace Inkscape

#endif

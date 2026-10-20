// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SnapManager class.
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *   Nathan Hurst <njh@njhurst.com>
 *   Carl Hetherington <inkscape@carlh.net>
 *   Diederik van Lierop <mail@diedenrezi.nl>
 *
 * Copyright (C) 2006-2007 Johan Engelen <johan@shouraizou.nl>
 * Copyright (C) 2004      Nathan Hurst
 * Copyright (C) 1999-2012 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "snap.h"

#include <memory>
#include <vector>

#include <glib.h>                                          // for g_assert
#include <glibmm/timer.h>

#include <2geom/transforms.h>

#include "desktop.h"
#include "preferences.h"
#include "pure-transform.h"
#include "selection.h"
#include "snap-enums.h"
#include "style.h"

#include "display/control/snap-indicator.h"
#include "helper/mathfns.h"
#include "live_effects/effect-enum.h"
#include "object/sp-clippath.h"
#include "object/sp-filter.h"
#include "object/sp-grid.h"
#include "object/sp-guide.h"
#include "object/sp-mask.h"
#include "object/sp-namedview.h"
#include "object/sp-object.h"
#include "object/sp-page.h"

using Inkscape::Util::round_to_upper_multiple_plus;
using Inkscape::Util::round_to_lower_multiple_plus;

SnapManager::SnapManager(SPNamedView const *v, Inkscape::SnapPreferences& preferences) :
    snapprefs(preferences),
    guide(this, 0),
    object(this, 0),
    alignment(this, 0),
    distribution(this, 0),
    _named_view(v),
    _rotation_center_source_items(std::vector<SPItem*>()),
    _desktop(nullptr),
    _snapindicator(true),
    _unselected_nodes(nullptr)
{
    _obj_snapper_candidates = std::make_unique<std::vector<Inkscape::SnapCandidateItem>>();
    _align_snapper_candidates = std::make_unique<std::vector<Inkscape::SnapCandidateItem>>();
}

SnapManager::~SnapManager()
{
    _obj_snapper_candidates->clear();
    _align_snapper_candidates->clear();
}

SnapManager::SnapperList SnapManager::getSnappers() const
{
    SnapManager::SnapperList s;
    s.push_back(&guide);
    s.push_back(&object);
    s.push_back(&alignment);
    s.push_back(&distribution);

    SnapManager::SnapperList gs = getGridSnappers();
    s.splice(s.begin(), gs);

    return s;
}

SnapManager::SnapperList SnapManager::getGridSnappers() const
{
    SnapperList s;

    if (snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_GRID)) {
        for (auto grid : _named_view->grids) {
            if (grid->isEnabled() && grid->isVisible()) {
                s.push_back(grid->snapper());
            }
        }
    }

    return s;
}

bool SnapManager::someSnapperMightSnap(bool immediately) const
{
    if ( !snapprefs.getSnapEnabledGlobally() ) {
        return false;
    }

    // If we're asking if some snapper might snap RIGHT NOW (without the snap being postponed)...
    if ( immediately && snapprefs.getSnapPostponedGlobally() ) {
        return false;
    }

    SnapperList const s = getSnappers();
    SnapperList::const_iterator i = s.begin();
    while (i != s.end() && (*i)->ThisSnapperMightSnap() == false) {
        ++i;
    }

    return (i != s.end());
}

bool SnapManager::gridSnapperMightSnap() const
{
    if ( !snapprefs.getSnapEnabledGlobally() || snapprefs.getSnapPostponedGlobally() ) {
        return false;
    }

    SnapperList const s = getGridSnappers();
    SnapperList::const_iterator i = s.begin();
    while (i != s.end() && (*i)->ThisSnapperMightSnap() == false) {
        ++i;
    }

    return (i != s.end());
}

void SnapManager::freeSnapReturnByRef(Geom::Point &p,
                                      Inkscape::SnapSourceType const source_type,
                                      Geom::OptRect const &bbox_to_snap) const
{
    Inkscape::SnappedPoint const s = freeSnap(Inkscape::SnapCandidatePoint(p, source_type, Inkscape::SNAPTARGET_PATH), bbox_to_snap);
    s.getPointIfSnapped(p);
}

Inkscape::SnappedPoint SnapManager::freeSnap(Inkscape::SnapCandidatePoint const &p,
                                             Geom::OptRect const &bbox_to_snap,
                                             bool to_paths_only) const
{
    if (!someSnapperMightSnap()) {
        return Inkscape::SnappedPoint(p, Inkscape::SNAPTARGET_UNDEFINED, Geom::infinity(), 0, false, false, false);
    }

    IntermSnapResults isr;
    SnapperList const snappers = getSnappers();

    for (auto snapper : snappers) {
        snapper->freeSnap(isr, p, bbox_to_snap, &_objects_to_ignore, _unselected_nodes);
    }

    return findBestSnap(p, isr, false, false, to_paths_only);
}

Geom::Point SnapManager::snapToPoints(Geom::Point const &p, Inkscape::SnapSourceType const source_type, std::vector<Inkscape::SnapCandidatePoint> const &points) const
{
    auto r = p;
    if (snapprefs.getSnapEnabledGlobally()) {
        SnappedPoint s;
        auto tolerance = getSnapperTolerance(snapprefs.getObjectTolerance());

        for (const auto &k : points) {
            Geom::Point target_pt = k.getPoint();
            Geom::Coord dist = Geom::L2(target_pt - p);
            if (dist < tolerance && dist < s.getSnapDistance()) {
                s = SnappedPoint(target_pt, source_type, 0, k.getTargetType(), dist, tolerance, false, false, false);
            }
        }

        s.getPointIfSnapped(r);
    }
    return r;
}

void SnapManager::hideSnapIndicator()
{
    if (_desktop) {
        _desktop->getSnapIndicator()->remove_snaptarget(true);
    }
}

void SnapManager::preSnap(Inkscape::SnapCandidatePoint const &p, bool to_paths_only)
{
    // setup() must have been called before calling this method!

    if (_snapindicator) {
        _snapindicator = false; // prevent other methods from drawing a snap indicator; we want to control this here
        Inkscape::SnappedPoint s = freeSnap(p, Geom::OptRect(), to_paths_only);
        g_assert(_desktop != nullptr);
        if (s.getSnapped()) {
            _desktop->getSnapIndicator()->set_new_snaptarget(s, true);
        } else {
            _desktop->getSnapIndicator()->remove_snaptarget(true);
        }
        _snapindicator = true; // restore the original value
    }
}

Geom::Point SnapManager::multipleOfGridPitch(Geom::Point const &t, Geom::Point const &origin)
{
    if (!snapprefs.getSnapEnabledGlobally() || snapprefs.getSnapPostponedGlobally())
        return t;

    bool success = false;
    Geom::Point nearest_multiple;
    Geom::Coord nearest_distance = Geom::infinity();
    Inkscape::SnappedPoint bestSnappedPoint(t);

    // It will snap to the grid for which we find the closest snap. This might be a different
    // grid than to which the objects were initially aligned. I don't see an easy way to fix
    // this, so when using multiple grids one can get unexpected results

    // Cannot use getGridSnappers() because we need both the grids AND their snappers
    // Therefore we iterate through all grids manually
    for (auto grid : _named_view->grids) {
        if (!grid->isEnabled() || !grid->isVisible()) {
            continue;
        }

        const Inkscape::Snapper* snapper = grid->snapper();
        if (snapper && snapper->ThisSnapperMightSnap()) {
            // To find the nearest multiple of the grid pitch for a given translation t, we
            // will use the grid snapper. Simply snapping the value t to the grid will do, but
            // only if the origin of the grid is at (0,0). If it's not then compensate for this
            // in the translation t
            Geom::Point const t_offset = t + grid->getOrigin();
            IntermSnapResults isr;
            // Only the first three parameters are being used for grid snappers
            snapper->freeSnap(isr, Inkscape::SnapCandidatePoint(t_offset, Inkscape::SNAPSOURCE_GRID_PITCH),Geom::OptRect(), nullptr, nullptr);
            // Find the best snap for this grid, including intersections of the grid-lines
            bool old_val = _snapindicator;
            _snapindicator = false;
            Inkscape::SnappedPoint s = findBestSnap(Inkscape::SnapCandidatePoint(t_offset, Inkscape::SNAPSOURCE_GRID_PITCH), isr, false, true);
            _snapindicator = old_val;
            if (s.getSnapped() && (s.getSnapDistance() < nearest_distance)) {
                // use getSnapDistance() instead of getWeightedDistance() here because the pointer's position
                // doesn't tell us anything about which node to snap
                success = true;
                nearest_multiple = s.getPoint() - grid->getOrigin();
                nearest_distance = s.getSnapDistance();
                bestSnappedPoint = s;
            }
        }
    }

    if (success) {
        bestSnappedPoint.setPoint(origin + nearest_multiple);
        _desktop->getSnapIndicator()->set_new_snaptarget(bestSnappedPoint);
        return nearest_multiple;
    }

    return t;
}

void SnapManager::constrainedSnapReturnByRef(Geom::Point &p,
                                             Inkscape::SnapSourceType const source_type,
                                             Inkscape::Snapper::SnapConstraint const &constraint,
                                             Geom::OptRect const &bbox_to_snap) const
{
    Inkscape::SnappedPoint const s = constrainedSnap(Inkscape::SnapCandidatePoint(p, source_type), constraint, bbox_to_snap);
    p = s.getPoint(); // If we didn't snap, then we will return the point projected onto the constraint
}

Inkscape::SnappedPoint SnapManager::constrainedSnap(Inkscape::SnapCandidatePoint const &p,
                                                    Inkscape::Snapper::SnapConstraint const &constraint,
                                                    Geom::OptRect const &bbox_to_snap) const
{
    // First project the mouse pointer onto the constraint
    Geom::Point pp = constraint.projection(p.getPoint());

    Inkscape::SnappedPoint no_snap = Inkscape::SnappedPoint(pp, p.getSourceType(), p.getSourceNum(), Inkscape::SNAPTARGET_CONSTRAINT, Geom::infinity(), 0, false, true, false);

    if (!someSnapperMightSnap()) {
        // Always return point on constraint
        return no_snap;
    }

    Inkscape::SnappedPoint result = no_snap;

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if ((prefs->getBool("/options/snapmousepointer/value", false)) && p.isSingleHandle()) {
        // Snapping the mouse pointer instead of the constrained position of the knot allows
        // to snap to things which don't intersect with the constraint line; this is basically
        // then just a freesnap with the constraint applied afterwards
        // We'll only do this if we're dragging a single handle, and for example not when transforming an object in the selector tool
        result = freeSnap(p, bbox_to_snap);
        if (result.getSnapped()) {
            // only change the snap indicator if we really snapped to something
            if (_snapindicator && _desktop) {
                _desktop->getSnapIndicator()->set_new_snaptarget(result);
            }
            // Apply the constraint
            result.setPoint(constraint.projection(result.getPoint()));
            return result;
        }
        return no_snap;
    }

    IntermSnapResults isr;
    SnapperList const snappers = getSnappers();
    for (auto snapper : snappers) {
        snapper->constrainedSnap(isr, p, bbox_to_snap, constraint, &_objects_to_ignore, _unselected_nodes);
    }

    result = findBestSnap(p, isr, true);


    if (result.getSnapped()) {
        // only change the snap indicator if we really snapped to something
        if (_snapindicator && _desktop) {
            _desktop->getSnapIndicator()->set_new_snaptarget(result);
        }
        return result;
    }
    return no_snap;
}

/* See the documentation for constrainedSnap() directly above for more details.
 * The difference is that multipleConstrainedSnaps() will take a list of constraints instead of a single one,
 * and will try to snap the SnapCandidatePoint to only the closest constraint
 *  \param p Source point to be snapped
 *  \param constraints List of directions or lines along which snapping must occur
 *  \param dont_snap If true then we will only apply the constraint, without snapping
 *  \param bbox_to_snap Bounding box hulling the set of points, all from the same selection and having the same transformation
 */


Inkscape::SnappedPoint SnapManager::multipleConstrainedSnaps(Inkscape::SnapCandidatePoint const &p,
                                                    std::vector<Inkscape::Snapper::SnapConstraint> const &constraints,
                                                    bool dont_snap,
                                                    Geom::OptRect const &bbox_to_snap) const
{

    Inkscape::SnappedPoint no_snap = Inkscape::SnappedPoint(p.getPoint(), p.getSourceType(), p.getSourceNum(), Inkscape::SNAPTARGET_CONSTRAINT, Geom::infinity(), 0, false, true, false);
    if (constraints.size() == 0) {
        return no_snap;
    }

    // We haven't tried to snap yet; we will first determine which constraint is closest to where we are now,
    // i.e. lets find out which of the constraints yields the closest projection of point p

    // Project the mouse pointer on each of the constraints
    std::vector<Geom::Point> projections;
    for (const auto & constraint : constraints) {
        // Project the mouse pointer onto the constraint; In case we don't snap then we will
        // return the projection onto the constraint, such that the constraint is always enforced
        Geom::Point pp = constraint.projection(p.getPoint());
        projections.push_back(pp);
    }

    // Select the closest constraint
    no_snap.setPoint(projections.front());
    Inkscape::Snapper::SnapConstraint cc = constraints.front(); //closest constraint

    std::vector<Inkscape::Snapper::SnapConstraint>::const_iterator c = constraints.begin();
    std::vector<Geom::Point>::iterator pp = projections.begin();
    for (; pp != projections.end(); ++pp) {
        if (Geom::L2(*pp - p.getPoint()) < Geom::L2(no_snap.getPoint() - p.getPoint())) {
            no_snap.setPoint(*pp); // Remember the projection onto the closest constraint
            cc = *c; // Remember the closest constraint itself
        }
        ++c;
    }

    if (!someSnapperMightSnap() || dont_snap) {
        return no_snap;
    }

    IntermSnapResults isr;
    SnapperList const snappers = getSnappers();
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool snap_mouse = prefs->getBool("/options/snapmousepointer/value", false);

    Inkscape::SnappedPoint result = no_snap;
    if (snap_mouse && p.isSingleHandle()) {
        // Snapping the mouse pointer instead of the constrained position of the knot allows
        // to snap to things which don't intersect with the constraint line; this is basically
        // then just a freesnap with the constraint applied afterwards
        // We'll only to this if we're dragging a single handle, and for example not when transforming an object in the selector tool
        result = freeSnap(p, bbox_to_snap);
        // Now apply the constraint afterwards
        result.setPoint(cc.projection(result.getPoint()));
    } else {
        // Try to snap along the closest constraint
        for (auto snapper : snappers) {
            snapper->constrainedSnap(isr, p, bbox_to_snap, cc, &_objects_to_ignore,_unselected_nodes);
        }
        result = findBestSnap(p, isr, true);
    }

    return result.getSnapped() ? result : no_snap;
}

Inkscape::SnappedPoint SnapManager::constrainedAngularSnap(Inkscape::SnapCandidatePoint const &p,
                                                            std::optional<Geom::Point> const &p_ref,
                                                            Geom::Point const &o,
                                                            unsigned const snaps) const
{
    Inkscape::SnappedPoint sp;
    if (snaps > 0) { // 0 means no angular snapping
        // p is at an arbitrary angle. Now we should snap this angle to specific increments.
        // For this we'll calculate the closest two angles, one at each side of the current angle
        Geom::Line y_axis(Geom::Point(0, 0), Geom::Point(0, 1));
        Geom::Line p_line(o, p.getPoint());
        double angle = Geom::angle_between(y_axis, p_line);
        double angle_incr = M_PI / snaps;
        double angle_offset = 0;
        if (p_ref) {
            Geom::Line p_line_ref(o, *p_ref);
            angle_offset = Geom::angle_between(y_axis, p_line_ref);
        }
        double angle_ceil = round_to_upper_multiple_plus(angle, angle_incr, angle_offset);
        double angle_floor = round_to_lower_multiple_plus(angle, angle_incr, angle_offset);
        // We have two angles now. The constrained snapper will try each of them and return the closest

        // Now do the snapping...
        std::vector<Inkscape::Snapper::SnapConstraint> constraints;
        constraints.emplace_back(Geom::Line(o, angle_ceil - M_PI/2));
        constraints.emplace_back(Geom::Line(o, angle_floor - M_PI/2));
        sp = multipleConstrainedSnaps(p, constraints); // Constraints will always be applied, even if we didn't snap
        if (!sp.getSnapped()) { // If we haven't snapped then we only had the constraint applied;
            sp.setTarget(Inkscape::SNAPTARGET_CONSTRAINED_ANGLE);
        }
    } else {
        sp = freeSnap(p);
    }
    return sp;
}

bool SnapManager::guideFreeSnap(Geom::Point &p, Geom::Point &origin_or_vector, bool origin, bool freeze_angle) const
{
    if (freeze_angle && origin) {
        g_warning("Dear developer, when snapping guides you shouldn't ask me to freeze the guide's vector when you haven't specified one");
        // You've supplied me with an origin instead of a vector
    }

    if (!snapprefs.getSnapEnabledGlobally() || snapprefs.getSnapPostponedGlobally() || !snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_GUIDE)) {
        return false;
    }

    Inkscape::SnapCandidatePoint candidate(p, Inkscape::SNAPSOURCE_GUIDE);
    if (origin) {
        candidate.addOrigin(origin_or_vector);
    } else {
        candidate = Inkscape::SnapCandidatePoint(p, Inkscape::SNAPSOURCE_GUIDE_ORIGIN);
        candidate.addVector(Geom::rot90(origin_or_vector));
    }

    IntermSnapResults isr;
    SnapperList snappers = getSnappers();
    for (SnapperList::const_iterator i = snappers.begin(); i != snappers.end(); ++i) {
        (*i)->freeSnap(isr, candidate, Geom::OptRect(), nullptr, nullptr);
    }

    Inkscape::SnappedPoint const s = findBestSnap(candidate, isr, false);

    s.getPointIfSnapped(p);

    if (!freeze_angle && s.getSnapped()) {
        if (!Geom::are_near(s.getTangent(), Geom::Point(0,0))) { // If the tangent has been set ...
            // PS: The tangent might not have been set if we snapped for example to a node
            origin_or_vector = Geom::rot90(s.getTangent()); // then use it to update the normal of the guide
            return true;
        }
    }

    return false;
}

void SnapManager::guideConstrainedSnap(Geom::Point &p, SPGuide const &guideline) const
{
    if (!snapprefs.getSnapEnabledGlobally() || snapprefs.getSnapPostponedGlobally() || !snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_GUIDE)) {
        return;
    }

    Inkscape::SnapCandidatePoint candidate(p, Inkscape::SNAPSOURCE_GUIDE_ORIGIN, Inkscape::SNAPTARGET_UNDEFINED);

    IntermSnapResults isr;
    Inkscape::Snapper::SnapConstraint cl(guideline.getPoint(), Geom::rot90(guideline.getNormal()));

    SnapperList snappers = getSnappers();
    for (SnapperList::const_iterator i = snappers.begin(); i != snappers.end(); ++i) {
        (*i)->constrainedSnap(isr, candidate, Geom::OptRect(), cl, nullptr, nullptr);
    }

    Inkscape::SnappedPoint const s = findBestSnap(candidate, isr, false);
    s.getPointIfSnapped(p);
}

void SnapManager::snapTransformed(
    std::vector<Inkscape::SnapCandidatePoint> const &points,
    Geom::Point const &pointer,
    Inkscape::PureTransform &transform
    )
{
    /* We have a list of points, which we are proposing to transform in some way.  We need to see
    ** if any of these points, when transformed, snap to anything.  If they do, we return the
    ** appropriate transformation with `true'; otherwise we return the original scale with `false'.
    */

    if (points.size() == 0) {
        transform.best_snapped_point = Inkscape::SnappedPoint(pointer);
        return;
    }

    // We will try to snap a set of points, but we don't want to have a snap indicator displayed
    // for each of them. That's why it's temporarily disabled here, and re-enabled again after we
    // have finished calling the freeSnap() and constrainedSnap() methods
    bool _orig_snapindicator_status = _snapindicator;
    _snapindicator = false;

    transform.snap(this, points, pointer);

    // Allow the snapindicator to be displayed again
    _snapindicator = _orig_snapindicator_status;

    if (_snapindicator) {
        if (transform.best_snapped_point.getSnapped()) {
            _desktop->getSnapIndicator()->set_new_snaptarget(transform.best_snapped_point);
        } else {
            _desktop->getSnapIndicator()->remove_snaptarget();
        }
    }

    if (points.size() == 1) {
        displaySnapsource(Inkscape::SnapCandidatePoint(transform.best_snapped_point.getPoint(), points.at(0).getSourceType()));
    }
}

Inkscape::SnappedPoint SnapManager::findBestSnap(Inkscape::SnapCandidatePoint const &p,
                                                 IntermSnapResults const &isr,
                                                 bool constrained,
                                                 bool allowOffScreen,
                                                 bool to_path_only) const
{
    g_assert(_desktop != nullptr);

    /*
    std::cout << "Type and number of snapped constraints: " << std::endl;
    std::cout << "  Points      : " << isr.points.size() << std::endl;
    std::cout << "  Grid lines  : " << isr.grid_lines.size()<< std::endl;
    std::cout << "  Guide lines : " << isr.guide_lines.size()<< std::endl;
    std::cout << "  Curves      : " << isr.curves.size()<< std::endl;
    */

    /*
    // Display all snap candidates on the canvas
    _desktop->getSnapIndicator()->remove_debugging_points();
    for (std::list<Inkscape::SnappedPoint>::const_iterator i = isr.points.begin(); i != isr.points.end(); i++) {
        _desktop->getSnapIndicator()->set_new_debugging_point((*i).getPoint());
    }
    for (std::list<Inkscape::SnappedCurve>::const_iterator i = isr.curves.begin(); i != isr.curves.end(); i++) {
        _desktop->getSnapIndicator()->set_new_debugging_point((*i).getPoint());
    }
    for (std::list<Inkscape::SnappedLine>::const_iterator i = isr.grid_lines.begin(); i != isr.grid_lines.end(); i++) {
        _desktop->getSnapIndicator()->set_new_debugging_point((*i).getPoint());
    }
    for (std::list<Inkscape::SnappedLine>::const_iterator i = isr.guide_lines.begin(); i != isr.guide_lines.end(); i++) {
        _desktop->getSnapIndicator()->set_new_debugging_point((*i).getPoint());
    }
    */

    // Store all snappoints
    std::list<Inkscape::SnappedPoint> sp_list;

    // search for the closest snapped point
    Inkscape::SnappedPoint closestPoint;
    if (getClosestSP(isr.points, closestPoint)) {
        sp_list.push_back(closestPoint);
    }

    // search for the closest snapped curve
    Inkscape::SnappedCurve closestCurve;
    // We might have collected the paths only to snap to their intersection, without the intention to snap to the paths themselves
    // Therefore we explicitly check whether the paths should be considered as snap targets themselves
    bool exclude_paths = !snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_PATH);
    if (getClosestCurve(isr.curves, closestCurve, exclude_paths, to_path_only)) {
        sp_list.emplace_back(closestCurve);
    }

    // search for the closest snapped grid line
    if (snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_GRID_LINE)) {
        Inkscape::SnappedLine closestGridLine;
        if (getClosestSL(isr.grid_lines, closestGridLine)) {
            closestGridLine.setSource(p.getSourceType());
            closestGridLine.setTarget(Inkscape::SNAPTARGET_GRID_LINE);
            sp_list.emplace_back(closestGridLine);
        }
    }

    // search for the closest snapped guide line
    Inkscape::SnappedLine closestGuideLine;
    if (getClosestSL(isr.guide_lines, closestGuideLine)) {
        sp_list.emplace_back(closestGuideLine);
    }

    // When freely snapping to a grid/guide/path, only one degree of freedom is eliminated
    // Therefore we will try get fully constrained by finding an intersection with another grid/guide/path

    // When doing a constrained snap however, we're already at an intersection of the constrained line and
    // the grid/guide/path we're snapping to. This snappoint is therefore fully constrained, so there's
    // no need to look for additional intersections
    if (!constrained) {
        if (snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_PATH_INTERSECTION)) {
            // search for the closest snapped intersection of curves
            Inkscape::SnappedPoint closestCurvesIntersection;
            if (getClosestIntersectionCS(isr.curves, p.getPoint(), closestCurvesIntersection, _desktop->dt2doc())) {
                closestCurvesIntersection.setSource(p.getSourceType());
                sp_list.push_back(closestCurvesIntersection);
            }
        }

        if (snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_PATH_GUIDE_INTERSECTION)) {
            // search for the closest snapped intersection of a guide with a curve
            Inkscape::SnappedPoint closestCurveGuideIntersection;
            if (getClosestIntersectionCL(isr.curves, isr.guide_lines, p.getPoint(), closestCurveGuideIntersection, _desktop->dt2doc())) {
                closestCurveGuideIntersection.setSource(p.getSourceType());
                sp_list.push_back(closestCurveGuideIntersection);
            }
        }

        // search for the closest snapped intersection of grid lines
        Inkscape::SnappedPoint closestGridPoint;
        if (getClosestIntersectionSL(isr.grid_lines, closestGridPoint)) {
            closestGridPoint.setSource(p.getSourceType());
            closestGridPoint.setTarget(Inkscape::SNAPTARGET_GRID_INTERSECTION);
            sp_list.push_back(closestGridPoint);
        }

        // search for the closest snapped intersection of guide lines
        Inkscape::SnappedPoint closestGuidePoint;
        if (getClosestIntersectionSL(isr.guide_lines, closestGuidePoint)) {
            closestGuidePoint.setSource(p.getSourceType());
            closestGuidePoint.setTarget(Inkscape::SNAPTARGET_GUIDE_INTERSECTION);
            sp_list.push_back(closestGuidePoint);
        }

        // search for the closest snapped intersection of grid with guide lines
        if (snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_GRID_GUIDE_INTERSECTION)) {
            Inkscape::SnappedPoint closestGridGuidePoint;
            if (getClosestIntersectionSL(isr.grid_lines, isr.guide_lines, closestGridGuidePoint)) {
                closestGridGuidePoint.setSource(p.getSourceType());
                closestGridGuidePoint.setTarget(Inkscape::SNAPTARGET_GRID_GUIDE_INTERSECTION);
                sp_list.push_back(closestGridGuidePoint);
            }
        }
    }

    // Filter out all snap targets that do NOT include a path; this is useful when we try to insert
    // a node in a path (on doubleclick in the node tool). We don't want to change the shape of the
    // path, so the snapped point must be on a path, and not e.g. on a grid intersection or on the
    // bounding box
    if (to_path_only) {
        sp_list.remove_if([](Inkscape::SnappedPoint sp) { return !sp.getOnPath(); });
    }

    // now let's see which snapped point gets a thumbs up
    Inkscape::SnappedPoint bestSnappedPoint(p.getPoint());
    // std::cout << "Finding the best snap..." << std::endl;
    for (std::list<Inkscape::SnappedPoint>::const_iterator i = sp_list.begin(); i != sp_list.end(); ++i) {
        // std::cout << "sp = " << (*i).getPoint() << " | source = " << (*i).getSource() << " | target = " << (*i).getTarget();
        bool onScreen = _desktop->get_display_area().contains((*i).getPoint());
        if (onScreen || allowOffScreen) { // Only snap to points which are not off the screen
            if ((*i).getAlwaysSnap() || (*i).getSnapDistance() <= (*i).getTolerance()) { // Only snap to points within snapping range
                // if it's the first point, or if it is closer than the best snapped point so far
                if (i == sp_list.begin() || bestSnappedPoint.isOtherSnapBetter(*i, false)) {
                    // then prefer this point over the previous one
                    bestSnappedPoint = *i;
                }
            }
        }
        // std::cout << std::endl;
    }

    // Update the snap indicator, if requested
    if (_snapindicator) {
        if (bestSnappedPoint.getSnapped()) {
            _desktop->getSnapIndicator()->set_new_snaptarget(bestSnappedPoint);
        } else {
            _desktop->getSnapIndicator()->remove_snaptarget();
        }
    }

    // std::cout << "findBestSnap = " << bestSnappedPoint.getPoint() << " | dist = " << bestSnappedPoint.getSnapDistance() << std::endl;
    return bestSnappedPoint;
}

void SnapManager::setup(SPDesktop const *desktop,
                        bool snapindicator,
                        SPObject const *item_to_ignore,
                        std::vector<Inkscape::SnapCandidatePoint> *unselected_nodes)
{
    g_assert(desktop != nullptr);
    if (_desktop != nullptr) {
        g_warning("The snapmanager has been set up before, but unSetup() hasn't been called afterwards. It possibly held invalid pointers");
    }
    _objects_to_ignore.clear();
    if (item_to_ignore) {
        _objects_to_ignore.push_back(item_to_ignore);
    }
    _desktop = desktop;
    _snapindicator = snapindicator;
    _unselected_nodes = unselected_nodes;
    _rotation_center_source_items.clear();
    _findCandidates_already_called = false;
}

void SnapManager::setup(SPDesktop const *desktop,
                        bool snapindicator,
                        std::vector<SPObject const *> &objects_to_ignore,
                        std::vector<Inkscape::SnapCandidatePoint> *unselected_nodes)
{
    g_assert(desktop != nullptr);
    if (_desktop != nullptr) {
        g_warning("The snapmanager has been set up before, but unSetup() hasn't been called afterwards. It possibly held invalid pointers");
    }
    _objects_to_ignore = objects_to_ignore;
    _desktop = desktop;
    _snapindicator = snapindicator;
    _unselected_nodes = unselected_nodes;
    _rotation_center_source_items.clear();
    _findCandidates_already_called = false;
}

/// Setup, taking the list of items to ignore from the desktop's selection.
void SnapManager::setupIgnoreSelection(SPDesktop const *desktop,
                                      bool snapindicator,
                                      std::vector<Inkscape::SnapCandidatePoint> *unselected_nodes)
{
    g_assert(desktop != nullptr);
    if (_desktop != nullptr) {
        // Someone has been naughty here! This is dangerous
        g_warning("The snapmanager has been set up before, but unSetup() hasn't been called afterwards. It possibly held invalid pointers");
    }
    _desktop = desktop;
    _snapindicator = snapindicator;
    _unselected_nodes = unselected_nodes;
    _rotation_center_source_items.clear();
    _findCandidates_already_called = false;
    _objects_to_ignore.clear();

    Inkscape::Selection *sel = _desktop->getSelection();
    auto items = sel->items();
    for (auto item : items) {
        _objects_to_ignore.push_back(item);
    }
}

SPDocument *SnapManager::getDocument() const
{
    return _named_view->document;
}

Geom::Coord SnapManager::getSnapperTolerance(double tolerance) const
{
    SPDesktop const *dt = getDesktop();
    double const zoom =  dt ? dt->current_zoom() : 1;
    return tolerance / zoom;
}


//Geom::Point SnapManager::_transformPoint(Inkscape::SnapCandidatePoint const &p,
//                                        Transformation const transformation_type,
//                                        Geom::Point const &transformation,
//                                        Geom::Point const &origin,
//                                        Geom::Dim2 const dim,
//                                        bool const uniform) const
//{
//    /* Work out the transformed version of this point */
//    Geom::Point transformed;
//    switch (transformation_type) {
//        case TRANSLATE:
//            transformed = p.getPoint() + transformation;
//            break;
//        case SCALE:
//            transformed = (p.getPoint() - origin) * Geom::Scale(transformation[Geom::X], transformation[Geom::Y]) + origin;
//            break;
//        case STRETCH:
//        {
//            Geom::Scale s(1, 1);
//            if (uniform)
//                s[Geom::X] = s[Geom::Y] = transformation[dim];
//            else {
//                s[dim] = transformation[dim];
//                s[1 - dim] = 1;
//            }
//            transformed = ((p.getPoint() - origin) * s) + origin;
//            break;
//        }
//        case SKEW:
//            // Apply the skew factor
//            transformed[dim] = (p.getPoint())[dim] + transformation[0] * ((p.getPoint())[1 - dim] - origin[1 - dim]);
//            // While skewing, mirroring and scaling (by integer multiples) in the opposite direction is also allowed.
//            // Apply that scale factor here
//            transformed[1-dim] = (p.getPoint() - origin)[1 - dim] * transformation[1] + origin[1 - dim];
//            break;
//        case ROTATE:
//            // for rotations: transformation[0] stores the angle in radians
//            transformed = (p.getPoint() - origin) * Geom::Rotate(transformation[0]) + origin;
//            break;
//        default:
//            g_assert_not_reached();
//    }
//
//    return transformed;
//}

/**
 * Mark the location of the snap source (not the snap target!) on the canvas by drawing a symbol.
 *
 * @param point_type Category of points to which the source point belongs: node, guide or bounding box
 * @param p The transformed position of the source point, paired with an identifier of the type of the snap source.
 */
void SnapManager::displaySnapsource(Inkscape::SnapCandidatePoint const &p) const {
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (prefs->getBool("/options/snapclosestonly/value")) {
        Inkscape::SnapSourceType t = p.getSourceType();
        bool p_is_a_node = t & Inkscape::SNAPSOURCE_NODE_CATEGORY;
        bool p_is_a_bbox = t & Inkscape::SNAPSOURCE_BBOX_CATEGORY;
        bool p_is_other = (t & Inkscape::SNAPSOURCE_OTHERS_CATEGORY) || (t & Inkscape::SNAPSOURCE_DATUMS_CATEGORY);

        g_assert(_desktop != nullptr);
        if (snapprefs.getSnapEnabledGlobally() && (p_is_other || (p_is_a_node && snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_NODE_CATEGORY)) || (p_is_a_bbox && snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_BBOX_CATEGORY)))) {
            _desktop->getSnapIndicator()->set_new_snapsource(p);
        } else {
            _desktop->getSnapIndicator()->remove_snapsource();
        }
    }
}

SPGuide const *SnapManager::getGuideToIgnore() const
{
    for (auto item : _objects_to_ignore) {
        if (auto guide = cast<SPGuide>(item)) {
            return guide;
        }
    }
    return nullptr;
}
SPPage const *SnapManager::getPageToIgnore() const
{
    for (auto item : _objects_to_ignore) {
        if (auto page = cast<SPPage>(item)) {
            return page;
        }
    }
    return nullptr;
}


void SnapManager::_findCandidates(SPObject* parent,
                                 std::vector<SPObject const *> const *it,
                                 Geom::Rect const &bbox_to_snap,
                                 bool const clip_or_mask,
                                 Geom::Affine const additional_affine)
{
    SPDesktop const *dt = getDesktop();
    if (dt == nullptr) {
        g_error("desktop == NULL, so we cannot snap; please inform the developers of this bug");
        return;
        // Apparently the setup() method from the SnapManager class hasn't been called before trying to snap.
    }

    static int recursion_level = 0;

    if (recursion_level == 0) {
        if (_findCandidates_already_called) { // In case we have already been called by another snapper,
            return; // then we don't need to search for candidates again
        }
        _findCandidates_already_called = true;
        _obj_snapper_candidates->clear();
        _align_snapper_candidates->clear();
    }
    recursion_level++;

    Geom::Rect bbox_to_snap_incl = bbox_to_snap; // _incl means: will include the snapper tolerance
    bbox_to_snap_incl.expandBy(object.getSnapperTolerance()); // see?

    for (auto& o: parent->children) {
        auto item = cast<SPItem>(&o);
        if (item && !(dt->itemIsHidden(item) && !clip_or_mask)) {
            // Fix LPE boolops self-snapping
            bool stop = false;
            if (item->style) {
                SPFilter *filt = item->style->getFilter();
                if (filt && filt->getId() && strcmp(filt->getId(), "selectable_hidder_filter") == 0) {
                    stop = true;
                }
                auto lpeitem = cast<SPLPEItem>(item);
                if (lpeitem && lpeitem->hasPathEffectOfType(Inkscape::LivePathEffect::EffectType::BOOL_OP)) {
                    stop = true;
                }
            }
            if (stop && it) {
                stop = false;
                for (auto skipitem : *it) {
                    if (skipitem && skipitem->style) {
                        auto toskip = cast<SPItem>(const_cast<SPObject *>(skipitem));
                        if (toskip) {
                            SPFilter *filt = toskip->style->getFilter();
                            if (filt && filt->getId() && strcmp(filt->getId(), "selectable_hidder_filter") == 0) {
                                stop = true;
                                break;
                            }

                            auto lpeitem = cast<SPLPEItem>(toskip);
                            if (!stop && lpeitem &&
                                lpeitem->hasPathEffectOfType(Inkscape::LivePathEffect::EffectType::BOOL_OP)) {
                                stop = true;
                                break;
                            }
                        }
                    }
                }
                if (stop) {
                    continue;
                }
            }
            // Snapping to items in a locked layer is allowed
            // Don't snap to hidden objects, unless they're a clipped path or a mask
            /* See if this item is on the ignore list */
            std::vector<SPObject const *>::const_iterator i;
            if (it != nullptr) {
                i = it->begin();
                while (i != it->end() && *i != &o) {
                    ++i;
                }
            }

            if (it == nullptr || i == it->end()) {
                if (item) {
                    if (!clip_or_mask) { // cannot clip or mask more than once
                        // The current item is not a clipping path or a mask, but might
                        // still be the subject of clipping or masking itself ; if so, then
                        // we should also consider that path or mask for snapping to
                        SPObject *obj = item->getClipObject();
                        if (obj && snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_PATH_CLIP)) {
                            _findCandidates(obj, it, bbox_to_snap, true, item->i2doc_affine());
                        }
                        obj = item->getMaskObject();
                        if (obj && snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_PATH_MASK)) {
                            _findCandidates(obj, it, bbox_to_snap, true, item->i2doc_affine());
                        }
                    }

                    if (is<SPGroup>(item)) {
                        _findCandidates(&o, it, bbox_to_snap, clip_or_mask, additional_affine);
                    } else {
                        Geom::OptRect bbox_of_item;
                        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
                        int prefs_bbox = prefs->getBool("/tools/bounding_box", false);
                        // We'll only need to obtain the visual bounding box if the user preferences tell
                        // us to, AND if we are snapping to the bounding box itself. If we're snapping to
                        // paths only, then we can just as well use the geometric bounding box (which is faster)
                        SPItem::BBoxType bbox_type = (!prefs_bbox && snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_BBOX_CATEGORY)) ?
                            SPItem::VISUAL_BBOX : SPItem::GEOMETRIC_BBOX;
                        if (clip_or_mask) {
                            // Oh oh, this will get ugly. We cannot use sp_item_i2d_affine directly because we need to
                            // insert an additional transformation in document coordinates (code copied from sp_item_i2d_affine)
                            bbox_of_item = item->bounds(bbox_type, item->i2doc_affine() * additional_affine * dt->doc2dt());
                        } else {
                            bbox_of_item = item->desktopBounds(bbox_type);
                        }
                        if (bbox_of_item) {
                            bool overflow = false;
                            // See if the item is within range
                            auto display_area = getDesktop()->get_display_area().bounds();
                            if (display_area.intersects(*bbox_of_item)) {
                                // Finally add the object to _candidates.
                                _align_snapper_candidates->emplace_back(item, clip_or_mask, additional_affine);
                                // For debugging: print the id of the candidate to the console
                                // SPObject *obj = (SPObject*)item;
                                // std::cout << "Snap candidate added: " << obj->getId() << std::endl;

                                if (bbox_to_snap_incl.intersects(*bbox_of_item)
                                        || (snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_ROTATION_CENTER) && bbox_to_snap_incl.contains(item->getCenter()))) { // rotation center might be outside of the bounding box
                                    // This item is within snapping range, so record it as a candidate
                                    _obj_snapper_candidates->emplace_back(item, clip_or_mask, additional_affine);
                                    // For debugging: print the id of the candidate to the console
                                    // SPObject *obj = (SPObject*)item;
                                    // std::cout << "Snap candidate added: " << obj->getId() << std::endl;
                                }

                                if (_align_snapper_candidates->size() > 200) { // This makes Inkscape crawl already
                                    overflow = true;
                                }
                            }

                            if (overflow) {
                                static Glib::Timer timer;
                                if (timer.elapsed() > 1.0) {
                                    timer.reset();
                                    std::cerr << "Warning: limit of 200 snap target paths reached, some will be ignored" << std::endl;
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    recursion_level--;
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

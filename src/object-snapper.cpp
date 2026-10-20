// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Snapping things to objects.
 *
 * Authors:
 *   Carl Hetherington <inkscape@carlh.net>
 *   Diederik van Lierop <mail@diedenrezi.nl>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2005 - 2012 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <2geom/circle.h>
#include <2geom/line.h>
#include <2geom/path-intersection.h>
#include <2geom/path-sink.h>
#include <memory>

#include "desktop.h"
#include "path/path-curve.h"
#include "document.h"
#include "preferences.h"
#include "snap-enums.h"
#include "text-editing.h"
#include "page-manager.h"

#include "object/sp-flowtext.h"
#include "object/sp-item.h"
#include "object/sp-path.h"
#include "object/sp-page.h"
#include "object/sp-root.h"
#include "object/sp-shape.h"
#include "object/sp-use.h"
#include "object/sp-text.h"
#include "path/path-util.h" // curve_for_item

Inkscape::ObjectSnapper::ObjectSnapper(SnapManager *sm, Geom::Coord const d)
    : Snapper(sm, d)
{
    _points_to_snap_to = std::make_unique<std::vector<SnapCandidatePoint>>();
    _paths_to_snap_to = std::make_unique<std::vector<SnapCandidatePath>>();
}

Inkscape::ObjectSnapper::~ObjectSnapper()
{
    _points_to_snap_to->clear();
    _clear_paths();
}

Geom::Coord Inkscape::ObjectSnapper::getSnapperTolerance() const
{
    return _snapmanager->getSnapperTolerance(_snapmanager->snapprefs.getObjectTolerance());
}

bool Inkscape::ObjectSnapper::getSnapperAlwaysSnap(SnapSourceType const &/*source*/) const
{
    return Preferences::get()->getBool("/options/snap/object/always", false);
}

void Inkscape::ObjectSnapper::_collectNodes(SnapSourceType const &t,
                                            bool const &first_point) const
{
    // Now, let's first collect all points to snap to. If we have a whole bunch of points to snap,
    // e.g. when translating an item using the selector tool, then we will only do this for the
    // first point and store the collection for later use. This significantly improves the performance
    if (first_point) {
        _points_to_snap_to->clear();

         // Determine the type of bounding box we should snap to
        SPItem::BBoxType bbox_type = SPItem::GEOMETRIC_BBOX;

        bool p_is_a_node = t & SNAPSOURCE_NODE_CATEGORY;
        bool p_is_a_bbox = t & SNAPSOURCE_BBOX_CATEGORY;
        bool p_is_other = (t & SNAPSOURCE_OTHERS_CATEGORY) || (t & SNAPSOURCE_DATUMS_CATEGORY);

        // A point considered for snapping should be either a node, a bbox corner or a guide/other. Pick only ONE!
        if (((p_is_a_node && p_is_a_bbox) || (p_is_a_bbox && p_is_other) || (p_is_a_node && p_is_other))) {
            g_warning("Snap warning: node type is ambiguous");
        }

        if (_snapmanager->snapprefs.isTargetSnappable(SNAPTARGET_BBOX_CORNER, SNAPTARGET_BBOX_EDGE_MIDPOINT, SNAPTARGET_BBOX_MIDPOINT)) {
            Preferences *prefs = Preferences::get();
            bool prefs_bbox = prefs->getBool("/tools/bounding_box");
            bbox_type = !prefs_bbox ?
                SPItem::VISUAL_BBOX : SPItem::GEOMETRIC_BBOX;
        }

        // Consider the page border for snapping to
        if (auto document = _snapmanager->getDocument()) {
            auto ignore_page = _snapmanager->getPageToIgnore();
            for (auto page : document->getPageManager().getPages()) {
                if (ignore_page == page)
                    continue;
                if (_snapmanager->snapprefs.isTargetSnappable(SNAPTARGET_PAGE_EDGE_CORNER)) {
                    getBBoxPoints(page->getDesktopRect(), _points_to_snap_to.get(), true,
                        SNAPSOURCE_PAGE_CORNER, SNAPTARGET_PAGE_EDGE_CORNER,
                        SNAPSOURCE_UNDEFINED, SNAPTARGET_UNDEFINED, // No edges
                        SNAPSOURCE_PAGE_CENTER, SNAPTARGET_PAGE_EDGE_CENTER);
                }
                if (_snapmanager->snapprefs.isTargetSnappable(SNAPTARGET_PAGE_MARGIN_CORNER)) {
                    getBBoxPoints(page->getDesktopMargin(), _points_to_snap_to.get(), true,
                        SNAPSOURCE_UNDEFINED, SNAPTARGET_PAGE_MARGIN_CORNER,
                        SNAPSOURCE_UNDEFINED, SNAPTARGET_UNDEFINED, // No edges
                        SNAPSOURCE_UNDEFINED, SNAPTARGET_PAGE_MARGIN_CENTER);
                    getBBoxPoints(page->getDesktopBleed(), _points_to_snap_to.get(), true,
                        SNAPSOURCE_UNDEFINED, SNAPTARGET_PAGE_BLEED_CORNER,
                        SNAPSOURCE_UNDEFINED, SNAPTARGET_UNDEFINED, // No edges or center
                        SNAPSOURCE_UNDEFINED, SNAPTARGET_UNDEFINED);
                }
            }
            if (_snapmanager->snapprefs.isTargetSnappable(SNAPTARGET_PAGE_EDGE_CORNER)) {
                // Only the corners get added here.
                getBBoxPoints(document->preferredBounds(), _points_to_snap_to.get(), false,
                    SNAPSOURCE_UNDEFINED, SNAPTARGET_PAGE_EDGE_CORNER,
                    SNAPSOURCE_UNDEFINED, SNAPTARGET_UNDEFINED,
                    SNAPSOURCE_PAGE_CENTER, SNAPTARGET_PAGE_EDGE_CENTER);
            }
        }

        for (const auto & _candidate : *_snapmanager->_obj_snapper_candidates) {
            SPItem *root_item = _candidate.item;
            g_return_if_fail(root_item);

            //Collect all nodes so we can snap to them
            if (p_is_a_node || p_is_other || (p_is_a_bbox && !_snapmanager->snapprefs.getStrictSnapping())) {
                // Note: there are two ways in which intersections are considered:
                // Method 1: Intersections are calculated for each shape individually, for both the
                //           snap source and snap target (see sp_shape_snappoints)
                // Method 2: Intersections are calculated for each curve or line that we've snapped to, i.e. only for
                //           the target (see the intersect() method in the SnappedCurve and SnappedLine classes)
                // Some differences:
                // - Method 1 doesn't find intersections within a set of multiple objects
                // - Method 2 only works for targets
                // When considering intersections as snap targets:
                // - Method 1 only works when snapping to nodes, whereas
                // - Method 2 only works when snapping to paths
                // - There will be performance differences too!
                // If both methods are being used simultaneously, then this might lead to duplicate targets!

                // Well, here we will be looking for snap TARGETS. Both methods can therefore be used.
                // When snapping to paths, we will get a collection of snapped lines and snapped curves. findBestSnap() will
                // go hunting for intersections (but only when asked to in the prefs of course). In that case we can just
                // temporarily block the intersections in sp_item_snappoints, we don't need duplicates. If we're not snapping to
                // paths though but only to item nodes then we should still look for the intersections in sp_item_snappoints()
                bool old_pref = _snapmanager->snapprefs.isTargetSnappable(SNAPTARGET_PATH_INTERSECTION);
                if (_snapmanager->snapprefs.isTargetSnappable(SNAPTARGET_PATH)) {
                    // So if we snap to paths, then findBestSnap will find the intersections
                    // and therefore we temporarily disable SNAPTARGET_PATH_INTERSECTION, which will
                    // avoid root_item->getSnappoints() below from returning intersections
                    _snapmanager->snapprefs.setTargetSnappable(SNAPTARGET_PATH_INTERSECTION, false);
                }

                // We should not snap a transformation center to any of the centers of the items in the
                // current selection (see the comment in SelTrans::centerRequest())
                bool old_pref2 = _snapmanager->snapprefs.isTargetSnappable(SNAPTARGET_ROTATION_CENTER);
                if (old_pref2) {
                    std::vector<SPItem*> rotationSource=_snapmanager->getRotationCenterSource();
                    for (auto itemlist : rotationSource) {
                        if (_candidate.item == itemlist) {
                            // don't snap to this item's rotation center
                            _snapmanager->snapprefs.setTargetSnappable(SNAPTARGET_ROTATION_CENTER, false);
                            break;
                        }
                    }
                }

                root_item->getSnappoints(*_points_to_snap_to, &_snapmanager->snapprefs);

                // restore the original snap preferences
                _snapmanager->snapprefs.setTargetSnappable(SNAPTARGET_PATH_INTERSECTION, old_pref);
                _snapmanager->snapprefs.setTargetSnappable(SNAPTARGET_ROTATION_CENTER, old_pref2);
            }

            //Collect the bounding box's corners so we can snap to them
            if (p_is_a_bbox || (!_snapmanager->snapprefs.getStrictSnapping() && p_is_a_node) || p_is_other) {
                // Discard the bbox of a clipped path / mask, because we don't want to snap to both the bbox
                // of the item AND the bbox of the clipping path at the same time
                if (!_candidate.clip_or_mask) {
                    Geom::OptRect b = root_item->desktopBounds(bbox_type);
                    getBBoxPoints(b, _points_to_snap_to.get(), true,
                            _snapmanager->snapprefs.isTargetSnappable(SNAPTARGET_BBOX_CORNER),
                            _snapmanager->snapprefs.isTargetSnappable(SNAPTARGET_BBOX_EDGE_MIDPOINT),
                            _snapmanager->snapprefs.isTargetSnappable(SNAPTARGET_BBOX_MIDPOINT));
                }
            }
        }
    }
}

void Inkscape::ObjectSnapper::_snapNodes(IntermSnapResults &isr,
                                         SnapCandidatePoint const &p,
                                         std::vector<SnapCandidatePoint> *unselected_nodes,
                                         SnapConstraint const &c,
                                         Geom::Point const &p_proj_on_constraint) const
{
    // Iterate through all nodes, find out which one is the closest to p, and snap to it!

    _collectNodes(p.getSourceType(), p.getSourceNum() <= 0);

    if (unselected_nodes != nullptr && unselected_nodes->size() > 0) {
        g_assert(_points_to_snap_to != nullptr);
        _points_to_snap_to->insert(_points_to_snap_to->end(), unselected_nodes->begin(), unselected_nodes->end());
    }

    SnappedPoint s;
    bool success = false;
    bool strict_snapping = _snapmanager->snapprefs.getStrictSnapping();

    for (const auto & k : *_points_to_snap_to) {
        if (_allowSourceToSnapToTarget(p.getSourceType(), k.getTargetType(), strict_snapping)) {
            Geom::Point target_pt = k.getPoint();
            Geom::Coord dist = Geom::L2(target_pt - p.getPoint()); // Default: free (unconstrained) snapping
            if (!c.isUndefined()) {
                // We're snapping to nodes along a constraint only, so find out if this node
                // is at the constraint, while allowing for a small margin
                if (Geom::L2(target_pt - c.projection(target_pt)) > 1e-9) {
                    // The distance from the target point to its projection on the constraint
                    // is too large, so this point is not on the constraint. Skip it!
                    continue;
                }
                dist = Geom::L2(target_pt - p_proj_on_constraint);
            }

            if (dist < getSnapperTolerance() && dist < s.getSnapDistance()) {
                bool always = getSnapperAlwaysSnap(p.getSourceType());
                s = SnappedPoint(target_pt, p.getSourceType(), p.getSourceNum(), k.getTargetType(), dist, getSnapperTolerance(), always, false, true, k.getTargetBBox());
                success = true;
            }
        }
    }

    if (success) {
        isr.points.push_back(s);
    }
}

void Inkscape::ObjectSnapper::_snapTranslatingGuide(IntermSnapResults &isr,
                                         Geom::Point const &p,
                                         Geom::Point const &guide_normal) const
{
    // Iterate through all nodes, find out which one is the closest to this guide, and snap to it!
    _collectNodes(SNAPSOURCE_GUIDE, true);

    if (_snapmanager->snapprefs.isTargetSnappable(SNAPTARGET_PATH, SNAPTARGET_PATH_INTERSECTION, SNAPTARGET_BBOX_EDGE, SNAPTARGET_PAGE_EDGE_BORDER, SNAPTARGET_TEXT_BASELINE)) {
        _collectPaths(p, SNAPSOURCE_GUIDE, true);
        _snapPaths(isr, SnapCandidatePoint(p, SNAPSOURCE_GUIDE), nullptr, nullptr);
    }

    SnappedPoint s;

    Geom::Coord tol = getSnapperTolerance();
    bool always = getSnapperAlwaysSnap(SNAPSOURCE_GUIDE);

    for (const auto & k : *_points_to_snap_to) {
        Geom::Point target_pt = k.getPoint();
        // Project each node (*k) on the guide line (running through point p)
        Geom::Point p_proj = Geom::projection(target_pt, Geom::Line(p, p + Geom::rot90(guide_normal)));
        Geom::Coord dist = Geom::L2(target_pt - p_proj); // distance from node to the guide
        Geom::Coord dist2 = Geom::L2(p - p_proj); // distance from projection of node on the guide, to the mouse location
        if ((dist < tol && dist2 < tol) || always) {
            s = SnappedPoint(target_pt, SNAPSOURCE_GUIDE, 0, k.getTargetType(), dist, tol, always, false, true, k.getTargetBBox());
            isr.points.push_back(s);
        }
    }
}

/// @todo investigate why Geom::Point p is passed in but ignored.
void Inkscape::ObjectSnapper::_collectPaths(Geom::Point /*p*/,
                                         SnapSourceType const source_type,
                                         bool const &first_point) const
{
    // Now, let's first collect all paths to snap to. If we have a whole bunch of points to snap,
    // e.g. when translating an item using the selector tool, then we will only do this for the
    // first point and store the collection for later use. This significantly improves the performance
    if (first_point) {
        _clear_paths();

        // Determine the type of bounding box we should snap to
        SPItem::BBoxType bbox_type = SPItem::GEOMETRIC_BBOX;

        bool p_is_a_node = source_type & SNAPSOURCE_NODE_CATEGORY;
        bool p_is_a_bbox = source_type & SNAPSOURCE_BBOX_CATEGORY;
        bool p_is_other = (source_type & SNAPSOURCE_OTHERS_CATEGORY) || (source_type & SNAPSOURCE_DATUMS_CATEGORY);

        if (_snapmanager->snapprefs.isTargetSnappable(SNAPTARGET_BBOX_EDGE)) {
            Preferences *prefs = Preferences::get();
            int prefs_bbox = prefs->getBool("/tools/bounding_box", false);
            bbox_type = !prefs_bbox ?
                SPItem::VISUAL_BBOX : SPItem::GEOMETRIC_BBOX;
        }

        auto document = _snapmanager->getDocument();
        auto &pm = document->getPageManager();
        for (auto page : document->getPageManager().getPages()) {
            if (_snapmanager->snapprefs.isTargetSnappable(SNAPTARGET_PAGE_EDGE_BORDER) && _snapmanager->snapprefs.isAnyCategorySnappable()) {
                auto pathv = _getPathvFromRect(page->getDocumentRect());
                _paths_to_snap_to->emplace_back(pathv, SNAPTARGET_PAGE_EDGE_BORDER, Geom::OptRect());
            }

            if (_snapmanager->snapprefs.isTargetSnappable(SNAPTARGET_PAGE_MARGIN_BORDER) && _snapmanager->snapprefs.isAnyCategorySnappable()) {
                auto margin = _getPathvFromRect(page->getDocumentMargin());
                _paths_to_snap_to->emplace_back(margin, SNAPTARGET_PAGE_MARGIN_BORDER, Geom::OptRect());
                auto bleed = _getPathvFromRect(page->getDocumentBleed());
                _paths_to_snap_to->emplace_back(bleed, SNAPTARGET_PAGE_BLEED_BORDER, Geom::OptRect());
            }
        }

        if (!pm.hasPages()) {
            // Consider the page border for snapping
            if (_snapmanager->snapprefs.isTargetSnappable(SNAPTARGET_PAGE_EDGE_BORDER) && _snapmanager->snapprefs.isAnyCategorySnappable()) {
                auto pathv = _getPathvFromRect(*(_snapmanager->getDocument()->preferredBounds()));
                _paths_to_snap_to->emplace_back(pathv, SNAPTARGET_PAGE_EDGE_BORDER, Geom::OptRect());
            }
        }

        for (const auto & _candidate : *_snapmanager->_obj_snapper_candidates) {
            /* Transform the requested snap point to this item's coordinates */
            Geom::Affine i2doc(Geom::identity());
            SPItem *root_item = nullptr;
            /* We might have a clone at hand, so make sure we get the root item */
            auto use = cast<SPUse>(_candidate.item);
            if (use) {
                i2doc = use->get_root_transform();
                root_item = use->root();
                g_return_if_fail(root_item);
            } else {
                i2doc = _candidate.item->i2doc_affine();
                root_item = _candidate.item;
            }

            //Build a list of all paths considered for snapping to

            //Add the item's path to snap to
            if (_snapmanager->snapprefs.isTargetSnappable(SNAPTARGET_PATH, SNAPTARGET_PATH_INTERSECTION, SNAPTARGET_TEXT_BASELINE)) {
                if (p_is_other || p_is_a_node || (!_snapmanager->snapprefs.getStrictSnapping() && p_is_a_bbox)) {
                    if (is<SPText>(root_item) || is<SPFlowtext>(root_item)) {
                        if (_snapmanager->snapprefs.isTargetSnappable(SNAPTARGET_TEXT_BASELINE)) {
                            // Snap to the text baselines
                            Text::Layout const *layout = te_get_layout(static_cast<SPItem *>(root_item));
                            if (layout != nullptr && layout->outputExists()) {
                                Geom::Affine transform = root_item->i2dt_affine() * _candidate.additional_affine * _snapmanager->getDesktop()->doc2dt();
                                Geom::PathVector pv;
                                for (auto const& baseline : layout->getBaselines()) {
                                    std::array<Geom::LineSegment, 1> const segments{baseline};
                                    Geom::Path const baseline_path{segments.begin(), segments.end()};
                                    pv.push_back(baseline_path * transform);
                                }
                                _paths_to_snap_to->emplace_back(std::move(pv), SNAPTARGET_TEXT_BASELINE, Geom::OptRect());
                            }
                        }
                    } else {
                        // Snapping for example to a traced bitmap is very stressing for
                        // the CPU, so we'll only snap to paths having no more than 500 nodes
                        // This also leads to a lag of approx. 500 msec (in my lousy test set-up).
                        bool very_complex_path = false;
                        auto path = cast<SPPath>(root_item);
                        if (path) {
                            very_complex_path = path->nodesInPath() > 500;
                        }

                        if (!very_complex_path && root_item && _snapmanager->snapprefs.isTargetSnappable(SNAPTARGET_PATH, SNAPTARGET_PATH_INTERSECTION)) {
                            if (auto const shape = cast<SPShape>(root_item)) {
                                if (auto const curve = shape->curve()) {
                                    Geom::Affine transform = use ? use->get_xy_offset(): Geom::Affine(); // If we're dealing with an SPUse, then account for any X/Y offset
                                    transform *= root_item->i2dt_affine();              // Because all snapping calculations are done in desktop coordinates
                                    transform *= _candidate.additional_affine;          // Only used for snapping to masks or clips; see SnapManager::_findCandidates()
                                    transform *= _snapmanager->getDesktop()->doc2dt();  // Account for inverted y-axis
                                    auto pv = *curve;
                                    pv *= transform;
                                    _paths_to_snap_to->emplace_back(std::move(pv), SNAPTARGET_PATH, Geom::OptRect()); // Perhaps for speed, get a reference to the Geom::pathvector, and store the transformation besides t
                                }
                            }
                        }
                    }
                }
            }

            //Add the item's bounding box to snap to
            if (_snapmanager->snapprefs.isTargetSnappable(SNAPTARGET_BBOX_EDGE)) {
                if (p_is_other || p_is_a_bbox || (!_snapmanager->snapprefs.getStrictSnapping() && p_is_a_node)) {
                    // Discard the bbox of a clipped path / mask, because we don't want to snap to both the bbox
                    // of the item AND the bbox of the clipping path at the same time
                    if (!_candidate.clip_or_mask) {
                        if (auto rect = root_item->bounds(bbox_type, i2doc)) {
                            auto path = _getPathvFromRect(*rect);
                            rect = root_item->desktopBounds(bbox_type);
                            _paths_to_snap_to->emplace_back(std::move(path), SNAPTARGET_BBOX_EDGE, rect);
                        }
                    }
                }
            }
        }
    }
}

void Inkscape::ObjectSnapper::_snapPaths(IntermSnapResults &isr,
                                     SnapCandidatePoint const &p,
                                     std::vector<SnapCandidatePoint> *unselected_nodes,
                                     SPPath const *selected_path) const
{
    _collectPaths(p.getPoint(), p.getSourceType(), p.getSourceNum() <= 0);
    // Now we can finally do the real snapping, using the paths collected above

    SPDesktop const *dt = _snapmanager->getDesktop();
    g_assert(dt != nullptr);
    Geom::Point const p_doc = dt->dt2doc(p.getPoint());

    bool const node_tool_active = _snapmanager->snapprefs.isTargetSnappable(SNAPTARGET_PATH, SNAPTARGET_PATH_INTERSECTION) && selected_path != nullptr;

    if (p.getSourceNum() <= 0) {
        /* findCandidates() is used for snapping to both paths and nodes. It ignores the path that is
         * currently being edited, because that path requires special care: when snapping to nodes
         * only the unselected nodes of that path should be considered, and these will be passed on separately.
         * This path must not be ignored however when snapping to the paths, so we add it here
         * manually when applicable.
         * */
        if (node_tool_active) {
            // TODO fix the function to be const correct:
            if (auto curve = curve_for_item(const_cast<SPPath *>(selected_path))) {
                _paths_to_snap_to->emplace_back(*curve * selected_path->i2doc_affine(),
                                                SNAPTARGET_PATH, Geom::OptRect(), true);
            }
        }
    }

    int num_path = 0; // _paths_to_snap_to contains multiple path_vectors, each containing multiple paths.
                      // num_path will count the paths, and will not be zeroed for each path_vector. It will
                      // continue counting

    bool strict_snapping = _snapmanager->snapprefs.getStrictSnapping();
    bool snap_perp = _snapmanager->snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_PATH_PERPENDICULAR);
    bool snap_tang = _snapmanager->snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_PATH_TANGENTIAL);

    //dt->getSnapIndicator()->remove_debugging_points();
    for (const auto & it_p : *_paths_to_snap_to) {
        if (_allowSourceToSnapToTarget(p.getSourceType(), it_p.target_type, strict_snapping)) {
            bool const being_edited = node_tool_active && it_p.currently_being_edited;
            //if true then this pathvector it_pv is currently being edited in the node tool

            for (auto &it_pv : it_p.path_vector) {
                // Find a nearest point for each curve within this path
                // n curves will return n time values with 0 <= t <= 1
                std::vector<double> anp = it_pv.nearestTimePerCurve(p_doc);

                //std::cout << "#nearest points = " << anp.size() << " | p = " << p.getPoint() << std::endl;
                // Now we will examine each of the nearest points, and determine whether it's within snapping range and if we should snap to it
                std::vector<double>::const_iterator np = anp.begin();
                unsigned int index = 0;
                for (; np != anp.end(); ++np, index++) {
                    Geom::Curve const *curve = &it_pv.at(index);
                    Geom::Point const sp_doc = curve->pointAt(*np);
                    //dt->getSnapIndicator()->set_new_debugging_point(sp_doc*dt->doc2dt());
                    bool c1 = true;
                    bool c2 = true;
                    if (being_edited) {
                        /* If the path is being edited, then we should only snap though to stationary pieces of the path
                         * and not to the pieces that are being dragged around. This way we avoid
                         * self-snapping. For this we check whether the nodes at both ends of the current
                         * piece are unselected; if they are then this piece must be stationary
                         */
                        g_assert(unselected_nodes != nullptr);
                        Geom::Point start_pt = dt->doc2dt(curve->pointAt(0));
                        Geom::Point end_pt = dt->doc2dt(curve->pointAt(1));
                        c1 = isUnselectedNode(start_pt, unselected_nodes);
                        c2 = isUnselectedNode(end_pt, unselected_nodes);
                        /* Unfortunately, this might yield false positives for coincident nodes. Inkscape might therefore mistakenly
                         * snap to path segments that are not stationary. There are at least two possible ways to overcome this:
                         * - Linking the individual nodes of the SPPath we have here, to the nodes of the NodePath::SubPath class as being
                         *   used in sp_nodepath_selected_nodes_move. This class has a member variable called "selected". For this the nodes
                         *   should be in the exact same order for both classes, so we can index them
                         * - Replacing the SPPath being used here by the NodePath::SubPath class; but how?
                         */
                    }

                    Geom::Point const sp_dt = dt->doc2dt(sp_doc);
                    if (!being_edited || (c1 && c2)) {
                        Geom::Coord dist = Geom::distance(sp_doc, p_doc);
                        // std::cout << "  dist -> " << dist << std::endl;
                        if (dist < getSnapperTolerance()) {
                            // Add the curve we have snapped to
                            Geom::Point sp_tangent_dt = Geom::Point(0,0);
                            if (p.getSourceType() == Inkscape::SNAPSOURCE_GUIDE_ORIGIN) {
                                // We currently only use the tangent when snapping guides, so only in this case we will
                                // actually calculate the tangent to avoid wasting CPU cycles
                                Geom::Point sp_tangent_doc = curve->unitTangentAt(*np);
                                sp_tangent_dt = dt->doc2dt(sp_tangent_doc) - dt->doc2dt(Geom::Point(0,0));
                            }
                            bool always = getSnapperAlwaysSnap(p.getSourceType());
                            isr.curves.emplace_back(sp_dt, sp_tangent_dt, num_path, index, dist, getSnapperTolerance(), always, false, curve, p.getSourceType(), p.getSourceNum(), it_p.target_type, it_p.target_bbox);
                            if (snap_tang || snap_perp) {
                                // For each curve that's within snapping range, we will now also search for tangential and perpendicular snaps
                                _snapPathsTangPerp(snap_tang, snap_perp, isr, p, curve, dt);
                            }
                        }
                    }
                }
                num_path++;
            } // End of: for (Geom::PathVector::iterator ....)
        }
    }
}

/* Returns true if point is coincident with one of the unselected nodes */
bool Inkscape::ObjectSnapper::isUnselectedNode(Geom::Point const &point, std::vector<SnapCandidatePoint> const *unselected_nodes) const
{
    if (unselected_nodes == nullptr) {
        return false;
    }

    if (unselected_nodes->size() == 0) {
        return false;
    }

    for (const auto & unselected_node : *unselected_nodes) {
        if (Geom::L2(point - unselected_node.getPoint()) < 1e-4) {
            return true;
        }
    }

    return false;
}

void Inkscape::ObjectSnapper::_snapPathsConstrained(IntermSnapResults &isr,
                                     SnapCandidatePoint const &p,
                                     SnapConstraint const &c,
                                     Geom::Point const &p_proj_on_constraint,
                                     std::vector<SnapCandidatePoint> *unselected_nodes,
                                     SPPath const *selected_path) const
{

    _collectPaths(p_proj_on_constraint, p.getSourceType(), p.getSourceNum() <= 0);

    // Now we can finally do the real snapping, using the paths collected above

    SPDesktop const *dt = _snapmanager->getDesktop();
    g_assert(dt != nullptr);

    Geom::Point direction_vector = c.getDirection();
    if (!is_zero(direction_vector)) {
        direction_vector = Geom::unit_vector(direction_vector);
    }

    // The intersection point of the constraint line with any path, must lie within two points on the
    // SnapConstraint: p_min_on_cl and p_max_on_cl. The distance between those points is twice the snapping tolerance
    Geom::Point const p_min_on_cl = dt->dt2doc(p_proj_on_constraint - getSnapperTolerance() * direction_vector);
    Geom::Point const p_max_on_cl = dt->dt2doc(p_proj_on_constraint + getSnapperTolerance() * direction_vector);
    Geom::Coord tolerance = getSnapperTolerance();

    // PS: Because the paths we're about to snap to are all expressed relative to document coordinate system, we will have
    // to convert the snapper coordinates from the desktop coordinates to document coordinates

    Geom::PathVector constraint_path;
    if (c.isCircular()) {
        Geom::Circle constraint_circle(dt->dt2doc(c.getPoint()), c.getRadius());
        Geom::PathBuilder pb;
        pb.feed(constraint_circle);
        pb.flush();
        constraint_path = pb.peek();
    } else {
        Geom::Path constraint_line;
        constraint_line.start(p_min_on_cl);
        constraint_line.appendNew<Geom::LineSegment>(p_max_on_cl);
        constraint_path.push_back(constraint_line);
    }

    bool const node_tool_active = _snapmanager->snapprefs.isTargetSnappable(SNAPTARGET_PATH, SNAPTARGET_PATH_INTERSECTION) && selected_path != nullptr;

    //TODO: code duplication
    if (p.getSourceNum() <= 0) {
        /* findCandidates() is used for snapping to both paths and nodes. It ignores the path that is
         * currently being edited, because that path requires special care: when snapping to nodes
         * only the unselected nodes of that path should be considered, and these will be passed on separately.
         * This path must not be ignored however when snapping to the paths, so we add it here
         * manually when applicable.
         * */
        if (node_tool_active) {
            // TODO fix the function to be const correct:
            if (auto curve = curve_for_item(const_cast<SPPath *>(selected_path))) {
                _paths_to_snap_to->emplace_back(*curve * selected_path->i2doc_affine(), SNAPTARGET_PATH, Geom::OptRect(), true);
            }
        }
    }

    bool strict_snapping = _snapmanager->snapprefs.getStrictSnapping();

    // Find all intersections of the constrained path with the snap target candidates
    for (const auto & k : *_paths_to_snap_to) {
        if (_allowSourceToSnapToTarget(p.getSourceType(), k.target_type, strict_snapping)) {
            // Do the intersection math
            std::vector<Geom::PVIntersection> inters = constraint_path.intersect(k.path_vector);

            bool const being_edited = node_tool_active && k.currently_being_edited;

            // Convert the collected intersections to snapped points
            for (const auto & inter : inters) {
                int index = inter.second.path_index; // index on the second path, which is the target path that we snapped to
                Geom::Curve const *curve = &k.path_vector.at(index).at(inter.second.curve_index);

                bool c1 = true;
                bool c2 = true;
                //TODO: Remove code duplication, see _snapPaths; it's documented in detail there
                if (being_edited) {
                    g_assert(unselected_nodes != nullptr);
                    Geom::Point start_pt = dt->doc2dt(curve->pointAt(0));
                    Geom::Point end_pt = dt->doc2dt(curve->pointAt(1));
                    c1 = isUnselectedNode(start_pt, unselected_nodes);
                    c2 = isUnselectedNode(end_pt, unselected_nodes);
                }

                if (!being_edited || (c1 && c2)) {
                    // Convert to desktop coordinates
                    Geom::Point p_inters = dt->doc2dt(inter.point());
                    // Construct a snapped point
                    Geom::Coord dist = Geom::L2(p.getPoint() - p_inters);
                    bool always = getSnapperAlwaysSnap(p.getSourceType());
                    SnappedPoint s = SnappedPoint(p_inters, p.getSourceType(), p.getSourceNum(), k.target_type, dist, getSnapperTolerance(), always, true, false, k.target_bbox);
                    // Store the snapped point
                    if (dist <= tolerance) { // If the intersection is within snapping range, then we might snap to it
                        isr.points.push_back(s);
                    }
                }
            }
        }
    }
}


void Inkscape::ObjectSnapper::freeSnap(IntermSnapResults &isr,
                                            SnapCandidatePoint const &p,
                                            Geom::OptRect const &bbox_to_snap,
                                            std::vector<SPObject const *> const *it,
                                            std::vector<SnapCandidatePoint> *unselected_nodes) const
{
    if (_snap_enabled == false || _snapmanager->snapprefs.isSourceSnappable(p.getSourceType()) == false || ThisSnapperMightSnap() == false) {
        return;
    }

    /* Get a list of all the SPItems that we will try to snap to; this only needs to be done for some snappers, and
    not for the grid snappers, so we'll do this here and not in the Snapmanager::freeSnap(). This saves us from wasting
    precious CPU cycles */
    if (p.getSourceNum() <= 0) {
        Geom::Rect const local_bbox_to_snap = bbox_to_snap ? *bbox_to_snap : Geom::Rect(p.getPoint(), p.getPoint());
        _snapmanager->_findCandidates(_snapmanager->getDocument()->getRoot(), it, local_bbox_to_snap, false, Geom::identity());
    }

    _snapNodes(isr, p, unselected_nodes);

    if (_snapmanager->snapprefs.isTargetSnappable(SNAPTARGET_PATH, SNAPTARGET_PATH_INTERSECTION, SNAPTARGET_BBOX_EDGE, SNAPTARGET_PAGE_EDGE_BORDER, SNAPTARGET_TEXT_BASELINE)) {
        unsigned n = (unselected_nodes == nullptr) ? 0 : unselected_nodes->size();
        if (n > 0) {
            /* While editing a path in the node tool, findCandidates must ignore that path because
             * of the node snapping requirements (i.e. only unselected nodes must be snapable).
             * That path must not be ignored however when snapping to the paths, so we add it here
             * manually when applicable
             */
            SPPath const *path = nullptr;
            if (it != nullptr && !it->empty()) {
                SPPath const *tmpPath = cast<SPPath>(*it->begin());
                if ((it->size() == 1) && tmpPath) {
                    path = tmpPath;
                } // else: *it->begin() might be a SPGroup, e.g. when editing a LPE of text that has been converted to a group of paths
                // as reported in bug #356743. In that case we can just ignore it, i.e. not snap to this item
            }
            _snapPaths(isr, p, unselected_nodes, path);
        } else {
            _snapPaths(isr, p, nullptr, nullptr);
        }
    }
}

void Inkscape::ObjectSnapper::constrainedSnap( IntermSnapResults &isr,
                                                  SnapCandidatePoint const &p,
                                                  Geom::OptRect const &bbox_to_snap,
                                                  SnapConstraint const &c,
                                                  std::vector<SPObject const *> const *it,
                                                  std::vector<SnapCandidatePoint> *unselected_nodes) const
{
    if (_snap_enabled == false || _snapmanager->snapprefs.isSourceSnappable(p.getSourceType()) == false || ThisSnapperMightSnap() == false) {
        return;
    }

    // project the mouse pointer onto the constraint. Only the projected point will be considered for snapping
    Geom::Point pp = c.projection(p.getPoint());

    /* Get a list of all the SPItems that we will try to snap to; this only needs to be done for some snappers, and
    not for the grid snappers, so we'll do this here and not in the Snapmanager::freeSnap(). This saves us from wasting
    precious CPU cycles */
    if (p.getSourceNum() <= 0) {
        Geom::Rect const local_bbox_to_snap = bbox_to_snap ? *bbox_to_snap : Geom::Rect(pp, pp); // Using the projected point here! Not so in freeSnap()!
        _snapmanager->_findCandidates(_snapmanager->getDocument()->getRoot(), it, local_bbox_to_snap, false, Geom::identity());
    }

    // A constrained snap, is a snap in only one degree of freedom (specified by the constraint line).
    // This is useful for example when scaling an object while maintaining a fixed aspect ratio. Its
    // nodes are only allowed to move in one direction (i.e. in one degree of freedom).

    _snapNodes(isr, p, unselected_nodes, c, pp);

    if (_snapmanager->snapprefs.isTargetSnappable(SNAPTARGET_PATH, SNAPTARGET_PATH_INTERSECTION, SNAPTARGET_BBOX_EDGE, SNAPTARGET_PAGE_EDGE_BORDER, SNAPTARGET_TEXT_BASELINE)) {
        //TODO: Remove code duplication; see freeSnap()
        unsigned n = (unselected_nodes == nullptr) ? 0 : unselected_nodes->size();
        if (n > 0) {
            /* While editing a path in the node tool, findCandidates must ignore that path because
             * of the node snapping requirements (i.e. only unselected nodes must be snapable).
             * That path must not be ignored however when snapping to the paths, so we add it here
             * manually when applicable
             */
            SPPath const *path = nullptr;
            if (it != nullptr) {
                SPPath const *tmpPath = cast<SPPath>(*it->begin());
                if ((it->size() == 1) && tmpPath) {
                    path = tmpPath;
                } // else: *it->begin() might be a SPGroup, e.g. when editing a LPE of text that has been converted to a group of paths
                // as reported in bug #356743. In that case we can just ignore it, i.e. not snap to this item
            }
            _snapPathsConstrained(isr, p, c, pp, unselected_nodes, path);
        } else {
            _snapPathsConstrained(isr, p, c, pp, nullptr, nullptr);
        }
    }
}

bool Inkscape::ObjectSnapper::ThisSnapperMightSnap() const
{
    return true;
}

void Inkscape::ObjectSnapper::_clear_paths() const
{
    _paths_to_snap_to->clear();
}

Geom::PathVector Inkscape::ObjectSnapper::_getPathvFromRect(Geom::Rect const rect) const
{
    return rect_to_open_path(rect);
}

/**
 * Default version of the getBBoxPoints with default corner source types.
 */
void Inkscape::getBBoxPoints(Geom::OptRect const bbox,
                             std::vector<SnapCandidatePoint> *points,
                             bool const isTarget,
                             bool const corners,
                             bool const edges,
                             bool const midpoint)
{
    getBBoxPoints(bbox, points, isTarget,
        corners ? SNAPSOURCE_BBOX_CORNER : SNAPSOURCE_UNDEFINED,
        corners ? SNAPTARGET_BBOX_CORNER : SNAPTARGET_UNDEFINED,
        edges ? SNAPSOURCE_BBOX_EDGE_MIDPOINT : SNAPSOURCE_UNDEFINED,
        edges ? SNAPTARGET_BBOX_EDGE_MIDPOINT : SNAPTARGET_UNDEFINED,
        midpoint ? SNAPSOURCE_BBOX_MIDPOINT : SNAPSOURCE_UNDEFINED,
        midpoint ? SNAPTARGET_BBOX_MIDPOINT : SNAPTARGET_UNDEFINED);
}

void Inkscape::getBBoxPoints(Geom::OptRect const bbox,
                             std::vector<SnapCandidatePoint> *points,
                             bool const /*isTarget*/,
                             Inkscape::SnapSourceType corner_src,
                             Inkscape::SnapTargetType corner_tgt,
                             Inkscape::SnapSourceType edge_src,
                             Inkscape::SnapTargetType edge_tgt,
                             Inkscape::SnapSourceType mid_src,
                             Inkscape::SnapTargetType mid_tgt)
{
    if (bbox) {
        // collect the corners of the bounding box
        for (unsigned k = 0; k < 4; k++) {
            if (corner_src || corner_tgt) {
                points->emplace_back(bbox->corner(k), corner_src, -1, corner_tgt, *bbox);
            }
            // optionally, collect the midpoints of the bounding box's edges too
            if (edge_src || edge_tgt) {
                points->emplace_back((bbox->corner(k) + bbox->corner((k + 1) % 4)) / 2, edge_src, -1, edge_tgt, *bbox);
            }
        }
        if (mid_src || mid_tgt) {
            points->emplace_back(bbox->midpoint(), mid_src, -1, mid_tgt, *bbox);
        }
    }
}

bool Inkscape::ObjectSnapper::_allowSourceToSnapToTarget(SnapSourceType source, SnapTargetType target, bool strict_snapping) const
{
    bool allow_this_pair_to_snap = true;

    if (strict_snapping) { // bounding boxes will not snap to nodes/paths and vice versa
        if (((source & SNAPSOURCE_BBOX_CATEGORY) && (target & SNAPTARGET_NODE_CATEGORY)) ||
            ((source & SNAPSOURCE_NODE_CATEGORY) && (target & SNAPTARGET_BBOX_CATEGORY))) {
            allow_this_pair_to_snap = false;
        }
    }

    return allow_this_pair_to_snap;
}

void Inkscape::ObjectSnapper::_snapPathsTangPerp(bool snap_tang, bool snap_perp, IntermSnapResults &isr, SnapCandidatePoint const &p, Geom::Curve const *curve, SPDesktop const *dt) const
{
    bool always = getSnapperAlwaysSnap(p.getSourceType());
    // Here we will try to snap either tangentially or perpendicularly to a single path; for this we need to know where the origin is located of the line that is currently being rotated,
    // or we need to know the vector of the guide which is currently being translated
    std::vector<std::pair<Geom::Point, bool> > const origins_and_vectors = p.getOriginsAndVectors();
    // Now we will iterate over all the origins and vectors and see which of these will get use a tangential or perpendicular snap
    for (const auto & origins_and_vector : origins_and_vectors) {
        Geom::Point origin_or_vector_doc = dt->dt2doc(origins_and_vector.first); // "first" contains a Geom::Point, denoting either a point or vector
        if (origins_and_vector.second) { // if "second" is true then "first" is a vector, otherwise it's a point
            // So we have a vector, which tells us what tangential or perpendicular direction we're looking for
            if (curve->degreesOfFreedom() <= 2) { // A LineSegment has order one, and therefore 2 DOF
                // When snapping to a point of a line segment that has a specific tangential or normal vector, then either all point
                // along that line will be snapped to or no points at all will be snapped to. This is not very useful, so let's skip
                // any line segments and lets only snap to higher order curves
                continue;
            }
            // The vector is being treated as a point (relative to the origin), and has been translated to document coordinates accordingly
            // We need however to make it a vector again, because also the origin has been transformed
            origin_or_vector_doc -= dt->dt2doc(Geom::Point(0,0));
        }

        Geom::Point point_dt;
        Geom::Coord dist;
        std::vector<double> ts;

        if (snap_tang) { // Find all points that lead to a tangential snap
            if (origins_and_vector.second) { // if "second" is true then "first" is a vector, otherwise it's a point
                ts = find_tangents_by_vector(origin_or_vector_doc, curve->toSBasis());
            } else {
                ts = find_tangents(origin_or_vector_doc, curve->toSBasis());
            }
            for (double t : ts) {
                point_dt = dt->doc2dt(curve->pointAt(t));
                dist = Geom::distance(point_dt, p.getPoint());
                isr.points.emplace_back(point_dt, p.getSourceType(), p.getSourceNum(), SNAPTARGET_PATH_TANGENTIAL, dist, getSnapperTolerance(), always, false, true);
            }
        }

        if (snap_perp) { // Find all points that lead to a perpendicular snap
            if (origins_and_vector.second) {
                ts = find_normals_by_vector(origin_or_vector_doc, curve->toSBasis());
            } else {
                ts = find_normals(origin_or_vector_doc, curve->toSBasis());
            }
            for (double t : ts) {
                point_dt = dt->doc2dt(curve->pointAt(t));
                dist = Geom::distance(point_dt, p.getPoint());
                isr.points.emplace_back(point_dt, p.getSourceType(), p.getSourceNum(), SNAPTARGET_PATH_PERPENDICULAR, dist, getSnapperTolerance(), always, false, true);
            }
        }
    }
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

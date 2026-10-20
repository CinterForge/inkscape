// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * A class for handling shape interaction with libavoid.
 *
 * Authors:
 *   Michael Wybrow <mjwybrow@users.sourceforge.net>
 *   Abhishek Sharma
 *
 * Copyright (C) 2005 Michael Wybrow
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#include <cstring>
#include <iostream>

#include <libavoid/router.h>
#include <libavoid/shape.h>
#include <2geom/convex-hull.h>
#include <2geom/line.h>

#include "conn-avoid-ref.h"
#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "inkscape.h"
#include "layer-manager.h"

#include "object/sp-namedview.h"
#include "object/sp-shape.h"

using Inkscape::DocumentUndo;

using Avoid::Router;

static Avoid::Polygon avoid_item_poly(SPItem const *item);


SPAvoidRef::SPAvoidRef(SPItem *spitem)
    : shapeRef(nullptr)
    , item(spitem)
    , setting(false)
    , new_setting(false)
    , _transformed_connection()
{
}


SPAvoidRef::~SPAvoidRef()
{
    _transformed_connection.disconnect();

    // If the document is being destroyed then the router instance
    // and the ShapeRefs will have been destroyed with it.
    Router *router = item->document->getRouter();

    if (shapeRef && router) {
        router->deleteShape(shapeRef);
    }
    shapeRef = nullptr;
}


void SPAvoidRef::setAvoid(char const *value)
{
    // Don't keep avoidance information for cloned objects.
    if ( !item->cloned ) {
        new_setting = false;
        if (value && (strcmp(value, "true") == 0)) {
            new_setting = true;
        }
    }
}

void SPAvoidRef::handleSettingChange()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop == nullptr) {
        return;
    }
    if (desktop->getDocument() != item->document) {
        // We don't want to go any further if the active desktop's document
        // isn't the same as the document that this item is part of.  This
        // case can happen if a new document is loaded from the file chooser
        // or via the recent file menu.  In this case, we can end up here
        // as a result of a ensureUpToDate performed on a
        // document not yet attached to the active desktop.
        return;
    }

    if (new_setting == setting) {
        // Don't need to make any changes
        return;
    }
    setting = new_setting;

    Router *router = item->document->getRouter();

    _transformed_connection.disconnect();
    if (new_setting) {
        Avoid::Polygon poly = avoid_item_poly(item);
        if (poly.size() > 0) {
            _transformed_connection = item->connectTransformed(
                    sigc::ptr_fun(&avoid_item_move));

            char const *id = item->getAttribute("id");
            g_assert(id != nullptr);

            // Get a unique ID for the item.
            GQuark itemID = g_quark_from_string(id);

            shapeRef = new Avoid::ShapeRef(router, poly, itemID);
        }
    }
    else if (shapeRef)
    {
        router->deleteShape(shapeRef);
        shapeRef = nullptr;
    }
}


std::vector<SPItem *> SPAvoidRef::getAttachedShapes(const unsigned int type)
{
    std::vector<SPItem *> list;

    Avoid::IntList shapes;
    GQuark shapeId = g_quark_from_string(item->getId());
    item->document->getRouter()->attachedShapes(shapes, shapeId, type);

    for (auto const shape: shapes) {
        auto const connId = g_quark_to_string(shape);
        SPObject *obj = item->document->getObjectById(connId);
        if (obj == nullptr) {
            g_warning("getAttachedShapes: Object with id=\"%s\" is not "
                      "found. Skipping.", connId);
            continue;
        }
        auto shapeItem = cast<SPItem>(obj);
        list.push_back(shapeItem);
    }
    return list;
}


std::vector<SPItem *> SPAvoidRef::getAttachedConnectors(const unsigned int type)
{
    std::vector<SPItem *> list;

    Avoid::IntList conns;
    GQuark shapeId = g_quark_from_string(item->getId());
    item->document->getRouter()->attachedConns(conns, shapeId, type);

    for (auto const conn: conns) {
        auto const connId = g_quark_to_string(conn);
        SPObject *obj = item->document->getObjectById(connId);
        if (obj == nullptr) {
            g_warning("getAttachedConnectors: Object with id=\"%s\" is not "
                      "found. Skipping.", connId);
            continue;
        }
        auto connItem = cast<SPItem>(obj);
        list.push_back(connItem);
    }
    return list;
}

Geom::Point SPAvoidRef::getConnectionPointPos()
{
    g_assert(item);
    // the center is all we are interested in now; we used to care
    // about non-center points, but that's moot.
    Geom::OptRect bbox = item->documentVisualBounds();
    return (bbox) ? bbox->midpoint() : Geom::Point(0, 0);
}

static std::vector<Geom::Point> approxCurveWithPoints(Geom::PathVector const &curve_pv)
{
    // The number of segments to use for not straight curves approximation
    constexpr unsigned NUM_SEGS = 4;
   
    // The structure to hold the output
    std::vector<Geom::Point> poly_points;

    // Iterate over all curves, adding the endpoints for linear curves and
    // sampling the other curves
    double seg_size = 1.0 / NUM_SEGS;
    double at;
    at = 0;
    Geom::PathVector::const_iterator pit = curve_pv.begin();
    while (pit != curve_pv.end())
    {
        Geom::Path::const_iterator cit = pit->begin();
        while (cit != pit->end())
        {
            if (cit == pit->begin())
            {
                poly_points.push_back(cit->initialPoint());
            }

            if (dynamic_cast<Geom::CubicBezier const*>(&*cit))
            {
                at += seg_size;
                if (at <= 1.0 )
                    poly_points.push_back(cit->pointAt(at));
                else
                {
                    at = 0.0;
                    ++cit;
                }
            }
            else
            {
                poly_points.push_back(cit->finalPoint());
                ++cit;
            }
        }
        ++pit;
    }
    return poly_points;
}

static std::vector<Geom::Point> approxItemWithPoints(SPItem const *item, const Geom::Affine& item_transform)
{
    auto item_mutable = const_cast<SPItem *>(item);

    if (auto group = cast<SPGroup>(item_mutable)) {
        std::vector<Geom::Point> poly_points;
        // consider all first-order children
        std::vector<SPItem*> itemlist = group->item_list();
        for (auto child_item : itemlist) {
            std::vector<Geom::Point> child_points = approxItemWithPoints(child_item, item_transform * child_item->transform);
            poly_points.insert(poly_points.end(), child_points.begin(), child_points.end());
        }
        return poly_points;
    } else if (auto shape = cast<SPShape>(item_mutable)) {
        shape->set_shape();
        // make sure it has an associated curve
        if (shape->curve()) {
            auto item_pathv = *shape->curve();
            // apply transformations (up to common ancestor)
            item_pathv *= item_transform;
            return approxCurveWithPoints(item_pathv);
        } else {
            return {};
        }
    } else {
        if (auto bbox = item->documentPreferredBounds()) {
            return approxCurveWithPoints(Geom::Path{*bbox});
        } else {
            return {};
        }
    }
}

static Avoid::Polygon avoid_item_poly(SPItem const *item)
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    g_assert(desktop != nullptr);
    auto const spacing = desktop->getNamedView()->connector_spacing;

    Geom::Affine itd_mat = item->i2doc_affine();
    std::vector<Geom::Point> hull_points;
    hull_points = approxItemWithPoints(item, itd_mat);

    // create convex hull from all sampled points
    Geom::ConvexHull hull(hull_points);

    // enlarge path by "desktop->namedview->connector_spacing"
    // store expanded convex hull in Avoid::Polygn
    Avoid::Polygon poly;
    if (hull.empty()) {
        return poly;
    }

    Geom::Line hull_edge(hull.back(), hull.front());
    Geom::Line prev_parallel_hull_edge;
    prev_parallel_hull_edge.setOrigin(hull_edge.origin() + hull_edge.versor().ccw() * spacing);
    prev_parallel_hull_edge.setVector(hull_edge.versor());

    std::size_t const hull_size = hull.size();
    for (std::size_t i = 0; i < hull_size; ++i)
    {
        if (i + 1 == hull_size) {
            hull_edge.setPoints(hull.back(), hull.front());
        } else {
            hull_edge.setPoints(hull[i], hull[i + 1]);
        }

        Geom::Line parallel_hull_edge;
        parallel_hull_edge.setOrigin(hull_edge.origin() + hull_edge.versor().ccw() * spacing);
        parallel_hull_edge.setVector(hull_edge.versor());

        // determine the intersection point
        try {
            Geom::OptCrossing int_pt = Geom::intersection(parallel_hull_edge, prev_parallel_hull_edge);
            if (int_pt) {
                Avoid::Point avoid_pt((parallel_hull_edge.origin()+parallel_hull_edge.versor()*int_pt->ta)[Geom::X],
                                        (parallel_hull_edge.origin()+parallel_hull_edge.versor()*int_pt->ta)[Geom::Y]);
                poly.ps.push_back(avoid_pt);
            } else {
                // something went wrong...
                std::cerr<<"conn-avoid-ref.cpp: avoid_item_poly: Geom:intersection failed."<<std::endl;
            }
        } catch (Geom::InfiniteSolutions const &e) {
            // the parallel_hull_edge and prev_parallel_hull_edge lie on top of each other, hence infinite crossings
            g_message("conn-avoid-ref.cpp: trying to get crossings of identical lines");
        }

        prev_parallel_hull_edge = parallel_hull_edge;
    }

    return poly;
}

static inline void get_avoided_items_rec(std::vector<SPItem *> &list, SPObject *from, SPDesktop *desktop, bool initialised);

std::vector<SPItem *> get_avoided_items(SPObject *from, SPDesktop *desktop, bool initialised)
{
    std::vector<SPItem *> list;
    get_avoided_items_rec(list, from, desktop, initialised);
    return list;
}

static inline void get_avoided_items_rec(std::vector<SPItem *> &list, SPObject *from, SPDesktop *desktop, bool initialised)
{
    for (auto &child: from->children) {
        if (is<SPItem>(&child) &&
            !desktop->layerManager().isLayer(cast<SPItem>(&child)) &&
            !cast_unsafe<SPItem>(&child)->isLocked() &&
            !desktop->itemIsHidden(cast<SPItem>(&child)) &&
            (!initialised || cast<SPItem>(&child)->getAvoidRef().shapeRef)
            )
        {
            list.push_back(cast<SPItem>(&child));
        }

        if (is<SPItem>(&child) && desktop->layerManager().isLayer(cast<SPItem>(&child))) {
            get_avoided_items_rec(list, &child, desktop, initialised);
        }
    }
}


void avoid_item_move(Geom::Affine const */*mp*/, SPItem *moved_item)
{
    Avoid::ShapeRef *shapeRef = moved_item->getAvoidRef().shapeRef;
    g_assert(shapeRef);

    Router *router = moved_item->document->getRouter();
    Avoid::Polygon poly = avoid_item_poly(moved_item);
    if (!poly.empty()) {
        router->moveShape(shapeRef, poly);
    }
}


void init_avoided_shape_geometry(SPDesktop *desktop)
{
    // Don't count this as changes to the document,
    // it is basically just late initialisation.
    SPDocument *document = desktop->getDocument();
    DocumentUndo::ScopedInsensitive _no_undo(document);

    bool initialised = false;
    auto items = get_avoided_items(desktop->layerManager().currentRoot(), desktop, initialised);

    for (auto item : items) {
        item->getAvoidRef().handleSettingChange();
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

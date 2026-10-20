// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Boolean operations.
 *//*
 * Authors:
 * see git history
 *  Created by fred on Fri Dec 05 2003.
 *  tweaked endlessly by bulia byak <buliabyak@users.sf.net>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "path-boolop.h"

#include <vector>

#include <glibmm/i18n.h>

#include <2geom/intersection-graph.h>
#include <2geom/svg-path-parser.h> // to get from SVG on boolean to Geom::Path
#include <2geom/utils.h>

#include "livarot/Path.h"
#include "livarot/Shape.h"

/*
 * Utilities
 */

/**
 * Create a flattened shape from a path.
 *
 * @param path The path to convert.
 * @param path_id The id to assign to all the edges in the resultant shape.
 * @param fill_rule The fill rule with which to flatten the path.
 * @param close_if_needed If the path is not closed, whether to add a closing segment.
 */
static Shape make_shape(Path &path, int path_id = -1, FillRule fill_rule = fill_nonZero, bool close_if_needed = true)
{
    Shape result;

    Shape tmp;
    path.Fill(&tmp, path_id, false, close_if_needed);
    result.ConvertToShape(&tmp, fill_rule);

    return result;
}

/**
 * Create a path with backdata from a pathvector,
 * automatically estimating a suitable conversion threshold.
 */
static Path make_path(Geom::PathVector const &pathv, std::vector<Geom::PathVectorTime> const &cuts)
{
    Path result;

    result.LoadPathVector(pathv, cuts);
    result.ConvertWithBackData(RELATIVE_THRESHOLD, true);

    return result;
}

/**
 * Return whether a path is a single open line segment.
 */
bool is_line(Path const &path)
{
    return path.pts.size() == 2 && path.pts[0].isMoveTo && !path.pts[1].isMoveTo;
}

void distribute_intersection_times(std::vector<Geom::PathVectorTime> &dst1,
                                   std::vector<Geom::PathVectorTime> &dst2,
                                   std::vector<Geom::PathVectorIntersection> const &intersections)
{
    auto filter_and_add = [] (auto const &x, auto &dst) {
        if (x.t > Geom::EPSILON && x.t < 1.0 - Geom::EPSILON) {
            dst.emplace_back(x);
        }
    };

    for (auto const &x : intersections) {
        filter_and_add(x.first, dst1);
        filter_and_add(x.second, dst2);
    }
}

void sort_and_clean_intersection_times(std::vector<Geom::PathVectorTime> &vec)
{
    std::sort(begin(vec), end(vec));

    auto prev = Geom::PathVectorTime{0, 0, 0.0};
    for (auto it = vec.begin(); it != vec.end(); ) {
        if (it->path_index == prev.path_index && it->curve_index == prev.curve_index && it->t < prev.t + Geom::EPSILON) {
            it = vec.erase(it);
        } else {
            prev = *it;
            ++it;
        }
    }
}

/*
 * Flattening
 */

Geom::PathVector flattened(Geom::PathVector const &pathv, FillRule fill_rule)
{
    std::vector<Geom::PathVectorTime> times;
    distribute_intersection_times(times, times, pathv.intersectSelf());
    sort_and_clean_intersection_times(times);

    auto path = make_path(pathv, times);
    auto shape = make_shape(path, 0, fill_rule);

    Path res;
    shape.ConvertToForme(&res, 1, std::begin({ &path }));

    return res.MakePathVector();
}

void flatten(Geom::PathVector &pathv, FillRule fill_rule)
{
    pathv = flattened(pathv, fill_rule);
}

/*
 * Boolean operations on pathvectors
 */

std::vector<Geom::PathVector> pathvector_cut(Geom::PathVector const &pathv, Geom::PathVector const &lines)
{
    std::vector<Geom::PathVectorTime> timesa, timesb;
    distribute_intersection_times(timesa, timesa, pathv.intersectSelf());
    distribute_intersection_times(timesb, timesb, lines.intersectSelf());
    distribute_intersection_times(timesa, timesb, pathv.intersect(lines));
    sort_and_clean_intersection_times(timesa);
    sort_and_clean_intersection_times(timesb);

    auto patha = make_path(pathv, timesa);
    auto pathb = make_path(lines, timesb);
    auto shapea = make_shape(patha, 0);
    auto shapeb = make_shape(pathb, 1, fill_justDont, is_line(pathb));

    Shape shape;
    shape.Booleen(&shapeb, &shapea, bool_op_cut, 1);

    Path path;
    int num_nesting = 0;
    int *nesting = nullptr;
    int *conts = nullptr;
    shape.ConvertToFormeNested(&path, 2, std::begin({ &patha, &pathb }), num_nesting, nesting, conts, true);

    int num_paths;
    auto paths = path.SubPathsWithNesting(num_paths, false, num_nesting, nesting, conts);

    std::vector<Geom::PathVector> result;
    result.reserve(num_paths);

    for (int i = 0; i < num_paths; i++) {
        result.emplace_back(paths[i]->MakePathVector());
        delete paths[i];
    }

    g_free(paths);
    g_free(conts);
    g_free(nesting);

    return result;
}

Geom::PathVector sp_pathvector_boolop(Geom::PathVector const &pathva, Geom::PathVector const &pathvb, BooleanOp bop, FillRule fra, FillRule frb)
{
    std::vector<Geom::PathVectorTime> timesa, timesb;
    distribute_intersection_times(timesa, timesa, pathva.intersectSelf());
    distribute_intersection_times(timesb, timesb, pathvb.intersectSelf());
    distribute_intersection_times(timesa, timesb, pathva.intersect(pathvb));
    sort_and_clean_intersection_times(timesa);
    sort_and_clean_intersection_times(timesb);

    auto patha = make_path(pathva, timesa);
    auto pathb = make_path(pathvb, timesb);

    Path result;

    if (bop == bool_op_inters || bop == bool_op_union || bop == bool_op_diff || bop == bool_op_symdiff) {
        // true boolean op
        // get the polygons of each path, with the winding rule specified, and apply the operation iteratively
        auto shapea = make_shape(patha, 0, fra);
        auto shapeb = make_shape(pathb, 1, frb);

        Shape shape;
        shape.Booleen(&shapeb, &shapea, bop);

        shape.ConvertToForme(&result, 2, std::begin({ &patha, &pathb }));

    } else if (bop == bool_op_cut) {
        // cuts= sort of a bastard boolean operation, thus not the axact same modus operandi
        // technically, the cut path is not necessarily a polygon (thus has no winding rule)
        // it is just uncrossed, and cleaned from duplicate edges and points
        // then it's fed to Booleen() which will uncross it against the other path
        // then comes the trick: each edge of the cut path is duplicated (one in each direction),
        // thus making a polygon. the weight of the edges of the cut are all 0, but
        // the Booleen need to invert the ones inside the source polygon (for the subsequent
        // ConvertToForme)

        // the cut path needs to have the highest pathID in the back data
        // that's how the Booleen() function knows it's an edge of the cut
        // fill_justDont doesn't compute winding numbers
        // see LP Bug 177956 for why is_line is needed
        auto shapea = make_shape(patha, 1, fill_justDont, is_line(patha));
        auto shapeb = make_shape(pathb, 0, frb);

        Shape shape;
        shape.Booleen(&shapea, &shapeb, bool_op_cut, 1);

        shape.ConvertToForme(&result, 2, std::begin({ &pathb, &patha }), true);

    } else if (bop == bool_op_slice) {
        // slice is not really a boolean operation
        // you just put the 2 shapes in a single polygon, uncross it
        // the points where the degree is > 2 are intersections
        // just check it's an intersection on the path you want to cut, and keep it
        // the intersections you have found are then fed to ConvertPositionsToMoveTo() which will
        // make new subpath at each one of these positions
        // inversion pour l'opération

        Shape tmp;
        pathb.Fill(&tmp, 0, false, false, false); // don't closeIfNeeded
        patha.Fill(&tmp, 1, true, false, false); // don't closeIfNeeded and just dump in the shape, don't reset it

        Shape shape;
        shape.ConvertToShape(&tmp, fill_justDont);

        std::vector<Path::cut_position> toCut;

        assert(shape.hasBackData());

        for (int i = 0; i < shape.numberOfPoints(); i++) {
            if (shape.getPoint(i).totalDegree() > 2) {
                // possibly an intersection
                // we need to check that at least one edge from the source path is incident to it
                // before we declare it's an intersection
                int nbOrig = 0;
                int nbOther = 0;
                int piece = -1;
                double t = 0.0;

                int cb = shape.getPoint(i).incidentEdge[FIRST];
                while (cb >= 0 && cb < shape.numberOfEdges()) {
                    if (shape.ebData[cb].pathID == 0) {
                        // the source has an edge incident to the point, get its position on the path
                        piece = shape.ebData[cb].pieceID;
                        t = shape.getEdge(cb).st == i ? shape.ebData[cb].tSt : shape.ebData[cb].tEn;
                        nbOrig++;
                    }
                    if (shape.ebData[cb].pathID == 1) {
                        nbOther++; // the cut is incident to this point
                    }
                    cb = shape.NextAt(i, cb);
                }

                if (nbOrig > 0 && nbOther > 0) {
                    // point incident to both path and cut: an intersection
                    // note that you only keep one position on the source; you could have degenerate
                    // cases where the source crosses itself at this point, and you wouyld miss an intersection
                    toCut.push_back({ .piece = piece, .t = t });
                }
            }
        }

        // I think it's useless now
        for (int i = shape.numberOfEdges() - 1; i >= 0; i--) {
            if (shape.ebData[i].pathID == 1) {
                shape.SubEdge(i);
            }
        }

        result.Copy(&pathb);
        result.ConvertPositionsToMoveTo(toCut.size(), toCut.data()); // cut where you found intersections
    }

    return result.MakePathVector();
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

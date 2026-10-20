// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file Test the stroke-to-path functionality.
 */
/*
 * Authors:
 *   KrIr17 <elendil.krir17@gmail.com>
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "path/path-outline.h"

#include <iostream>
#include <gtest/gtest.h>

#include "object/sp-path.h"
#include <2geom/pathvector.h>
#include "svg/svg.h"
#include "test-with-svg-object-pairs.h"

class PathoutlineTest : public Inkscape::TestWithSvgObjectPairs
{
protected:
    PathoutlineTest()
        : Inkscape::TestWithSvgObjectPairs("data/livarot-pathoutline.svg", 3) {}
};

double approximate_directed_hausdorff_distance(const Geom::Path *path1, const Geom::Path *path2)
{
    auto const time_range = path1->timeRange();
    double dist_max = 0.;
    for (size_t i = 0; i <= 25; i += 1) {
        auto const time = time_range.valueAt(static_cast<double>(i) / 25);
        auto const search_point = path1->pointAt(time);
        Geom::Coord dist;
        path2->nearestTime(search_point, &dist);
        if (dist > dist_max) {
            dist_max = dist;
        }
    }

    return dist_max;
}

TEST_F(PathoutlineTest, BoundedHausdorffDistance)
{
    double const tolerance = 0.1;

    unsigned case_index = 0;
    for (auto test_case : getTestCases()) {
        auto const *test_item = cast<SPShape>(test_case.test_object);
        auto const *comp_item = cast<SPPath>(test_case.reference_object);
        ASSERT_TRUE(test_item && comp_item);

        /* auto const outline_pathvector = *(item_to_paths(test_item)); */
        Geom::PathVector test_fill;
        Geom::PathVector test_stroke;
        item_find_paths(test_item, test_fill, test_stroke);
        auto const outline_pathvector = test_stroke;
        auto const comp_curve = comp_item->curve();
        auto const &comp_pathvector = *comp_curve;
        ASSERT_EQ(outline_pathvector.size(), comp_pathvector.size());

        double error = 0;
        for (unsigned i = 0; i < outline_pathvector.size(); i++) {
            double const error1 = approximate_directed_hausdorff_distance(&outline_pathvector[i], &comp_pathvector[i]);
            double const error2 = approximate_directed_hausdorff_distance(&comp_pathvector[i], &outline_pathvector[i]);
            error = std::max({error, error1, error2});
        }
        EXPECT_LE(error, tolerance) << "Hausdorff distance above tolerance in test case #" << case_index
                                    << "\nactual d " << sp_svg_write_path(Geom::PathVector(outline_pathvector), true)
                                    << "\nexpected d " << sp_svg_write_path(Geom::PathVector(comp_pathvector), true)
                                    << std::endl;
        case_index++;
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

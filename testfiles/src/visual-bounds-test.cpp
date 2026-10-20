// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file Test the computation of visual bounding boxes.
 */
/*
 * Authors:
 *   Rafał Siejakowski <rs@rs-math.net>
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include <gtest/gtest.h>

#include <2geom/rect.h>
#include <iostream>

#include "test-with-svg-object-pairs.h"
#include "object/sp-item.h"
#include "object/sp-rect.h"

class VisualBoundsTest : public Inkscape::TestWithSvgObjectPairs
{
public:
    VisualBoundsTest()
        : TestWithSvgObjectPairs("data/visual-bounds.svg", 29) {}
};

TEST_F(VisualBoundsTest, ShapeBounds)
{
    double constexpr epsilon = 1e-4;
    constexpr const char* coordinate_names[] = {"x", "y"};

    unsigned case_index = 0;
    for (auto test_case : getTestCases()) {
        auto const *item = cast<SPItem>(test_case.test_object);
        auto const *bbox = cast<SPItem>(test_case.reference_object);

        Geom::Rect const expected_bbox = cast<SPRect>(bbox)->getRect();

        auto const actual_opt_bbox = item->visualBounds(item->transform);
        ASSERT_TRUE(bool(actual_opt_bbox));
        Geom::Rect const actual_bbox = *actual_opt_bbox;

        // Check that the item's visual bounding box is as expected, up to epsilon.
        for (auto const dim : {Geom::X, Geom::Y}) {
            EXPECT_GE(actual_bbox[dim].min(), expected_bbox[dim].min() - epsilon) << "Lower " << coordinate_names[dim]
                                                                                  << "-extremum of bounding box #"
                                                                                  << case_index << " below tolerance";
            EXPECT_LE(actual_bbox[dim].min(), expected_bbox[dim].min() + epsilon) << "Lower " << coordinate_names[dim]
                                                                                  << "-extremum of bounding box #"
                                                                                  << case_index << " above tolerance";
            EXPECT_GE(actual_bbox[dim].max(), expected_bbox[dim].max() - epsilon) << "Upper " << coordinate_names[dim]
                                                                                  << "-extremum of bounding box #"
                                                                                  << case_index << " below tolerance";
            EXPECT_LE(actual_bbox[dim].max(), expected_bbox[dim].max() + epsilon) << "Upper " << coordinate_names[dim]
                                                                                  << "-extremum of bounding box #"
                                                                                  << case_index << " above tolerance";
        }
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

// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file Test the geom-pathstroke functionality.
 */
/*
 * Authors:
 *   Hendrik Roehm <git@roehm.ws>
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "helper/geom-pathstroke.h"

#include <iostream>
#include <gtest/gtest.h>

#include <2geom/ellipse.h>
#include <2geom/pathvector.h>
#include <2geom/path-sink.h>
#include <2geom/svg-path-parser.h>

#include "helper/geom.h"
#include "object/sp-path.h"
#include "svg/svg.h"
#include "test-with-svg-object-pairs.h"

static Geom::PathVector parse_svgd(char const *str)
{
    Geom::PathVector pathv;
    Geom::PathBuilder builder(pathv);
    Geom::SVGPathParser parser(builder);
    parser.parse(str);
    return pathv;
}

class GeomPathstrokeTest : public Inkscape::TestWithSvgObjectPairs
{
protected:
    GeomPathstrokeTest()
        : Inkscape::TestWithSvgObjectPairs("data/geom-pathstroke.svg", 8) {}
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

TEST_F(GeomPathstrokeTest, BoundedHausdorffDistance)
{
    double const tolerance = 0.1;
    // same as 0.1 inch in the document (only works without viewBox and transformations)
    auto const offset_width = -9.6;

    unsigned case_index = 0;
    for (auto test_case : getTestCases()) {
        auto const *test_item = cast<SPPath>(test_case.test_object);
        auto const *comp_item = cast<SPPath>(test_case.reference_object);
        ASSERT_TRUE(test_item && comp_item);

        // Note that transforms etc are not considered. Therefore the objects shoud have equal transforms.
        auto const test_curve = test_item->curve();
        auto const comp_curve = comp_item->curve();
        ASSERT_TRUE(test_curve && comp_curve);

        auto const &test_pathvector = *test_curve;
        auto const &comp_pathvector = *comp_curve;
        ASSERT_EQ(test_pathvector.size(), 1);
        ASSERT_EQ(comp_pathvector.size(), 1);

        auto const &test_path = test_pathvector.at(0);
        auto const &comp_path = comp_pathvector.at(0);

        auto const offset_path = Inkscape::half_outline(test_path, offset_width, 0, Inkscape::JOIN_EXTRAPOLATE, 0.);
        double const error1 = approximate_directed_hausdorff_distance(&offset_path, &comp_path);
        double const error2 = approximate_directed_hausdorff_distance(&comp_path, &offset_path);
        double const error = std::max(error1, error2);

        EXPECT_LE(error, tolerance) << "Hausdorff distance above tolerance in test case #" << case_index
                                    << "\nactual d " << sp_svg_write_path(Geom::PathVector(offset_path), true)
                                    << "\nexpected d " << sp_svg_write_path(Geom::PathVector(comp_path), true)
                                    << std::endl;
        case_index++;
    }
}

TEST_F(GeomPathstrokeTest, CheckSimpleRect)
{
    auto const rect = Geom::Rect::from_xywh(2, 3, 5, 8);
    ASSERT_EQ(rect, check_simple_rect(Geom::Path{rect}));
}

TEST_F(GeomPathstrokeTest, PathvFullyContainsTest)
{
    ASSERT_TRUE(pathv_fully_contains(Geom::Path{Geom::Rect::from_xywh(2, 3, 5, 9)},
                                     Geom::Path{Geom::Rect::from_xywh(2, 3, 5, 8)},
                                     fill_nonZero));

    ASSERT_FALSE(pathv_fully_contains(Geom::Path{Geom::Rect::from_xywh(1, 3, 5, 8)},
                                      Geom::Path{Geom::Rect::from_xywh(2, 3, 5, 8)},
                                      fill_nonZero));

    auto const arc = Geom::Ellipse(Geom::Point(0, 0), Geom::Point(2, 2), 2 * M_PI);

    ASSERT_TRUE(pathv_fully_contains(Geom::Path{arc},
                                     Geom::Path{Geom::Rect::from_xywh(-1, -1, 2, 2)},
                                     fill_nonZero));

    ASSERT_FALSE(pathv_fully_contains(Geom::Path{arc},
                                      Geom::Path{Geom::Rect::from_xywh(0, -1, 2, 2)},
                                      fill_nonZero));

    // Regression test for Issue #6067
    EXPECT_FALSE(pathv_fully_contains(parse_svgd("M 474.48 517.5600000000001 z"),
                                      parse_svgd("M 325.98 286.0800000000001 z"), fill_nonZero));
}

TEST_F(GeomPathstrokeTest, SplitNonintersectingTest)
{
    auto const nested_rects = parse_svgd("M 10.036275,56.773065 V 176.003 H 196.39337 V 56.773065 Z M 31.320764,75.324395 H 168.35426 V 146.67146 H 31.320764 Z M 43.254919,82.135335 V 102.95733 H 109.52116 V 82.135335 Z M 56.649967,87.515375 H 93.853898 V 99.151355 H 56.649967 Z M 68.251326,89.934865 V 95.849225 H 83.5491 V 89.934865 Z M 64.413318,121.06165 V 142.35699 H 147.13851 V 121.06165 Z");
    ASSERT_EQ(Inkscape::split_non_intersecting_paths(Geom::PathVector{nested_rects}, fill_oddEven).size(), 4);
    ASSERT_EQ(Inkscape::split_non_intersecting_paths(Geom::PathVector{nested_rects}, fill_nonZero).size(), 4);

    auto const panda = parse_svgd("M 66.48912,262.8455 C 49.59187,260.54506 33.87005,255.89955 20.05231,249.12428 -1.70434,238.45632 -17.15748,219.38576 -22.94335,196.06379 -25.56696,185.48837 -25.91378,157.58007 -23.55872,146.54455 -21.17322,135.36639 -16.40742,121.50481 -11.85377,112.50007 L -7.78451,104.4532 -10.93468,97.6443 C -13.6107,91.86024 -14.08485,89.06523 -14.08485,79.07459 -14.08485,68.73352 -13.65531,66.38385 -10.52669,59.61059 -6.61243,51.13651 2.70394,41.35849 10.76561,37.26324 26.06942,29.48904 46.76215,31.4896 59.85199,42.00888 L 64.66423,45.8761 77.53019,43.28664 C 94.77554,39.81577 114.74711,39.81577 131.99245,43.28664 L 144.85842,45.8761 149.67066,42.00888 C 162.7605,31.4896 183.45323,29.48904 198.75704,37.26324 206.81871,41.35849 216.13508,51.13651 220.04933,59.61059 223.17796,66.38385 223.6075,68.73352 223.6075,79.07459 223.6075,89.1797 223.15059,91.80973 220.36197,97.75627 L 217.11644,104.67713 220.04289,109.51709 C 223.80071,115.73202 230.00476,132.86173 232.54135,144.02605 235.35837,156.42458 235.32313,184.64393 232.47635,196.06379 226.65357,219.42195 211.21553,238.46713 189.47033,249.11827 165.97732,260.62551 142.49054,264.84995 103.15043,264.64417 89.73774,264.57397 73.24014,263.76461 66.48911,262.8455 Z M 137.56782,246.11469 C 174.39637,241.78427 199.72008,227.89791 210.41825,206.16686 222.91169,180.78907 219.63264,143.54939 202.22515,113.11906 198.21132,106.10243 192.92331,99.6058 184.086,90.83403 169.69464,76.54941 157.56387,69.00855 138.8058,62.68645 127.29601,58.80727 126.56934,58.72284 104.76132,58.73089 83.61107,58.73889 81.94245,58.91739 71.95483,62.24429 43.94926,71.57299 22.95792,87.70161 8.82246,110.75182 -3.42981,130.73117 -9.30146,153.20216 -8.14584,175.69002 -6.15814,214.36988 15.37651,235.88807 60.72502,244.50824 79.4804,248.07339 114.65583,248.80876 137.56782,246.11469 Z M 111.57906,207.09051 C 109.20081,206.05916 106.08115,204.11259 104.64648,202.76479 102.27463,200.53655 101.88893,200.49385 100.39487,202.29408 95.62299,208.04385 84.14645,209.58772 77.28351,205.40312 71.83238,202.07936 69.07634,197.41598 68.95907,191.31769 68.87657,187.02551 69.25808,186.15994 71.23267,186.15994 72.93082,186.15994 74.11375,187.68477 75.39311,191.52287 77.57854,198.07914 83.11477,202.05954 88.66757,201.06683 93.30899,200.23706 99.19041,193.94847 99.19041,189.8155 99.19041,188.10265 100.03314,186.37784 101.06315,185.98259 103.71453,184.96516 106.6183,187.77533 106.6183,191.35868 106.6183,193.06649 108.20315,195.98213 110.23701,198.01599 114.15724,201.93621 119.03271,202.69398 124.45111,200.22519 127.50095,198.83559 131.37792,192.49165 131.37792,188.89075 131.37792,186.82743 133.66374,185.62344 136.02034,186.44547 138.3107,187.24439 137.89738,194.24994 135.27095,199.1472 132.7212,203.90147 124.22241,209.09127 119.11289,209.01413 117.34753,208.98743 113.95731,208.12185 111.57906,207.09051 Z M 93.88245,175.90407 C 87.63819,169.56448 86.42304,165.05969 89.99398,161.48875 93.01264,158.47009 112.91424,158.7166 115.71128,161.8073 119.29253,165.76453 118.2146,169.71162 111.83719,175.99314 104.11418,183.60002 101.44966,183.5868 93.88245,175.90404 Z M 41.6243,172.87463 C 35.45338,170.6664 29.57217,166.08294 25.90686,160.62541 19.29007,150.77325 22.0099,139.09068 33.83498,126.57147 43.05812,116.80693 50.93326,112.52638 59.70144,112.51169 67.8629,112.49799 72.78283,115.26397 78.92787,123.32066 90.91505,139.0369 84.68702,162.96911 66.39293,171.48824 59.60043,174.65135 48.34922,175.28112 41.6243,172.87463 Z M 69.47887,155.82941 C 71.46998,153.8383 71.95483,151.7028 71.95483,144.92419 71.95483,135.99584 70.67431,133.0959 66.13775,131.75046 64.26131,131.19395 62.62672,131.73206 60.87633,133.48242 57.21796,137.14079 57.17531,152.19172 60.81303,155.82941 62.17481,157.19119 64.12463,158.30537 65.14596,158.30537 66.1673,158.30537 68.11712,157.19119 69.4789,155.82941 Z M 139.35595,171.46053 C 128.68733,166.49264 120.23609,153.56422 120.23609,142.21162 120.23609,135.79293 124.10291,125.95578 128.56929,121.01203 134.51251,114.4336 138.5919,112.49772 146.4817,112.51169 154.52816,112.52599 162.69198,116.87966 171.20771,125.69792 186.21022,141.23342 187.27226,155.10758 174.33097,166.49896 164.69911,174.97726 151.06255,176.91175 139.35595,171.46053 Z M 144.99571,155.82941 C 147.00006,153.82506 147.47167,151.7028 147.47167,144.68758 147.47167,137.67235 147.00006,135.5501 144.99571,133.54575 143.63393,132.18397 141.68411,131.06979 140.66277,131.06979 139.64144,131.06979 137.69162,132.18397 136.32984,133.54575 134.32549,135.5501 133.85388,137.67235 133.85388,144.68758 133.85388,151.7028 134.32549,153.82506 136.32984,155.82941 137.69162,157.19119 139.64144,158.30537 140.66277,158.30537 141.68411,158.30537 143.63393,157.19119 144.99571,155.82941 Z");
    ASSERT_EQ(Inkscape::split_non_intersecting_paths(Geom::PathVector{panda}, fill_oddEven).size(), 5);
    ASSERT_EQ(Inkscape::split_non_intersecting_paths(Geom::PathVector{panda}, fill_nonZero).size(), 5);

    auto const speaker = parse_svgd("M 22.903016,30.264989 V 112.59169 H 120.77829 V 30.264989 Z m 48.17079,11.059578 A 36.093046,30.520484 0 0 1 107.16717,71.844866 36.093046,30.520484 0 0 1 71.073806,102.36517 36.093046,30.520484 0 0 1 34.980876,71.844866 36.093046,30.520484 0 0 1 71.073806,41.324567 Z m 0.04458,21.60374 A 10.187913,10.010936 0 0 0 60.930378,72.939312 10.187913,10.010936 0 0 0 71.11838,82.950314 10.187913,10.010936 0 0 0 81.306382,72.939312 10.187913,10.010936 0 0 0 71.11838,62.928307 Z");
    ASSERT_EQ(Inkscape::split_non_intersecting_paths(Geom::PathVector{speaker}, fill_oddEven).size(), 2);
    ASSERT_EQ(Inkscape::split_non_intersecting_paths(Geom::PathVector{speaker}, fill_nonZero).size(), 2);

    // Case with overlapping paths, so there is no clear correct answer. Less important to preserve this result.
    auto const nested_ellipses = parse_svgd("M 811.8652,-249.32468 A 254.83218,148.75134 0 0 1 557.03302,-100.57333 254.83218,148.75134 0 0 1 302.20084,-249.32468 254.83218,148.75134 0 0 1 557.03302,-398.07602 254.83218,148.75134 0 0 1 811.8652,-249.32468 Z M 692.70743,-179.64305 A 254.83218,148.75134 0 0 1 437.87524,-30.891708 254.83218,148.75134 0 0 1 183.04306,-179.64305 254.83218,148.75134 0 0 1 437.87524,-328.39439 254.83218,148.75134 0 0 1 692.70743,-179.64305 Z M 760.70816,-122.19524 A 393.17526,290.94778 0 0 0 367.5329,-413.14302 393.17526,290.94778 0 0 0 -25.642365,-122.19524 393.17526,290.94778 0 0 0 367.5329,168.75255 393.17526,290.94778 0 0 0 760.70816,-122.19524 Z M 944.29803,-160.14374 A 619.93835,508.60034 0 0 0 324.35968,-668.74408 619.93835,508.60034 0 0 0 -295.57867,-160.14374 619.93835,508.60034 0 0 0 324.35968,348.4566 619.93835,508.60034 0 0 0 944.29803,-160.14374 Z");
    ASSERT_EQ(Inkscape::split_non_intersecting_paths(Geom::PathVector{nested_ellipses}, fill_nonZero).size(), 1);
    ASSERT_EQ(Inkscape::split_non_intersecting_paths(Geom::PathVector{nested_ellipses}, fill_oddEven).size(), 2);
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

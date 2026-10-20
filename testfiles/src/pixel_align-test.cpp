// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Unit tests for pixel-alignment functions.
 */

#include <gtest/gtest.h>

#include "test-utils.h"
#include "ui/pixel-alignment.h"

struct PosAlignData : traced_data
{
    double const x;
    int physical_thickess;
    int scale_factor;
    double const expected;
};
void PrintTo(PosAlignData const &obj, std::ostream *oo)
{
    *oo << "{x:" << obj.x << " line:" << obj.physical_thickess << " scale:" << obj.scale_factor << "}";
}
class TestAlignPixelsLine : public testing::TestWithParam<PosAlignData>
{
};

TEST_P(TestAlignPixelsLine, position_result)
{
    PosAlignData test = GetParam();
    auto scope = test.enable_scope();
    EXPECT_DOUBLE_EQ(Inkscape::pixel_align_line(test.x, test.physical_thickess, test.scale_factor), test.expected);
    // changing physical thickness by 2 should have no effect
    EXPECT_DOUBLE_EQ(Inkscape::pixel_align_line(test.x, test.physical_thickess + 2.0, test.scale_factor),
                     test.expected);
    // scaling factor increase
    EXPECT_DOUBLE_EQ(Inkscape::pixel_align_line(test.x * 0.5, test.physical_thickess, test.scale_factor * 2),
                     test.expected * 0.5);
    EXPECT_DOUBLE_EQ(Inkscape::pixel_align_line(test.x / 3.0, test.physical_thickess, test.scale_factor * 3),
                     test.expected / 3.0);
    EXPECT_DOUBLE_EQ(Inkscape::pixel_align_line(test.x / 4.0, test.physical_thickess, test.scale_factor * 4),
                     test.expected / 4.0);
}

INSTANTIATE_TEST_SUITE_P(TestAlignPixelsLineTests, TestAlignPixelsLine, testing::Values(
    // basic cases on all sides of 0
    _P(PosAlignData, 0.44, 1, 1, 0.5),
    _P(PosAlignData, 0.1, 1, 1, 0.5),
    _P(PosAlignData, 1.44, 1, 1, 1.5),
    _P(PosAlignData, 1.1, 1, 1, 1.5),
    _P(PosAlignData, -1 + 0.44, 1, 1, -0.5),
    _P(PosAlignData, -1 + 0.1, 1, 1, -0.5),
    // even size
    _P(PosAlignData, 0.44, 2, 1, 0.0),
    _P(PosAlignData, 0.1, 2, 1, 0.0),
    _P(PosAlignData, 1.44, 2, 1, 1.0),
    _P(PosAlignData, 1.1, 2, 1, 1.0),
    _P(PosAlignData, -1 + 0.44, 2, 1, -1),
    _P(PosAlignData, -1 + 0.1, 2, 1, -1),
    // near 0 odd size
    _P(PosAlignData, 0.00001, 1, 1, 0.5),
    _P(PosAlignData, -0.00001, 1, 1, 0.5),
    _P(PosAlignData, 1.00001, 1, 1, 1.5),
    _P(PosAlignData, 1 - 0.00001, 1, 1, 1.5),
    _P(PosAlignData, -2.00001, 1, 1, -1.5),
    _P(PosAlignData, -2 - 0.00001, 1, 1, -1.5),
    // even size in 0.5-1 range
    _P(PosAlignData, 0.5, 2, 1, 1),
    _P(PosAlignData, -0.5, 2, 1, 0),
    _P(PosAlignData, 0.8, 2, 1, 1),
    _P(PosAlignData, 1.8, 2, 1, 2),
    _P(PosAlignData, -1 + 0.8, 2, 1, 0),
    _P(PosAlignData, -2 + 0.8, 2, 1, -1),
    // final test
    _P(PosAlignData, 0, 0, 1, 0.0)
));

TEST(TestAlignPixelsLine, range_05_1)
{
    // The choice of rounding direction in the [0.5, 1) somewhat subjective, but it should be consistent.
    double p1, p2;
    p1 = Inkscape::pixel_align_line(0.75, 1, 1);
    p2 = Inkscape::pixel_align_line(1.75, 1, 1);
    EXPECT_DOUBLE_EQ(p2 - p1, 1.0);
    EXPECT_LE(p1, Inkscape::pixel_align_line(1, 1, 1));
    EXPECT_GE(p1, Inkscape::pixel_align_line(0.5, 1, 1));
    EXPECT_GE(p1, Inkscape::pixel_align_line(0.25, 1, 1));
    for (int i = -3; i <= 3; i++) {
        double pos = 0.75 + double(i);
        p2 = Inkscape::pixel_align_line(pos, 1, 1);
        EXPECT_DOUBLE_EQ(p2 - p1, pos - 0.75);
    }
}

TEST(TestAlignPixels, RectInsideOutside)
{
    Geom::Interval range(1, 10);
    auto r1 = Inkscape::pixel_align(range, Inkscape::RectLineAlignment::CenterInside, 1, 1);
    auto r2 = Inkscape::pixel_align(range, Inkscape::RectLineAlignment::CenterOutside, 1, 1);
    EXPECT_DOUBLE_EQ(r2.max() - r1.max(), 1);
    EXPECT_DOUBLE_EQ(r1.min() - r2.min(), 1);
    EXPECT_DOUBLE_EQ(1.5, r1.min());
    r1 = Inkscape::pixel_align(range, Inkscape::RectLineAlignment::Inside, 1, 1);
    r2 = Inkscape::pixel_align(range, Inkscape::RectLineAlignment::Outside, 1, 1);
    EXPECT_DOUBLE_EQ(r2.max() - r1.max(), 1);
    EXPECT_DOUBLE_EQ(r1.min() - r2.min(), 1);
    EXPECT_DOUBLE_EQ(1.5, r1.min());

    r1 = Inkscape::pixel_align(range, Inkscape::RectLineAlignment::CenterInside, 2, 1);
    r2 = Inkscape::pixel_align(range, Inkscape::RectLineAlignment::CenterOutside, 2, 1);
    EXPECT_DOUBLE_EQ(r2.max() - r1.max(), 0);
    EXPECT_DOUBLE_EQ(r1.min() - r2.min(), 0);
    EXPECT_DOUBLE_EQ(1, r1.min());
    r1 = Inkscape::pixel_align(range, Inkscape::RectLineAlignment::Inside, 2, 1);
    r2 = Inkscape::pixel_align(range, Inkscape::RectLineAlignment::Outside, 2, 1);
    EXPECT_DOUBLE_EQ(r2.max() - r1.max(), 2);
    EXPECT_DOUBLE_EQ(r1.min() - r2.min(), 2);
    EXPECT_DOUBLE_EQ(2, r1.min());
    EXPECT_DOUBLE_EQ(9, r1.max());

    r1 = Inkscape::pixel_align(range, Inkscape::RectLineAlignment::CenterInside, 1, 2);
    r2 = Inkscape::pixel_align(range, Inkscape::RectLineAlignment::CenterOutside, 1, 2);
    EXPECT_DOUBLE_EQ(r2.max() - r1.max(), 0.5);
    EXPECT_DOUBLE_EQ(r1.min() - r2.min(), 0.5);
    EXPECT_DOUBLE_EQ(1.25, r1.min());
    r1 = Inkscape::pixel_align(range, Inkscape::RectLineAlignment::Inside, 1, 2);
    r2 = Inkscape::pixel_align(range, Inkscape::RectLineAlignment::Outside, 1, 2);
    EXPECT_DOUBLE_EQ(r2.max() - r1.max(), 0.5);
    EXPECT_DOUBLE_EQ(r1.min() - r2.min(), 0.5);
    EXPECT_DOUBLE_EQ(1.25, r1.min());
}

TEST(TestAlignPixels, RectNone)
{
    Geom::Interval range(1.23, 10.31);
    auto r1 = Inkscape::pixel_align(range, Inkscape::RectLineAlignment::None, 1, 1);
    EXPECT_DOUBLE_EQ(range.min(), r1.min());
    EXPECT_DOUBLE_EQ(range.max(), r1.max());
}
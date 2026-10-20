// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Test Inkscape::SVG::PathString
 */
/*
 * Authors:
 *   Kārlis Seņko
 *
 * Copyright (C) 2026 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "2geom/path-sink.h"
#include "2geom/svg-path-parser.h"
#include "svg/path-string.h"
#include "svg/svg.h"
#include "test-utils.h"

using namespace Inkscape::SVG;

struct PathFormatCase : traced_data
{
    std::string const expected;
    Inkscape::SVG::PATHSTRING_FORMAT const format;
    int const precision;
    int const minexp;
    bool const force_repeat = false;
};
void PrintTo(PathFormatCase const &obj, std::ostream *oo)
{
    *oo << " f: " << obj.format << " " << obj.precision << " " << obj.minexp << " " << obj.force_repeat;
}

class PathStringBasicTest : public testing::TestWithParam<PathFormatCase>
{
protected:
    // dummy defaults, avoid default constructor which reads options from preferences
    Inkscape::SVG::PathString formatter{Inkscape::SVG::PATHSTRING_ABSOLUTE, 1, 1, true};

    void SetUp() override
    {
        auto const &arg = GetParam();
        formatter = Inkscape::SVG::PathString(arg.format, arg.precision, arg.minexp, arg.force_repeat);
    }
};

class FormatterSink : public Geom::PathSink
{
    PathString &out;

public:
    FormatterSink(PathString &out)
        : out(out)
    {}
    void arcTo(Geom::Coord rx, Geom::Coord ry, Geom::Coord angle, bool large_arc, bool sweep,
               Geom::Point const &p) override
    {
        out.arcTo(rx, ry, angle, large_arc, sweep, p);
    }
    void closePath() override { out.closePath(); }
    void curveTo(Geom::Point const &c0, Geom::Point const &c1, Geom::Point const &p) override
    {
        out.curveTo(c0, c1, p);
    }
    void lineTo(Geom::Point const &p) override { out.lineTo(p); }
    void moveTo(Geom::Point const &p) override { out.moveTo(p); }
    void quadTo(Geom::Point const &c, Geom::Point const &p) override { out.quadTo(c, p); }
    void flush() override {}
};

class PathStringBasicMove : public PathStringBasicTest
{
};
TEST_P(PathStringBasicMove, PathStringBasicMove)
{
    auto &param = GetParam();
    auto scope = param.enable_scope();
    formatter.moveTo(Geom::Point(12, 13));
    formatter.moveTo(Geom::Point(15.1, 18.7));
    EXPECT_EQ(static_cast<std::string>(formatter), GetParam().expected);
}
INSTANTIATE_TEST_SUITE_P(
    PathStringBasicMove, PathStringBasicMove,
    testing::Values(_P(PathFormatCase, "M 12,13 M 15.1,18.7", Inkscape::SVG::PATHSTRING_ABSOLUTE, 4, -8, true),
                    _P(PathFormatCase, "m 12,13 m 3.1,5.7", Inkscape::SVG::PATHSTRING_RELATIVE, 4, -8, true),
                    _P(PathFormatCase, "m 12,13 m 3.1,5.7", Inkscape::SVG::PATHSTRING_OPTIMIZE, 4, -8, true),
                    // M M doesn't get merged see explanation in line test
                    _P(PathFormatCase, "M 12,13 M 15.1,18.7", Inkscape::SVG::PATHSTRING_ABSOLUTE, 4, -8, false),
                    _P(PathFormatCase, "m 12,13 m 3.1,5.7", Inkscape::SVG::PATHSTRING_RELATIVE, 4, -8, false),
                    _P(PathFormatCase, "m 12,13 m 3.1,5.7", Inkscape::SVG::PATHSTRING_OPTIMIZE, 4, -8, false)));

class PathStringBasicLine : public PathStringBasicTest
{
};
TEST_P(PathStringBasicLine, PathStringBasicLine)
{
    auto &param = GetParam();
    auto scope = param.enable_scope();
    formatter.moveTo(Geom::Point(12, 13));
    formatter.lineTo(Geom::Point(15, 18));
    formatter.lineTo(Geom::Point(16, 19));

    formatter.verticalLineTo(21);
    formatter.verticalLineTo(25);

    formatter.horizontalLineTo(26);
    formatter.horizontalLineTo(29);
    EXPECT_EQ(static_cast<std::string>(formatter), GetParam().expected);
}
INSTANTIATE_TEST_SUITE_P(
    PathStringBasicLine, PathStringBasicLine,
    testing::Values(_P(PathFormatCase, "M 12,13 L 15,18 L 16,19 V 21 V 25 H 26 H 29", PATHSTRING_ABSOLUTE, 2, -8, true),
                    _P(PathFormatCase, "m 12,13 l 3,5 l 1,1 v 2 v 4 h 10 h 3", PATHSTRING_RELATIVE, 2, -8, true),
                    _P(PathFormatCase, "m 12,13 l 3,5 l 1,1 v 2 v 4 h 10 h 3", PATHSTRING_OPTIMIZE, 2, -8, true),
                    // M x1, y1 x2,y2 is a shorthand for M x1,y1 L x2,y2 not M x1, y1, M x2, y2
                    // similar for relative
                    _P(PathFormatCase, "M 12,13 15,18 16,19 V 21 25 H 26 29", PATHSTRING_ABSOLUTE, 2, -8, false),
                    _P(PathFormatCase, "m 12,13 3,5 1,1 v 2 4 h 10 3", PATHSTRING_RELATIVE, 2, -8, false),
                    _P(PathFormatCase, "m 12,13 3,5 1,1 v 2 4 h 10 3", PATHSTRING_OPTIMIZE, 2, -8, false)));

// Absolute movement followed by relative
TEST(SvgPathString, HVAbsRel)
{
    PathString formatter(PATHSTRING_OPTIMIZE, 8, -8, false);
    Geom::Coord positions[] = {10.5, 21, 31.5, 42, 43, 44, 45};
    for (auto p : positions) {
        formatter.horizontalLineTo(p);
    }
    for (auto p : positions) {
        formatter.verticalLineTo(p);
    }
    auto res = static_cast<std::string>(formatter);
    EXPECT_EQ(res, "H 10.5 21 31.5 42 h 1 1 1 V 10.5 21 31.5 42 v 1 1 1");
}

class PathStringBasicCurve : public PathStringBasicTest
{
};
TEST_P(PathStringBasicCurve, PathStringBasicCurve)
{
    auto &param = GetParam();
    auto scope = param.enable_scope();
    formatter.moveTo(Geom::Point(12, 13));
    formatter.quadTo(14, 16, 16, 19);
    formatter.quadTo({18, 22}, {22, 27});

    formatter.curveTo(23, 29, 25, 31, 25, 34);
    formatter.curveTo({33, 39}, {35, 31}, {35, 44});
    EXPECT_EQ(static_cast<std::string>(formatter), GetParam().expected);
}
INSTANTIATE_TEST_SUITE_P(
    PathStringBasicCurve, PathStringBasicCurve,
    testing::Values(_P(PathFormatCase, "M 12,13 Q 14,16 16,19 Q 18,22 22,27 C 23,29 25,31 25,34 C 33,39 35,31 35,44",
                       PATHSTRING_ABSOLUTE, 2, -8, true),
                    _P(PathFormatCase, "m 12,13 q 2,3 4,6 q 2,3 6,8 c 1,2 3,4 3,7 c 8,5 10,-3 10,10",
                       PATHSTRING_RELATIVE, 2, -8, true),
                    _P(PathFormatCase, "m 12,13 q 2,3 4,6 q 2,3 6,8 c 1,2 3,4 3,7 c 8,5 10,-3 10,10",
                       PATHSTRING_OPTIMIZE, 2, -8, true),
                    _P(PathFormatCase, "M 12,13 Q 14,16 16,19 18,22 22,27 C 23,29 25,31 25,34 33,39 35,31 35,44",
                       PATHSTRING_ABSOLUTE, 2, -8, false),
                    _P(PathFormatCase, "m 12,13 q 2,3 4,6 2,3 6,8 c 1,2 3,4 3,7 8,5 10,-3 10,10", PATHSTRING_RELATIVE,
                       2, -8, false),
                    _P(PathFormatCase, "m 12,13 q 2,3 4,6 2,3 6,8 c 1,2 3,4 3,7 8,5 10,-3 10,10", PATHSTRING_OPTIMIZE,
                       2, -8, false)));

class PathStringBasicCircle : public PathStringBasicTest
{
};
TEST_P(PathStringBasicCircle, PathStringBasicCircle)
{
    auto &param = GetParam();
    auto scope = param.enable_scope();
    formatter.moveTo(Geom::Point(10, 10));
    formatter.arcTo(6, 5, 0, false, true, {16, 15});
    formatter.arcTo(6, 5, 0, true, false, {10, 10});
    formatter.closePath();
    EXPECT_EQ(static_cast<std::string>(formatter), GetParam().expected);
}
INSTANTIATE_TEST_SUITE_P(
    PathStringBasicCircle, PathStringBasicCircle,
    testing::Values(
        _P(PathFormatCase, "M 10,10 A 6,5 0 0 1 16,15 A 6,5 0 1 0 10,10 Z", PATHSTRING_ABSOLUTE, 2, -8, true),
        _P(PathFormatCase, "m 10,10 a 6,5 0 0 1 6,5 a 6,5 0 1 0 -6,-5 z", PATHSTRING_RELATIVE, 2, -8, true),
        _P(PathFormatCase, "m 10,10 a 6,5 0 0 1 6,5 a 6,5 0 1 0 -6,-5 z", PATHSTRING_OPTIMIZE, 2, -8, true),
        _P(PathFormatCase, "M 10,10 A 6,5 0 0 1 16,15 6,5 0 1 0 10,10 Z", PATHSTRING_ABSOLUTE, 2, -8, false),
        _P(PathFormatCase, "m 10,10 a 6,5 0 0 1 6,5 6,5 0 1 0 -6,-5 z", PATHSTRING_RELATIVE, 2, -8, false),
        _P(PathFormatCase, "m 10,10 a 6,5 0 0 1 6,5 6,5 0 1 0 -6,-5 z", PATHSTRING_OPTIMIZE, 2, -8, false)));

class PathStringMixedCase : public PathStringBasicTest
{
};
// test case where best choice is mixing absolute and relative commands
TEST_P(PathStringMixedCase, PathStringMixedCase)
{
    auto &param = GetParam();
    auto scope = param.enable_scope();
    FormatterSink sink(formatter);
    Geom::SVGPathParser parser(sink);
    parser.parse("M 100,100 L 101,101 102,102, 103,103 104,104"
                 " 1,1 2,2 1,1 2,2"
                 " 10,10 11,11 12,12"
                 " 105,105 206.5,206.5 307,307 408.5,408.5");
    EXPECT_EQ(static_cast<std::string>(formatter), GetParam().expected);
}
INSTANTIATE_TEST_SUITE_P(
    PathStringMixedCase, PathStringMixedCase,
    testing::Values(
        _P(PathFormatCase,
           "M 100,100 L 101,101 L 102,102 L 103,103 L 104,104 L 1,1 L 2,2 L 1,1 L 2,2 L 10,10 L 11,11 L 12,12 L "
           "105,105 L 206.5,206.5 L 307,307 L 408.5,408.5",
           PATHSTRING_ABSOLUTE, 5, -8, true),
        _P(PathFormatCase,
           "m 100,100 l 1,1 l 1,1 l 1,1 l 1,1 l -103,-103 l 1,1 l -1,-1 l 1,1 l 8,8 l 1,1 l 1,1 l 93,93 l 101.5,101.5 "
           "l 100.5,100.5 l 101.5,101.5",
           PATHSTRING_RELATIVE, 5, -8, true),
        _P(PathFormatCase,
           "m 100,100 l 1,1 l 1,1 l 1,1 l 1,1 L 1,1 L 2,2 L 1,1 l 1,1 l 8,8 l 1,1 l 1,1 l 93,93 L 206.5,206.5 L "
           "307,307 l 101.5,101.5",
           PATHSTRING_OPTIMIZE, 5, -8, true),
        _P(PathFormatCase,
           "M 100,100 101,101 102,102 103,103 104,104 1,1 2,2 1,1 2,2 10,10 11,11 12,12 105,105 206.5,206.5 307,307 "
           "408.5,408.5",
           PATHSTRING_ABSOLUTE, 5, -8, false),
        _P(PathFormatCase,
           "m 100,100 1,1 1,1 1,1 1,1 -103,-103 1,1 -1,-1 1,1 8,8 1,1 1,1 93,93 101.5,101.5 100.5,100.5 101.5,101.5",
           PATHSTRING_RELATIVE, 5, -8, false),
        _P(PathFormatCase,
           "m 100,100 1,1 1,1 1,1 1,1 L 1,1 2,2 1,1 l 1,1 8,8 1,1 1,1 93,93 L 206.5,206.5 307,307 408.5,408.5",
           PATHSTRING_OPTIMIZE, 5, -8, false)));

TEST(SvgPathString, testMinexpPrecision)
{
    Geom::PathVector pv;
    pv = sp_svg_read_pathv(
        "M 123456781,1.23456781e-8 L 123456782,1.23456782e-8 L 123456785,1.23456785e-8 L 10123456400,1.23456785e-8 L "
        "123456789,1.23456789e-8 L 123456789,101.234564e-8 L 123456789,1.23456789e-8");
    PathString formatter(PATHSTRING_OPTIMIZE, 8, -8, false);
    FormatterSink sink(formatter);
    sink.feed(pv);
    auto res = static_cast<std::string>(formatter);
    EXPECT_EQ(res, "m 123456780,1.2345678e-8 0,0 10,1e-15 9999999210,0 -9999999210,0 0,9.99999921e-7 0,-9.99999921e-7");
}

// Test to check that adding up large amount of relative movements, doesn't cause error accumulation.
TEST(SvgPathString, relativeMoveErrorAccumulation)
{
    int const STEPS = 80000;
    double const ANGLE = M_PI / 2;
    double const R = 100;
    std::vector<Geom::Point> points;

    for (auto i = 0; i < STEPS; i++) {
        auto angle = ANGLE * i / STEPS;
        points.push_back({std::sin(angle) * R, std::cos(angle) * R});
    }

    PathString formatter(PATHSTRING_RELATIVE, 8, -8, false);
    formatter.moveTo(0, 0);
    for (auto p : points) {
        formatter.lineTo(p);
    }
    auto res = static_cast<std::string>(formatter);
    Geom::PathVector pv = sp_svg_read_pathv(res.c_str());

    auto spacing = (points[1] - points[0]).length();
    auto err = (points.back() - pv.finalPoint()).length();
    // std::cerr << "spacing: " << spacing << " error:" << err << std::endl;

    EXPECT_LE(err, spacing);
}

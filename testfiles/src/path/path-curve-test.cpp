// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Curve test
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2020 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include <gtest/gtest.h>

#include "path/path-curve.h"
#include <2geom/curves.h>
#include <2geom/path.h>
#include <2geom/pathvector.h>

class CurveTest : public ::testing::Test {
  public:
    Geom::Path path1;
    Geom::Path path2;
    Geom::Path path3;
    Geom::Path path4;

  protected:
    CurveTest()
        : path4(Geom::Point(3, 5)) // Just a moveto
    {
        // Closed path
        path1.append(Geom::LineSegment(Geom::Point(0, 0), Geom::Point(1, 0)));
        path1.append(Geom::LineSegment(Geom::Point(1, 0), Geom::Point(1, 1)));
        path1.close();
        // Closed path (ClosingSegment is zero length)
        path2.append(Geom::LineSegment(Geom::Point(2, 0), Geom::Point(3, 0)));
        path2.append(Geom::CubicBezier(Geom::Point(3, 0), Geom::Point(2, 1), Geom::Point(1, 1), Geom::Point(2, 0)));
        path2.close();
        // Open path
        path3.setStitching(true);
        path3.append(Geom::EllipticalArc(Geom::Point(4, 0), 1, 2, M_PI, false, false, Geom::Point(5, 1)));
        path3.append(Geom::LineSegment(Geom::Point(5, 1), Geom::Point(5, 2)));
        path3.append(Geom::LineSegment(Geom::Point(6, 4), Geom::Point(2, 4)));
    }
};

TEST_F(CurveTest, testCurveCount)
{
    { // Zero segments
        Geom::PathVector pv;
        ASSERT_EQ(pv.curveCount(), 0u);
    }
    { // Zero segments
        Geom::PathVector pv;
        pv.push_back(Geom::Path());
        ASSERT_EQ(pv.curveCount(), 0u);
    }
    { // Individual paths
        Geom::PathVector pv((Geom::Path()));
        pv[0] = path1;
        ASSERT_EQ(pv.curveCount(), 3u);
        pv[0] = path2;
        ASSERT_EQ(pv.curveCount(), 2u);
        pv[0] = path3;
        ASSERT_EQ(pv.curveCount(), 4u);
        pv[0] = path4;
        ASSERT_EQ(pv.curveCount(), 0u);
        pv[0].close();
        ASSERT_EQ(pv.curveCount(), 0u);
    }
    { // Combination
        Geom::PathVector pv;
        pv.push_back(path1);
        pv.push_back(path2);
        pv.push_back(path3);
        pv.push_back(path4);
        ASSERT_EQ(pv.curveCount(), 9u);
    }
}

TEST_F(CurveTest, testNodesInPathForZeroSegments)
{
    { // Zero segments
        Geom::PathVector pv;
        ASSERT_EQ(node_count(pv), 0u);
    }
    { // Zero segments
        Geom::PathVector pv;
        pv.push_back(Geom::Path());
        ASSERT_EQ(node_count(pv), 1u);
    }
}

TEST_F(CurveTest, testNodesInPathForIndividualPaths)
{
    Geom::PathVector pv((Geom::Path()));
    pv[0] = path1;
    ASSERT_EQ(node_count(pv), 3u);
    pv[0] = path2;
    ASSERT_EQ(node_count(pv), 2u); // zero length closing segments do not increase the nodecount.
    pv[0] = path3;
    ASSERT_EQ(node_count(pv), 5u);
    pv[0] = path4;
    ASSERT_EQ(node_count(pv), 1u);
}

TEST_F(CurveTest, testNodesInPathForNakedMoveToClosedPath)
{
    Geom::PathVector pv((Geom::Path()));
    pv[0] = path4; // just a MoveTo
    pv[0].close();
    ASSERT_EQ(node_count(pv), 1u);
}

TEST_F(CurveTest, testIsEmpty)
{
    ASSERT_TRUE(Geom::PathVector().empty());
    ASSERT_FALSE(Geom::PathVector{path1}.empty());
    ASSERT_FALSE(Geom::PathVector{path2}.empty());
    ASSERT_FALSE(Geom::PathVector{path3}.empty());
    ASSERT_FALSE(Geom::PathVector{path4}.empty());
}

TEST_F(CurveTest, testIsClosed)
{
    ASSERT_FALSE(is_closed(Geom::PathVector()));
    Geom::PathVector pv((Geom::Path()));
    ASSERT_FALSE(is_closed(pv));
    pv[0].close();
    ASSERT_TRUE(is_closed(pv));
    ASSERT_TRUE(is_closed(path1));
    ASSERT_TRUE(is_closed(path2));
    ASSERT_FALSE(is_closed(path3));
    ASSERT_FALSE(is_closed(path4));
}

TEST_F(CurveTest, testFirstPoint)
{
    ASSERT_EQ(path1.initialPoint(), Geom::Point(0, 0));
    ASSERT_EQ(path2.initialPoint(), Geom::Point(2, 0));
    ASSERT_EQ(path3.initialPoint(), Geom::Point(4, 0));
    ASSERT_EQ(path4.initialPoint(), Geom::Point(3, 5));
    Geom::PathVector pv;
    pv.push_back(path1);
    pv.push_back(path2);
    pv.push_back(path3);
    ASSERT_EQ(pv.initialPoint(), Geom::Point(0, 0));
    pv.insert(pv.begin(), path4);
    ASSERT_EQ(pv.initialPoint(), Geom::Point(3, 5));
}

TEST_F(CurveTest, PathFromCurve)
{
    auto const curve = Geom::LineSegment{Geom::Point{}, Geom::Point{1, 0}};
    auto const path = path_from_curve(curve);
    ASSERT_EQ(path.size(), 1);
    ASSERT_FALSE(path.closed());
    ASSERT_EQ(path.initialCurve(), curve);
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

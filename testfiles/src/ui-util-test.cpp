// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Test utilities from src/ui/
 */
/*
 * Authors:
 *   Martin Owens
 *
 * Copyright (C) 2024 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtest/gtest.h>
#include "test-utils.h"

#include "ui/widget/canvas/util.h"

using namespace Inkscape::UI::Widget;

::testing::AssertionResult Array3IsNear(std::array<float, 3> const &A, std::vector<double> const &B, double epsilon)
{
    std::vector<double> av;
    for (auto v : A) {
        av.emplace_back((double)v);
    }
    return VectorIsNear(av, B, epsilon);
}

TEST(UtilTest, CheckerboardDarken)
{
    EXPECT_TRUE(Array3IsNear(checkerboard_darken(0x00000000), {0.08, 0.08, 0.08}, 0.01));
    EXPECT_TRUE(Array3IsNear(checkerboard_darken(0x00000080), {0.0398, 0.0398, 0.0398}, 0.01));
    EXPECT_TRUE(Array3IsNear(checkerboard_darken(0x000000ff), {0, 0, 0}, 0.01));
    EXPECT_TRUE(Array3IsNear(checkerboard_darken(0x00000080), {0.0398, 0.0398, 0.0398}, 0.01));
    EXPECT_TRUE(Array3IsNear(checkerboard_darken(0xffffff00), {0.92, 0.92, 0.92}, 0.01));
    EXPECT_TRUE(Array3IsNear(checkerboard_darken(0xffffffff), {1, 1, 1}, 0.01));
    EXPECT_TRUE(Array3IsNear(checkerboard_darken(0x80808000), {0.422, 0.422, 0.422}, 0.01));
    EXPECT_TRUE(Array3IsNear(checkerboard_darken(0x80808080), {0.462, 0.462, 0.462}, 0.01));
    EXPECT_TRUE(Array3IsNear(checkerboard_darken(0x808080ff), {0.502, 0.502, 0.502}, 0.01));
}

// vim: filetype=cpp:expandtab:shiftwidth=4:softtabstop=4:fileencoding=utf-8:textwidth=99 :

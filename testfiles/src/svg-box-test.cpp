// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Test for SVG box
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2010 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "svg/svg-box.h"
#include "svg/svg.h"

#include <cmath>
#include <glib.h>
#include <gtest/gtest.h>
#include <utility>

struct read_test_t
{
    const std::string str;
    int top;
    int right;
    int bottom;
    int left;
};
struct write_test_t
{
    const std::string in;
    const std::string out;
};

// clang-format off
read_test_t read_tests[5] = {
    {"0", 0, 0, 0, 0},
    {"1", 1, 1, 1, 1},
    {"1 2 3 4", 1, 2, 3, 4},
    {"1,2,3,4", 1, 2, 3, 4},
    {"2cm 4cm", 76, 151, 76, 151},
};
const char* fail_tests[4] = {
    "",
    "a b c d",
    "12miles",
    "14mmm",
};
write_test_t write_tests[7] = {
    {"0", "0"},
    {"1", "1"},
    {"1 1 1 1", "1"},
    {"1cm", "37.79527559"},
    {"4cm 2in", "151.1811024 192"},
    {"7 2 4cm", "7 2 151.1811024"},
    {"1,2,3", "1 2 3"},
};
read_test_t set_tests[3] = {
    {"1", 1, 1, 1, 1},
    {"1 2", 1, 2, 1, 2},
    {"1 2 3 4", 1, 2, 3, 4},
};
// clang-format on

TEST(SvgBoxTest, testRead)
{
    for (size_t i = 0; i < G_N_ELEMENTS(read_tests); i++) {
        SVGBox box;
        ASSERT_TRUE(box.read(read_tests[i].str, Geom::Scale(1))) << read_tests[i].str;
        ASSERT_EQ(round(box.top().computed), read_tests[i].top) << read_tests[i].str;
        ASSERT_EQ(round(box.right().computed), read_tests[i].right) << read_tests[i].str;
        ASSERT_EQ(round(box.bottom().computed), read_tests[i].bottom) << read_tests[i].str;
        ASSERT_EQ(round(box.left().computed), read_tests[i].left) << read_tests[i].str;
    }
}

TEST(SvgBoxTest, testFailures)
{
    for (size_t i = 0; i < G_N_ELEMENTS(fail_tests); i++) {
        SVGLength box;
        ASSERT_TRUE( !box.read(fail_tests[i])) << fail_tests[i];
    }
}

TEST(SvgBoxTest, testWrite)
{
    for (size_t i = 0; i < G_N_ELEMENTS(write_tests); i++) {
        SVGBox box;
        ASSERT_TRUE(box.read(write_tests[i].in, Geom::Scale(1))) << write_tests[i].in;
        ASSERT_EQ(box.write(), write_tests[i].out) << write_tests[i].in;
    }
}

TEST(SvgBoxTest, testSet)
{
    for (auto t : set_tests) {
        SVGBox box;
        box.set(t.top, t.right, t.bottom, t.left);
        ASSERT_EQ(box.write(), t.str);
    }
}

TEST(SvgBoxTest, testToFromString)
{
    SVGBox box;
    ASSERT_TRUE(box.fromString("10mm 5", "mm", Geom::Scale(5)));
    ASSERT_EQ(box.toString("mm", Geom::Scale(5)), "10mm 5mm");
    // This result is mm to px (internal conversion) plus scale test.
    ASSERT_EQ(box.write(), "7.559055118 3.779527559");
}

TEST(SvgBoxTest, testConfine)
{
    SVGBox box;
    box.set(10, 20, 10, 20);
    ASSERT_EQ(box.write(), "10 20");
    box.set(BOX_TOP, 5, true);
    ASSERT_EQ(box.write(), "5 20");
    box.set(BOX_LEFT, 10, true);
    ASSERT_EQ(box.write(), "5 10");
    box.set(BOX_LEFT, 5, true);
    ASSERT_EQ(box.write(), "5");
    box.set(BOX_BOTTOM, 7, true);
    ASSERT_EQ(box.write(), "7");
}

// vim: filetype=cpp:expandtab:shiftwidth=4:softtabstop=4:fileencoding=utf-8:textwidth=99 :

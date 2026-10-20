// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file Test the SVG path parser util.
 */
/*
 * Authors:
 *   Vaibhav Malik <vaibhavmalik2018@gmail.com>
 *
 * Copyright (C) 2026 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "util/svg-path-parser.h"

#include <unordered_map>
#include <gtest/gtest.h>

class SvgPathParserTest : public ::testing::Test
{
public:
    std::unordered_map<std::string, Glib::ustring> const svg_path_map = {
        {"M 1,2 L 4,2 L 4,8 L 1,8 z", "M 1 2\nL 4 2\nL 4 8\nL 1 8\nz"},
        {"M 10,10L90 90V 10 H50", "M 10 10\nL 90 90\nV 10\nH 50"},
        {"M110,10l80 80v-80h-40", "M 110 10\nl 80 80\nv -80\nh -40"},
        {"M110,90 c20, 0 15, -80 40 ,-80s 20 80 40 80", "M 110 90\nc 20 0 15 -80 40 -80\ns 20 80 40 80"},
        {"M10,50Q25,25 40,50t30,0 30,0 30,0 30,0 30,0",
         "M 10 50\nQ 25 25 40 50\nt 30 0\n  30 0\n  30 0\n  30 0\n  30 0"},
        {"M 15.7,1.7l -0.47,8 -2.8,-10 -4.45 -8", "M 15.7 1.7\nl -0.47 8\n  -2.8 -10\n  -4.45 -8"},
        {"M 157e-1,17e-1l -047e-2,8 -2.8,-10e0 -4.45 -8e+1",
         "M 157e-1 17e-1\nl -047e-2 8\n  -2.8 -10e0\n  -4.45 -8e+1"},
        {"M10,30A20,20,0,0,1,50,30,20,20,0,0,1,90,30Q90,60,50,90,10,60,10,30z",
         "M 10 30\nA 20 20 0 0 1 50 30\n  20 20 0 0 1 90 30\nQ 90 60 50 90\n  10 60 10 30\nz"},
        {"M72,229.4 173.4,248.3,274.2,232.1,253.7,268.9, 268.8 308.1", "M 72 229.4\n  173.4 248.3\n  274.2 232.1\n  253.7 268.9\n  268.8 308.1"},
        {"M10,20 50,60 70 80 90 100 110 120 120 130 140 150 150 160\nC 100, 100, 250 100 250,200\nz",
         "M 10 20\n  50 60\n  70 80\n  90 100\n  110 120\n  120 130\n  140 150\n  150 160\nC 100 100 250 100 250 "
         "200\nz"},
        {"M600,350 l 50,-25a25,25 -30 0,1 50,-25 l 50,-25a25,50 -30 0,1 50,-25 l 50,-25a25,75 -30 0,1 50,-25 l "
         "50,-25a25,100 -30 0,1 50,-25 l 50,-25",
         "M 600 350\nl 50 -25\na 25 25 -30 0 1 50 -25\nl 50 -25\na 25 50 -30 0 1 50 -25\nl 50 -25\na 25 75 -30 0 1 50 "
         "-25\nl 50 -25\na 25 100 -30 0 1 50 -25\nl 50 -25"}};
};

TEST_F(SvgPathParserTest, testParseSvgPaths)
{
    for (auto const &[path, expected_prettified_path] : svg_path_map) {
        auto prettified_path = Inkscape::SvgPathParser::prettify_svgd(path);
        ASSERT_EQ(prettified_path, expected_prettified_path);
    }
}

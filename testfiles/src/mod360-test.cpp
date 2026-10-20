// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Tests for the function mod360()
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2016-2025 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "mod360.h"

#include <cmath>
#include <gtest/gtest.h>

namespace {
struct Mod360TestCase
{
    double input;
    double expected_output;
};

constexpr double inf()
{
    return INFINITY;
}
double nan()
{
    return std::nan("");
}

Mod360TestCase const test_cases[] = {{0, 0},     {10, 10},   {360, 0},    {361, 1},   {-1, 359},
                                     {-359, 1},  {-360, -0}, {-361, 359}, {inf(), 0}, {-inf(), 0},
                                     {nan(), 0}, {720, 0},   {-721, 359}, {-1000, 80}};

} // namespace

struct Mod360Test : testing::TestWithParam<Mod360TestCase>
{
};

TEST_P(Mod360Test, BasicMod360Test)
{
    auto const &[input, expected_output] = GetParam();
    EXPECT_DOUBLE_EQ(mod360(input), expected_output);
}

INSTANTIATE_TEST_SUITE_P(Mod360TestWithParams, Mod360Test, ::testing::ValuesIn(test_cases));

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests for in-gamut functionality
 *
 * Copyright (C) 2025 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtest/gtest.h>
#include "../test-utils.h"

// #include "colors/spaces/oklab.h"
#include "colors/spaces/oklch.h"
#include "colors/color.h"
#include "colors/manager.h"
#include "colors/spaces/base.h"

using namespace Inkscape::Colors;

namespace {

using Space::Type::OKLCH;
using Space::Type::OKLAB;
using Space::Type::RGB;

struct inb : traced_data {
    const Space::Type space_in;
    const std::vector<double> in;
    const Space::Type space_out;
    const std::vector<double> out;
};

class inCssGamut : public  testing::TestWithParam<inb> {};

/**
 * Test CSS in-gamut functionality
 *
 * CSS Level 4 gamut mapping: https://www.w3.org/TR/css-color-4/#gamut-mapping
 */
TEST_P(inCssGamut, values) {
    auto& cm = Manager::get();
    auto rgb = cm.find(Space::Type::RGB);
    ASSERT_TRUE(rgb);

    inb test = GetParam();
    auto color = Color(test.space_in, test.in);
    auto result = rgb->toGamut(color);
    auto scope = test.enable_scope();
    EXPECT_TRUE(VectorIsNear(result.getValues(), test.out, 0.001));
}

INSTANTIATE_TEST_SUITE_P(GamutTesting, inCssGamut, testing::Values(
    // red oversaturated and out of gamut in sRGB, but fits in Rec2020
    _P(inb, OKLCH, { 0.70,  0.25/0.40,  20/360.0 }, RGB, { 1.0, 0.332, 0.393 }),
    // dark cyan, desaturated below sRGB, but fits in P3
    _P(inb, OKLCH, { 0.53,  0.10/0.40, 209/360.0 }, RGB, { 0.0, 0.486, 0.553 })
));

} // namespace

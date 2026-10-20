// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests for the OkHsv color space
 *
 * Copyright (C) 2025 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "colors/spaces/okhsv.h"
#include "spaces-testbase.h"

namespace {

using Space::Type::RGB;
using Space::Type::OKHSV;

// clang-format off
// There is no CSS for OkHsv
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(fromString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(badColorString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(toString);

INSTANTIATE_TEST_SUITE_P(ColorsSpacesOkHSV, convertColorSpace, testing::Values(
    // No conversion
    _P(inb, OKHSV, { 1.0, 0.400, 0.200 }, OKHSV, { 1.0, 0.400, 0.200 })
));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesOkHSV, normalize, testing::Values(
    _P(inb, OKHSV, { 0.5,   0.5,   0.5,   0.5  }, OKHSV, { 0.5,   0.5,   0.5,   0.5  }),
    _P(inb, OKHSV, { 1.2,   1.2,   1.2,   1.2  }, OKHSV, { 0.2,   1.0,   1.0,   1.0  }),
    _P(inb, OKHSV, {-0.2,  -0.2,  -0.2,  -0.2  }, OKHSV, { 0.8,   0.0,   0.0,   0.0  }),
    _P(inb, OKHSV, { 0.0,   0.0,   0.0,   0.0  }, OKHSV, { 0.0,   0.0,   0.0,   0.0  }),
    _P(inb, OKHSV, { 1.0,   1.0,   1.0,   1.0  }, OKHSV, { 1.0,   1.0,   1.0,   1.0  })
));
// clang-format on

TEST(ColorsSpacesOkHSV, randomConversion)
{
    // Isolate conversion functions
    // EXPECT_TRUE(RandomPassFunc(Space::OkHsv::toOkLab, Space::OkHsv::toOkLab, 1000));

    // Full stack conversion
    EXPECT_TRUE(RandomPassthrough(OKHSV, RGB, 1000, true));
}

TEST(ColorsSpacesOkHSV, components)
{
    auto c = Manager::get().find(OKHSV)->getComponents();
    ASSERT_EQ(c.size(), 3);
    EXPECT_EQ(c[0].id, "h");
    EXPECT_EQ(c[1].id, "s");
    EXPECT_EQ(c[2].id, "v");
}

} // namespace

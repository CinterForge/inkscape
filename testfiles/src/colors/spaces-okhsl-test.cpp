// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests for the OkHsl color space
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "spaces-testbase.h"

namespace {

using Space::Type::RGB;
using Space::Type::OKHSL;

// clang-format off
// There is no CSS for OkHsl
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(fromString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(badColorString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(toString);

INSTANTIATE_TEST_SUITE_P(ColorsSpacesOkHSL, convertColorSpace, testing::Values(
    // No conversion
    _P(inb, OKHSL, { 1.0, 0.400, 0.200 }, OKHSL, { 1.0, 0.400, 0.200 }),

    // Test conversion from RGB to OkHsl when total chrominance is low.
    _P(inb, RGB, { 1.0, 1.0, 1.0 }, OKHSL, { 0.0, 0.0, 1.0 })
));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesOkHSL, normalize, testing::Values(
    _P(inb, OKHSL, { 0.5,   0.5,   0.5,   0.5  }, OKHSL, { 0.5,   0.5,   0.5,   0.5  }),
    _P(inb, OKHSL, { 1.2,   1.2,   1.2,   1.2  }, OKHSL, { 0.2,   1.0,   1.0,   1.0  }),
    _P(inb, OKHSL, {-0.2,  -0.2,  -0.2,  -0.2  }, OKHSL, { 0.8,   0.0,   0.0,   0.0  }),
    _P(inb, OKHSL, { 0.0,   0.0,   0.0,   0.0  }, OKHSL, { 0.0,   0.0,   0.0,   0.0  }),
    _P(inb, OKHSL, { 1.0,   1.0,   1.0,   1.0  }, OKHSL, { 1.0,   1.0,   1.0,   1.0  })
));
// clang-format on

TEST(ColorsSpacesOkHSL, randomConversion)
{
    // Isolate conversion functions
    // EXPECT_TRUE(RandomPassFunc(Space::OkHsl::toOkLab, Space::OkHsl::toOkLab, 1000));

    // Full stack conversion
    EXPECT_TRUE(RandomPassthrough(OKHSL, RGB, 1000, true));
}

TEST(ColorsSpacesOkHSL, components)
{
    auto c = Manager::get().find(OKHSL)->getComponents();
    ASSERT_EQ(c.size(), 3);
    EXPECT_EQ(c[0].id, "h");
    EXPECT_EQ(c[1].id, "s");
    EXPECT_EQ(c[2].id, "l");
}

} // namespace

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

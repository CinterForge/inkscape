// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests for the LCH color space
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "colors/spaces/oklch.h"
#include "spaces-testbase.h"

namespace {

using Space::Type::OKLCH;
using Space::Type::RGB;

// clang-format off
// Run out of time before the rest of the features could be done
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(fromString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(badColorString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(toString);

/*INSTANTIATE_TEST_SUITE_P(ColorsSpacesOkLCH, fromString, testing::Values(
    _P(in, "oklch(50% 0.1 180)",      { 0.5,  0.133, 0.5        }, 0x557f79ff),
    _P(in, "oklch(100 0.4 360)",      { 1.0,  1.0,   1.0        }, 0x95b4ecff),
    _P(in, "oklch(0 0 0)",            { 0.0,  0.0,   0.0        }, 0x000000ff),
    _P(in, "oklch(20% 0.2 72 / 20%)", { 0.2,  0.133, 0.2,   0.2 }, 0x38300933)
));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesOkLCH, badColorString, testing::Values(
    "oklch", "oklch(", "oklch(100"
));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesOkLCH, toString, testing::Values(
    _P(out, OKLCH, {                           }, "", true),
    _P(out, OKLCH, {                           }, "", false),
    _P(out, OKLCH, { 0.0,   0.667, 0.945       }, "oklch(0 100.05 340.2)"),
    _P(out, OKLCH, { 0.3,   0.8,   0.258       }, "oklch(30 120 92.88)"),
    _P(out, OKLCH, { 1.0,   0.5,   0.004       }, "oklch(100 75 1.44)"),
    _P(out, OKLCH, { 0,     1,     0.2,   0.8  }, "oklch(0 150 72 / 80%)", true),
    _P(out, OKLCH, { 0,     1,     0.2,   0.8  }, "oklch(0 150 72)", false)
));*/

INSTANTIATE_TEST_SUITE_P(ColorsSpacesOkLCH, convertColorSpace, testing::Values(
    // No conversion
    _P(inb, OKLCH, { 1.0, 0.400, 0.200 }, OKLCH, { 1.0, 0.400, 0.200 })
));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesOkLCH, normalize, testing::Values(
    _P(inb, OKLCH, { 0.5,   0.5,   0.5,   0.5  }, OKLCH, { 0.5,   0.5,   0.5,   0.5  }),
    _P(inb, OKLCH, { 1.2,   1.2,   1.2,   1.2  }, OKLCH, { 1.0,   1.0,   0.2,   1.0  }),
    _P(inb, OKLCH, {-0.2,  -0.2,  -0.2,  -0.2  }, OKLCH, { 0.0,   0.0,   0.8,   0.0  }),
    _P(inb, OKLCH, { 0.0,   0.0,   0.0,   0.0  }, OKLCH, { 0.0,   0.0,   0.0,   0.0  }),
    _P(inb, OKLCH, { 1.0,   1.0,   1.0,   1.0  }, OKLCH, { 1.0,   1.0,   1.0,   1.0  })
));
// clang-format on

TEST(ColorsSpacesOkLCH, randomConversion)
{
    // Isolate conversion functions
    // EXPECT_TRUE(RandomPassFunc(Space::OkLch::toOkLab, Space::OkLch::toOkLab, 1000));

    // Full stack conversion
    // EXPECT_TRUE(RandomPassthrough(OKLCH, RGB, 1000));
}

TEST(ColorsSpacesOkLCH, components)
{
    auto c = Manager::get().find(OKLCH)->getComponents();
    ASSERT_EQ(c.size(), 3);
    EXPECT_EQ(c[0].id, "l");
    EXPECT_EQ(c[1].id, "c");
    EXPECT_EQ(c[2].id, "h");
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

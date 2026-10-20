// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests for the Linear RGB color space
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "colors/spaces/linear-rgb.h"
#include "spaces-testbase.h"

namespace {

using Space::Type::linearRGB;
using Space::Type::RGB;

// clang-format off
INSTANTIATE_TEST_SUITE_P(ColorsSpacesLinearRGB, fromString, testing::Values(
    _P(in, "color(srgb-linear 0.1 1 0.5)",     { 0.1,  1.0, 0.5       }, 0x59ffbcff),
    _P(in, "color(srgb-linear 0.03 0 0.12)",   { 0.03, 0.0, 0.12      }, 0x300061ff),
    _P(in, "color(srgb-linear 0 1 0.5 / 0.8)", { 0.0,  1.0, 0.5,  0.8 }, 0x01ffbccc)

));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesLinearRGB, badColorString, testing::Values(
    "color(srgb-linear", "color(srgb-linear", "color(srgb-linear 360"
));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesLinearRGB, toString, testing::Values(
    _P(out, linearRGB, { 0.3,   0.2,   0.8         }, "color(srgb-linear 0.3 0.2 0.8)"),
    _P(out, linearRGB, { 0.3,   0.8,   0.258       }, "color(srgb-linear 0.3 0.8 0.258)"),
    _P(out, linearRGB, { 1.0,   0.5,   0.004       }, "color(srgb-linear 1 0.5 0.004)"),
    _P(out, linearRGB, { 0,     1,     0.2,   0.8  }, "color(srgb-linear 0 1 0.2 / 80%)", true),
    _P(out, linearRGB, { 0,     1,     0.2,   0.8  }, "color(srgb-linear 0 1 0.2)", false)
));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesLinearRGB, convertColorSpace, testing::Values(
    // Example from w3c css-color-4 documentation
    _P(inb, linearRGB, {0.435, 0.017, 0.055}, RGB, {0.691, 0.139, 0.259}),
    // No conversion
    _P(inb, linearRGB, {1.000, 0.400, 0.200}, linearRGB, {1.000, 0.400, 0.200})
));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesLinearRGB, normalize, testing::Values(
    _P(inb, linearRGB, { 0.5,   0.5,   0.5,   0.5  }, linearRGB, { 0.5,   0.5,   0.5,   0.5  }),
    _P(inb, linearRGB, { 1.2,   1.2,   1.2,   1.2  }, linearRGB, { 1.0,   1.0,   1.0,   1.0  }),
    _P(inb, linearRGB, {-0.2,  -0.2,  -0.2,  -0.2  }, linearRGB, { 0.0,   0.0,   0.0,   0.0  }),
    _P(inb, linearRGB, { 0.0,   0.0,   0.0,   0.0  }, linearRGB, { 0.0,   0.0,   0.0,   0.0  }),
    _P(inb, linearRGB, { 1.0,   1.0,   1.0,   1.0  }, linearRGB, { 1.0,   1.0,   1.0,   1.0  })
));
// clang-format on

TEST(ColorsSpacesLinearRGB, randomConversion)
{
    // Using the functions directly
    EXPECT_TRUE(RandomPassFunc(Space::LinearRGB::fromRGB<double>,
                               Space::LinearRGB::toRGB<double>, 1000));

    // Using the color conversion stack
    EXPECT_TRUE(RandomPassthrough(linearRGB, RGB, 1000));
}

TEST(ColorsSpacesLinearRGB, components)
{
    auto c = Manager::get().find(linearRGB)->getComponents();
    ASSERT_EQ(c.size(), 3);
    EXPECT_EQ(c[0].id, "r");
    EXPECT_EQ(c[1].id, "g");
    EXPECT_EQ(c[2].id, "b");
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

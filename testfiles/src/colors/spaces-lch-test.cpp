// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests for the LCH color space
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "colors/spaces/lch.h"
#include "spaces-testbase.h"

namespace {

using Space::Type::LCH;
using Space::Type::XYZ;
using Space::Type::RGB;

// clang-format off
INSTANTIATE_TEST_SUITE_P(ColorsSpacesLCH, fromString, testing::Values(
    _P(in, "lch(50% 20 180)",      { 0.5,  0.133, 0.5        }, 0x4d8176ff),
    // this color is outside sRGB gamut, it will be naively clipped to fit
    _P(in, "lch(100 150 360)",     { 1.0,  1.0,   1.0        }, 0xff00ffff),
    _P(in, "lch(0 0 0)",           { 0.0,  0.0,   0.0        }, 0x000000ff),
    _P(in, "lch(20% 20 72 / 20%)", { 0.2,  0.133, 0.2,   0.2 }, 0x3f2d1433)
));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesLCH, badColorString, testing::Values(
    "lch", "lch(", "lch(100"
));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesLCH, toString, testing::Values(
    _P(out, LCH, { 0.0,   0.667, 0.945       }, "lch(0 100.05 340.2)"),
    _P(out, LCH, { 0.3,   0.8,   0.258       }, "lch(30 120 92.88)"),
    _P(out, LCH, { 1.0,   0.5,   0.004       }, "lch(100 75 1.44)"),
    _P(out, LCH, { 0,     1,     0.2,   0.8  }, "lch(0 150 72 / 80%)", true),
    _P(out, LCH, { 0,     1,     0.2,   0.8  }, "lch(0 150 72)", false)
));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesLCH, convertColorSpace, testing::Values(
    _P(inb, LCH, { 0.181, 0.399, 0.810 }, RGB, { 0.0, 0.14,  0.5   }),
    _P(inb, LCH, { 0.907, 0.352, 0.546 }, RGB, { 0.0, 1.0,   1.0   }),
    _P(inb, LCH, { 0.546, 0.623, 0.0817}, RGB, { 1.0, 0.0,   0.230 }),
    _P(inb, LCH, { 0.945, 0.052, 0.035 }, RGB, { 1.0, 0.918, 0.926 }),
    _P(inb, LCH, { 0.526, 0.500, 0.373 }, RGB, { 0.0, 0.574, 0.0   }),
    _P(inb, LCH, { 0.567, 0.300, 0.4617}, RGB, { 0.0, 0.609, 0.453 }),
    // No conversion
    _P(inb, LCH, { 1.0, 0.400, 0.200 }, LCH, { 1.0, 0.400, 0.200 })
));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesLCH, normalize, testing::Values(
    _P(inb, LCH, { 0.5,   0.5,   0.5,   0.5  }, LCH, { 0.5,   0.5,   0.5,   0.5  }),
    _P(inb, LCH, { 1.2,   1.2,   1.2,   1.2  }, LCH, { 1.0,   1.0,   0.2,   1.0  }),
    _P(inb, LCH, {-0.2,  -0.2,  -0.2,  -0.2  }, LCH, { 0.0,   0.0,   0.8,   0.0  }),
    _P(inb, LCH, { 0.0,   0.0,   0.0,   0.0  }, LCH, { 0.0,   0.0,   0.0,   0.0  }),
    _P(inb, LCH, { 1.0,   1.0,   1.0,   1.0  }, LCH, { 1.0,   1.0,   1.0,   1.0  })
));
// clang-format on

TEST(ColorsSpacesLCH, randomConversion)
{
    // Isolate conversion functions
    EXPECT_TRUE(RandomPassFunc(Space::Lch::profileToSpace<double>,
                               Space::Lch::spaceToProfile<double>, 1000));

    // Full stack conversion, can not be enabled until clamp is taken off.
    //EXPECT_TRUE(RandomPassthrough(LCH, XYZ, 1000));
}

TEST(ColorsSpacesLCH, components)
{
    auto c = Manager::get().find(LCH)->getComponents();
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

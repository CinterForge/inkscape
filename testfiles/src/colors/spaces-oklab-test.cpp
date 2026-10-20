// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests for the LAB color space
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "colors/spaces/oklab.h"
#include "spaces-testbase.h"

namespace {

using Space::Type::OKLAB;

// clang-format off
INSTANTIATE_TEST_SUITE_P(ColorsSpacesOkLAB, fromString, testing::Values(
    _P(in, "oklab(50% -0.4 -0.4)",     { 0.5,  0.0,   0.0        }, 0x0045ffff),
    _P(in, "oklab(1 0.4 0.4)",         { 1.0,  1.0,   1.0        }, 0xff0000ff),
    _P(in, "oklab(0 0 0)",             { 0.0,  0.5,   0.5        }, 0x000000ff),
    _P(in, "oklab(20% 0.2 0.2 / 20%)", { 0.2,  0.75,  0.75,  0.2 }, 0x62000033)
));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesOkLAB, badColorString, testing::Values(
    "oklab", "oklab(", "oklab(100"
));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesOkLAB, toString, testing::Values(
    _P(out, OKLAB, { 0.0,   0.667, 0.945       }, "oklab(0 0.134 0.356)"),
    _P(out, OKLAB, { 0.3,   0.8,   0.258       }, "oklab(0.3 0.24 -0.194)"),
    _P(out, OKLAB, { 1.0,   0.5,   0.004       }, "oklab(1 0 -0.397)"),
    _P(out, OKLAB, { 0,     1,     0.2,   0.8  }, "oklab(0 0.4 -0.24 / 80%)", true),
    _P(out, OKLAB, { 0,     1,     0.2,   0.8  }, "oklab(0 0.4 -0.24)", false)
));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesOkLAB, convertColorSpace, testing::Values(
    //_P(inb, OKLAB, { 0.6, 0.125, 0.0   }, RGB, { 0.0, 0.196, 1.0 }),
    // No conversion
    _P(inb, OKLAB, { 1.0, 0.400, 0.200 }, OKLAB, { 1.0, 0.400, 0.200 })
));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesOkLAB, normalize, testing::Values(
    _P(inb, OKLAB, { 0.5,   0.5,   0.5,   0.5  }, OKLAB, { 0.5,   0.5,   0.5,   0.5  }),
    _P(inb, OKLAB, { 1.2,   1.2,   1.2,   1.2  }, OKLAB, { 1.0,   1.0,   1.0,   1.0  }),
    _P(inb, OKLAB, {-0.2,  -0.2,  -0.2,  -0.2  }, OKLAB, { 0.0,   0.0,   0.0,   0.0  }),
    _P(inb, OKLAB, { 0.0,   0.0,   0.0,   0.0  }, OKLAB, { 0.0,   0.0,   0.0,   0.0  }),
    _P(inb, OKLAB, { 1.0,   1.0,   1.0,   1.0  }, OKLAB, { 1.0,   1.0,   1.0,   1.0  })
));
// clang-format on

TEST(ColorsSpacesOkLAB, randomConversion)
{
    // Isolate conversion functions
    EXPECT_TRUE(RandomPassFunc(Space::OkLab::profileToSpace<double>,
                               Space::OkLab::spaceToProfile<double>, 1000));

    // Full stack conversion
    // EXPECT_TRUE(RandomPassthrough(OKLAB, RGB, 1));
}

TEST(ColorsSpacesOkLAB, components)
{
    auto c = Manager::get().find(OKLAB)->getComponents();
    ASSERT_EQ(c.size(), 3);
    EXPECT_EQ(c[0].id, "l");
    EXPECT_EQ(c[1].id, "a");
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

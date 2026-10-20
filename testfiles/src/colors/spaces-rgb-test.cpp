// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests for the RGB color space
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "colors/spaces/components.h"
#include "spaces-testbase.h"

namespace {

using Space::Type::RGB;
using Space::Type::HSL;

// clang-format off
INSTANTIATE_TEST_SUITE_P(ColorsSpacesRgb, fromString, testing::Values(
    _P(in, "#f0f",                        { 1,     0,     1           }, 0xff00ffff),
    _P(in, "#FFC",                        { 1,     1,     0.8         }, 0xffffccff),
    _P(in, "#0F3c",                       { 0,     1,     0.2,   0.8  }, 0x00ff33cc),
    _P(in, "#5533Cc",                     { 0.333, 0.2,   0.8         }, 0x5533ccff),
    _P(in, "#5533Cc66",                   { 0.333, 0.2,   0.8,   0.4  }, 0x5533cc66),
    _P(in, "   #55Cc42  ",                { 0.333, 0.8,   0.258       }, 0x55cc42ff),
    _P(in, "rgb(100%, 50%, 1)",           { 1.0,   0.5,   0.004       }, 0xff8001ff),
    _P(in, "rgb(100% 50% 51)",            { 1.0,   0.5,   0.2         }, 0xff8033ff),
    _P(in, "rgb(100% ,50% , 51   )",      { 1.0,   0.5,   0.2         }, 0xff8033ff),
    _P(in, "rgb(100% 50% 102 / 50%)",     { 1.0,   0.5,   0.4,   0.5  }, 0xff806680),
    _P(in, "   rgb(128, 128, 128)",       { 0.501, 0.501, 0.501       }, 0x808080ff),
    _P(in, "rgba(255, 255, 128,   0.5) ", { 1.0,   1.0,   0.501, 0.5  }, 0xffff8080),
    _P(in, "RGBA(255, 255, 128,   0.5) ", { 1.0,   1.0,   0.501, 0.5  }, 0xffff8080),
    _P(in, "rgba(255  255  128)",         { 1.0,   1.0,   0.501       }, 0xffff80ff),
    _P(in, "color(srgb 1 0.5 0.4 / 50%)", { 1.0,   0.5,   0.4,   0.5  }, 0xff806680),
    _P(in, "color(sRGb 1 0.5 0.4 / 50%)", { 1.0,   0.5,   0.4,   0.5  }, 0xff806680)
));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesRgb, badColorString, testing::Values(
    "", "#", "#1", "#12",
    "rgb", "rgb(", "rgb(255,", "rgb(1 2 3", "rgb(1 2 3 / 4",
    "rgba(1 2 3",
    "color(srgb 3"
));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesRgb, toString, testing::Values(
    _P(out, RGB, { 0.333, 0.2,   0.8         }, "#5533cc"),
    _P(out, RGB, { 0.333, 0.8,   0.258       }, "#55cc42"),
    _P(out, RGB, { 1.0,   0.5,   0.004       }, "#ff8001"),
    _P(out, RGB, { 0,     1,     0.2,   0.8  }, "#00ff33cc")
));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesRgb, convertColorSpace, testing::Values(
    _P(inb, RGB, { 1.0,   0.0,   0.0         }, RGB, { 1.0,   0.0,   0.0         }, false),
    _P(inb, RGB, { 1.0,   0.0,   0.0,   0.5  }, RGB, { 1.0,   0.0,   0.0,   0.5  }, false),
    // All other tests are in their respective color space test, for example spoaces-hsl-test.cpp
    _P(inb, RGB, { 1.0,   0.0,   0.0         }, HSL, { 0.0,   1.0,   0.5         }),
    _P(inb, RGB, { 1.0,   0.0,   0.0,   0.5  }, HSL, { 0.0,   1.0,   0.5,   0.5  })
));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesRgb, normalize, testing::Values(
    _P(inb, RGB, { 0.5,   0.5,   0.5,   0.5  }, RGB, { 0.5,   0.5,   0.5,   0.5  }),
    _P(inb, RGB, { 1.2,   1.2,   1.2,   1.2  }, RGB, { 1.0,   1.0,   1.0,   1.0  }),
    _P(inb, RGB, {-0.2,  -0.2,  -0.2,  -0.2  }, RGB, { 0.0,   0.0,   0.0,   0.0  }),
    _P(inb, RGB, { 0.0,   0.0,   0.0,   0.0  }, RGB, { 0.0,   0.0,   0.0,   0.0  }),
    _P(inb, RGB, { 1.0,   1.0,   1.0,   1.0  }, RGB, { 1.0,   1.0,   1.0,   1.0  })
));
// clang-format on

TEST(ColorsSpacesRgb, randomConversion)
{
    EXPECT_TRUE(RandomPassthrough(RGB, RGB, 1)); // Not really needed
}

TEST(ColorsSpacesRgb, components)
{
    auto c = Manager::get().find(RGB)->getComponents();
    ASSERT_EQ(c.size(), 3);
    ASSERT_EQ(c[0].id, "r");
    ASSERT_EQ(c[1].id, "g");
    ASSERT_EQ(c[2].id, "b");
    ASSERT_EQ(c[0].index, 0);
    ASSERT_EQ(c[1].index, 1);
    ASSERT_EQ(c[2].index, 2);

    auto c2 = Manager::get().find(RGB)->getComponents(true);
    ASSERT_EQ(c2.size(), 4);
    ASSERT_EQ(c2[3].id, "alpha");
    ASSERT_EQ(c2[3].index, 3);
}

TEST(ColorsSpacesRgb, isDirect)
{
    ASSERT_TRUE(std::dynamic_pointer_cast<Space::ProfileSpace<true>>(Manager::get().find(RGB)));
}

/*TEST(ColorsSpacesRgb, colorVarFallback)
{
    auto &cm = Manager::get();
    ASSERT_EQ(Color("var(--foo, white)").toString(), "white");
    ASSERT_EQ(Color("var(--foo, black)").toString(), "white");
    ASSERT_EQ(Color("var(--foo, #00ff00)").toString(), "#00ff00");
}*/

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

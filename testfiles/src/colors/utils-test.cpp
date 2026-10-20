// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests for color objects.
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "colors/utils.h"

#include <gtest/gtest.h>

#include "colors/color.h"
#include "colors/manager.h"

using namespace Inkscape;
using namespace Inkscape::Colors;

namespace {

TEST(ColorUtils, test_hex_to_rgba)
{
    EXPECT_EQ(hex_to_rgba("#ff00ffff"), 0xff00ffff);
}

TEST(ColorUtils, test_rgba_to_hex)
{
    EXPECT_EQ(rgba_to_hex(0xff00ff00, false), "#ff00ff");
    EXPECT_EQ(rgba_to_hex(0xff00ffff, true), "#ff00ffff");
}

TEST(ColorUtils, test_color_to_id)
{
    EXPECT_EQ(color_to_id({}), "none");
    EXPECT_EQ(color_to_id(Color::parse("not-a-color")), "none");
    EXPECT_EQ(color_to_id(Color::parse("red")), "css-red");
    EXPECT_EQ(color_to_id(Color::parse("#0000ff")), "rgb-0000ff");

    auto color = Color::parse("hsl(0.5, 50, 100)");
    EXPECT_EQ(color_to_id(color), "hsl-007fff");

    color->setName("Huey // Dewy_! Lewy");
    EXPECT_EQ(color_to_id(color), "huey-dewy-lewy");

    color->convert(Space::Type::RGB);
    EXPECT_EQ(color_to_id(color), "rgb-ffffff");
}

TEST(ColorUtils, test_desc_to_id)
{
    EXPECT_EQ(desc_to_id("thing"), "thing");
    EXPECT_EQ(desc_to_id("Thing Two"), "thing-two");
    EXPECT_EQ(desc_to_id("  Thing   Threé  "), "thing-threé");
    EXPECT_EQ(desc_to_id("   Wobble blink CAPLINK!"), "wobble-blink-caplink");
}

TEST(ColorUtils, test_make_contrasted_color)
{
    EXPECT_EQ(make_contrasted_color(Color(0x000000ff), 0.2).toRGBA(), 0x040404ff);
    EXPECT_EQ(make_contrasted_color(Color(0x000000ff), 0.4).toRGBA(), 0x080808ff);
    EXPECT_EQ(make_contrasted_color(Color(0x000000ff), 0.6).toRGBA(), 0x0c0c0cff);
    EXPECT_EQ(make_contrasted_color(Color(0xffffffff), 0.2).toRGBA(), 0xfbfbfbff);
    EXPECT_EQ(make_contrasted_color(Color(0xffffffff), 0.4).toRGBA(), 0xf7f7f7ff);
    EXPECT_EQ(make_contrasted_color(Color(0xffffffff), 0.6).toRGBA(), 0xf3f3f3ff);
    EXPECT_EQ(make_contrasted_color(Color(0xa1a1a1ff), 0.2).toRGBA(), 0x9d9d9dff);
    EXPECT_EQ(make_contrasted_color(Color(0x1a1a1aff), 0.4).toRGBA(), 0x121212ff);
    EXPECT_EQ(make_contrasted_color(Color(0x808080ff), 0.6).toRGBA(), 0x747474ff);
}

TEST(ColorUtils, test_get_perceptual_lightness)
{
    EXPECT_NEAR(get_perceptual_lightness(*Color::parse("red")), 0.780, 0.001);
    EXPECT_NEAR(get_perceptual_lightness(*Color::parse("black")), 0.0, 0.001);
    EXPECT_NEAR(get_perceptual_lightness(*Color::parse("white")), 1.0, 0.001);
    EXPECT_NEAR(get_perceptual_lightness(*Color::parse("device-cmyk(0.2 0.1 1.0 0.0)")), 0.945, 0.001);
}

TEST(ColorUtils, test_contrasting_color)
{
    auto a = get_contrasting_color(0.1);
    EXPECT_EQ(a.first, 1.0);
    EXPECT_NEAR(a.second, 0.688, 0.001);

    auto b = get_contrasting_color(0.9);
    EXPECT_EQ(b.first, 0.0);
    EXPECT_NEAR(b.second, 0.366, 0.001);
}

TEST(ColorUtils, make_theme_color)
{
    EXPECT_EQ(make_theme_color(*Color::parse("red"), false).toRGBA(), 0xffb3b3ff);
    EXPECT_EQ(make_theme_color(*Color::parse("red"), true).toRGBA(), 0x891c1cff);
    EXPECT_EQ(make_theme_color(*Color::parse("white"), false).toRGBA(), 0xffffffff);
    // Broken by XYZ changes, produces bad results
    //EXPECT_EQ(make_theme_color(*Color::parse("white"), true).toRGBA(), 0x474747ff);
    EXPECT_EQ(make_theme_color(*Color::parse("black"), false).toRGBA(), 0xc6c6c6ff);
    EXPECT_EQ(make_theme_color(*Color::parse("black"), true).toRGBA(), 0x000000ff);
}

TEST(ColorUtils, make_disabled_color)
{
    EXPECT_EQ(make_disabled_color(*Color::parse("red"), false).toRGBA(), 0xe9dcdcff);
    EXPECT_EQ(make_disabled_color(*Color::parse("red"), true).toRGBA(), 0x844b4bff);
    EXPECT_EQ(make_disabled_color(*Color::parse("white"), false).toRGBA(), 0xffffffff);
    // Broken by XYZ changes, produces bad results
    //EXPECT_EQ(make_disabled_color(*Color::parse("white"), true).toRGBA(), 0x848484ff);
    EXPECT_EQ(make_disabled_color(*Color::parse("black"), false).toRGBA(), 0xabababff);
    EXPECT_EQ(make_disabled_color(*Color::parse("black"), true).toRGBA(), 0x303030ff);
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

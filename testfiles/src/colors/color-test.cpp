// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests for color objects.
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "colors/color.h"

#include <gtest/gtest.h>
#include "../test-utils.h"

#include "colors/manager.h"
#include "colors/spaces/base.h"

using namespace Inkscape::Colors;

namespace {

TEST(ColorsColor, construct_space_obj)
{
    auto space = Manager::get().find(Space::Type::HSL);
    ASSERT_TRUE(space);

    ASSERT_EQ(Color(space, {0, 1, 0.5}).toString(), "hsl(0, 100%, 50%)");
}

TEST(ColorsColor, construct_space_type)
{
    ASSERT_EQ(Color(Space::Type::HSL, {0, 1, 0.5}).toString(), "hsl(0, 100%, 50%)");
}

TEST(ColorsColor, construct_css_string)
{
    ASSERT_EQ(Color::parse("red")->toString(), "red");
    // document tested in cms tests
}

TEST(ColorsColor, construct_rgba)
{
    ASSERT_EQ(Color(0xff00ff00, false).toString(), "#ff00ff");
    ASSERT_EQ(Color(0xff00ff00, true).toString(), "#ff00ff00");
}

TEST(ColorsColor, construct_other)
{
    auto color = *Color::parse("red");
    auto other = Color(color);
    ASSERT_EQ(other.toString(), "red");
}

TEST(ColorsColor, parse)
{
    ASSERT_TRUE(Color::parse("red"));
    ASSERT_FALSE(Color::parse("none"));
    ASSERT_FALSE(Color::parse(nullptr));
}

TEST(ColorsColor, setter)
{
    auto color = *Color::parse("purple");
    color = *Color::parse("green");
    ASSERT_EQ(color.toString(), "green");

    EXPECT_TRUE(color.set(Color(0x0000ffff), true));
    ASSERT_EQ(color.toString(), "blue");
    EXPECT_TRUE(color.set(Color(0x0000ffff, false), false));
    ASSERT_EQ(color.toString(), "#0000ff");

    EXPECT_TRUE(color.set(1, 1.0));
    EXPECT_FALSE(color.set(1, 1.0));
    ASSERT_EQ(color.toString(), "#00ffff");

    EXPECT_TRUE(color.set("red", true));
    EXPECT_FALSE(color.set("red", true));
    ASSERT_EQ(color.toString(), "#ff0000");
    EXPECT_TRUE(color.set("red", false));
    EXPECT_FALSE(color.set("red", false));
    ASSERT_EQ(color.toString(), "red");

    EXPECT_TRUE(color.set(0x0, false));
    EXPECT_FALSE(color.set(0x0, false));
    ASSERT_EQ(color.toString(), "#000000");
    EXPECT_TRUE(color.set(0x00ff00ff, true));
    ASSERT_EQ(color.toString(), "#00ff00ff");
    EXPECT_TRUE(color.set(0x00ff00, false));
    ASSERT_EQ(color.toString(), "#0000ff");

    color.setValues({0.2, 1.0, 0.5});
    ASSERT_EQ(color.toString(), "#33ff80");
}

TEST(ColorsColor, conditionals)
{
    // ==
    ASSERT_EQ(*Color::parse("red"), *Color::parse("red"));
    // !=
    ASSERT_NE(*Color::parse("green"), Color(0xff0000));
    // bool
    ASSERT_TRUE(Color::parse("blue"));
}

TEST(ColorsColor, getSpace)
{
    auto color = *Color::parse("red");
    ASSERT_TRUE(color.getSpace());
    ASSERT_EQ(color.getSpace()->getName(), "CSSNAME");
}

TEST(ColorsColor, values)
{
    auto color = *Color::parse("red");
    EXPECT_TRUE(VectorIsNear(color.getValues(), {1.0, 0.0, 0.0}, 0.01));
}

TEST(ColorsColor, Opacity)
{
    auto color = *Color::parse("red");
    ASSERT_FALSE(color.hasOpacity());
    ASSERT_FALSE(color.converted(Space::Type::HSL)->hasOpacity());
    EXPECT_TRUE(color.setOpacity(1.0));
    EXPECT_FALSE(color.setOpacity(1.0));
    EXPECT_FALSE(color.addOpacity(1.0));
    ASSERT_TRUE(color.hasOpacity());
    ASSERT_EQ(color.getOpacity(), 1.0);
    ASSERT_EQ(color.toString(), "#ff0000ff");
    EXPECT_TRUE(color.setOpacity(0.5));
    EXPECT_FALSE(color.setOpacity(0.5));
    ASSERT_TRUE(color.hasOpacity());
    ASSERT_EQ(color.getOpacity(), 0.5);
    ASSERT_EQ(color.toString(), "#ff000080");
    EXPECT_TRUE(color.addOpacity(0.5));
    ASSERT_EQ(color.getOpacity(), 0.25);
    ASSERT_EQ(color.toString(), "#ff000040");
    color.enableOpacity(false);
    ASSERT_FALSE(color.hasOpacity());
    ASSERT_EQ(color.toString(), "red");
    EXPECT_TRUE(color.addOpacity(0.5));
    ASSERT_TRUE(color.hasOpacity());
    ASSERT_EQ(color.getOpacity(), 0.5);
    ASSERT_EQ(color.stealOpacity(), 0.5);
    ASSERT_FALSE(color.hasOpacity());

    auto copy = color.withOpacity(0.5);
    ASSERT_TRUE(copy.hasOpacity());
    ASSERT_FALSE(color.hasOpacity());
    ASSERT_EQ(copy.getOpacity(), 0.5);
    ASSERT_EQ(copy.toString(), "#ff000080");
    auto copy2 = copy.withOpacity(0.5);
    ASSERT_EQ(copy2.getOpacity(), 0.25);
    ASSERT_EQ(copy2.toString(), "#ff000040");
}

TEST(ColorsColor, colorOpacityPin)
{
    auto color = Color::parse("red");
    ASSERT_EQ(color->getOpacityChannel(), 3);
    ASSERT_EQ(color->getPin(3), 8);
    color->convert(Space::Type::CMYK);
    ASSERT_EQ(color->getOpacityChannel(), 4);
    ASSERT_EQ(color->getPin(4), 16);
}

TEST(ColorsColor, difference)
{
    auto color = Color::parse("green");
    ASSERT_NEAR(color->difference(*Color::parse("red")), 1.251, 0.001);
    ASSERT_NEAR(color->difference(*Color::parse("blue")), 1.251, 0.001);
    ASSERT_NEAR(color->difference(*Color::parse("black")), 0.251, 0.001);
}

TEST(ColorsColor, similarAndClose)
{
    double one_hex_away = 0.004;
    auto c1 = Color(0xff0000ff, false);
    auto c2 = Color(0x0000ffff, false);
    ASSERT_FALSE(c1.isClose(c2));
    ASSERT_FALSE(c1.isSimilar(c2));

    ASSERT_TRUE(c1.isClose(c1));
    ASSERT_TRUE(c1.isSimilar(c1));

    c2 = *Color::parse("red");
    ASSERT_FALSE(c1.isClose(c2));
    ASSERT_TRUE(c1.isSimilar(c2));

    c2 = Color(0xfe0101ff, false);
    ASSERT_TRUE(c1.isClose(c2, one_hex_away));
    ASSERT_TRUE(c1.isSimilar(c2, one_hex_away));

    c2 = Color(0xfe0102ff, false);
    ASSERT_FALSE(c1.isClose(c2, one_hex_away));
    ASSERT_FALSE(c1.isSimilar(c2, one_hex_away));
}

TEST(ColorsColor, convert_other)
{
    auto other = *Color::parse("red");
    auto color = Color::parse("hsl(120, 100, 25.1)");
    color->convert(other);
    EXPECT_EQ(color->toString(), "green");
    other.addOpacity();
    color->convert(other);
    EXPECT_EQ(color->toString(), "#008000ff");
}

TEST(ColorsColor, convert_space_obj)
{
    auto space = Manager::get().find(Space::Type::HSL);
    ASSERT_TRUE(space);

    auto color = Color(0xff0000ff, false);
    color.convert(space);
    ASSERT_EQ(color.toString(), "hsl(0, 100%, 50%)");
}

TEST(ColorsColor, convert_space_type)
{
    auto color = Color(0xff0000ff, false);
    ASSERT_TRUE(color.convert(Space::Type::HSL));
    ASSERT_EQ(color.toString(), "hsl(0, 100%, 50%)");
    ASSERT_FALSE(color.convert(Space::Type::NONE));
    ASSERT_EQ(color.toString(), "hsl(0, 100%, 50%)");
}

TEST(ColorsColor, converted_other)
{
    auto other = *Color::parse("red");
    ASSERT_EQ(Color::parse("hsl(120, 100, 25.1)")->converted(other)->toString(), "green");
    other.addOpacity();
    ASSERT_EQ(Color::parse("hsl(120, 100, 25.1)")->converted(other)->toString(), "#008000ff");
}

TEST(ColorsColor, converted_space_obj)
{
    auto space = Manager::get().find(Space::Type::HSL);
    ASSERT_TRUE(space);
    ASSERT_EQ(Color::parse("red")->converted(space)->toString(), "hsl(0, 100%, 50%)");
}

TEST(ColorsColor, converted_space_type)
{
    auto color = Color::parse("red");
    ASSERT_EQ(color->converted(Space::Type::HSL)->toString(), "hsl(0, 100%, 50%)");

    auto none = color->converted(Space::Type::NONE);
    ASSERT_FALSE(none);
}

TEST(ColorsColor, toString)
{
    ASSERT_EQ(Color::parse("red")->toString(), "red");
    ASSERT_EQ(Color::parse("#ff0")->toString(), "#ffff00");
    ASSERT_EQ(Color::parse("rgb(80 90 255 / 0.5)")->toString(true), "#505aff80");
    ASSERT_EQ(Color::parse("rgb(80 90 255 / 0.5)")->toString(false), "#505aff");
    // Each type of space tested in it's own testcase here after.
}

TEST(ColorsColor, toRGBA)
{
    ASSERT_EQ(Color(0x123456cc).toRGBA(1.0), 0x123456cc);
    ASSERT_EQ(Color(0x123456cc).toRGBA(0.5), 0x12345666);
    // Each type of space tested in it's own testcase here after.
}

TEST(ColorsColor, toARGB)
{
    ASSERT_EQ(Color(0x123456cc).toARGB(1.0), 0xcc123456);
    ASSERT_EQ(Color(0x123456cc).toARGB(0.5), 0x66123456);
}

TEST(ColorsColor, toABGR)
{
    ASSERT_EQ(Color(0x123456cc).toABGR(1.0), 0xcc563412);
    ASSERT_EQ(Color(0x123456cc).toABGR(0.5), 0x66563412);
}

TEST(ColorsColor, name)
{
    auto color = *Color::parse("red");
    ASSERT_FALSE(color.getName().size());
    color.setName("Rouge");
    ASSERT_EQ(color.getName(), "Rouge");

    color.setName("Rouge");
    color.convert(Space::Type::HSL);
    ASSERT_FALSE(color.getName().size());
}

TEST(ColorsColor, normalizeColor)
{
    auto color = *Color::parse("rgb(0, 0, 0)");
    color.set(0, 2.0);
    ASSERT_EQ(color[0], 2.0);
    color.set(1, 1.0);
    color.set(2, -0.5);
    color.normalize();
    ASSERT_EQ(color[0], 1.0);
    ASSERT_EQ(color[1], 1.0);
    ASSERT_EQ(color[2], 0.0);

    color.convert(Space::Type::HSL);
    color.set(0, 4.1);
    color.normalize();
    ASSERT_NEAR(color[0], 0.1, 0.001);

    color.set(0, -0.2);
    color.normalize();
    ASSERT_NEAR(color[0], 0.8, 0.001);

    color.set(0, -2.2);
    color.normalize();
    ASSERT_NEAR(color[0], 0.8, 0.001);

    color.setOpacity(4.2);
    auto copy = color.normalized();
    ASSERT_NEAR(color[3], 4.2, 0.001);
    ASSERT_NEAR(copy[3], 1.0, 0.001);
}

TEST(ColorsColor, invertColor)
{
    auto color = *Color::parse("red");
    color.invert();
    ASSERT_EQ(color.toString(), "aqua");
    color.invert();
    ASSERT_EQ(color.toString(), "red");

    color = *Color::parse("hsl(90,50,10)");
    color.invert();
    ASSERT_EQ(color.toString(), "hsl(270, 50%, 90%)");

    color.invert(2);
    ASSERT_EQ(color.toString(), "hsl(90, 50%, 10%)");

    color = *Color::parse("rgb(255 255 255 0.2)");
    ASSERT_NEAR(color[0], 1, 0.001);
    color.invert();
    ASSERT_NEAR(color[0], 0, 0.001);
    ASSERT_NEAR(color[3], 0.2, 0.001);

    color.invert(0);
    ASSERT_NEAR(color[0], 1, 0.001);
    ASSERT_NEAR(color[3], 0.8, 0.001);
}

TEST(ColorsColor, jitterColor)
{
    auto color = *Color::parse("gray");

    std::srand(1); // fixed random seed

    color.jitter(0.1, 0xff);
    EXPECT_EQ(color.toString(), "gray");

#ifdef __APPLE__
    // The hasher on llvm works differently so the random results are different
    color.jitter(0.1);
    EXPECT_EQ(color.toString(), "#737787");
    color.jitter(0.2);
    EXPECT_EQ(color.toString(), "#717878");
    color.jitter(0.2, 0x02);
    EXPECT_EQ(color.toString(), "#5a7881");
#elif defined(_WIN32)
    // Random results from from UCRT on Windows 10 22H2
    color.jitter(0.1);
    EXPECT_EQ(color.toString(), "#738278");
    color.jitter(0.2);
    EXPECT_EQ(color.toString(), "#838677");
    color.jitter(0.2, 0x02);
    EXPECT_EQ(color.toString(), "#7b868b");
#else
    color.jitter(0.1);
    EXPECT_EQ(color.toString(), "#897d87");
    color.jitter(0.2);
    EXPECT_EQ(color.toString(), "#989278");
    color.jitter(0.2, 0x02);
    EXPECT_EQ(color.toString(), "#8f9285");
#endif

    color.setOpacity(0.5);
    color.jitter(0.5, color.getPin(color.getOpacityChannel()));
    EXPECT_EQ(color.getOpacity(), 0.5);
}

TEST(Colorscolor, compose)
{
    auto c1 = *Color::parse("#ff0000");
    auto c2 = *Color::parse("#0000ff");
    EXPECT_EQ(c1.composed(c2).toString(), "#0000ffff");
    EXPECT_EQ(c2.composed(c1).toString(), "#ff0000ff");
    c1.setOpacity(0.5);
    EXPECT_EQ(c1.composed(c2).toString(), "#0000ffff");
    EXPECT_EQ(c2.composed(c1).toString(), "#800080ff");
    c2.setOpacity(0.5);
    EXPECT_EQ(c1.composed(c2).toString(), "#800080bf");
    EXPECT_EQ(c2.composed(c1).toString(), "#800080bf");
}

TEST(ColorsColor, average)
{
    auto c1 = *Color::parse("#ff0000");
    auto c2 = *Color::parse("#0000ff");
    ASSERT_EQ(c1.averaged(c2).toString(), "#800080");
    ASSERT_EQ(c2.averaged(c1).toString(), "#800080");
    c1.setOpacity(0.5);
    ASSERT_EQ(c1.averaged(c2, 0.25).toString(), "#bf00409f");
    c1.enableOpacity(false);
    c2.setOpacity(0.5);
    ASSERT_EQ(c1.averaged(c2, 0.75).toString(), "#4000bf");

    c1 = Color(0x0);
    c1.average(Color(0xffffffff), 0.25, 1);
    EXPECT_EQ(c1.toString(), "#00404040");

    c1 = Color(0x0);
    c1.average(Color(0xffffffff), 0.25, 2);
    EXPECT_EQ(c1.toString(), "#40004040");

    c1 = Color(0x0);
    c1.average(Color(0xffffffff), 0.25, 4 + 2);
    EXPECT_EQ(c1.toString(), "#40000040");

    c1 = Color(0x0);
    c1.average(Color(0xffffffff), 0.25, c1.getPin(3));
    EXPECT_EQ(c1.toString(), "#40404000");

    auto c3 = Color(0x1a1a1a1a);
    c3.average(Color(0xffffffff), 0.2, 2);
    EXPECT_EQ(c3.toString(), "#481a4848");
    c3.average(Color(0xffffffff), 0.3, 4 + 2);
    EXPECT_EQ(c3.toString(), "#7f1a487f");
    c3.average(Color(0xffffffff), 0.5, c3.getPin(3));
    EXPECT_EQ(c3.toString(), "#bf8da37f");

    c1 = Color(0x00000000);
    c1.average(Color(0xffffffff), 0.1, 0);
    ASSERT_NEAR(c1[0], 0.1, 0.001);
    ASSERT_EQ(c1.toString(), "#1a1a1a1a");
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

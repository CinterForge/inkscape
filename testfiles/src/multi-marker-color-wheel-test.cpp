// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Test the multimarker color wheel public functions  functionality of Recolor Art Widget
 *
 * Authors:
 *   Fatma Omara <ftomara647@gmail.com>
 *
 * Copyright (C) 2025 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtest/gtest.h>

#include "src/colors/color.h"
#include "ui/widget/ink-color-wheel.h"

using namespace Inkscape;
using Inkscape::Colors::Color;
using namespace Inkscape::Colors::Space;
class ColorWheelTestFixture : public ::testing::Test
{
protected:
    std::unique_ptr<Inkscape::UI::Widget::MultiMarkerWheel> wheel;
    std::vector<Color> colors;

    void SetUp() override
    {
        char const *gui_env = std::getenv("INKSCAPE_TEST_GUI");
        if (!gui_env || std::string(gui_env) != "1") {
            GTEST_SKIP() << "Skipping GUI tests: GUI testing not enabled";
        } else {
            gtk_init();
        }
        colors = {Color(Type::CMYK, {0.1, 0.8, 0.0, 0.0}),
                  Color(0xff0000ff),
                  Color(0x00ff007f),
                  Color(0x0000ff32),
                  Color(0x7e1a9cff),
                  Color(Type::HSLUV, {120.0, 100.0, 50.0}),
                  Color(Type::HSL, {0.33, 1.0, 0.5}),
                  Color(Type::HSV, {0.66, 1.0, 1.0}),
                  Color(Type::LAB, {60.0, -40.0, 30.0})};

        wheel = std::make_unique<Inkscape::UI::Widget::MultiMarkerWheel>();
    }
};

TEST_F(ColorWheelTestFixture, TestColorWheelBasics)
{
    EXPECT_TRUE(wheel->getColors().empty());
    EXPECT_EQ(wheel->getActiveIndex(), -1);

    wheel->setColors(colors);
    EXPECT_EQ(wheel->getColors().size(), 9);
    EXPECT_EQ(wheel->getActiveIndex(), 0);
}

TEST_F(ColorWheelTestFixture, TestColorWheelActiveIndex)
{
    wheel->setColors(colors);
    EXPECT_TRUE(wheel->setActiveIndex(8));
    EXPECT_EQ(wheel->getActiveIndex(), 8);

    EXPECT_FALSE(wheel->setActiveIndex(-1));
    EXPECT_FALSE(wheel->setActiveIndex(99));
    EXPECT_EQ(wheel->getActiveIndex(), 8);
}

TEST_F(ColorWheelTestFixture, TestColorWheelLightnessAndSaturation)
{
    wheel->setColors(colors);
    wheel->setLightness(90);
    EXPECT_EQ(wheel->getColor()[2], 0.9);

    wheel->setSaturation(40);
    EXPECT_EQ(wheel->getColor()[1], 0.4);

    auto color = Color(0xffffffff);
    EXPECT_TRUE(wheel->changeColor(8, color));
    EXPECT_TRUE(wheel->setActiveIndex(8));
    EXPECT_EQ(wheel->getColor().toRGBA(), color.toRGBA());
}

TEST_F(ColorWheelTestFixture, TestColorWheelHueLocking)
{
    wheel->setColors(colors);
    EXPECT_FALSE(wheel->getColors().empty());
    wheel->toggleHueLock(true);
    EXPECT_EQ(wheel->getHueLock(), true);

    wheel->setLightness(50);
    wheel->setSaturation(83);
    EXPECT_EQ(wheel->getColors()[4][2], 0.5);

    for (auto color : wheel->getColors()) {
        EXPECT_EQ(color[2], 0.5);
        EXPECT_EQ(color[1], 0.83);
    }
}

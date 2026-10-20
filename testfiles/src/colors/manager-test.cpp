// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests for color objects.
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "colors/manager.h"

#include <gtest/gtest.h>

#include "colors/color.h"
#include "colors/spaces/base.h"
#include "colors/spaces/rgb.h"
#include "colors/spaces/components.h"
#include "colors/spaces/enum.h"

using namespace Inkscape;
using namespace Inkscape::Colors;

namespace {

class TestManager : public Manager
{
public:
    TestManager() = default;
    ~TestManager() = default;

    std::shared_ptr<Space::AnySpace> testAddSpace(Space::AnySpace *space)
    {
        return addSpace(space);
    }

    bool testRemoveSpace(std::shared_ptr<Space::AnySpace> space)
    {
        return removeSpace(std::move(space));
    }

};

TEST(ColorManagerTest, spaceComponents)
{
    auto &cm = Manager::get();

    ASSERT_TRUE(cm.find(Space::Type::RGB));
    auto comp = cm.find(Space::Type::RGB)->getComponents();
    ASSERT_EQ(comp.size(), 3);
    ASSERT_EQ(comp[0].name, "_R");
    ASSERT_EQ(comp[1].name, "_G");
    ASSERT_EQ(comp[2].name, "_B");

    ASSERT_TRUE(cm.find(Space::Type::HSL));
    comp = cm.find(Space::Type::HSL)->getComponents(true);
    ASSERT_EQ(comp.size(), 4);
    ASSERT_EQ(comp[0].name, "_H");
    ASSERT_EQ(comp[1].name, "_S");
    ASSERT_EQ(comp[2].name, "_L");
    ASSERT_EQ(comp[3].name, "_A");

    ASSERT_TRUE(cm.find(Space::Type::CMYK));
    comp = cm.find(Space::Type::CMYK)->getComponents(false);
    ASSERT_EQ(comp.size(), 4);
    ASSERT_EQ(comp[0].name, "_C");
    ASSERT_EQ(comp[1].name, "_M");
    ASSERT_EQ(comp[2].name, "_Y");
    ASSERT_EQ(comp[3].name, "_K");
}

TEST(ColorManagerTest, isUnbounded) {
    auto cm = TestManager();

    auto rgb = cm.find(Space::Type::RGB);
    ASSERT_TRUE(rgb);
    ASSERT_FALSE(rgb->isUnbounded());

    auto hsl = cm.find(Space::Type::HSL);
    ASSERT_TRUE(hsl);
    ASSERT_FALSE(hsl->isUnbounded());

    auto cmyk = cm.find(Space::Type::CMYK);
    ASSERT_TRUE(cmyk);
    ASSERT_FALSE(cmyk->isUnbounded());

    // auto cms = cm.find(Space::Type::CMS);
    // ASSERT_TRUE(cms);
    // ASSERT_FALSE(cms->isUnbounded());

    auto xyz = cm.find(Space::Type::XYZ);
    ASSERT_TRUE(xyz);
    ASSERT_TRUE(xyz->isUnbounded());

    auto lab = cm.find(Space::Type::LAB);
    ASSERT_TRUE(lab);
    ASSERT_TRUE(lab->isUnbounded());

    auto oklab = cm.find(Space::Type::OKLAB);
    ASSERT_TRUE(oklab);
    ASSERT_TRUE(oklab->isUnbounded());

}

TEST(ColorManagerTest, addAndRemoveSpaces)
{
    auto cm = TestManager();

    auto rgb = cm.find(Space::Type::RGB);
    ASSERT_TRUE(rgb);

    EXPECT_THROW(cm.testAddSpace(rgb.get()), ColorError);

    ASSERT_TRUE(cm.testRemoveSpace(rgb));
    ASSERT_FALSE(cm.testRemoveSpace(rgb));
    ASSERT_FALSE(cm.find(Space::Type::RGB));

    cm.testAddSpace(new Space::RGB());
    ASSERT_TRUE(cm.find(Space::Type::RGB));
}

TEST(ColorManagerTest, getSpaces)
{
    auto cm = TestManager();

    auto none = cm.spaces(Space::Traits::None);
    ASSERT_EQ(none.size(), 0);

    auto internal = cm.spaces(Space::Traits::Internal);
    ASSERT_GT(internal.size(), 0);
    ASSERT_EQ(internal[0]->getComponents().traits() & Space::Traits::Internal, Space::Traits::Internal);

    auto pickers = cm.spaces(Space::Traits::Picker);
    ASSERT_GT(pickers.size(), 0);
    ASSERT_EQ(pickers[0]->getComponents().traits() & Space::Traits::Picker, Space::Traits::Picker);

    auto mix = cm.spaces(Space::Traits::Picker | Space::Traits::Internal);
    ASSERT_EQ(mix.size(), internal.size() + pickers.size());
}

TEST(ColorsParser, findSvgColorSpace)
{
    auto _pass = [](std::string svgName, std::string resultSpaceId)
    {
        auto space = Manager::get().findSvgColorSpace(svgName);
        EXPECT_TRUE(space) << "Svg value '" + svgName + "' parsing failed.";
        if (space) {
            EXPECT_EQ(space->getName(), resultSpaceId);
        }
    };
    auto _fail = [](std::string interpolationName)
    {
        EXPECT_FALSE(Manager::get().findSvgColorSpace(interpolationName))
            << "Interpolation value '" + interpolationName + "' should not have parsed, yet it did.";
    };
    // SVG 2.0 specification interpolations
    _pass("sRGB", "RGB");
    _pass("linearRGB", "linearRGB");
    // CSS Color Module 4 interpolations
    _pass("srgb", "RGB");
    _pass("srgb-linear", "linearRGB");
    _fail("display-p3");
    _fail("a98-rgb");
    _fail("prophoto-rgb");
    _fail("rec2020");
    _pass("lab", "Lab");
    _pass("oklab", "OkLab");
    _pass("xyz", "XYZ"); // D65
    _pass("xyz-d50", "XYZ D50");
    _pass("xyz-d65", "XYZ");
    // CSS Color Module 4 Polar
    _pass("hsl", "HSL");
    _fail("hwb");
    _pass("lch", "Lch");
    _pass("oklch", "OkLch");
    // Extra values for other interpolations not in SVG spec
    _pass("device-cmyk", "DeviceCMYK");
    // Things we want to protect against
    _fail("");
    _fail("rgb");
    _fail("cmyk");
    _fail("icc-color");
    // Valid css value 'auto' is handled by SPStyle
    _fail("auto");
}


TEST(ColorsParser, printSvgColorSpace)
{
    auto _test = [](std::shared_ptr<Space::AnySpace> const &space, std::string svgName)
    {
        EXPECT_EQ(space->getSvgName(), svgName);
    };
    _test(Manager::get().find(Space::Type::RGB), "sRGB");
    _test(Manager::get().find(Space::Type::linearRGB), "linearRGB");
    _test(Manager::get().find(Space::Type::XYZ), "xyz-d65");
    _test(Manager::get().find(Space::Type::XYZ50), "xyz-d50");
    _test(Manager::get().find(Space::Type::CMYK), "device-cmyk");
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

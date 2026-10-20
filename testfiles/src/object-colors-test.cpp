// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Test the Object Colors Extraction and Data Population  functionality of Recolor Art Widget
 *
 * Authors:
 *   Fatma Omara <ftomara647@gmail.com>
 *
 * Copyright (C) 2025 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "src/object-colors.h"

#include <doc-per-case-test.h>
#include <gtest/gtest.h>

#include "object/sp-defs.h"
#include "object/sp-object.h"
#include "object/sp-root.h"
#include "object/sp-stop.h"
#include "src/colors/color.h"
#include "src/document.h"
#include "xml/node.h"
#include "style.h"
using namespace Inkscape;

class ObjectColorSetFixture : public DocPerCaseTest
{
protected:
    std::vector<Inkscape::XML::Node *> nodes;
    std::vector<SPObject *> vector;

    ObjectColorSet set;

    ObjectColorSetFixture()
    {
        SetUpTestCase();

        Inkscape::XML::Document *xml_doc = _doc->getReprDoc();

        Inkscape::XML::Node *grad_node = xml_doc->createElement("svg:linearGradient");
        grad_node->setAttribute("id", "test-gradient");

        Inkscape::XML::Node *stop1_node = xml_doc->createElement("svg:stop");
        stop1_node->setAttribute("offset", "0");
        stop1_node->setAttribute("stop-color", "#ffA000ff");
        stop1_node->setAttribute("stop-opacity", "1");
        grad_node->appendChild(stop1_node);

        Inkscape::XML::Node *stop2_node = xml_doc->createElement("svg:stop");
        stop2_node->setAttribute("offset", "1");
        stop2_node->setAttribute("stop-color", "#00ffffff");
        stop2_node->setAttribute("stop-opacity", "1");
        grad_node->appendChild(stop2_node);

        for (int i = 0; i < 6; i++) {
            nodes.push_back(xml_doc->createElement("svg:rect"));
        }

        nodes[0]->setAttribute("fill", "#ffff00ff");
        nodes[0]->setAttribute("stroke", "#6c7ad2ff");
        nodes[1]->setAttribute("fill", "#6c7ad2ff");
        nodes[1]->setAttribute("stroke", "#ffff00ff");
        nodes[2]->setAttribute("fill", "#ff00d4ff");
        nodes[2]->setAttribute("stroke", "#ff00d4ff");
        nodes[3]->setAttribute("fill", "#ff0000ff");
        nodes[3]->setAttribute("stroke", "#ff70ffff");
        nodes[4]->setAttribute("fill", "#00ff00ff");
        nodes[4]->setAttribute("stroke", "#ba6cd2ff");
        nodes[5]->setAttribute("fill", "url(#test-gradient)");

        _doc->getDefs()->getRepr()->appendChild(grad_node);

        for (int i = 0; i < 6; i++) {
            _doc->getRoot()->getRepr()->appendChild(nodes[i]);
            vector.push_back(_doc->getObjectByRepr(nodes[i]));
        }

        set = collect_colours(vector);
    }

    ~ObjectColorSetFixture()
    {
        TearDownTestCase();
    }
};

TEST(ObjectColorSet, HandleEmptyObjects)
{
    EXPECT_TRUE(collect_colours({}).isColorsEmpty());
}

TEST(ObjectColorSet, HandleNullObjects)
{
    EXPECT_TRUE(collect_colours({nullptr}).isColorsEmpty());
}

TEST_F(ObjectColorSetFixture, PopulateAndFindColor)
{
    EXPECT_FALSE(set.isColorsEmpty());
    EXPECT_FALSE(set.isGradientStopsEmpty());
    EXPECT_EQ(set.getColors().size(), 9);

    auto key = Colors::Color(0xffff00ff).toRGBA();

    EXPECT_EQ(set.getColorIndex(key), 0);
    EXPECT_EQ(set.getColor(0).value().toRGBA(), key);

    auto false_key = Colors::Color(0x000000ff).toRGBA();
    EXPECT_EQ(set.getColorIndex(false_key), -1);
}

TEST_F(ObjectColorSetFixture, ClearData)
{
    EXPECT_EQ(set.getColors().size(), 9);
    set.clearData();
    EXPECT_TRUE(set.isColorsEmpty());
    EXPECT_TRUE(set.isGradientStopsEmpty());
}

TEST_F(ObjectColorSetFixture, SetAndGetSelectedColors)
{
    Colors::Color new_color(0xff00ffff);
    auto key = Colors::Color(0xffff00ff).toRGBA();
    set.setSelectedNewColor(key, new_color);
    EXPECT_EQ(set.getSelectedNewColor(key).value().toRGBA(), new_color.toRGBA());
}

TEST_F(ObjectColorSetFixture, SetSelectedNewColors)
{
    std::vector<Colors::Color> colors{Colors::Color(Colors::Space::Type::CMYK, {0.1, 0.8, 0.0, 0.0}),
                              Colors::Color(0xff0000ff),
                              Colors::Color(0x00ff00ff),
                              Colors::Color(0x0000ffff),
                              Colors::Color(0x7e1a9cff),
                              Colors::Color(Colors::Space::Type::HSLUV, {120.0, 100.0, 50.0}),
                              Colors::Color(Colors::Space::Type::HSL, {0.33, 1.0, 0.5}),
                              Colors::Color(Colors::Space::Type::HSV, {0.66, 1.0, 1.0}),
                              Colors::Color(Colors::Space::Type::LAB, {60.0, -40.0, 30.0})};
    set.setSelectedNewColor(colors);
    std::vector<uint32_t> new_colors;
    for (auto [key, value] : set.getSelectedColorsMap()) {
        new_colors.push_back(value.second.value().new_color.toRGBA());
    }
    std::vector<uint32_t> color{
                                Colors::Color(Colors::Space::Type::CMYK, {0.1, 0.8, 0.0, 0.0}).toRGBA(),
                                Colors::Color(0xff0000ff).toRGBA(),
                                Colors::Color(0x00ff00ff).toRGBA(),
                                Colors::Color(0x0000ffff).toRGBA(),
                                Colors::Color(0x7e1a9cff).toRGBA(),
                                Colors::Color(Colors::Space::Type::HSLUV, {120.0, 100.0, 50.0}).toRGBA(),
                                Colors::Color(Colors::Space::Type::HSL, {0.33, 1.0, 0.5}).toRGBA(),
                                Colors::Color(Colors::Space::Type::HSV, {0.66, 1.0, 1.0}).toRGBA(),
                                Colors::Color(Colors::Space::Type::LAB, {60.0, -40.0, 30.0}).toRGBA()};
    sort(color.begin(), color.end());
    sort(new_colors.begin(), new_colors.end());
    EXPECT_EQ(color, new_colors);

    set.convertToRecoloredColors();
    EXPECT_EQ(vector[0]->style->fill.getColor().toRGBA(), Colors::Color(Colors::Space::Type::CMYK, {0.1, 0.8, 0.0, 0.0}).toRGBA());
    char const *style1 = nodes[0]->attribute("style");
    EXPECT_NE(strstr(style1, "fill:device-cmyk(0.1 0.8 0 0)"), nullptr);

    // test reseting just object color without reseting it in the map for livepreview purposes 
    set.revertToOriginalColors(false);
    EXPECT_EQ(vector[0]->style->fill.getColor().toRGBA(), Colors::Color(0xffff00ff).toRGBA());
    char const *style2 = nodes[0]->attribute("style");
    EXPECT_NE(strstr(style2, "fill:#ffff00ff"), nullptr);
    auto map1 = set.getSelectedColorsMap();
    auto value1 = map1[Colors::Color(0xffff00ff).toRGBA()].second.value().new_color.toRGBA();
    EXPECT_NE(value1, Colors::Color(0xffff00ff).toRGBA());

    // test with resetting map entry for reset button
    set.revertToOriginalColors(true);
    auto map2 = set.getSelectedColorsMap();
    auto value2 = map2[Colors::Color(0xffff00ff).toRGBA()].second.value().new_color.toRGBA();
    EXPECT_EQ(value2, Colors::Color(0xffff00ff).toRGBA());
}

TEST_F(ObjectColorSetFixture, ChangeObjectsColors)
{
    EXPECT_FALSE(set.isColorsEmpty());

    EXPECT_FALSE(set.isGradientStopsEmpty());
    EXPECT_EQ(set.getColors().size(), 9);

    set.applyNewColorToSelection(Colors::Color(0xffff00ff).toRGBA(), Colors::Color(0x7e1a9cff));
    EXPECT_EQ(vector[0]->style->fill.getColor().toRGBA(), Colors::Color(0x7e1a9cff).toRGBA());
    EXPECT_EQ(vector[1]->style->stroke.getColor().toRGBA(), Colors::Color(0x7e1a9cff).toRGBA());

    char const *style1 = nodes[0]->attribute("style");
    char const *style2 = nodes[1]->attribute("style");
    EXPECT_NE(strstr(style1, "fill:#7e1a9cff"), nullptr);
    EXPECT_NE(strstr(style2, "stroke:#7e1a9cff"), nullptr);
}

TEST_F(ObjectColorSetFixture, HandleLargeColorSets)
{
    set.clearData();
    Inkscape::XML::Document *xml_doc = _doc->getReprDoc();
    std::vector<SPObject *> large_vector;
    
    for (int i = 0; i < 100000; ++i) {
        Inkscape::XML::Node *rect = xml_doc->createElement("svg:rect");
        char color_str[16];
        snprintf(color_str, sizeof(color_str), "#%06xff", i % 0xFFFFFF);
        rect->setAttribute("fill", color_str);
        
        _doc->getDefs()->getRepr()->appendChild(rect);
        large_vector.push_back(_doc->getObjectByRepr(rect));
    }
    
    set = collect_colours(large_vector);
    EXPECT_EQ(set.getColors().size(), 100000);
}

TEST_F(ObjectColorSetFixture, TestColorIndexBoundaryConditions)
{
    EXPECT_EQ(set.getColor(-1), std::nullopt);
    EXPECT_EQ(set.getColor(set.getColors().size()), std::nullopt);
    EXPECT_EQ(set.getColorIndex(0x99999999), -1);
}

TEST_F(ObjectColorSetFixture, TestColorApplicationFailure)
{
    auto false_key = Colors::Color(0x99999999).toRGBA();
    EXPECT_FALSE(set.applyNewColorToSelection(false_key, Colors::Color(0xff0000ff)));
}

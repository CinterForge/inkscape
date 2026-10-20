// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Test the multi-page functionality of Inkscape
 *
 * Authors:
 *   Martin Owens
 *
 * Copyright (C) 2025 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtest/gtest.h>

#include <src/document.h>
#include <src/inkscape.h>
#include <src/object/sp-page.h>
#include <src/object/sp-rect.h>
#include <src/page-manager.h>

#include "geom-predicates.h"

using namespace Inkscape;
using namespace Inkscape::XML;
using namespace std::literals;

class MultiPageTest : public ::testing::Test {
public:
    static void SetUpTestCase() {
        Inkscape::Application::create(false);
    }

    void SetUp() override {
        constexpr auto docString = R"A(<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<svg width="100mm" height="100mm" viewBox="0 0 100 100" version="1.1" id="svg1" xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape" xmlns="http://www.w3.org/2000/svg">
  <defs>
    <view viewBox="0 0 100 100" id="luz"/>
    <view viewBox="-100 200 10 190" id="amity"/>
  </defs>
  <g inkscape:groupmode="layer" id="layer1" transform="translate(100, 100)">
    <rect id="rect1" x="-100" y="-100" width="50" height="50" fill="red"/>
    <rect id="rect2" x="-200" y="145" width="5" height="95" fill="green"/>
  </g>
</svg>)A"sv;
        doc = SPDocument::createNewDocFromMem(docString);

        ASSERT_TRUE(doc);
        ASSERT_TRUE(doc->getRoot());

        doc->ensureUpToDate();
    }

    std::unique_ptr<SPDocument> doc;
};

TEST_F(MultiPageTest, preserveId)
{
    auto &pm = doc->getPageManager();

    EXPECT_EQ(std::string(pm.getPage(0)->getId()), "luz");
    EXPECT_EQ(std::string(pm.getPage(1)->getId()), "amity");
}

TEST_F(MultiPageTest, doNotVacuum)
{
    doc->vacuumDocument();

    auto &pm = doc->getPageManager();

    ASSERT_EQ(pm.getPageCount(), 2);
    EXPECT_EQ(std::string(pm.getPage(0)->getId()), "luz");
    EXPECT_EQ(std::string(pm.getPage(1)->getId()), "amity");
}

TEST_F(MultiPageTest, copyDocument)
{
    auto copy = doc->copy();
    auto &pm = copy->getPageManager();

    EXPECT_EQ(std::string(pm.getPage(0)->getId()), "luz");
    EXPECT_EQ(std::string(pm.getPage(1)->getId()), "amity");
}

TEST_F(MultiPageTest, swapPages)
{
    auto &pm = doc->getPageManager();
    auto page1 = pm.getPage(0);
    auto page2 = pm.getPage(1);
    auto rect1 = cast<SPRect>(doc->getObjectById("rect1"));
    auto rect2 = cast<SPRect>(doc->getObjectById("rect2"));

    const double eps = 0.01;

    EXPECT_RECT_NEAR(page1->getRect(), Geom::Rect(0, 0, 100, 100), eps);
    EXPECT_RECT_NEAR(page2->getRect(), Geom::Rect(-100, 200, -90, 390), eps);
    EXPECT_RECT_NEAR(*rect1->geometricBounds(), Geom::Rect(-100, -100, -50, -50), eps);
    EXPECT_RECT_NEAR(*rect2->geometricBounds(), Geom::Rect(-200, 145, -195, 240), eps);
    EXPECT_TRUE(page1->itemOnPage(rect1));
    EXPECT_TRUE(page2->itemOnPage(rect2));
    EXPECT_TRUE(page1->isViewportPage());
    EXPECT_FALSE(page2->isViewportPage());

    page1->swapPage(page2, true);
    // This causes the viewport page to be resized if it's incorrectly positioned.
    doc->ensureUpToDate();

    EXPECT_RECT_NEAR(page1->getRect(), Geom::Rect(-100, 200, 0, 300), eps);
    EXPECT_RECT_NEAR(page2->getRect(), Geom::Rect(0, 0, 10, 190), eps);
    EXPECT_RECT_NEAR(*rect1->geometricBounds(), Geom::Rect(-200, 100, -150, 150), eps);
    EXPECT_RECT_NEAR(*rect2->geometricBounds(), Geom::Rect(-100, -55, -95, 40), eps);
    EXPECT_FALSE(page1->isViewportPage());
    EXPECT_TRUE(page2->isViewportPage());
    
    page1->swapPage(page2, true);
    doc->ensureUpToDate();

    EXPECT_RECT_NEAR(page1->getRect(), Geom::Rect(0, 0, 100, 100), eps);
    EXPECT_RECT_NEAR(page2->getRect(), Geom::Rect(-100, 200, -90, 390), eps);
    EXPECT_RECT_NEAR(*rect1->geometricBounds(), Geom::Rect(-100, -100, -50, -50), eps);
    EXPECT_RECT_NEAR(*rect2->geometricBounds(), Geom::Rect(-200, 145, -195, 240), eps);
    EXPECT_TRUE(page1->isViewportPage());
    EXPECT_FALSE(page2->isViewportPage());
}

class OldPageTest : public ::testing::Test {
public:
    static void SetUpTestCase() {
        Inkscape::Application::create(false);
    }

    void SetUp() override {
        constexpr auto docString = R"A(<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<svg width="100mm" height="100mm" viewBox="0 0 100 100" version="1.1" id="svg1" xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape" xmlns:sodipodi="http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd" xmlns="http://www.w3.org/2000/svg" inkscape:version="1.4">
  <sodipodi:namedview id="nv1">
    <inkscape:page x="0" y="0" width="100" height="100" id="willow"/>
    <inkscape:page x="-100" y="200" width="10" height="190" id="gus"/>
  </sodipodi:namedview>
  <g inkscape:groupmode="layer" id="layer1" transform="translate(100, 100)">
    <rect id="rect1" x="-100" y="-100" width="50" height="50" fill="red"/>
    <rect id="rect2" x="-200" y="145" width="5" height="95" fill="green"/>
  </g>
</svg>)A"sv;
        doc = SPDocument::createNewDocFromMem(docString);

        ASSERT_TRUE(doc);
        ASSERT_TRUE(doc->getRoot());
    }

    std::unique_ptr<SPDocument> doc;
};

TEST_F(OldPageTest, oldPagesTransitioned)
{
    doc->ensureUpToDate();

    auto &pm = doc->getPageManager();
    // Pages transformed
    ASSERT_EQ(pm.getPageCount(), 2);
    ASSERT_EQ(doc->getNamedView()->children.size(), 0);

    EXPECT_EQ(std::string(pm.getPage(0)->getId()), "willow");
    EXPECT_EQ(std::string(pm.getPage(1)->getId()), "gus");
}

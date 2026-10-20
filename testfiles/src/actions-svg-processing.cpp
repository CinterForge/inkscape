// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests migrated from cxxtest
 *
 * Authors:
 *   Martin Owens
 *
 * Copyright (C) 2024 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtest/gtest.h>

#include "src/actions/actions-helper.h"
#include "src/document.h"
#include "src/inkscape.h"
#include "src/object/sp-root.h"
#include "src/object/sp-object.h"

#include "geom-predicates.h"

using namespace Inkscape;
using namespace Inkscape::XML;
using namespace std::literals;

class ObjectLinksTest : public ::testing::Test {
public:
    static void SetUpTestCase() {
        Inkscape::Application::create(false);
    }

    void SetUp() override {
        constexpr auto docString = R"A(<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<svg width="62.256149mm" height="55.27673mm" viewBox="0 0 62.256149 55.27673" version="1.1" id="svg1" inkscape:version="1.3.2 (1:1.3.2+202311252150+091e20ef0f)" sodipodi:docname="g.svg" xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape" xmlns:sodipodi="http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd" xmlns="http://www.w3.org/2000/svg" xmlns:svg="http://www.w3.org/2000/svg">
  <sodipodi:namedview id="namedview1" pagecolor="#ffffff" bordercolor="#000000" borderopacity="0.25" inkscape:showpageshadow="2" inkscape:pageopacity="0.0" inkscape:pagecheckerboard="0" inkscape:deskcolor="#d1d1d1" inkscape:document-units="mm" inkscape:zoom="2.7086912" inkscape:cx="246.79816" inkscape:cy="174.62308" inkscape:window-width="2560" inkscape:window-height="1295" inkscape:window-x="0" inkscape:window-y="32" inkscape:window-maximized="1" inkscape:current-layer="layer1" />
  <defs id="defs1">
    <clipPath clipPathUnits="userSpaceOnUse" id="clipPath2">
      <rect style="fill:#241f31;stroke-width:0.7;stroke-linejoin:round" id="rect3" width="61.610233" height="54.703255" x="16.41909" y="45.084824" transform="rotate(-18.396241)" />
    </clipPath>
    <clipPath clipPathUnits="userSpaceOnUse" id="clipPath5">
      <rect style="fill:#241f31;stroke-width:0.7;stroke-linejoin:round" id="rect6" width="61.610233" height="54.703255" x="16.41909" y="45.084824" transform="rotate(-18.396241)" />
    </clipPath>
  </defs>
  <g inkscape:label="Layer 1" inkscape:groupmode="layer" id="layer1" transform="translate(-31.562698,-31.792045)">
    <path id="rect1" clip-path="url(#clipPath2)" style="fill:#a51d2d;stroke-width:0.7;stroke-linejoin:round" transform="rotate(5.1388646)" d="M 36.865765,26.479748 H 98.475998 V 81.183002 H 36.865765 Z" />
    <path id="rect4" clip-path="url(#clipPath5)" style="fill:#a51d2d;stroke-width:0.7;stroke-linejoin:round" transform="rotate(-5.1388646)" d="M 36.865765,26.479748 H 98.475998 V 81.183002 H 36.865765 Z" />
    <path d="m 88.414007,34.281815 -27.684619,-2.48977 -27.05571,6.37429 -2.11098,23.47298 5.40432,22.93969 27.68461,2.48977 27.056229,-6.37429 2.11099,-23.47299 z" style="mix-blend-mode:difference;fill:#a51d2d;fill-opacity:1;stroke-width:0.7;stroke-linejoin:round" id="path5" />
    <g id="g14" transform="translate(-69.610709,-8.3331963)" style="fill:#f6f5f4">
      <g id="g9" transform="translate(-3.2234202,-4.2002143)" style="fill:#f6f5f4">
        <g id="g6" style="fill:#f6f5f4">
          <path id="path6" style="stroke-width:0.7;stroke-linejoin:round" d="m 128.36298,61.92614 a 3.9560158,4.1513743 0 0 1 -3.95601,4.151374 3.9560158,4.1513743 0 0 1 -3.95602,-4.151374 3.9560158,4.1513743 0 0 1 3.95602,-4.151375 3.9560158,4.1513743 0 0 1 3.95601,4.151375 z" />
          <path id="ellipse6" style="stroke-width:0.7;stroke-linejoin:round" d="m 150.73156,62.512215 a 3.9560158,4.1513743 0 0 1 -3.95602,4.151374 3.9560158,4.1513743 0 0 1 -3.95601,-4.151374 3.9560158,4.1513743 0 0 1 3.95601,-4.151375 3.9560158,4.1513743 0 0 1 3.95602,4.151375 z" />
        </g>
        <g id="g8" transform="translate(0.0976794,20.610354)" style="fill:#f6f5f4">
          <path id="ellipse7" style="stroke-width:0.7;stroke-linejoin:round" transform="scale(0.9)" d="m 128.36298,61.92614 a 3.9560158,4.1513743 0 0 1 -3.95601,4.151374 3.9560158,4.1513743 0 0 1 -3.95602,-4.151374 3.9560158,4.1513743 0 0 1 3.95602,-4.151375 3.9560158,4.1513743 0 0 1 3.95601,4.151375 z" />
          <path id="ellipse8" style="stroke-width:0.7;stroke-linejoin:round" transform="scale(1.1)" d="m 150.73156,62.512215 a 3.9560158,4.1513743 0 0 1 -3.95602,4.151374 3.9560158,4.1513743 0 0 1 -3.95601,-4.151374 3.9560158,4.1513743 0 0 1 3.95601,-4.151375 3.9560158,4.1513743 0 0 1 3.95602,4.151375 z" />
        </g>
      </g>
    </g>
  </g>
</svg>)A"sv;
        doc = SPDocument::createNewDocFromMem(docString);

        ASSERT_TRUE(doc);
        ASSERT_TRUE(doc->getRoot());
    }

    std::vector<SPObject *> getObjects(std::vector<std::string> const &lst) {
        std::vector<SPObject *> ret;
        for (auto &id : lst) {
            ret.push_back(doc->getObjectById(id));
        }
        return ret;
    }

    std::unique_ptr<SPDocument> doc;
};

TEST_F(ObjectLinksTest, removeTransforms)
{
    doc->ensureUpToDate();

    std::vector<std::string> watch = {"rect1", "rect4", "path5", "g14", "g9", "g6", "path6", "ellipse7", "g8"};
    std::map<std::string, Geom::Rect> boxes;

    for (auto id : watch) {
        auto item = cast<SPItem>(doc->getObjectById(id));
        ASSERT_TRUE(item) << " item by id '" << id << "'";
        auto obox = item->documentGeometricBounds();
        ASSERT_TRUE(obox) << " item bounds '" << id << "'";
        boxes[id] = *obox;
    }

    doc->getActionGroup()->activate_action("remove-all-transforms");
    doc->ensureUpToDate();

    for (auto id : watch) {
        auto item = cast<SPItem>(doc->getObjectById(id));
        ASSERT_TRUE(item) << " item by id '" << id << "'";
        ASSERT_FALSE(item->getAttribute("transform")) << " item transform '" << id << "'";
        auto old_box = boxes[id];
        auto new_box = item->documentGeometricBounds();
        ASSERT_TRUE(new_box) << " item bounds '" << id << "'";
        EXPECT_RECT_NEAR(old_box, *new_box, 0.01) << "id";
    }
}

TEST(ActionsTest, to_string_for_actions)
{
    ASSERT_EQ(to_string_for_actions(90.0), "90.0");
    ASSERT_EQ(to_string_for_actions(-90.0), "-90.0");
}

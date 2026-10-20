// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * SPGroup test
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2024 Authors
 *
 * Released under GNU GPL version 2 or later, read the file 'COPYING' for more information
 */

#include <gtest/gtest.h>

#include <2geom/pathvector.h>

#include "document.h"
#include "inkscape.h"
#include "object/sp-item.h"
#include "svg/svg.h"

using namespace Inkscape;
using namespace std::literals;

class SPItemTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // setup hidden dependency
        Application::create(false);
    }
};

TEST_F(SPItemTest, getClipPathVector)
{
    constexpr auto svg = R"""(<?xml version="1.0"?>
<svg width="100" height="100">
  <defs id="defs1">
    <clipPath clipPathUnits="userSpaceOnUse" id="clipPath1">
      <rect id="cliprect1" width="34.33456" height="33.829079" x="13.165109" y="13.165109" transform="translate(10,10)" />
    </clipPath>
    <clipPath clipPathUnits="userSpaceOnUse" id="clipPath2">
      <rect id="cliprect2" width="33.794209" height="33.794209" x="0" y="0" transform="translate(10,10)" />
    </clipPath>
    <clipPath clipPathUnits="userSpaceOnUse" id="clipPath3">
      <rect id="cliprect3" width="30.837675" height="30.837675" x="0" y="0" transform="translate(10,10)" />
    </clipPath>
  </defs>
  <g id="group1" transform="translate(10,10)" clip-path="url(#clipPath1)">
    <g id="group2" transform="translate(10,10)" clip-path="url(#clipPath2)">
      <rect id="rect1" x="0" y="0" width="50" height="50" clip-path="url(#clipPath3)" style="fill: blue" />
    </g>
  </g>
  <g id="group3" transform="translate(-10,-10)" clip-path="url(#clipPath1)">
    <rect id="rect2" x="0" y="0" width="50" height="50" style="fill: red" />
  </g>
</svg>)"""sv;

    auto doc = SPDocument::createNewDocFromMem(svg);

    // This has to be run or all the path vectors are empty.
    doc->ensureUpToDate();

    // Item with no clip.
    auto no_item = cast<SPItem>(doc->getObjectById("rect2"));
    ASSERT_FALSE(no_item->getClipPathVector().has_value());
    
    auto parent = cast<SPItem>(doc->getObjectById("group3"));
    auto pathv1 = no_item->getClipPathVector(parent);
    auto pathv2 = parent->getClipPathVector();
    ASSERT_FALSE(pathv1->empty());
    ASSERT_FALSE(pathv2->empty());
    ASSERT_EQ(sp_svg_write_path(*pathv1), sp_svg_write_path(*pathv2));

    auto r_item = cast<SPItem>(doc->getObjectById("rect1"));
    auto pathv3 = r_item->getClipPathVector();
    ASSERT_EQ(sp_svg_write_path(*pathv3), "M 10,10 H 40.837675 V 40.837675 H 10 Z");

    auto r_parent = cast<SPItem>(doc->getObjectById("group1"));
    auto pathv4 = r_item->getClipPathVector(r_parent);
    ASSERT_EQ(sp_svg_write_path(*pathv4), "M 13.16601563,13.16601563 V 40.83789062 H 40.83789062 V 13.16601563 Z");
}

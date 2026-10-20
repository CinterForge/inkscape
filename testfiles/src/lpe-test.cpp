// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * LPE tests
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2020 Authors
 *
 * Released under GNU GPL version 2 or later, read the file 'COPYING' for more information
 */

#include <gtest/gtest.h>
#include <src/document-update.h>
#include <src/document.h>
#include <src/inkscape.h>
#include <src/live_effects/lpe-bool.h>
#include <src/live_effects/lpe-tiling.h>
#include <src/object/sp-ellipse.h>
#include <src/object/sp-lpe-item.h>
#include <testfiles/lpespaths-test.h>

using namespace Inkscape;
using namespace Inkscape::LivePathEffect;
using namespace std::literals;

class LPETest : public LPESPathsTest {
public:
    void run() {
        testDoc(svg);
    }
};

// A) FILE BASED TESTS
TEST_F(LPETest, Inkscape_0_92) { run(); }
TEST_F(LPETest, Inkscape_1_0)  { run(); }
TEST_F(LPETest, Inkscape_1_1)  { run(); }
TEST_F(LPETest, Inkscape_1_2)  { run(); }
TEST_F(LPETest, Inkscape_1_3)  { run(); }
// B) CUSTOM TESTS
// BOOL LPE
TEST_F(LPETest, Bool_canBeApplyedToNonSiblingPaths)
{
    constexpr auto svg = R"A(
<svg width='100' height='100'
  xmlns:sodipodi='http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd'
  xmlns:inkscape='http://www.inkscape.org/namespaces/inkscape'>
  <defs>
    <inkscape:path-effect
      id='path-effect1'
      effect='bool_op'
      operation='diff'
      operand-path='#circle1'
      lpeversion='1'
      hide-linked='true' />
  </defs>
  <path id='rect1'
    inkscape:path-effect='#path-effect1'
    sodipodi:type='rect'
    width='100' height='100' fill='#ff0000' />
  <g id='group1'>
    <circle id='circle1'
      r='40' cy='50' cx='50' fill='#ffffff' style='display:inline'/>
  </g>
</svg>)A"sv;

    auto doc = SPDocument::createNewDocFromMem(svg);
    doc->ensureUpToDate();

    auto lpe_item = cast<SPLPEItem>(doc->getObjectById("rect1"));
    ASSERT_TRUE(lpe_item);

    auto lpe_bool_op_effect = dynamic_cast<LPEBool *>(lpe_item->getFirstPathEffectOfType(EffectType::BOOL_OP));
    ASSERT_TRUE(lpe_bool_op_effect);

    auto operand_path = lpe_bool_op_effect->getParameter("operand-path")->param_getSVGValue();
    auto circle = cast<SPGenericEllipse>(doc->getObjectById(operand_path.substr(1)));
    ASSERT_TRUE(circle);
}

// TILING LPE
TEST_F(LPETest, Tiling_preservesGapValuesOnLoadWithNonPxUnit)
{
    // Verify fix for https://gitlab.com/inkscape/inkscape/-/work_items/5940
    constexpr auto svg = R"A(
<svg width='200' height='200'
  xmlns:sodipodi='http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd'
  xmlns:inkscape='http://www.inkscape.org/namespaces/inkscape'>
  <defs>
    <inkscape:path-effect
      id='path-effect1'
      effect='tiling'
      lpeversion='1.3.1'
      unit='mm'
      num_rows='2'
      num_cols='2'
      gapx='10'
      gapy='5'
      offset='0'
      offset_type='false'
      scale='0'
      rotate='0'
      mirrorrowsx='false'
      mirrorrowsy='false'
      mirrorcolsx='false'
      mirrorcolsy='false'
      mirrortrans='false'
      shrink_interp='true'
      random_scale='false'
      random_rotate='false'
      random_gap_x='false'
      random_gap_y='false'
      seed='1;1'
      lpesatellites='' />
  </defs>
  <path id='path1'
    inkscape:path-effect='#path-effect1'
    d='M 10,10 H 50 V 50 H 10 Z' />
</svg>)A"sv;

    auto doc = SPDocument::createNewDocFromMem(svg);
    ASSERT_TRUE(doc != nullptr);

    sp_file_fix_lpe(doc.get());
    doc->ensureUpToDate();

    auto lpe_item = cast<SPLPEItem>(doc->getObjectById("path1"));
    ASSERT_TRUE(lpe_item);

    sp_lpe_item_update_patheffect(lpe_item, true, true, true);

    auto tiling = dynamic_cast<LPETiling *>(lpe_item->getFirstPathEffectOfType(EffectType::TILING));
    ASSERT_TRUE(tiling);

    auto gapx_str = tiling->getParameter("gapx")->param_getSVGValue();
    auto gapy_str = tiling->getParameter("gapy")->param_getSVGValue();
    EXPECT_NEAR(std::stod(gapx_str.raw()), 10.0, 1e-6);
    EXPECT_NEAR(std::stod(gapy_str.raw()), 5.0, 1e-6);
}

// TILING LPE - unit conversion after load
TEST_F(LPETest, Tiling_convertsGapsCorrectlyWhenUnitChangedAfterLoad)
{
    // Load document with unit="mm", change to unit="in" and verify.
    constexpr auto svg = R"A(
<svg width='200' height='200'
  xmlns:sodipodi='http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd'
  xmlns:inkscape='http://www.inkscape.org/namespaces/inkscape'>
  <defs>
    <inkscape:path-effect
      id='path-effect1'
      effect='tiling'
      lpeversion='1.3.1'
      unit='mm'
      num_rows='2'
      num_cols='2'
      gapx='25.4'
      gapy='12.7'
      offset='0'
      offset_type='false'
      scale='0'
      rotate='0'
      mirrorrowsx='false'
      mirrorrowsy='false'
      mirrorcolsx='false'
      mirrorcolsy='false'
      mirrortrans='false'
      shrink_interp='true'
      random_scale='false'
      random_rotate='false'
      random_gap_x='false'
      random_gap_y='false'
      seed='1;1'
      lpesatellites='' />
  </defs>
  <path id='path1'
    inkscape:path-effect='#path-effect1'
    d='M 10,10 H 50 V 50 H 10 Z' />
</svg>)A"sv;

    auto doc = SPDocument::createNewDocFromMem(svg);
    ASSERT_TRUE(doc != nullptr);

    sp_file_fix_lpe(doc.get());
    doc->ensureUpToDate();

    auto lpe_item = cast<SPLPEItem>(doc->getObjectById("path1"));
    ASSERT_TRUE(lpe_item);

    sp_lpe_item_update_patheffect(lpe_item, true, true, true);

    auto tiling = dynamic_cast<LPETiling *>(lpe_item->getFirstPathEffectOfType(EffectType::TILING));
    ASSERT_TRUE(tiling);

    // Confirm gaps loaded correctly in mm.
    auto gapx_str = tiling->getParameter("gapx")->param_getSVGValue();
    auto gapy_str = tiling->getParameter("gapy")->param_getSVGValue();
    ASSERT_NEAR(std::stod(gapx_str.raw()), 25.4, 1e-6);
    ASSERT_NEAR(std::stod(gapy_str.raw()), 12.7, 1e-6);

    // Change unit to inches; the next update should convert 25.4 mm→1.0 in and 12.7 mm→0.5 in.
    // prev_unit is "mm" (set during the is_load pass), so the conversion uses the correct source unit.
    tiling->setParameter("unit", "in");
    sp_lpe_item_update_patheffect(lpe_item, true, true, true);

    gapx_str = tiling->getParameter("gapx")->param_getSVGValue();
    gapy_str = tiling->getParameter("gapy")->param_getSVGValue();
    EXPECT_NEAR(std::stod(gapx_str.raw()), 1.0, 1e-6);
    EXPECT_NEAR(std::stod(gapy_str.raw()), 0.5, 1e-6);
}

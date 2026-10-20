// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Tests for Style internal classes
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2020 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include <gtest/gtest.h>
#include <src/style-internal.h>

#include "colors/cms/system.h"
#include "colors/document-cms.h"
#include "colors/spaces/cms.h"
#include "document.h"
#include "inkscape.h"
#include "style.h"

TEST(StyleInternalTest, testSPIDashArrayInequality)
{
	SPIDashArray array;
	array.read("0 1 2 3");
	SPIDashArray subsetArray;
	subsetArray.read("0 1");
	
	ASSERT_FALSE(array == subsetArray);
	ASSERT_FALSE(subsetArray == array);
}

TEST(StyleInternalTest, testSPIDashArrayEquality)
{
	SPIDashArray anArray;
	anArray.read("0 1 2 3");
	SPIDashArray sameArray;
	sameArray.read("0 1 2 3");
	
	ASSERT_TRUE(anArray == sameArray);
	ASSERT_TRUE(sameArray == anArray);
}

TEST(StyleInternalTest, testSPIDashArrayValidity)
{
    // valid dash arrays
	SPIDashArray array10;
	array10.read("");

	SPIDashArray array11;
	array11.read("0");

	SPIDashArray array12;
	array12.read("0 1e3");

    // invalid dash arrayas
	SPIDashArray array20;
	array20.read("1-1");

	SPIDashArray array21;
	array21.read("10 10 -10");

	SPIDashArray array22;
	array22.read("-1");

	SPIDashArray array23;
	array23.read("0 -5e3");


    EXPECT_TRUE(array10.is_valid());
    EXPECT_TRUE(array11.is_valid());
    EXPECT_TRUE(array12.is_valid());

    // SPIDashArray::read is geared towards happy path, so it may reject negative entries:

    // EXPECT_FALSE(array20.is_valid()); // cannot read "1-1" as numbers, so 0
    EXPECT_FALSE(array21.is_valid());
    // EXPECT_FALSE(array22.is_valid()); // lone negative number is deemed invalid and removed by 'read'
    // EXPECT_FALSE(array23.is_valid()); // negative total: invalid and removed by 'read'
}

TEST(StyleInternalTest, testSPIPaint)
{
    SPIPaint paint;
    EXPECT_EQ(paint.get_value(), "");
    paint.read("red");
    EXPECT_EQ(paint.get_value(), "red");
    paint.clear();
    EXPECT_EQ(paint.get_value(), "");
}

class SPIColorInterpolationTest : public ::testing::Test {
protected:
    void SetUp() override {
        Inkscape::Application::create(false);

        auto &cms = Inkscape::Colors::CMS::System::get();
        cms.clearDirectoryPaths();
        std::string icc_dir = INKSCAPE_TESTS_DIR "/data/colors/";
        cms.addDirectoryPath(icc_dir, false);
        cms.refreshProfiles();

        std::string svg_objs_file = INKSCAPE_TESTS_DIR "/data/colors/cms-in-objs.svg";
        doc = SPDocument::createNewDoc(svg_objs_file.c_str());

        style = new SPStyle(doc.get());
    }

    void TearDown() override {
        delete style;
    }

    std::unique_ptr<SPDocument> doc;
    SPStyle* style;
};


TEST_F(SPIColorInterpolationTest, ReadsStandardGlobalProfile) {
    /* Global standard profiles */
    style->color_interpolation.read("sRGB");
    auto space_srgb = style->color_interpolation.getInterpolationSpace();

    ASSERT_NE(space_srgb, nullptr) << "Failed to load standard global profile: sRGB";
    EXPECT_EQ(space_srgb->getSvgName(), "sRGB");

    style->color_interpolation.read("linearRGB");
    auto space_linear = style->color_interpolation.getInterpolationSpace();

    ASSERT_NE(space_linear, nullptr) << "Failed to load standard global profile: linearRGB";
    EXPECT_EQ(space_linear->getSvgName(), "linearRGB");
}

TEST_F(SPIColorInterpolationTest, ReadsCustomDocumentProfile) {
    /* Document ICC Profiles */
    ASSERT_TRUE(doc->getDocumentCMS().getSpace("grb"))
        << "Test SVG failed to load the custom profile.";

    style->color_interpolation.read("grb");
    auto space_grb = style->color_interpolation.getInterpolationSpace();

    ASSERT_NE(space_grb, nullptr) << "Interpolation space is null!";
    EXPECT_EQ(space_grb->getSvgName(), "grb");

    ASSERT_TRUE(doc->getDocumentCMS().getSpace("cmyk-rcm"))
        << "Test SVG failed to load the custom profile.";

    style->color_interpolation.read("cmyk-rcm");
    auto space_rcm = style->color_interpolation.getInterpolationSpace();

    ASSERT_NE(space_rcm, nullptr) << "Interpolation space is null!";
    EXPECT_EQ(space_rcm->getSvgName(), "cmyk-rcm");
}

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

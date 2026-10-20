// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Test Inkscape::Extensions::Internal::PdfOutput
 */
/*
 * Authors:
 *   Charlotte Curtis
 *
 * Copyright (C) 2025 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtest/gtest.h>
#include <2geom/svg-path-writer.h>
#include <2geom/rect.h>
#include <GfxState.h>
#include <Gfx.h>
#include "extension/internal/pdfinput/pdf-utils.h"
#include "extension/internal/pdfinput/poppler-transition-api.h"
#include "svg/svg.h"

class PdfUtilsTest : public ::testing::Test
{
public:
    // Test vectors and comparePaths method copied from path-boolop-test.cpp
    Geom::PathVector const rectangle_bigger = sp_svg_read_pathv("M 0,0 L 0,2 L 2,2 L 2,0 z");
    Geom::PathVector const rectangle_smaller = sp_svg_read_pathv("M 0.5,0.5 L 0.5,1.5 L 1.5,1.5 L 1.5,0.5 z");
    Geom::PathVector const rectangle_outside = sp_svg_read_pathv("M 0,1.5 L 0.5,1.5 L 0.5,2.5 L 0,2.5 z");
    Geom::PathVector const empty = sp_svg_read_pathv("");
    
    // shapes to test fill rules
    Geom::PathVector const star = sp_svg_read_pathv("M 0,10 20,0 15,25 5,0 25,15 z");
    Geom::PathVector const star_odd_even =
        sp_svg_read_pathv("M 5 0 L 7.5 6.25 L 11 4.5 z M 11 4.5 L 18.04296875 9.783203125 L 20 0 z M 18.04296875 "
                          "9.783203125 L 17.30859375 13.4609375 L 25 15 z M 17.30859375 13.4609375 L 9.783203125 "
                          "11.95703125 L 15 25 z M 9.783203125 11.95703125 L 7.5 6.25 L 0 10 z");
    Geom::PathVector const star_non_zero =
        sp_svg_read_pathv("M 5 0 L 7.5 6.25 L 0 10 L 9.783203125 11.95703125 L 15 25 L 17.30859375 13.4609375 L 25 15 "
                          "L 18.04296875 9.783203125 L 20 0 L 11 4.5 z");
    Geom::PathVector const rectangle_star = sp_svg_read_pathv("M 0,0 L 0,25 L 25,25 L 25,0 z");

    PDFRectangle *page_bbox;
    GfxState *state;

    // Dynamic Poppler stuff
    void SetUp() override {
        // A sufficiently large fake page bounding box for Poppler state object use
        page_bbox = new PDFRectangle(0, 0, 30, 30);
        state = new _POPPLER_GFX_STATE(72, 72, *page_bbox, 0, false);
    }

    // Clean up
    void TearDown() override {
        delete page_bbox;
        delete state;
    }
    
    static void comparePaths(Geom::PathVector const &result, Geom::PathVector const &reference)
    {
        Geom::SVGPathWriter wr;
        wr.feed(result);
        auto const resultD = wr.str();
        wr.clear();
        wr.feed(reference);
        auto const referenceD = wr.str();
        EXPECT_EQ(resultD, referenceD);
        EXPECT_EQ(result, reference);
    }

    static void writeGfxState(GfxState *state, Geom::PathVector const &path) {
        for (const auto &path : path) {
            if (path.empty()) continue;

            // Start a new subpath with the first point
            auto startPoint = path.initialPoint();
            state->moveTo(startPoint.x(), startPoint.y());

            // Process each curve in the path
            for (const auto &curve : path) {
                if (curve.isLineSegment()) {
                    auto endPoint = curve.finalPoint();
                    state->lineTo(endPoint.x(), endPoint.y());
                } else {
                    // Lazy, but I'm only using straight line segments in this test program
                    std::cout << "Unsupported curve type" << std::endl;
                }
            }
            if (path.closed()) {
                state->closePath();
            }
        }
    }
};

// Tests for ClipHistoryEntry class
TEST_F(PdfUtilsTest, ClipHistoryEntryConstructor) {
    // Test default constructor (empty path)
    ClipHistoryEntry clip_history;
    EXPECT_FALSE(clip_history.hasClipPath());
    EXPECT_FALSE(clip_history.hasSaves());
    EXPECT_EQ(clip_history.getFillRule(), FillRule::fill_nonZero);
}

TEST_F(PdfUtilsTest, ClipHistoryEntryWithPath) {
    // Test constructor with path
    ClipHistoryEntry clip_history(rectangle_bigger, clipNormal);
    EXPECT_TRUE(clip_history.hasClipPath());
    EXPECT_FALSE(clip_history.hasSaves());
    EXPECT_EQ(clip_history.getFillRule(), FillRule::fill_nonZero);
    comparePaths(clip_history.getClipPath(), rectangle_bigger);
}

TEST_F(PdfUtilsTest, ClipHistoryEntrySaveRestore) {
    ClipHistoryEntry *clip_history = new ClipHistoryEntry(rectangle_bigger, clipNormal);
    
    // Save the current state
    ClipHistoryEntry *saved = clip_history->save();
    EXPECT_TRUE(saved->hasSaves());
    EXPECT_TRUE(saved->hasClipPath());
    EXPECT_TRUE(saved->isCopied());
    
    // Restore should return the original clip_history
    ClipHistoryEntry *restored = saved->restore();
    EXPECT_EQ(restored, clip_history);
    EXPECT_FALSE(restored->hasSaves());
    
    delete restored;
}

TEST_F(PdfUtilsTest, ClipHistoryEntrySetClipPathV) {
    ClipHistoryEntry *clip_history = new ClipHistoryEntry();
    // push another instance to the stack to call setClip
    clip_history = clip_history->save();
    clip_history->setClip(rectangle_bigger, FillRule::fill_oddEven);
    
    EXPECT_TRUE(clip_history->hasClipPath());
    EXPECT_EQ(clip_history->getFillRule(), FillRule::fill_oddEven);
    comparePaths(clip_history->getClipPath(), rectangle_bigger);
    
    delete clip_history;
}

TEST_F(PdfUtilsTest, ClipHistoryEntrySetClipGfxState) {
    ClipHistoryEntry *clip_history = new ClipHistoryEntry();
    clip_history = clip_history->save();    
    writeGfxState(state, rectangle_bigger);
    clip_history->setClip(state);
    EXPECT_TRUE(clip_history->hasClipPath());
    EXPECT_EQ(clip_history->getFillRule(), FillRule::fill_nonZero);
    comparePaths(clip_history->getClipPath(), rectangle_bigger);
    
    state->clearPath();
    delete clip_history;
}

// Tests for helper functions
TEST_F(PdfUtilsTest, GetRectFromPDFRectangle) {
    PDFRectangle pdf_rect = PDFRectangle(10, 20, 30, 40);
    Geom::Rect result = getRect(&pdf_rect);
    Geom::Rect expected(10.0, 20.0, 30.0, 40.0);
    
    EXPECT_EQ(result, expected);
}

TEST_F(PdfUtilsTest, MaybeIntersectBothEmpty) {
    auto result = maybeIntersect(empty, empty);
    comparePaths(result, empty);
}

TEST_F(PdfUtilsTest, MaybeIntersectOneEmpty) {
    // If first is empty, return second
    auto result1 = maybeIntersect(empty, rectangle_bigger);
    comparePaths(result1, rectangle_bigger);
    
    // If second is empty, return first
    auto result2 = maybeIntersect(rectangle_bigger, empty);
    comparePaths(result2, rectangle_bigger);
}

TEST_F(PdfUtilsTest, MaybeIntersectBothFilled) {
    // Test intersection of two overlapping rectangles
    auto result = maybeIntersect(rectangle_bigger, rectangle_smaller);
    comparePaths(result, rectangle_smaller);
}

TEST_F(PdfUtilsTest, MaybeIntersectSimpleDifferentFills) {
    // for these basic rectangles the fill rule shouldn't matter
    auto result = maybeIntersect(rectangle_bigger, rectangle_smaller, 
                                FillRule::fill_nonZero, FillRule::fill_oddEven);
    comparePaths(result, rectangle_smaller);
}

TEST_F(PdfUtilsTest, MaybeIntersectNoOverlap) {
    Geom::PathVector not_overlapping = sp_svg_read_pathv("M 2,2 L 2,3 L 3,3 L 3,2 z");
    auto result = maybeIntersect(rectangle_smaller, not_overlapping);
    // Non-overlapping rectangles should result in empty intersection
    EXPECT_TRUE(result.empty());
}


TEST_F(PdfUtilsTest, ClipHistoryEntryFlattenedClipPath) {
    ClipHistoryEntry *clip_history = new ClipHistoryEntry(rectangle_bigger, clipNormal);
    clip_history = clip_history->save();
    
    // The flattened path should be the same as the original when there's only one level
    comparePaths(clip_history->getFlattenedClipPath(), rectangle_bigger);
    
    delete clip_history;
}

TEST_F(PdfUtilsTest, ClipHistoryEntryFlattenedPathSimple) {
    // Test multiple levels of clipping
    ClipHistoryEntry *clip_history = new ClipHistoryEntry(rectangle_bigger, clipNormal);
    clip_history = clip_history->save();
    clip_history->setClip(rectangle_smaller, FillRule::fill_nonZero);
    
    // The flattened clip should be the same as the smaller rectangle
    auto flattened = clip_history->getFlattenedClipPath();
    comparePaths(flattened, rectangle_smaller);
    
    delete clip_history;
}

TEST_F(PdfUtilsTest, ClipHistoryEntryFlattenedPathSkipLevel) {
    // Test multiple levels of clipping
    ClipHistoryEntry *clip_history = new ClipHistoryEntry(rectangle_bigger, clipNormal);
    clip_history = clip_history->save();
    clip_history->clear();
    clip_history = clip_history->save();
    clip_history->setClip(rectangle_smaller, FillRule::fill_nonZero);

    // Still should be the same as the smaller rectangle
    auto flattened = clip_history->getFlattenedClipPath();
    comparePaths(flattened, rectangle_smaller);
        
    delete clip_history;
}

TEST_F(PdfUtilsTest, ClipHistoryEntryFlattenedOddEven) {
    ClipHistoryEntry *clip_history = new ClipHistoryEntry(star, clipEO);
    clip_history = clip_history->save();
    clip_history->setClip(rectangle_star, FillRule::fill_nonZero);
    auto result = clip_history->getFlattenedClipPath();
    comparePaths(result, star_odd_even);
    
    delete clip_history;
}

TEST_F(PdfUtilsTest, ClipHistoryEntryFlattenedOddEvenSkipLevel) {
    ClipHistoryEntry *clip_history = new ClipHistoryEntry(star, clipEO);
    clip_history = clip_history->save();
    clip_history->clear();
    clip_history = clip_history->save();
    clip_history->setClip(rectangle_star, FillRule::fill_nonZero);
    auto result = clip_history->getFlattenedClipPath();
    comparePaths(result, star_odd_even);
    
    delete clip_history;
}


TEST_F(PdfUtilsTest, ClipHistoryEntryStarIntersectionNonZero) {
    ClipHistoryEntry *clip_history = new ClipHistoryEntry(star, clipNormal);
    clip_history = clip_history->save();
    clip_history->setClip(rectangle_star, FillRule::fill_nonZero);
    auto result = clip_history->getFlattenedClipPath();
    comparePaths(result, star_non_zero);
    
    delete clip_history;
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

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests for Layout computations.
 *
 * Copyright (C) 2025 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtest/gtest.h>

#include "document.h"
#include "inkscape.h"
#include "object/sp-item.h"
#include "object/sp-text.h"
#include "svg/svg.h"

using namespace std::literals;

struct LayoutPositionTestData
{
    char const *id;
    int expected_glyphs;
    int expected_positions;
    std::vector<int> permutation = {};
    int expected_characters = -1;
    Geom::Dim2 direction = Geom::Dim2::X;
};

// clang-format off
LayoutPositionTestData const LIGATURE_CURSOR_TESTS[] = {
    {"id0", 3, 6},
    {"id1", 4, 7},
    {"id2", 4, 5, {4, 3, 2, 1, 0}},
    {"id3", 3, 5, {4, 3, 2, 1, 0}},
    {"id5_precheck", 5, 6, {5, 1, 3, 4, 1, 0}},
    {"id5", 5, 7, {6, 1, 3, 4, 5, 1, 0}},
    {"id6", 3, 5, {0, 2, 1, 2, 4}},
    {"id7", 3, 4, {}, 4},
    {"id8", 3, 4, {}, 5},
    {"id9", 2, 6, {}, -1, Geom::Dim2::Y},
    {"id10", 4, 6, {}, -1, Geom::Dim2::Y},
    {"id11", 2, 6, {}, -1, Geom::Dim2::Y},
};
// clang-format on

class LayoutTNGComputeTestFixture : public ::testing::TestWithParam<LayoutPositionTestData>
{
public:
    static void SetUpTestSuite()
    {
        // SPDocument currently depends on this
        Inkscape::Application::create(false);

        constexpr auto svg = R"""(<?xml version="1.0"?>
        <svg>
        <text id="id0">affib</text>
        <text id="id1" style="writing-mode: lr-tb;direction: rtl;">affibb</text><!-- english text within rtl text tag -->
        <text id="id2" style="writing-mode: lr-tb;direction: rtl;font-family: 'Noto Sans Arabic';">ﮎﮎﮎﮎ</text><!-- RTl text within RTL tag, no ligatures -->
        <text id="id3" style="writing-mode: lr-tb;direction: rtl;font-family: 'Noto Sans Arabic';font-variant-ligatures: discretionary-ligatures;">ﮎﻋﺞﮎ</text>

        <!-- Sanity check for text behavior on LTR inside RTL, no ligatures yet -->
        <text id="id5_precheck" style="writing-mode: lr-tb;direction: rtl;">צabcצ</text>
        <!-- Real test, LTR ligature inside RTL -->
        <text id="id5" style="writing-mode: lr-tb;direction: rtl;">צafiaצ</text>

        <!-- RTL ligature inside LTR text -->
        <text id="id6" style="writing-mode: lr-tb;direction: ltr;font-family: 'Noto Sans Arabic';font-variant-ligatures: discretionary-ligatures;">aﻋﺞa</text>

        <!-- Not every ligature should have cursor position in the middle  -->
        <text id="id7" style="font-family: 'Noto Sans'" >aǪa</text>
        <text id="id8" style="font-family: 'Noto Sans CJK JP'" >aᄀᆞᆮa</text>

        <g style="font-family: 'Noto Sans CJK JP'; font-variant-ligatures: discretionary-ligatures;">
        <text id="id9" style="writing-mode: tb-rl;direction: ltr;" >ffi明治</text>
        <text id="id10" style="writing-mode: tb-rl;direction: ltr;text-orientation: upright;" >ffi明治</text>
        <text id="id11" style="writing-mode: tb-rl;direction: ltr;text-orientation: sidways;" >ffi明治</text>
        </g>

        </svg>)"""sv;

        document = SPDocument::createNewDocFromMem(svg);
        document->ensureUpToDate();
    }

    static void TearDownTestSuite() { document.reset(); }

protected:
    static std::unique_ptr<SPDocument> document;
    SPText *text = nullptr;

    void SetUp() override
    {
        text = cast<SPText>(document->getObjectById(GetParam().id));
        ASSERT_NE(text, nullptr);
    }

    Inkscape::Text::Layout &GetLayout() { return text->layout; }
};

std::unique_ptr<SPDocument> LayoutTNGComputeTestFixture::document;

INSTANTIATE_TEST_SUITE_P(LayoutTNGComputeTest, LayoutTNGComputeTestFixture, testing::ValuesIn(LIGATURE_CURSOR_TESTS));

TEST_P(LayoutTNGComputeTestFixture, CursorPositionsInsideLigature)
{
    auto &config = GetParam();
    auto &layout = GetLayout();

    // Incorrect glyph count implies that ligature didn't get applied.
    // This check is important, if the ligature isn't applied rest of the test will succeed without detecting
    // any errors in code it was supposed test.
    ASSERT_EQ(layout.glyphs().size(), config.expected_glyphs);

    auto it = layout.begin();
    auto last_x = layout.characterAnchorPoint(it)[config.direction];
    auto position_count = 1; // start
    std::vector<float> positions;
    positions.push_back(last_x);
    while (it.nextCursorPosition()) {
        position_count++;
        auto pos = layout.characterAnchorPoint(it);
        if (config.permutation.empty()) {
            EXPECT_GT(pos[config.direction], last_x);
        }
        last_x = pos[config.direction];
        positions.push_back(last_x);
    };
    positions.push_back(layout.characterAnchorPoint(it)[config.direction]);
    position_count += 1; // end position
    EXPECT_EQ(position_count, config.expected_positions);
    if (config.expected_characters > 0) {
        auto last_char = it;
        last_char.prevCharacter();
        EXPECT_EQ(layout.iteratorToCharIndex(it), config.expected_characters);
    }
    if (config.permutation.size()) {
        ASSERT_EQ(positions.size(), config.permutation.size());
        std::vector<int> actual_permutation(positions.size());
        for (int i = 0; i < positions.size(); i++) {
            auto actual_pos =
                std::count_if(positions.begin(), positions.end(), [&](float v) { return v < positions[i]; });
            actual_permutation[i] = actual_pos;
        }
        EXPECT_EQ(actual_permutation, config.permutation);
    }
}

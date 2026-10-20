// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Test Inkscape::Extensions::Internal::PdfOutput
 */
/*
 * Authors:
 *   Martin Owens
 *
 * Copyright (C) 2025 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "attributes.h"
#include "style.h"

#include "extension/internal/pdfoutput/remember-styles.h"

#include <gtest/gtest.h>

using Inkscape::Extension::Internal::StyleMemory;

TEST(StyleMemeoryTest, MapFiltersStyle)
{
    SPStyle *style = new SPStyle(); // No document style
    style->mergeString("opacity:1.0;fill:black;stroke:red");

    auto memory = StyleMemory({SPAttr::OPACITY, SPAttr::FILL});
    auto map = memory.get_changes(style);
    EXPECT_TRUE(map.contains(SPAttr::OPACITY));
    EXPECT_TRUE(map.contains(SPAttr::FILL));
    EXPECT_FALSE(map.contains(SPAttr::STROKE));

    EXPECT_EQ(map[SPAttr::OPACITY], "1");
    EXPECT_EQ(map[SPAttr::FILL], "black");
}

TEST(StyleMemoryTest, MapContainsUnsetStyle)
{
    SPStyle *style = new SPStyle(); // No document style
    style->mergeString("fill:black;stroke:red");

    auto memory = StyleMemory({SPAttr::OPACITY, SPAttr::FILL});
    auto map = memory.get_changes(style);
    EXPECT_TRUE(map.contains(SPAttr::OPACITY));
    EXPECT_TRUE(map.contains(SPAttr::FILL));

    EXPECT_EQ(map[SPAttr::OPACITY], "1");
    EXPECT_EQ(map[SPAttr::FILL], "black");
}

TEST(StyleMemoryTest, MemoryState)
{
    SPStyle *style = new SPStyle(); // No document style
    style->mergeString("fill:black;");

    auto memory = StyleMemory({SPAttr::OPACITY, SPAttr::FILL});
    ASSERT_EQ(memory.get_state().size(), 0);

    auto map = memory.get_changes(style);
    ASSERT_EQ(map.size(), 2);

    {
        auto scope = memory.remember(map);
        ASSERT_EQ(memory.get_state().size(), 2);
        ASSERT_EQ(memory.get_state().find(SPAttr::FILL)->second, "black");
        ASSERT_EQ(memory.get_state().find(SPAttr::OPACITY)->second, "1");

        // Nothing has changed, so nothing should change
        ASSERT_EQ(memory.get_changes(style).size(), 0);

        style->clear(SPAttr::FILL);
        style->mergeString("fill:red");
        auto map2 = memory.get_changes(style);
        ASSERT_EQ(map2.size(), 1);
        ASSERT_EQ(map2[SPAttr::FILL], "red");

        {
            auto scope2 = memory.remember(map2);
            ASSERT_EQ(memory.get_state().find(SPAttr::FILL)->second, "red");
            ASSERT_EQ(memory.get_state().find(SPAttr::OPACITY)->second, "1");
            ASSERT_EQ(memory.get_changes(style).size(), 0);
        }

        ASSERT_EQ(memory.get_state().find(SPAttr::FILL)->second, "black");
        ASSERT_EQ(memory.get_state().find(SPAttr::OPACITY)->second, "1");
        ASSERT_EQ(memory.get_changes(style).size(), 1);
    }
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

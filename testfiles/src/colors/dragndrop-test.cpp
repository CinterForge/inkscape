// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests for color xml conversions.
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "colors/dragndrop.h"

#include <gtest/gtest.h>

#include "colors/color.h"

using namespace Inkscape;
using namespace Inkscape::Colors;

namespace {

TEST(ColorDragAndDrop, test_getMimeData_none)
{
    auto data = getMIMEData(*Color::parse("red"), "text/bad-format");
    ASSERT_EQ(data.size(), 0);
    data = getMIMEData(NoColor(), "text/bad-format");
    ASSERT_EQ(data.size(), 0);
}

TEST(ColorDragAndDrop, test_getMimeData_oswb)
{
    auto data = getMIMEData(*Color::parse("red"), "application/x-oswb-color");
    ASSERT_EQ(data.size(), 138);
    ASSERT_EQ(data[0], '<');
    ASSERT_EQ(data[10], 'i');
    ASSERT_EQ(data[20], 'e');
    ASSERT_EQ(data[30], 'U');
}

TEST(ColorDragAndDrop, test_getMimeData_x_color)
{
    auto data = getMIMEData(*Color::parse("red"), "application/x-color");
    ASSERT_EQ(data.size(), 8);
    ASSERT_EQ(data[0], '\xFF');
    ASSERT_EQ(data[2], '\x0');
    ASSERT_EQ(data[4], '\x0');

    data = getMIMEData(NoColor(), "application/x-color");
    ASSERT_EQ(data.size(), 8);
    ASSERT_EQ(data[0], '\x0');
    ASSERT_EQ(data[2], '\x0');
    ASSERT_EQ(data[4], '\x0');
}

TEST(ColorDragAndDrop, test_getMimeData_text)
{
    auto data = getMIMEData(*Color::parse("red"), "text/plain");
    ASSERT_EQ(data.size(), 3);
    ASSERT_EQ(data[0], 'r');
    ASSERT_EQ(data[2], 'd');

    data = getMIMEData(NoColor(), "text/plain");
    ASSERT_EQ(std::string(data.begin(), data.end()), "none");
}

TEST(ColorDragAndDrop, test_fromMimeData)
{
    auto data = getMIMEData(*Color::parse("red"), "application/x-oswb-color");
    auto paint = fromMIMEData(data, "application/x-oswb-color");
    ASSERT_EQ(std::get<Color>(paint).toString(), "red");
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

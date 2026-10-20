// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Authors:
 *   See git history
 *
 * @copyright
 * Copyright (C) 2024 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 *
 * @file @brief Unit tests for PDF parsing utilities
 */

#include "extension/internal/pdfinput/poppler-utils.h"

#include <gtest/gtest.h>

namespace Inkscape {

TEST(PopplerUtilsTest, SanitizeId)
{
    ASSERT_EQ(sanitizeId(""), "_");
    ASSERT_EQ(sanitizeId("hello"), "hello");
    ASSERT_EQ(sanitizeId("a bc"), "a_20bc");
    ASSERT_EQ(sanitizeId("a\xff"
                         "bc"),
              "a_ffbc");
}
} // namespace Inkscape

TEST(PopplerUtilsTest, GetNameWithoutSubsetTag)
{
  ASSERT_EQ(getNameWithoutSubsetTag("AAAAAA+aff65d+OpenSans"), "OpenSans");
  ASSERT_EQ(getNameWithoutSubsetTag("AAAAAA+OpenSans"), "OpenSans");
  ASSERT_EQ(getNameWithoutSubsetTag("OpenSn+Regular"), "Regular");
  ASSERT_EQ(getNameWithoutSubsetTag("AAAAAAAAAAAAAA+OpenSans-Regular"), "AAAAAAAAAAAAAA+OpenSans-Regular");
  ASSERT_EQ(getNameWithoutSubsetTag("AAAAA0+NotoSans-Regular"), "NotoSans-Regular");
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

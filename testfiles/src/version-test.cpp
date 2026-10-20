// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Authors:
 *   Rafał Siejakowski <rs@rs-math.net>
 *
 * @copyright
 * Copyright (C) 2024 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 *
 * @file @brief Unit tests for Inkscape::Version
 */

#include "version.h"

#include <gtest/gtest.h>

namespace Inkscape {

TEST(VersionTest, DefaultConstruct)
{
    const Version test_version{};
    EXPECT_EQ(test_version.str(), "0.0");
}

TEST(VersionTest, FromString)
{
    auto const v7_5 = Version::from_string("7.5-suffix");
    ASSERT_TRUE(v7_5);
    EXPECT_EQ(v7_5->str(), "7.5-suffix");

    auto const v3_4 = Version::from_string("3.4");
    ASSERT_TRUE(v3_4);
    EXPECT_EQ(*v3_4, Version(3, 4));
}

TEST(VersionTest, NoHex)
{
    auto const with_b = Version::from_string("1.4beta");
    ASSERT_TRUE(with_b);
    // Ensure "4be" wasn't parsed as 0x4be
    EXPECT_EQ(*with_b, Version(1, 4));
}

TEST(VersionTest, FromStringBad)
{
    EXPECT_FALSE(Version::from_string("foo.bar-baz"));
    EXPECT_FALSE(Version::from_string("13"));
    EXPECT_FALSE(Version::from_string("666.evil"));
    EXPECT_FALSE(Version::from_string(nullptr));
}

TEST(VersionTest, StringFormat)
{
    EXPECT_EQ(Version(42, 69).str(), "42.69");
    EXPECT_EQ(Version(1, 2, "-suffix").str(), "1.2-suffix");
}

TEST(VersionTest, Comparisons)
{
    EXPECT_EQ(Version(2, 3), Version(2, 3));
    EXPECT_EQ(Version(4, 5), Version(4, 5, "-suffix"));

    EXPECT_LT(Version(1, 0), Version(2, 0));
    EXPECT_LT(Version(1, 1), Version(1, 2));

    EXPECT_GT(Version(7, 2), Version(6, 999));
    EXPECT_GT(Version(4, 8), Version(4, 7));
}
} // namespace Inkscape

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

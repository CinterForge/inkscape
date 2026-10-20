// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests for color objects.
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtest/gtest.h>

#include "colors/color.h"

using namespace Inkscape::Colors;

namespace {

TEST(ColorsSpacesRgb, fromString)
{
    ASSERT_EQ(Color::parse("  red  ")->toRGBA(), 0xff0000ff);
    ASSERT_EQ(Color::parse("BLUE ")->toRGBA(0.5), 0x0000ff80);
}

TEST(ColorsSpaceRgb, fromStringFailures)
{
    ASSERT_FALSE(Color::parse("réd"));
}

TEST(ColorsSpaceRgb, toString)
{
    ASSERT_EQ(Color(0xff000000, false).converted(Space::Type::CSSNAME)->toString(), "red");
    ASSERT_EQ(Color::parse("mediumpurple")->toString(), "mediumpurple");
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

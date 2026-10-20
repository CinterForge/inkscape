// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests for a Alpha "color space"
 *
 * Copyright (C) 2026 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "spaces-testbase.h"

namespace {

using Space::Type::Alpha;
using Space::Type::RGB;

// There is no CSS for Alpha
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(fromString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(badColorString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(toString);

// clang-format off
INSTANTIATE_TEST_SUITE_P(ColorsSpacesAlpha, convertColorSpace, testing::Values(
    // These test the icc profile but are not "sensible" results for color conversions
    _P(inb, Alpha, {1.0},           RGB,   {1, 1, 1}),
    _P(inb, Alpha, {0.7},           RGB,   {0.711, 0.711, 0.711}),
    _P(inb, RGB,   {0.3, 0.3, 0.3}, Alpha, {0.3}),
    _P(inb, Alpha, {0.2},           Alpha, {0.2})
));
// clang-format on

// There is no regular color conversion for Alpha
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(normalize);

TEST(ColorsSpacesAlpha, components)
{
    auto c = Manager::get().find(Alpha)->getComponents();
    ASSERT_EQ(c.size(), 0);
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

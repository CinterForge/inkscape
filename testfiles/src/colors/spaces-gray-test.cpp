// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests for a Grayscale color space
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "spaces-testbase.h"

namespace {

using Space::Type::Gray;
using Space::Type::RGB;

// There is no CSS for Gray, it was remoed from css color module 4 draft in 2018
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(fromString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(badColorString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(toString);

// clang-format off
INSTANTIATE_TEST_SUITE_P(ColorsSpacesGray, convertColorSpace, testing::Values(
    _P(inb, Gray, {0.7}, RGB, {0.7, 0.7, 0.7}),

    // No conversion
    _P(inb, Gray, {0.2}, Gray, {0.2})
));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesGray, normalize, testing::Values(
    _P(inb, Gray, { 0.5 }, Gray, { 0.5 }),
    _P(inb, Gray, { 1.2 }, Gray, { 1.0 }),
    _P(inb, Gray, {-0.2 }, Gray, { 0.0 }),
    _P(inb, Gray, { 0.0 }, Gray, { 0.0 }),
    _P(inb, Gray, { 1.0 }, Gray, { 1.0 })
));
// clang-format on

TEST(ColorsSpacesGray, randomConversion)
{
    EXPECT_TRUE(RandomPassthrough(Gray, RGB, 100));
}

TEST(ColorsSpacesGray, components)
{
    auto c = Manager::get().find(Gray)->getComponents();
    ASSERT_EQ(c.size(), 1);
    ASSERT_EQ(c[0].id, "gray");
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

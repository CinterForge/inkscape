// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Authors:
 *   Rafał Siejakowski <rs@rs-math.net>
 *
 * @copyright
 * Copyright (C) 2025 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtest/gtest.h>

namespace {
void failing_assertion_function() {
    assert(false);
}
}

/// Ensure that a failed assertion crashes a test
TEST(TechicalTestSuiteTest, AssertionsArmedInTests) {
    EXPECT_DEATH(failing_assertion_function(), ".*");
}
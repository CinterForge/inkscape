// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * LPE 64B tests 
 * Because some issues rounding in 32B windows we move this tests only to 64B
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2020 Authors
 *
 * Released under GNU GPL version 2 or later, read the file 'COPYING' for more information
 */

#include <gtest/gtest.h>
#include "testfiles/lpespaths-test.h"

class LPE64Test : public LPESPathsTest
{
public:
    void run() {
        testDoc(svg);
    }
};

// A) FILE BASED TESTS
TEST_F(LPE64Test, Inkscape_0_92_64) { run(); }
TEST_F(LPE64Test, Inkscape_1_0_64)  { run(); }
// B) CUSTOM TESTS

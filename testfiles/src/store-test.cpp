// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * LPE tests
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL version 2 or later, read the file 'COPYING' for more information
 */

#include <gtest/gtest.h>
#include <testfiles/store-integrity-test.h>

using namespace Inkscape;

class StoreTest : public StoreIntegrityTest {
public:
    StoreIntegrityMode mode = NO_UPDATE;
    void run() {
        testDoc(svg, mode);
    }
};
// @mode 0 raw files
TEST_F(StoreTest, store0) { mode = StoreIntegrityMode::NO_UPDATE;       run(); }
// @mode 1 update stored
TEST_F(StoreTest, store1) { mode = StoreIntegrityMode::UPDATE_ORIGINAL; run(); }
// @mode 2 update copy saved
TEST_F(StoreTest, store2) { mode = StoreIntegrityMode::UPDATE_SAVED;    run(); }
// @mode 3 update both
TEST_F(StoreTest, store3) { mode = StoreIntegrityMode::UPDATE_BOTH;     run(); }

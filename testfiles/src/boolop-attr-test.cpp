// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Test for boolean operations with fill-rule as attributes
 *
 * https://gitlab.com/inkscape/inkscape/-/issues/5437
 */
/*
 * Authors:
 *   Thomas Holder
 *
 * Copyright (C) 2024 Authors
 */

#include <gtest/gtest.h>

#include "doc-per-case-test.h"
#include "object/object-set.h"

using namespace Inkscape;
using namespace std::literals;

class BoolopAttrTest : public DocPerCaseTest
{
public:
    BoolopAttrTest()
    {
        constexpr auto docString = R"A(
<svg viewBox="0 0 210 110" xmlns="http://www.w3.org/2000/svg">
  <g id="union">
    <path fill-rule="evenodd" d="M 20,40 H 40 V 20 H 20 Z M 10,10 H 50 V 50 H 10 Z" />
    <path fill-rule="evenodd" d="M 70,20 H 90 V 40 H 70 Z M 60,10 h 40 V 50 H 60 Z" />
    <path fill-rule="nonzero" d="m 120,40 h 20 V 20 H 120 Z M 110,10 h 40 v 40 h -40 z " />
    <path fill-rule="nonzero" d="m 170,20 h 20 V 40 H 170 Z M 160,10 h 40 v 40 h -40 z " />
    <path style="fill-rule:evenodd" d="M 20,90 H 40 V 70 H 20 Z M 10,60 h 40 v 40 H 10 Z" />
    <path style="fill-rule:evenodd" d="M 70,70 H 90 V 90 H 70 Z M 60,60 h 40 v 40 H 60 Z" />
    <path style="fill-rule:nonzero" d="m 120,90 h 20 V 70 H 120 Z M 110,60 h 40 v 40 h -40 z " />
    <path style="fill-rule:nonzero" d="m 170,70 h 20 V 90 H 170 Z M 160,60 h 40 v 40 h -40 z " />
  </g>
</svg>
        )A"sv;
        doc = SPDocument::createNewDocFromMem(docString);
    }

    std::unique_ptr<SPDocument> doc;
};

TEST_F(BoolopAttrTest, Union)
{
    auto const d_combined = //
        "M 10 10 L 10 50 L 50 50 L 50 10 L 10 10 z "
        "M 60 10 L 60 50 L 100 50 L 100 10 L 60 10 z "
        "M 110 10 L 110 50 L 150 50 L 150 10 L 110 10 z "
        "M 160 10 L 160 50 L 200 50 L 200 10 L 160 10 z "
        "M 20 20 L 40 20 L 40 40 L 20 40 L 20 20 z "
        "M 70 20 L 90 20 L 90 40 L 70 40 L 70 20 z "
        "M 120 20 L 140 20 L 140 40 L 120 40 L 120 20 z "
        "M 10 60 L 10 100 L 50 100 L 50 60 L 10 60 z "
        "M 60 60 L 60 100 L 100 100 L 100 60 L 60 60 z "
        "M 110 60 L 110 100 L 150 100 L 150 60 L 110 60 z "
        "M 160 60 L 160 100 L 200 100 L 200 60 L 160 60 z "
        "M 20 70 L 40 70 L 40 90 L 20 90 L 20 70 z "
        "M 70 70 L 90 70 L 90 90 L 70 90 L 70 70 z "
        "M 120 70 L 140 70 L 140 90 L 120 90 L 120 70 z ";

    auto const paths = doc->getObjectsBySelector("#union path");
    ASSERT_EQ(paths.size(), 8);

    auto object_set = ObjectSet(doc.get());
    object_set.setList(paths);
    object_set.pathUnion(true);

    auto combined = object_set.single();
    ASSERT_TRUE(combined);
    ASSERT_STREQ(combined->getAttribute("d"), d_combined);
}

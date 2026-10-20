// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Test for https://gitlab.com/inkscape/inkscape/-/issues/3393
 *//*
 *
 * Authors:
 *   Thomas Holder
 *
 * Copyright (C) 2022 Authors
 *
 * Released under GNU GPL version 2 or later, read the file 'COPYING' for more information
 */

#include <doc-per-case-test.h>
#include <gtest/gtest.h>
#include <src/object/object-set.h>
#include <src/object/sp-shape.h>

using namespace Inkscape;
using namespace std::literals;

constexpr auto docString = R"""(<?xml version="1.0"?>
<svg xmlns="http://www.w3.org/2000/svg"
   xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape">
  <defs>
    <inkscape:path-effect effect="skeletal" copytype="repeated" 
       id="lpe1" pattern="M 0,0 5,5 0,10" />
  </defs>
  <path id="path1"
     inkscape:path-effect="#lpe1"
     inkscape:original-d="M 5,10 H 15"
     d="M 5,5 10,10 5,15 M 10,5 15,10 10,15" />
</svg>
)"""sv;

TEST_F(DocPerCaseTest, PathReverse)
{
    auto doc = SPDocument::createNewDocFromMem(docString);
    doc->ensureUpToDate();

    auto path1 = cast<SPShape>(doc->getObjectById("path1"));
    auto oset = ObjectSet(doc.get());
    oset.add(path1);

    ASSERT_EQ(path1->curve()->initialPoint(), Geom::Point(5, 5));

    oset.pathReverse();

    ASSERT_EQ(path1->curve()->initialPoint(), Geom::Point(15, 15));
}

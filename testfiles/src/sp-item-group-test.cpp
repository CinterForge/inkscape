// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * SPGroup test
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2020 Authors
 *
 * Released under GNU GPL version 2 or later, read the file 'COPYING' for more information
 */

#include <gtest/gtest.h>

#include "document.h"
#include "inkscape.h"
#include "live_effects/effect.h"
#include "object/sp-item-group.h"
#include "object/sp-lpe-item.h"

using namespace Inkscape;
using namespace Inkscape::LivePathEffect;
using namespace std::literals;

class SPGroupTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // setup hidden dependency
        Application::create(false);
    }
};

TEST_F(SPGroupTest, applyingPowerClipEffectToGroupWithoutClipIsIgnored)
{
    constexpr auto svg = R"A(
<svg width='100' height='100'>
    <g id='group1'>
        <rect id='rect1' width='100' height='50' />
        <rect id='rect2' y='50' width='100' height='50' />
    </g>
</svg>)A"sv;

    auto doc = SPDocument::createNewDocFromMem(svg);

    auto group = cast<SPGroup>(doc->getObjectById("group1"));
    Effect::createAndApply(POWERCLIP, doc.get(), group);

    ASSERT_FALSE(group->hasPathEffect());
}

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests for parsing units xml.
 *
 * Copyright (C) 2025 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glib.h>
#include <gtest/gtest.h>

#include "test-utils.h"
#include "util/units.h"

namespace {

using Inkscape::Util::Unit;
using Inkscape::Util::UnitMetric;
using Inkscape::Util::UnitTable;

auto const xmlpath = INKSCAPE_TESTS_DIR "/data/units-test.xml";

TEST(UtilUnitsTest, DataLoad)
{
    auto ut = UnitTable(xmlpath);

    {
        auto pc = ut.getUnit("%");
        ASSERT_TRUE(pc);
        ASSERT_EQ(pc->name, "%");
    }
    {
        auto pt = ut.getUnit("pt");
        ASSERT_TRUE(pt);
        ASSERT_EQ(pt->name, "point");
        ASSERT_EQ(pt->name_plural, "points");
        ASSERT_EQ(pt->abbr, "pt");
        ASSERT_EQ(pt->description, "PostScript points (72/inch)");
    }
    {
        auto deg = ut.getUnit("°");
        ASSERT_TRUE(deg);
        EXPECT_EQ(deg->name, "degree");
        EXPECT_EQ(deg->name_plural, "degrees");
    }
}

TEST(UtilUnitsTest, UnitMetricLoad)
{
    auto ut = UnitTable(xmlpath);

    {
        auto metric = ut.getUnitMetric("general");
        ASSERT_TRUE(metric);
        ASSERT_EQ(metric->name, "general");
        EXPECT_EQ(metric->ruler_scale.size(), 11);
        EXPECT_EQ(metric->ruler_scale[0], 1);
        EXPECT_EQ(metric->ruler_scale[2], 5);
        EXPECT_EQ(metric->ruler_scale[4], 25);
        EXPECT_EQ(metric->ruler_scale[5], 50);
        EXPECT_EQ(metric->ruler_scale[10], 2500);
        EXPECT_EQ(metric->subdivide.size(), 5);
        EXPECT_EQ(metric->subdivide[4], 100);
    }
}

TEST(UtilUnitsTest, UnitMetricGet)
{
    // These load from the global space, so our tests aren't using test data :(
    {
        auto mm = UnitTable::get().getUnit("mm");
        ASSERT_TRUE(mm);
        ASSERT_EQ(mm->abbr, "mm");
        auto m = mm->getUnitMetric();
        ASSERT_TRUE(m);
        ASSERT_EQ(m->name, "general");
    }
    {
        auto inch = UnitTable::get().getUnit("in");
        ASSERT_TRUE(inch);
        ASSERT_EQ(inch->abbr, "in");
        auto m = inch->getUnitMetric();
        ASSERT_TRUE(m);
        ASSERT_EQ(m->name, "dyadic");
    }
}

class UnitLocale : public GlobalLocaleTestFixture
{
};

TEST_P(UnitLocale, UnitScale)
{
    UnitTable units;
    auto mm = units.getUnit("mm");
    auto inch = units.getUnit("in");
    EXPECT_NE(mm, nullptr);
    EXPECT_NE(inch, nullptr);
    ASSERT_DOUBLE_EQ(25.4, inch->convert(1, mm));

    auto cm = units.getUnit("cm");
    EXPECT_NE(cm, nullptr);
    ASSERT_DOUBLE_EQ(10, cm->convert(1, mm));
}

INSTANTIATE_TEST_SUITE_P(UtilUnitsTest, UnitLocale, testing::Values("C", "de_DE.UTF8"));

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

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests for parsing user entered mathamatic strings
 *
 * Copyright (C) 2025 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glib.h>
#include <gtest/gtest.h>

#include "test-utils.h"

#include "util/units.h"
#include "util/expression-evaluator.h"

using Inkscape::Util::UnitTable;
using Inkscape::Util::ExpressionEvaluator;

namespace {

struct in : traced_data
{
    const std::string expr;
    const double value;
    const std::string unit = "mm";
    const int dimension = 1;
};

class evaluateString : public testing::TestWithParam<in>
{
};

void PrintTo(const in &obj, std::ostream *oo)
{
    *oo << "'" << obj.expr << "'" ;
}

  
TEST_P(evaluateString, testResult)
{
    in test = GetParam();
    auto scope = test.enable_scope();
    auto unit = UnitTable::get().getUnit(test.unit);
    auto res = ExpressionEvaluator(test.expr.c_str(), unit).evaluate();
    EXPECT_TRUE(IsNear(res.value, test.value)) << "'" << test.expr << "': " << test.value << " != " << res.value;
    //EXPECT_EQ(res.dimension, test.dimension); // FIXME: This is often a negative number (except it cant be)
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(expressionEvaluation, evaluateString, testing::Values(
      _P(in, "2",         2.0   )
    , _P(in, "2.2",       2.2   )
    , _P(in, "2+2",       4.0   )
    , _P(in, "2+2+4",     8.0   )
    , _P(in, "2 + 2",     4.0   )
    , _P(in, "2*4",       8.0   )
    , _P(in, "2^4",       16.0  )
    , _P(in, "5.3 * 2.2", 11.66 )
    , _P(in, "10cm",      100.0 )
    , _P(in, "2in",       50.79 )
    , _P(in, "3in/8",     9.52  )
    , _P(in, "(3/8)in",   9.52  )
//  , _P(in, "3/8in",     9.52  ) // FIXME: This should allow "3/8in" to be the same as above
    , _P(in, "3/8in",     0.375,  "in")
    , _P(in, "50.79mm",   2.0,    "in")
    , _P(in, "4cm + 2in", 90.79 )
    , _P(in, "(2cm * 2) + (1in * 5)", 167 )
    , _P(in, "1:50",      0.02 )
    , _P(in, "2:100",     0.02 )
    , _P(in, "50:1",      50 )
    , _P(in, "50:2",      25 )
));


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

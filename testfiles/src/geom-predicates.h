// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef GEOM_PREDICATES_H
#define GEOM_PREDICATES_H

#include <cmath>
#include <gtest/gtest.h>
#include <2geom/rect.h>

inline ::testing::AssertionResult RectNear(char const *expr1,
                                           char const *expr2,
                                           char const *,
                                           Geom::Rect const &val1,
                                           Geom::Rect const &val2,
                                           double abs_error = 0.01)
{
    double diff = 0;
    for (auto x = 0; x < 2; x++) {
        for (auto y = 0; y < 2; y++) {
            diff += std::abs(val1[x][y] - val2[x][y]);
        }
    }

    if (diff <= abs_error) {
        return ::testing::AssertionSuccess();
    }

    return ::testing::AssertionFailure()
        << "The difference between " << expr1 << " and " << expr2
        << " is " << diff << ", which exceeds " << abs_error << ", where\n"
        << expr1 << " evaluates to " << val1 << ",\n"
        << expr2 << " evaluates to " << val2 << ".\n";
}

#define EXPECT_RECT_NEAR(a, b, eps) EXPECT_PRED_FORMAT3(RectNear, a, b, eps)

#endif

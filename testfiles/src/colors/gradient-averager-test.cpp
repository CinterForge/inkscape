// SPDX-License-Identifier: GPL-2.0-or-later
#include <gtest/gtest.h>

#include "colors/gradient-averager.h"

using namespace Inkscape::Colors;

TEST(LinearGradientAverage, rgb)
{
    auto const a = Color{0x880000ff};
    auto const b = Color{0x008800ff};

    {
        LinearGradientAverager grad;
        grad.addStop(0.25, a);
        grad.addStop(0.75, b);
        ASSERT_EQ(grad.finish().toString(), a.averaged(b).toString());
    }

    {
        LinearGradientAverager grad;
        grad.addStop(0.2, a);
        grad.addStop(0.6, b);
        ASSERT_EQ(grad.finish().toString(), a.averaged(b, (0.6 - 0.2) / 2 + (1 - 0.6)).toString());
    }
}

TEST(LinearGradientAverage, rgba)
{
    auto const a = Color{0x88000000};
    auto const b = Color{0x008800ff};

    LinearGradientAverager grad;
    grad.addStop(0, a);
    grad.addStop(1, b);
    ASSERT_EQ(grad.finish().toString(), b.withOpacity(0.5).toString());
}

TEST(NonlinearGradientAverage, hsl)
{
    auto const hsl = Manager::get().find(Space::Type::HSL);
    auto const rgb = Manager::get().find(Space::Type::RGB);
    NonlinearGradientAverager grad{hsl, rgb, 6};
    grad.addStop(0, Color{hsl, {0, 1, 0.5}});
    grad.addStop(1, Color{hsl, {1, 1, 0.5}});
    ASSERT_TRUE(grad.finish().isClose(Color{rgb, {0.5, 0.5, 0.5, 1}}));
}

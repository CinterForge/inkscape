// SPDX-License-Identifier: GPL-2.0-or-later
#include <functional>
#include <optional>
#include <cmath>
#include <gtest/gtest.h>
#include "async/progress.h"
#include "async/progress-splitter.h"
using namespace Inkscape::Async;

TEST(ProgressTest, subprogress)
{
    class ProgressMock final
        : public Progress<double>
    {
    public:
        mutable bool k_called;
        double p_arg_saved;
        bool ret;

        void reset(bool ret_)
        {
            k_called = false;
            p_arg_saved = -1.0;
            ret = ret_;
        }

    protected:
        bool _keepgoing() const override { k_called = true; return ret; }
        bool _report(double const &progress) override { p_arg_saved = progress; return ret; }
    };

    auto a = ProgressMock();
    auto b = SubProgress(a, 0.25, 0.5);
    auto c = SubProgress(b, 0.1, 0.2);

    for (bool ret : { true, false }) {
        for (double progress = 0.0; progress < 1.0; progress += 0.3) {
            a.reset(ret);
            EXPECT_EQ(c.report(progress), ret);
            EXPECT_NEAR(a.p_arg_saved, 0.25 + 0.5 * (0.1 + 0.2 * progress), 1e-5);
            EXPECT_FALSE(a.k_called);
        }
    }

    for (bool ret : { true, false }) {
        a.reset(ret);
        EXPECT_EQ(c.keepgoing(), ret);
        EXPECT_EQ(a.p_arg_saved, -1.0);
        EXPECT_TRUE(a.k_called);
    }

    a.reset(false);
    EXPECT_THROW(c.report_or_throw(0.5), CancelledException);
    EXPECT_THROW(c.throw_if_cancelled(), CancelledException);
    a.reset(true);
    EXPECT_NO_THROW(c.report_or_throw(0.5));
    EXPECT_NO_THROW(c.throw_if_cancelled());
}

TEST(ProgressTest, throttler)
{
    class ProgressMock final
       : public Progress<double>
    {
    public:
        int calls = 0;
        double saved = 0.0;

    protected:
        bool _keepgoing() const override { return true; }
        bool _report(double const &progress) override { saved = progress; calls++; return true; }
    };

    double constexpr step = 0.1;
    auto a = ProgressMock();
    auto b = ProgressStepThrottler(a, step);

    int constexpr N = 1000;
    for (int i = 0; i < N; i++) {
        double progress = (double)i / N;
        b.report(progress);
        ASSERT_LE(std::abs(progress - a.saved), 1.1 * step);
    }
    ASSERT_GE(a.calls, 9);
    ASSERT_LE(a.calls, 11);
}

TEST(ProgressTest, splitter)
{
    class ProgressMock final
       : public Progress<double>
    {
    public:
        double saved;

    protected:
        bool _keepgoing() const override { return true; }
        bool _report(double const &progress) override { saved = progress; return true; }
    };

    auto a = ProgressMock();
    std::optional<SubProgress<double>> x, y, z;

    auto reset = [&] {
        a.saved = -1.0;
        x = y = z = {};
    };

    reset();
    ProgressSplitter(a)
        .add(x, 0.25)
        .add(y, 0.5)
        .add(z, 0.25);
    ASSERT_TRUE(x);
    ASSERT_TRUE(y);
    ASSERT_TRUE(z);
    x->report(0.5); EXPECT_NEAR(a.saved, 0.125, 1e-5);
    y->report(0.5); EXPECT_NEAR(a.saved, 0.5  , 1e-5);
    z->report(0.5); EXPECT_NEAR(a.saved, 0.875, 1e-5);

    reset();
    ProgressSplitter(a)
        .add_if(x, 0.25, true)
        .add_if(y, 0.5,  false)
        .add_if(z, 0.25, true);
    ASSERT_TRUE(x);
    ASSERT_FALSE(y);
    ASSERT_TRUE(z);
    x->report(0.5); EXPECT_NEAR(a.saved, 0.25, 1e-5);
    z->report(0.5); EXPECT_NEAR(a.saved, 0.75, 1e-5);
}

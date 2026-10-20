// SPDX-License-Identifier: GPL-2.0-or-later
#include <vector>
#include <gtest/gtest.h>
#include "util/funclog.h"
using namespace Inkscape::Util;

static int counter;

class LoggedInt
{
public:
    LoggedInt(int x) : x(x) { counter++; }
    LoggedInt(LoggedInt const &other) noexcept : x(other.x) { counter++; }
    LoggedInt &operator=(LoggedInt const &other) noexcept { x = other.x; return *this; }
    ~LoggedInt() { counter--; }
    operator int() const { return x; }
    LoggedInt &operator=(int x2) { x = x2; return *this; }

private:
    int x;
};

TEST(FuncLogTest, funclog)
{
    counter = 0;

    std::vector<int> results;
    auto write = [&] (int x) { return [&, x = LoggedInt(x)] { results.emplace_back(x); }; };

    auto compare = [&] (std::vector<int> expected) {
        EXPECT_EQ(results, expected);
        results.clear();
    };

    FuncLog a;
    EXPECT_TRUE(a.empty());
    a();
    compare({});

    a.emplace(write(1));
    a.emplace(write(2));
    EXPECT_EQ(counter, 2);
    EXPECT_FALSE(a.empty());
    a();
    compare({ 1, 2 });

    a.emplace(write(3));
    auto b = std::move(a);
    a();
    compare({});
    b();
    compare({ 3 });
    auto c = std::move(a);
    c();
    compare({});

    b.emplace(write(4));
    a = std::move(b);
    b();
    compare({});
    a();
    compare({ 4 });
    a();
    compare({});

    for (int N : { 10, 50, 10, 100, 10, 500, 10 }) {
        for (int i = 0; i < N; i++) {
            a.emplace(write(4));
            a.emplace([&, x = i, y = 2 * i, z = 3 * i, w = 4 * i] { results.emplace_back(x + y + z + w); });
        }

        a();

        ASSERT_EQ(results.size(), 2 * N);
        for (int i = 0; i < N; i++) {
            ASSERT_EQ(results[2 * i], 4);
            ASSERT_EQ(results[2 * i + 1], 10 * i);
        }
        results.clear();
    }

    {
        auto f1 = [&, x = 1] {
            results.emplace_back(x);
        };
        a.emplace(f1);

        auto f2 = [&, x = 2] {
            results.emplace_back(x);
        };
        a.emplace(std::move(f2));
    }

    a();
    compare({ 1, 2 });

    a.emplace([&, x = std::make_unique<int>(5)] { results.emplace_back(*x); });
    a();
    compare({ 5 });

    FuncLog().emplace(write(6));
    compare({});

    for (int i = 0; i < 5; i++) {
        a.emplace(write(i));
    }
    a.exec_while([counter = 0] () mutable { return ++counter <= 3; });
    compare({ 0, 1, 2 });
    EXPECT_TRUE(a.empty());

    struct ExceptionMock {};
    for (int i = 0; i < 5; i++) {
        a.emplace([&, i] { if (i == 3) throw ExceptionMock(); results.emplace_back(i); });
    }
    EXPECT_THROW(a(), ExceptionMock);
    compare({ 0, 1, 2 });
    EXPECT_TRUE(a.empty());

    ASSERT_EQ(counter, 0);
}

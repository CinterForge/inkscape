// SPDX-License-Identifier: GPL-2.0-or-later
#include <thread>
#include <vector>
#include <glibmm/main.h>
#include <gtest/gtest.h>
#include "async/channel.h"
using namespace Inkscape::Async;

TEST(Channel, channel)
{
    auto test_one = [] (bool soft_close, bool delay_src_destroy, bool delay_dst_destroy) {
        auto mainloop = Glib::MainLoop::create();

        std::optional<Channel::Source> src;
        std::optional<Channel::Dest> dst;
        std::tie(src, dst) = Channel::create();

        std::thread thread;
        std::vector<int> results;

        Glib::signal_idle().connect([&] {
            thread = std::thread([&] {
                EXPECT_TRUE(src);

                EXPECT_TRUE(src->run([&] { results.emplace_back(1); })); // insert temporary function

                auto f = [&, x = 2] { results.emplace_back(x); };
                EXPECT_TRUE(src->run(f)); // insert copy of function

                auto g = [&, x = 3] { results.emplace_back(x); };
                EXPECT_TRUE(src->run(std::move(g))); // insert function by move

                // insert function which closes channel
                EXPECT_TRUE(src->run([&] {
                    ASSERT_TRUE(dst);
                    ASSERT_TRUE(*dst);

                    if (delay_dst_destroy) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    }

                    if (soft_close) {
                        dst->close();
                        EXPECT_FALSE(*dst);
                    } else {
                        dst.reset();
                    }

                    mainloop->quit();
                }));

                src->run([&] { results.emplace_back(4); });

                if (delay_src_destroy) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    src->run([&] { results.emplace_back(5); });
                }

                src.reset();
            });

            return false;
        });

        mainloop->run();
        thread.join();

        EXPECT_EQ(results, (std::vector<int>{ 1, 2, 3 }));
    };

    for (bool x : { true, false }) {
        test_one(x, false, false);
        test_one(x, true, false);
        test_one(x, false, true);
    }
}

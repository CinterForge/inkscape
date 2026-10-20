// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author: Liam White
 * Copyright (C) 2024 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "threading.h"

#include <atomic>
#include <mutex>

#include "dispatch-pool.h"
#include "util/statics.h"

namespace Inkscape {

namespace {

/*
 * Cannot simply be a static global variable or function-local static,
 * due to the Windows-specific deadlock on exit described in
 * https://gitlab.com/inkscape/inkscape/-/issues/6003.
 */
struct DispatchPoolHolder : Util::EnableSingleton<DispatchPoolHolder>
{
    std::shared_ptr<dispatch_pool> g_dispatch_pool;
};

std::mutex g_dispatch_lock;

std::atomic<int> g_num_dispatch_threads = 4;

} // namespace

void set_num_dispatch_threads(int num_dispatch_threads)
{
    g_num_dispatch_threads.store(num_dispatch_threads, std::memory_order_relaxed);
}

std::shared_ptr<dispatch_pool> get_global_dispatch_pool()
{
    int const num_threads = g_num_dispatch_threads.load(std::memory_order_relaxed);

    auto &g_dispatch_pool = DispatchPoolHolder::get().g_dispatch_pool;

    std::scoped_lock lk(g_dispatch_lock);

    if (g_dispatch_pool && num_threads == g_dispatch_pool->size()) {
        return g_dispatch_pool;
    }

    g_dispatch_pool = std::make_shared<dispatch_pool>(num_threads);
    return g_dispatch_pool;
}

} // namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

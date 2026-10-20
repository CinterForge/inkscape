// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author: Liam White
 * Copyright (C) 2024 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_DISPLAY_DISPATCH_POOL_H
#define INKSCAPE_DISPLAY_DISPATCH_POOL_H

#include <condition_variable>
#include <functional>
#include <mutex>
#include <stop_token>
#include <thread>
#include <vector>

namespace Inkscape {

/**
 * General-purpose, parallel thread dispatch mechanism.
 *
 * A dispatch is a compute job which is parameterized by a counter. It can also be thought of
 * as a way to parallelize a for loop. For example, the following single-threaded loop
 *
 *     for (int i = 0; i < count; ++i) {
 *         do_work(i);
 *     }
 *
 * can be rewritten to use a dispatch_pool and operate in parallel like this:
 *
 *     pool.dispatch(count, [&](int i, int local_id) {
 *         do_work(i);
 *     });
 *
 * Finally, it is also possible to perform all jobs on the calling thread unless a threshold
 * condition is met (like dispatch size). This can be used if threading the operation would be
 * less efficient unless the work is at least a certain size:
 *
 *     pool.dispatch_threshold(count, count > 1024, [&](int i, int local_id) {
 *         do_work(i);
 *     });
 *
 * Unlike boost's asio::thread_pool, which pushes work for threads onto a queue, this class only
 * supports operation via a counter. The simpler design allows dispatching a very large amount of
 * work (potentially millions of jobs, for every pixel in a megapixel image) with constant
 * memory and space used.
 *
 * A pool's thread count is fixed upon construction and cannot change during operation. If you
 * allocate work buffers for each thread in the pool, you can use the size() method to determine
 * how many threads it has been created with.
 *
 * By design, only one dispatch may run at a time. It is safe to call dispatch() from multiple
 * threads without extra locking.
 *
 * Terminology used is designed to loosely follow that of OpenCL kernels or GL/VK compute shaders:
 * - Global ID within a dispatch refers to the 0-based counter value for a given job.
 * - Local ID within a dispatch refers to the 0-based index of thread which is processing the job.
 *   This will always be less than the pool's size().
 *
 * The first parameter to the callback is global ID. The second parameter, which is unused in the
 * example, is the local ID. The local ID is primarily useful if a work buffer is allocated for
 * each thread in the dispatch_pool ahead of time.
 */
class dispatch_pool
{
public:
    using global_id = int;
    using local_id = int;
    using dispatch_func = std::function<void(global_id, local_id)>;

    explicit dispatch_pool(int size);
    ~dispatch_pool();

    void dispatch(int count, dispatch_func function);

    template <typename F>
    void dispatch_threshold(int count, bool threshold, F &&function)
    {
        if (threshold) {
            dispatch(count, std::forward<F>(function));
        } else {
            for (auto i = global_id{}; i < global_id{count}; i++) {
                function(i, local_id{});
            }
        }
    }

    int size() const
    {
        // The calling thread participates in the dispatch
        return _threads.size() + 1;
    }

private:
    void thread_func(std::stop_token const &stop_token, local_id id);
    void execute_batch(std::unique_lock<std::mutex> &lk, local_id id, int thread_count);

    global_id _available_work{};
    global_id _completed_work{};
    global_id _target_work{};

    std::mutex _dispatch_lock;
    std::mutex _lock;
    std::condition_variable_any _available_cv;
    std::condition_variable _completed_cv;
    dispatch_func _function;
    std::vector<std::jthread> _threads;
};

} // namespace Inkscape

#endif // INKSCAPE_DISPLAY_DISPATCH_POOL_H

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

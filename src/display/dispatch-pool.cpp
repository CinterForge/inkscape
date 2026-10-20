// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author: Liam White
 * Copyright (C) 2024 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "dispatch-pool.h"

namespace Inkscape {

dispatch_pool::dispatch_pool(int size)
{
    int const num_threads = std::max(size, 1) - 1;

    _threads.reserve(num_threads);

    for (int i = 0; i < num_threads; ++i) {
        // local_id of created threads is offset by 1 to allow calling thread to always be 0
        _threads.emplace_back([i, this] (std::stop_token stop_token) { thread_func(stop_token, local_id{i + 1}); });
    }
}

dispatch_pool::~dispatch_pool() = default;

void dispatch_pool::dispatch(int count, dispatch_func function)
{
    std::scoped_lock lk(_dispatch_lock);
    std::unique_lock lk2(_lock);

    _available_work = global_id{};
    _completed_work = global_id{};
    _target_work = global_id{count};
    _function = std::move(function);

    // Execute the caller's batch, and signal to the next waiting thread
    execute_batch(lk2, local_id{}, size());

    // Wait for other threads to finish
    _completed_cv.wait(lk2, [&] { return _completed_work == _target_work; });

    // Release any extra memory held by the function
    _function = {};
}

void dispatch_pool::thread_func(std::stop_token const &stop_token, local_id id)
{
    int const thread_count = size();

    std::unique_lock lk(_lock);

    while (_available_cv.wait(lk, stop_token, [&] { return _available_work < _target_work; })) {
        execute_batch(lk, id, thread_count);
    }
}

void dispatch_pool::execute_batch(std::unique_lock<std::mutex> &lk, local_id id, int thread_count)
{
    // Determine how much work to take
    global_id const batch_size = (_target_work + thread_count - 1) / thread_count;
    global_id const start = _available_work;
    global_id const end = std::min(start + batch_size, _target_work);

    // Take that much work
    _available_work = end;

    // Unlock and begin executing the function
    {
        lk.unlock();

        // Now that the lock is released, potentially signal work availability
        // to the next waiting thread
        _available_cv.notify_one();

        // Execute the function
        for (global_id index = start; index < end; index++) {
            _function(index, id);
        }

        lk.lock();
    }

    // Signal completion
    _completed_work += (end - start);

    if (_completed_work == _target_work) {
        _completed_cv.notify_one();
    }
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

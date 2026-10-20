// SPDX-License-Identifier: GPL-2.0-or-later
/** \file Background task
 * A \a background task that encapsulates worker thread and reports progress, results and state transitions in a thread-safe manner.
 */
#ifndef INKSCAPE_ASYNC_BACKGROUND_TASK_H
#define INKSCAPE_ASYNC_BACKGROUND_TASK_H

#include <cassert>
#include <chrono>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include "async/async.h"
#include "async/background-progress.h"
#include "async/channel.h"
#include "async/progress.h"

namespace Inkscape {
namespace Async {

// BackgroundTask object simplifies off-loading lengthy operations to a worker thread.
// It accepts a workload to perform and optionally several callbacks.
//
// Example:
// auto task = BackgroundTask<int, double>({
//   [required]
//  .work = <function to perform async work taking 'Async::Progress&' as input and returning 'int' result>,
//   [optional]
//  .on_started = <function invoked right after thread starts execution of async work>,
//   [optional]
//  .on_progress = <function invoked periodically on a GUI thread in response to worker reporting progress 0..1>,
//   [optional]
//  .throttle_time = <time interval to throttle progress reports>
//   [optional]
//  .on_complete = <function invoked with the result of successfull async work>,
//   [optional]
//  .on_cancelled = <function invoked when the task has been cancelled>
//   [optional]
//  .on_finished = <function invoked always after the task has finished (either successfully, or cancelled, or exceptionally)>
// });
//
// For a worker function to be cancellable, it should periodically report progress or call `Progress::keepongoing()`
//
template<typename R, typename... T>
class BackgroundTask final {
    using duration = std::chrono::steady_clock::duration;
    struct params {
        // work to perform asynchronously
        std::function<R (Progress<T...>&)> work;
        // notification after thread started
        std::function<void ()> on_started = {};
        // periodic notifications reported by working async function
        std::function<void (T...)> on_progress = {};
        // throttle progress
        duration throttle_time = duration::zero();
        // result of async function after is successfully completed
        std::function<void (R)> on_complete = {};
        // notification sent when task has been requested to cancel execution (but it may still be running)
        std::function<void ()> on_cancelled = {};
        // notification sent when async work threw an exception
        std::function<void (std::exception_ptr)> on_exception = {};
        // notification sent when task completed, or threw an exception or has been requested to stop;
        // it's a conterpart to "start" notification
        std::function<void ()> on_finished = {};
    };
    enum class Status { start, complete, cancelled, exception };

public:
    BackgroundTask(params p) :
        _started(std::move(p.on_started)),
        _complete(std::move(p.on_complete)),
        _cancelled(std::move(p.on_cancelled)),
        _finished(std::move(p.on_finished))
    {
        run(std::move(p.work), std::move(p.on_progress), p.throttle_time);
    }

    BackgroundTask(BackgroundTask&& task) = default;
    BackgroundTask& operator = (BackgroundTask&& task) = default;

    BackgroundTask(const BackgroundTask&) = delete;
    BackgroundTask& operator = (const BackgroundTask&) = delete;

    BackgroundTask() {}

    ~BackgroundTask() {
        _task.close();
    }

    bool is_running() const {
        return !!_task;
    }

    void cancel() {
        // ask thread to stop by closing the channel
        _task.close();
        // conclude with cancel; thread won't report anything anymore after we closed the channel
        notify(Status::cancelled, std::optional<R>());
    }

private:
    class Task {
    public:
        Task(Async::Channel::Source source,
            std::function<R (Progress<T...>&)> work,
            std::function<void (T...)> progress,
            duration throttle,
            std::function<void (Status, std::optional<R>)> notify) :

            _channel(std::move(source)),
            _async_work(std::move(work)),
            _progress(std::move(progress)),
            _throttle(throttle),
            _notify(std::move(notify))
        {
        }

        Task(Task&&) = default;

        Task(const Task&) = delete;

        ~Task() {
        }

        void run_async() {
            // if background task is already closed/destructed then there's nothing to do
            if (!_channel) {
                return;
            }

            std::optional<R> maybe_result;
            auto status = Status::start;
            _channel.run([notify = _notify, status](){
                notify(status, std::optional<R>());
            });

            try {
                auto base_progress = Async::BackgroundProgress<T...>(_channel, _progress);
                auto throttled_progress = Async::ProgressTimeThrottler<T...>(base_progress, _throttle);
                auto progress = _throttle != duration::zero() ?
                    static_cast<Progress<T...>*>(&throttled_progress) :
                    static_cast<Progress<T...>*>(&base_progress);

                maybe_result.emplace(_async_work(*progress));

                status = Status::complete;
            }
            catch (CancelledException&) {
                status = Status::cancelled;
            }
            catch (...) {
                // todo: capture exception details
                status = Status::exception;
            }

            // conclude operation with status and result
            _channel.run([notify = std::move(_notify), result = std::move(maybe_result), status](){
                notify(status, std::move(result));
            });

            _channel.close();
        }

        Async::Channel::Source _channel;
        std::function<R (Progress<T...>&)> _async_work;
        std::function<void (T...)> _progress;
        duration _throttle;
        std::function<void (Status, std::optional<R>)> _notify;
    };

    void run(std::function<R (Progress<T...>&)> work, std::function<void (T...)> progress, duration throttle) {
        auto [src, dest] = Async::Channel::create();
        _task = std::move(dest); // keep our end of the channel to talk to the task

        // create async task now; it will call 'notify' after it's done on a GUI thread
        // (unless we cancel or close the channel first)
        Task task(std::move(src), std::move(work), std::move(progress), throttle,
            [this](Status s, std::optional<R> result){ notify(s, result); });

        // move task state into lambda capture and run it
        _future = std::async(std::launch::async, [this, task = std::move(task)] () mutable {
            task.run_async();
        });
    }

    // Emit notification(s) on a GUI thread
    void notify(Status status, std::optional<R> result) {
        if (status == Status::start) {
            if (_started) _started();

            return;
        }
        else if (status == Status::complete) {
            assert(result);
            if (_complete) _complete(*result);
        }
        else if (status == Status::cancelled) {
            if (_cancelled) _cancelled();
        }
        else if (status == Status::exception) {
            //todo
            // if (_exception) _exception(...);
        }

        _task.close();

        // unconditional: call finish
        if (_finished) _finished();
    }

    Async::Channel::Dest _task;
    std::function<void ()> _started;
    std::function<void (R)> _complete;
    std::function<void ()> _cancelled;
    std::function<void ()> _finished;
    std::future<void> _future;
};

} // namespace Async
} // namespace Inkscape

#endif // INKSCAPE_ASYNC_BACKGROUND_TASK_H

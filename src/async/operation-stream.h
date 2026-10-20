// SPDX-License-Identifier: GPL-2.0-or-later

/// Helper class to stream background task notifications as a series of messages.
/// It can be used to expose background task to clients, as well as intercept results.

#include <chrono>
#include <exception>
#include <functional>
#include <memory>
#include <sigc++/connection.h>
#include <sigc++/signal.h>
#include <tuple>
#include <utility>
#include <variant>
#include "async/background-task.h"
#include "async/progress.h"
#include <sigc++/scoped_connection.h>

namespace Inkscape {
namespace Async {

namespace Msg {

struct OperationStarted {};
template<typename R>
struct OperationResult { R result; };
template<typename... T>
struct OperationProgress { std::tuple<T...> progress; };
struct OperationCancelled {};
struct OperationException { std::exception_ptr exception; };
struct OperationFinished {};

template<typename R, typename... T>
using Message = std::variant<
    OperationStarted,
    OperationProgress<T...>,
    OperationResult<R>,
    OperationCancelled,
    OperationException,
    OperationFinished
>;

// helpers to retrieve individual messages; to be retired after pattern matching becomes available
template<typename R, typename... T>
const R* get_result(const Msg::Message<R, T...>& msg) {
    if (auto&& r = std::get_if<OperationResult<R>>(&msg)) {
        return &r->result;
    }
    return nullptr;
}

template<typename R, typename... T>
const std::tuple<T...>* get_progress(const Msg::Message<R, T...>& msg) {
    if (auto&& p = std::get_if<OperationProgress<T...>>(&msg)) {
        return &p->progress;
    }
    return nullptr;
}

template<typename R, typename... T>
bool is_finished(const Msg::Message<R, T...>& msg) {
    return !!std::get_if<OperationFinished>(&msg);
}

} // Msg


template<typename R, typename... T>
class OperationStream {
    using duration = std::chrono::steady_clock::duration;
public:
    OperationStream() {
    }

    ~OperationStream() {
    }

    void start(std::function<R (Progress<T...>&)> work, duration throttle_interval = duration::zero()) {
        _task.reset(new BackgroundTask<R, T...>({
            .work = std::move(work),
            .on_started    = [this]() { emit(Msg::OperationStarted {}); },
            .on_progress   = [this](T... p) { emit(Msg::OperationProgress<T...> {std::tuple<T...>(p...)}); },
            .throttle_time = throttle_interval,
            .on_complete   = [this](R result) { emit(Msg::OperationResult<R> {result = std::move(result)}); },
            .on_cancelled  = [this]() { emit(Msg::OperationCancelled {}); },
            .on_exception  = [this](std::exception_ptr ex) { emit(Msg::OperationException {ex}); },
            .on_finished   = [this]() { emit(Msg::OperationFinished {}); },
        }));
    }

    bool is_running() const { return _task && _task->is_running(); }

    sigc::connection subscribe(std::function<void (const Msg::Message<R, T...>&)> fn) {
        return _signal.connect(fn);
    }

    void cancel() {
        if (_task) {
            _task->cancel();
            _task.reset();
        }
    }

private:
    void emit(Msg::Message<R, T...> msg) {
        _signal.emit(msg);
    }

    std::unique_ptr<BackgroundTask<R, T...>> _task;
    sigc::signal<void (const Msg::Message<R, T...>&)> _signal;
};


} // namespace Async
} // namespace Inkscape

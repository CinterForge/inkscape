// SPDX-License-Identifier: GPL-2.0-or-later
/** \file Channel
 * Thread-safe communication channel for asyncs.
 */
#ifndef INKSCAPE_ASYNC_CHANNEL_H
#define INKSCAPE_ASYNC_CHANNEL_H

#include <memory>
#include <optional>
#include <mutex>
#include <utility>
#include <glibmm/dispatcher.h>
#include "util/funclog.h"

namespace Inkscape {
namespace Async {
namespace Channel {
namespace detail {

class Shared final
    : public std::enable_shared_from_this<Shared>
{
public:
    Shared()
    {
        dispatcher->connect([this] {
            auto ref = shared_from_this();
            grab().exec_while([this] { return is_open; });
        });
    }

    Shared(Shared const &) = delete;
    Shared &operator=(Shared const &) = delete;

    operator bool() const
    {
        auto g = std::lock_guard(mutables);
        return is_open;
    }

    template <typename F>
    bool run(F &&f) const
    {
        auto g = std::lock_guard(mutables);
        if (!is_open) return false;
        if (funclog.empty()) dispatcher->emit();
        funclog.emplace(std::forward<F>(f));
        return true;
    }

    void close()
    {
        disconnect_source();
        dispatcher.reset();
        funclog.clear();
    }

private:
    mutable std::mutex mutables;
    mutable std::optional<Glib::Dispatcher> dispatcher = std::make_optional<Glib::Dispatcher>();
    mutable Util::FuncLog funclog;
    bool is_open = true;

    Util::FuncLog grab() const
    {
        auto g = std::lock_guard(mutables);
        return std::move(funclog);
    }

    void disconnect_source()
    {
        auto g = std::lock_guard(mutables);
        is_open = false;
    }
};

struct Create;

} // namespace detail

class Source final
{
public:
    Source() = default;
    Source(Source const &) = delete;
    Source &operator=(Source const &) = delete;
    Source(Source &&) = default;
    Source &operator=(Source &&) = default;

    /**
     * Check whether the channel is still open.
     */
    explicit operator bool() const { return shared && shared->operator bool(); }

    /**
     * Attempt to run a function on the main loop that the Channel was created in. This will
     * either succeed and execute or destroy the function in the main loop's thread, or fail and
     * leave it untouched.
     *
     * \return Whether the Channel is still open at the time of calling.
     *
     * Note that a return value of true doesn't indicate whether the function will actually run,
     * because the Channel could be closed in the meantime. If it does run, it is guaranteed the
     * Dest object still exists and \a close() has not been called on it.
     */
    template <typename F>
    bool run(F &&f) const { return shared && shared->run(std::forward<F>(f)); }

    /**
     * Close the channel. No more functions submitted through run() will be run.
     */
    void close() { shared.reset(); }

private:
    std::shared_ptr<detail::Shared const> shared;
    explicit Source(std::shared_ptr<detail::Shared> shared_) : shared(std::move(shared_)) {}
    friend struct detail::Create;
};

class Dest final
{
public:
    Dest() = default;
    Dest(Dest const &) = delete;
    Dest &operator=(Dest const &) = delete;
    Dest(Dest &&) = default;
    Dest &operator=(Dest &&) = default;
    ~Dest() { close(); }

    /**
     * Close the channel. No further functions submitted by the other end will be run, and it will
     * be notified of closure whenever it checks.
     */
    void close() { if (shared) { shared->close(); shared.reset(); } }

    /**
     * Check whether \a close() has already been called, or if the channel was never opened.
     *
     * Note: This does not check whether the corresponding \a close() method of Source has been
     * called. In fact, this condition is meaningless without further synchronization. If you need
     * to know whether the Source has closed, you can have it manually send this information
     * over the Channel instead.
     */
    explicit operator bool() const { return (bool)shared; }

private:
    std::shared_ptr<detail::Shared> shared;
    explicit Dest(std::shared_ptr<detail::Shared> shared_) : shared(std::move(shared_)) {}
    friend struct detail::Create;
};

namespace detail {

struct Create
{
    Create() = delete;

    static auto create()
    {
        auto shared = std::make_shared<detail::Shared>();
        auto src = Source(shared);
        auto dst = Dest(std::move(shared));
        return std::make_pair(std::move(src), std::move(dst));
    }
};

} // namespace detail

/**
 * Create a linked Source - Destination pair forming a thread-safe communication channel.
 *
 * As long as the channel is still open, the Source can use it to run commands in the main loop of
 * the creation thread and check if the channel is still open. Destructing either end closes the channel.
 */
inline std::pair<Source, Dest> create()
{
    return detail::Create::create();
}

} // namespace Channel
} // namespace Async
} // namespace Inkscape

#endif // INKSCAPE_ASYNC_CHANNEL_H

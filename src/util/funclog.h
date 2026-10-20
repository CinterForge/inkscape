// SPDX-License-Identifier: GPL-2.0-or-later
/** \file FuncLog
 * A log of functions that can be appended to and played back later.
 */
#ifndef INKSCAPE_UTIL_FUNCLOG_H
#define INKSCAPE_UTIL_FUNCLOG_H

#include <utility>
#include <exception>
#include "util/pool.h"

namespace Inkscape {
namespace Util {

/**
 * A FuncLog is effectively a std::vector<std::function<void()>>, with the ability to hold
 * move-only function types and enforced run-once semantics.
 *
 * The main difference is an efficient internal representation that stores the contents nearly
 * contiguously. This gives a 2x speedup when std::function uses the small-lambda optimisation,
 * and a 7x speedup when it has to heap-allocate.
 */
class FuncLog final
{
public:
    FuncLog() = default;
    FuncLog(FuncLog &&other) noexcept { movefrom(other); }
    FuncLog &operator=(FuncLog &&other) noexcept { destroy(); movefrom(other); return *this; }
    ~FuncLog() { destroy(); }

    /**
     * Append a callable object to the log.
     * On exception, no object is inserted, though memory will not be returned immediately.
     */
    template <typename F>
    void emplace(F &&f)
    {
        using Fd = typename std::decay<F>::type;
        auto entry = pool.allocate<Entry<Fd>>();
        new (entry) Entry<Fd>(std::forward<F>(f));
        *lastnext = entry;
        lastnext = &entry->next;
        entry->next = nullptr;
    }

    /**
     * Execute and destroy each callable in the log.
     * On exception, all remaining callables are destroyed.
     * \post empty() == true
     */
    void exec();

    /// Convenience alias for exec().
    void operator()() { exec(); }

    /**
     * Execute and destroy each callable in the log while condition \a c() is true, then destroy the rest.
     * On exception, all remaining callables are destroyed.
     * \post empty() == true
     */
    template <typename C>
    void exec_while(C &&c)
    {
        for (auto h = first; h; destroy_and_advance(h)) {
            try {
                if (!c()) {
                    destroy_from(h);
                    break;
                }
                (*h)();
            } catch (...) {
                destroy_from(h);
                reset();
                std::rethrow_exception(std::current_exception());
            }
        }
        reset();
    }

    /**
     * Destroy all callables in the log without executing them.
     * \post empty() == true
     */
    void clear() { destroy(); reset(); }

    bool empty() const { return !first; }

private:
    struct Header
    {
        Header *next;
        virtual ~Header() = default;
        virtual void operator()() = 0;
    };

    template <typename Fd>
    struct Entry : Header
    {
        Fd f;
        template <typename F>
        Entry(F &&f) : f(std::forward<F>(f)) {}
        void operator()() override { f(); }
    };

    Pool pool;
    Header *first = nullptr;
    Header **lastnext = &first;

    void destroy() { destroy_from(first); }
    static void destroy_from(Header *h) { while (h) destroy_and_advance(h); }
    static void destroy_and_advance(Header *&h) noexcept;
    void reset() noexcept;
    void movefrom(FuncLog &other) noexcept;
};

} // namespace Util
} // namespace Inkscape

#endif // INKSCAPE_UTIL_FUNCLOG_H

// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INITLOCK_H
#define INITLOCK_H

#include <atomic>
#include <mutex>

/**
 * Almost entirely analogous to std::once_flag, but with the ability to be reset if the caller knows it is safe to do so.
 */
class InitLock
{
public:
    template <typename F>
    void init(F &&f) const
    {
        // Make sure the fast path is a single load-acquire - not guaranteed with std::once_flag.
        [[likely]] if (inited.load(std::memory_order_acquire)) {
            return;
        }

        std::call_once(once, [&] {
            f();
            inited.store(true, std::memory_order_release);
        });
    }

    void reset()
    {
        inited.store(false, std::memory_order_relaxed);

        // Abomination, but tenuously allowed by Standard.
        // (The alternative, std::mutex, is too large - we want to create a lot of these objects.)
        once.~once_flag();
        new (&once) std::once_flag();
    }

private:
    mutable std::once_flag once;
    mutable std::atomic<bool> inited = false;
};

#endif // INITLOCK_H

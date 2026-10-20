// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Static objects with destruction before main() exit.
 */
#ifndef INKSCAPE_UTIL_STATICS_H
#define INKSCAPE_UTIL_STATICS_H

#include <atomic>
#include <cassert>
#include <mutex>
#include "unsafe-optional.h"

/**
 * The following system provides a way of dealing with statics/singletons with unusual lifetime requirements,
 * specifically the requirement that they be destroyed before the end of main().
 *
 * This isn't guaranteed by the usual static initialisation idiom
 *
 *     X &get()
 *     {
 *         static X x;
 *         return x;
 *     }
 *
 * because X will be destroyed just *after* main() exits. And sometimes that's a deal-breaker!
 *
 *  - To use the system with a singleton class X, derive it from EnableSingleton<X>:
 *
 *        class X : public EnableSingleton<X> { ...
 *
 *    This endows it with a ::get() method that initialises and returns the static instance.
 *    It is safe against concurrent initialisation, like the idiom above.
 *    If the class has a private ctor/dtor, then adding
 * 
 *        friend class EnableSingleton;
 * 
 *    to X will also be necessary.
 *
 *  - To ensure that X is outlived by another singleton Y, pass in the dependency using Depends:
 *
 *        class X : public EnableSingleton<X, Depends<Y>> { ...
 *
 *    Multiple dependencies can be specified. Then Y will be destructed after X.
 *
 *    Note: Y will still be lazily-initialised, for startup efficiency. So X's lifetime isn't
 *    necessarily completely contained in Y's lifetime.
 *
 *  - To destruct all singletons at any time, call
 *
 *        Statics::destroy();
 *
 *    They will be recreated again if re-accessed. This is mainly intended for unit testing,
 *    so that the state of statics can be reset between tests.
 */

namespace Inkscape::Util {

/// Tag class used to represent a list of dependencies.
template <typename... Ts>
struct Depends;

namespace detail {

/// Helper class for unpacking @a Depends.
template <typename Deps>
struct ForEachDep;

template <typename... Ts>
struct ForEachDep<Depends<Ts...>>
{
    template <typename F>
    ForEachDep(F &&f)
    {
        ([&]<typename T> { // for each T in Ts
            f.template operator()<T>();
        }.template operator()<Ts>(), ...);
    }
};

/// Simple non-owning singly-linked list of callbacks.
struct FuncListItem
{
    virtual void exec() = 0;
    FuncListItem *next = nullptr;
};

} // namespace detail

/// Manages the global list of statics.
class Statics
{
public:
    Statics();
    ~Statics();

    /// Destroy all active statics. Only to be used for unit testing.
    static void destroy() { instance->clear_list(); }

private:
    /// Pointer to the global instance which lives on the stack of main().
    static Statics *instance;

    std::mutex lock;
    detail::FuncListItem *head = nullptr;

    void add_to_list(detail::FuncListItem *item);
    void clear_list();

    template <typename, typename> friend class EnableSingleton;
};

/// CRTP mixin class used to imbue a class with singleton functionality.
template <typename T, typename Deps = Depends<>>
class EnableSingleton
{
public:
    EnableSingleton(EnableSingleton const &) = delete;
    EnableSingleton &operator=(EnableSingleton const &) = delete;

    template <typename... Args>
    static T &get(Args&&... args)
    {
        assert(Statics::instance);

        auto &holder = getholder();

        [[unlikely]] if (!std::atomic_ref(inited).load(std::memory_order_acquire)) {
            // Create the holders in the correct order under the global lock.
            {
                auto guard = std::lock_guard(Statics::instance->lock);
                ensure_holder();
            }

            // Create the static instance.
            std::call_once(holder->once_flag, [&] {
                new (holder->opt.get_bytes()) T(std::forward<Args>(args)...); // bypass construct() to allow calling private ctors
                std::atomic_ref(inited).store(true, std::memory_order_release);
            });
        }

        return *holder->opt;
    }

protected:
    EnableSingleton() = default;

private:
    struct Holder : detail::FuncListItem
    {
        std::once_flag once_flag;
        UnsafeOptional<T> opt;

        void exec() override
        {
            auto &holder = getholder();
            if (inited) {
                holder->opt->~T(); // bypass destruct() to allow calling private dtors
            }
            holder.destruct();
            inited = false;
            holder_created = false;
        }
    };

    inline static bool inited = false; // Protected by std::atomic_ref.
    inline static bool holder_created = false; // Protected by Statics::lock.

    // inline static UnsafeOptional<Holder> holder;
    // Above doesn't compile with GCC (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=63296).
    // Use the following workaround:
    static auto &getholder()
    {
        static UnsafeOptional<Holder> holder;
        return holder;
    }

    /**
     * Recursively register our holder and all dependencies' holders.
     *
     * The order is such that dependencies' holders are always destroyed
     * after our holder.
     *
     * This function is always called while holding Statics::lock.
     *
     * Note that this function does no construction of the actual static
     * instances held by each holder; they are lazily-initialised.
     */
    static void ensure_holder()
    {
        if (holder_created) {
            return;
        }

        detail::ForEachDep<Deps>([]<typename Dep> {
            Dep::ensure_holder();
        });

        auto &holder = getholder();
        holder.construct();
        Statics::instance->add_to_list(holder.get());

        holder_created = true;
    }

    template <typename, typename> friend class EnableSingleton; // to allow recursion of ensure_holder()
};

} // namespace Inkscape::Util

#endif // INKSCAPE_UTIL_STATICS_H

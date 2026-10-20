// SPDX-License-Identifier: GPL-2.0-or-later
/** \file Async
 * Fire-and-forget asyncs without UB at program exit.
 *
 * This file provides asyncs whose futures do not block on destruction, while
 * ensuring program exit is delayed until all such asyncs have terminated, in
 * order to ensure clean termination of asyncs and avoid undefined behaivour.
 *
 * Related: https://open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3451.pdf
 */
#ifndef INKSCAPE_ASYNC_H
#define INKSCAPE_ASYNC_H

#include <future>
#include <utility>

namespace Inkscape::Async {
namespace detail {

void extend(std::future<void> &&future);

} // namespace detail

/**
 * Launch an async which will delay program exit until its termination.
 */
template <typename F>
inline void fire_and_forget(F &&f)
{
    detail::extend(std::async(std::launch::async, std::forward<F>(f)));
}

} // namespace Inkscape::Async

#endif // INKSCAPE_ASYNC_H

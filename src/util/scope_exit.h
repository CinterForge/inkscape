// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 *  Run code on scope exit.
 */

#ifndef INKSCAPE_UTIL_SCOPE_EXIT_H
#define INKSCAPE_UTIL_SCOPE_EXIT_H

#include <type_traits>
#include <utility>

// Todo: (C++23?) Replace with now-standardised version.
template <typename F>
class scope_exit
{
public:
    scope_exit(F &&f) : f(std::forward<F>(f)) {}
    scope_exit(scope_exit const &) = delete;
    scope_exit &operator=(scope_exit const &) = delete;
    ~scope_exit() { f(); }

private:
    std::decay_t<F> f;
};

#endif // INKSCAPE_UTIL_SCOPE_EXIT_H

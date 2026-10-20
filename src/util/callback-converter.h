// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 *  Callback converter for interfacing with C APIs.
 */
/*
 * Author: PBS <pbs3141@gmail.com>
 * Copyright (C) 2022 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef INKSCAPE_UTIL_CALLBACK_CONVERTER_H
#define INKSCAPE_UTIL_CALLBACK_CONVERTER_H

#include <utility>

namespace Inkscape::Util {
namespace detail {

template <auto Fp>
struct CallbackConverter
{
    template <typename T>
    struct Helper;

    template <typename Ret, typename Obj, typename... Args>
    struct Helper<Ret(Obj::*)(Args...)>
    {
        // Unary plus converts lambda to function pointer.
        static constexpr auto result = +[] (Args... args, void *data) -> Ret {
            return (reinterpret_cast<Obj*>(data)->*Fp)(std::forward<Args>(args)...);
        };
    };

    static constexpr auto result = Helper<decltype(Fp)>::result;
};

} // namespace detail

/**
 * Given a member function, make_c_callback produces a pure function with an extra void* argument at the end,
 * into which an object pointer can be passed. Calling the pure function then invokes the original
 * function on this object. In other words
 *
 *     make_c_callback<&X::f>(..., &x)
 *
 * is equivalent to
 *
 *     x->f(...);
 *
 * This is useful for passing member functions as callbacks to C code.
 *
 * Note: Actually they're not completely equivalent in that some extra forwarding might go on.
 * Specifically, if your member function takes a T (by value) then the resulting callback
 * will also take a T by value (because make_c_callback always exactly preserves argument types). That means
 * your T will have to be moved from the wrapping function's argument into the wrapped function's
 * argument. This won't make much difference if you only use this with C-compatible types.
 */
template <auto Fp>
constexpr auto make_c_callback = detail::CallbackConverter<Fp>::result;

/**
 * A worse version of make_c_callback that also casts the result to a GCallback, losing even more type-safety.
 * Commonly needed to interface with Glib and GTK. (See make_c_callback for more details.)
 */
template <auto Fp>
inline auto make_g_callback = reinterpret_cast<void(*)()>(make_c_callback<Fp>); // inline instead of constexpr due to reinterpret_cast

} // namespace Inkscape::Util

#endif // INKSCAPE_UTIL_CALLBACK_CONVERTER_H

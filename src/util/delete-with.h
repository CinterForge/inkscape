// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Ad-hoc smart pointer useful when interfacing with C code.
 */
#ifndef INKSCAPE_UTIL_DELETE_WITH_H
#define INKSCAPE_UTIL_DELETE_WITH_H

#include <memory>

namespace Inkscape::Util {

/// Turn a function into a function object that can be used as a deleter for a smart pointer.
template <auto f>
struct Deleter
{
    void operator()(auto p) { f(p); }
};

/**
 * Wrap a raw pointer in a std::unique_ptr with a custom function as the deleter.
 * Example:
 *
 *     auto x = delete_with<g_free>(g_strdup(...));
 */
template <auto f, typename T>
auto delete_with(T *p)
{
    return std::unique_ptr<T, Deleter<f>>(p);
}

} // namespace Inkscape::Util

#endif // INKSCAPE_UTIL_DELETE_WITH_H

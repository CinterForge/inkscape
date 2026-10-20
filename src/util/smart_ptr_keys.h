// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Enable smart pointers to be used as map keys.
 */
#ifndef INKSCAPE_UTIL_SMART_PTR_KEYS_H
#define INKSCAPE_UTIL_SMART_PTR_KEYS_H

#include <functional> // for std::hash

template <typename T>
struct TransparentPtrHash
{
    using is_transparent = void;

    template <typename U>
    size_t operator()(U const &u) const
    {
        return std::hash<T const *>{}(&*u);
    }
};

template <typename T>
struct TransparentPtrLess
{
    using is_transparent = void;

    template <typename U, typename V>
    bool operator()(U const &u, V const &v) const
    {
        using Tp = T const *;
        return Tp{&*u} < Tp{&*v};
    }
};

template <typename T>
struct TransparentPtrEqual
{
    using is_transparent = void;

    template <typename U, typename V>
    bool operator()(U const &u, V const &v) const
    {
        using Tp = T const *;
        return Tp{&*u} == Tp{&*v};
    }
};

#endif // INKSCAPE_UTIL_SMART_PTR_KEYS_H

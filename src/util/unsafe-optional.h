// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Optional-like type with manual construction/destruction for low-level code.
 */
#ifndef INKSCAPE_UTIL_UNSAFE_OPTIONAL_H
#define INKSCAPE_UTIL_UNSAFE_OPTIONAL_H

#include <cstddef>
#include <new>
#include <utility>

namespace Inkscape::Util {

template <typename T>
class alignas(alignof(T)) UnsafeOptional
{
public:
    UnsafeOptional() = default;

    UnsafeOptional(UnsafeOptional const &) = delete;
    UnsafeOptional &operator=(UnsafeOptional const &) = delete;

    template <typename... Args>
    void construct(Args&&... args) { new (get()) T(std::forward<Args>(args)...); }

    void destruct() { get()->~T(); }

    std::byte *get_bytes() { return &_storage[0]; }
    std::byte const *get_bytes() const { return &_storage[0]; }

    T *get() { return std::launder(reinterpret_cast<T *>(get_bytes())); }
    T const *get() const { return std::launder(reinterpret_cast<T const *>(get_bytes())); }

    T *operator->() { return get(); }
    T const *operator->() const { return get(); }

    T &operator*() { return *get(); }
    T const &operator*() const { return *get(); }

private:
    std::byte _storage[sizeof(T)];
};

} // namespace Inkscape::Util

#endif // INKSCAPE_UTIL_UNSAFE_OPTIONAL_H

// SPDX-License-Identifier: GPL-2.0-or-later
/** \file Pool
 * Block allocator optimised for many allocations that are freed all at once.
 */

#ifndef INKSCAPE_UTIL_POOL_H
#define INKSCAPE_UTIL_POOL_H

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace Inkscape::Util {

/**
 * A Pool is a block allocator with the following characteristics:
 *
 *   - When a block cannot be allocated from the current buffer, a new 50% larger buffer is requested.
 *   - When all blocks are freed, the largest buffer is retained.
 *
 * The only difference from std::pmr::monotonic_buffer_resource is the second point.
 * Like std::pmr::monotonic_buffer_resource, it is also not thread-safe.
 */
class Pool final
{
public:
    Pool() = default;
    Pool(Pool const &) = delete;
    Pool &operator=(Pool const &) = delete;
    Pool(Pool &&other) noexcept { movefrom(other); }
    Pool &operator=(Pool &&other) noexcept { movefrom(other); return *this; }

    /// Ensure that the next buffer requested has at least the specified size.
    void reserve(std::size_t size) { nextsize = std::max(nextsize, size); }

    /// Allocate a block of the given size and alignment.
    std::byte *allocate(std::size_t size, std::size_t alignment);

    /// Convenience function: allocate a block of size and aligment for T.
    template<typename T>
    T *allocate() { return reinterpret_cast<T*>(allocate(sizeof(T), alignof(T))); }

    /// Free all previous allocations, retaining the largest existing buffer for re-use.
    void free_all() noexcept;

private:
    std::vector<std::unique_ptr<std::byte[]>> buffers;
    std::byte *cur = nullptr, *end = nullptr;
    std::size_t cursize = 0, nextsize = 2;

    void movefrom(Pool &other) noexcept;
    void resetblock() noexcept;
};

} // namespace Inkscape::Util

#endif // INKSCAPE_UTIL_POOL_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim:filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99:

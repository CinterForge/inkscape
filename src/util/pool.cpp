// SPDX-License-Identifier: GPL-2.0-or-later

#include "pool.h"

#include <cassert>
#include <cstdint>
#include <utility>

namespace Inkscape::Util {

// Round up x to the next multiple of m.
static std::byte *round_up(std::byte *x, std::size_t m)
{
    auto y = reinterpret_cast<uintptr_t>(x);
    y = ((y - 1) / m + 1) * m;
    return reinterpret_cast<std::byte*>(y);
}

std::byte *Pool::allocate(std::size_t size, std::size_t alignment)
{
    auto a = round_up(cur, alignment);
    auto b = a + size;

    if (b <= end) {
        cur = b;
        return a;
    }

    cursize = std::max(nextsize, size + alignment - 1);
    buffers.push_back(std::make_unique_for_overwrite<std::byte[]>(cursize));

    resetblock();
    nextsize = cursize * 3 / 2;

    a = round_up(cur, alignment);
    b = a + size;

    assert(b <= end);
    cur = b;
    return a;
};

void Pool::free_all() noexcept
{
    if (buffers.empty()) return;
    if (buffers.size() > 1) {
        buffers.front() = std::move(buffers.back());
        buffers.resize(1);
    }
    resetblock();
}

void Pool::movefrom(Pool &other) noexcept
{
    buffers = std::move(other.buffers);
    cur = other.cur;
    end = other.end;
    cursize = other.cursize;
    nextsize = other.nextsize;

    other.buffers.clear();
    other.cur = nullptr;
    other.end = nullptr;
    other.cursize = 0;
    other.nextsize = 2;
}

void Pool::resetblock() noexcept
{
    assert(!buffers.empty());
    cur = buffers.back().get();
    end = cur + cursize;
}

} // namespace Inkscape::Util

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

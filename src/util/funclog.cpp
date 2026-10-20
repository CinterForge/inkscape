// SPDX-License-Identifier: GPL-2.0-or-later
#include "funclog.h"

namespace Inkscape {
namespace Util {

void FuncLog::exec()
{
    for (auto h = first; h; destroy_and_advance(h)) {
        try {
            (*h)();
        } catch (...) {
            destroy_from(h);
            reset();
            std::rethrow_exception(std::current_exception());
        }
    }
    reset();
}

void FuncLog::destroy_and_advance(Header *&h) noexcept
{
    auto next = h->next;
    h->~Header();
    h = next;
}

void FuncLog::reset() noexcept
{
    pool.free_all();
    first = nullptr;
    lastnext = &first;
}

void FuncLog::movefrom(FuncLog &other) noexcept
{
    pool = std::move(other.pool);
    first = other.first;
    lastnext = first ? other.lastnext : &first;

    other.first = nullptr;
    other.lastnext = &other.first;
}

} // namespace Util
} // namespace Inkscape

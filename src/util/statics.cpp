// SPDX-License-Identifier: GPL-2.0-or-later
#include "statics.h"

namespace Inkscape::Util {

Statics *Statics::instance = nullptr;

Statics::Statics()
{
    assert(!instance);
    instance = this;
}

Statics::~Statics()
{
    assert(instance);
    clear_list();
    instance = nullptr;
}

void Statics::add_to_list(detail::FuncListItem *holder)
{
    holder->next = head;
    head = holder;
}

void Statics::clear_list()
{
    auto guard = std::unique_lock(lock);

    while (head) {
        auto n = head;
        head = head->next;

        guard.unlock();

        // Don't run arbitrary code while holding a lock.
        n->exec();

        guard.lock();
    }
}

} // namespace Inkscape::Util

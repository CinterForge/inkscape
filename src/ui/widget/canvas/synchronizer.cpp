// SPDX-License-Identifier: GPL-2.0-or-later

#include "synchronizer.h"
#include <cassert>

namespace Inkscape::UI::Widget {

Synchronizer::Synchronizer()
{
    dispatcher.connect([this] { on_dispatcher(); });
}

void Synchronizer::signalExit() const
{
    auto lock = std::unique_lock(mutables);
    awaken();
    assert(slots.empty());
    exitposted = true;
}

void Synchronizer::runInMain(std::function<void()> const &f) const
{
    auto lock = std::unique_lock(mutables);
    awaken();
    auto s = Slot{ &f };
    slots.emplace_back(&s);
    assert(!exitposted);
    slots_cond.wait(lock, [&] { return !s.func; });
}

void Synchronizer::waitForExit() const
{
    auto lock = std::unique_lock(mutables);
    main_blocked = true;
    while (true) {
        if (!slots.empty()) {
            process_slots(lock);
        } else if (exitposted) {
            exitposted = false;
            break;
        }
        main_cond.wait(lock);
    }
    main_blocked = false;
}

sigc::connection Synchronizer::connectExit(sigc::slot<void()> const &slot)
{
    return signal_exit.connect(slot);
}

void Synchronizer::awaken() const
{
    if (exitposted || !slots.empty()) {
        return;
    }

    if (main_blocked) {
        main_cond.notify_all();
    } else {
        const_cast<Glib::Dispatcher&>(dispatcher).emit(); // Glib::Dispatcher is const-incorrect.
    }
}

void Synchronizer::on_dispatcher() const
{
    auto lock = std::unique_lock(mutables);
    if (!slots.empty()) {
        process_slots(lock);
    } else if (exitposted) {
        exitposted = false;
        lock.unlock();
        signal_exit.emit();
    }
}

void Synchronizer::process_slots(std::unique_lock<std::mutex> &lock) const
{
    while (!slots.empty()) {
        auto slots_grabbed = std::move(slots);
        lock.unlock();
        for (auto &s : slots_grabbed) {
            (*s->func)();
        }
        lock.lock();
        for (auto &s : slots_grabbed) {
            s->func = nullptr;
        }
        slots_cond.notify_all();
    }
}

} // namespace Inkscape::UI::Widget

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

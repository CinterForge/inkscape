// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_WIDGET_CANVAS_SYNCHRONIZER_H
#define INKSCAPE_UI_WIDGET_CANVAS_SYNCHRONIZER_H

#include <condition_variable>
#include <glibmm/dispatcher.h>

namespace Inkscape::UI::Widget {

// Synchronisation primitive suiting the canvas's needs. All synchronisation between the main/render threads goes through here.
class Synchronizer
{
public:
    Synchronizer();

    // Background side:

    // Indicate that the background process has exited, causing EITHER signal_exit to be emitted OR waitforexit() to unblock.
    void signalExit() const;

    // Block until the given function has executed in the main thread, possibly waking it up if it is itself blocked.
    // (Note: This is necessary for servicing occasional buffer mapping requests where one can't be pulled from a pool.)
    void runInMain(std::function<void()> const &f) const;

    // Main-thread side:

    // Block until the background process has exited, gobbling the emission of signal_exit in the process.
    void waitForExit() const;

    // Connect to signal_exit.
    sigc::connection connectExit(sigc::slot<void()> const &slot);

private:
    struct Slot
    {
        std::function<void()> const *func;
    };

    Glib::Dispatcher dispatcher; // Used to wake up main thread if idle in GTK main loop.
    sigc::signal<void()> signal_exit;

    mutable std::mutex mutables;
    mutable bool exitposted = false;
    mutable bool main_blocked = false; // Whether main thread is blocked in waitForExit().
    mutable std::condition_variable main_cond; // Used to wake up main thread if blocked.
    mutable std::vector<Slot*> slots; // List of functions from runInMain() waiting to be run.
    mutable std::condition_variable slots_cond; // Used to wake up render threads blocked in runInMain().

    void awaken() const;
    void on_dispatcher() const;
    void process_slots(std::unique_lock<std::mutex> &lock) const;
};

} // namespace Inkscape::UI::Widget

#endif // INKSCAPE_UI_WIDGET_CANVAS_SYNCHRONIZER_H

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

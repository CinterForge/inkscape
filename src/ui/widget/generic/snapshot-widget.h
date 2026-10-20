// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 8/24/25.
//

#ifndef SNAPSHOT_WIDGET_H
#define SNAPSHOT_WIDGET_H

#include <gtkmm/widget.h>

namespace Gtk {
class Builder;
}

namespace Inkscape::UI::Widget {

// Simple widget rendering custom content by delegating it to a snapshot taking function

class SnapshotWidget : public Gtk::Widget {
public:
    SnapshotWidget() = default;
    SnapshotWidget(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder) : Gtk::Widget(cobject) {}
    ~SnapshotWidget() override = default;

    void set_snapshot_func(const sigc::slot<void (const Glib::RefPtr<Gtk::Snapshot>& snapshot, int width, int height)>& fn) {
        _take_snapshot = fn;
        queue_draw();
    }

private:
    void snapshot_vfunc(const Glib::RefPtr<Gtk::Snapshot>& snapshot) override;

    sigc::slot<void (const Glib::RefPtr<Gtk::Snapshot>& snapshot, int width, int height)> _take_snapshot;
};

} // namespace

#endif //SNAPSHOT_WIDGET_H

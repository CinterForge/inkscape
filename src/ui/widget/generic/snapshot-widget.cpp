// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 8/24/25.
//

#include "snapshot-widget.h"

namespace Inkscape::UI::Widget {

void SnapshotWidget::snapshot_vfunc(const Glib::RefPtr<Gtk::Snapshot>& snapshot) {
    int width = get_width();
    int height = get_height();
    if (!width || !height) return;

    if (_take_snapshot) {
        _take_snapshot(snapshot, width, height);
    }
}

} // namespace

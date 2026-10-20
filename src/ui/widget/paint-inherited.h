// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 11/1/25.
//

#ifndef INKSCAPE_PAINT_INHERITED_H
#define INKSCAPE_PAINT_INHERITED_H

#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <optional>

#include "paint-enums.h"
#include "ui/operation-blocker.h"

namespace Inkscape::UI::Widget {

class PaintInherited : public Gtk::Box {
public:
    PaintInherited();
    PaintInherited(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>&);

    // update UI to reflect 'mode'
    void set_mode(std::optional<PaintDerivedMode> maybe_mode);

    // get current UI state
    PaintDerivedMode get_mode() const;

    // signal fired when the user changes inherited paint mode
    sigc::signal<void (PaintDerivedMode)>& signal_mode_changed() {
        return _signal_mode_changed;
    }
private:
    void construct();
    OperationBlocker _update;
    sigc::signal<void (PaintDerivedMode)> _signal_mode_changed;
    Glib::RefPtr<Gtk::Builder> _builder;
};

}

#endif //INKSCAPE_PAINT_INHERITED_H

// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 11/1/25.
//

#include "paint-inherited.h"

#include <gtkmm/checkbutton.h>

#include "style-internal.h"
#include "ui/builder-utils.h"

namespace Inkscape::UI::Widget {

namespace {

// inherited paint variants
const std::array<std::pair<const char*, PaintDerivedMode>, 5> derived_paints = {
    std::pair{"paint-unset", PaintDerivedMode::Unset},
    {"paint-inherit",        PaintDerivedMode::Inherit},
    {"paint-context-stroke", PaintDerivedMode::ContextStroke},
    {"paint-context-fill",   PaintDerivedMode::ContextFill},
    {"paint-current-color",  PaintDerivedMode::CurrentColor}
};

} // namespace

void PaintInherited::construct() {
    _builder = create_builder("paint-inherit.ui");

    for (auto& [id, mode] : derived_paints) {
        auto& btn = get_widget<Gtk::CheckButton>(_builder, id);
        btn.signal_toggled().connect([this, mode] {
            if (!_update.pending()) {
                _signal_mode_changed.emit(mode);
            }
        });
    }

    append(get_widget<Gtk::Box>(_builder, "main"));
}

PaintInherited::PaintInherited() {
    construct();
}

PaintInherited::PaintInherited(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>&):
    Gtk::Box(cobject) {

    construct();
}

void PaintInherited::set_mode(std::optional<PaintDerivedMode> maybe_mode) {
    auto scoped(_update.block());

    if (!maybe_mode) {
        // mode cannot be determined, is mixed, or does not apply; clear all radio buttons
        for (auto [id, val] : derived_paints) {
            auto& btn = get_widget<Gtk::CheckButton>(_builder, id);
            btn.set_active(false);
        }
        return;
    }

    auto mode = maybe_mode.value();
    if (mode == PaintDerivedMode::Inherit) {
        // "inherit" keyword and unset paint are currently both represented by "from ancestor" button
        mode = PaintDerivedMode::Unset;
    }
    for (auto [id, val] : derived_paints) {
        if (mode == val) {
            auto& btn = get_widget<Gtk::CheckButton>(_builder, id);
            btn.set_active();
            break;
        }
    }
}

PaintDerivedMode PaintInherited::get_mode() const {
    for (auto& [id, mode] : derived_paints) {
        auto& btn = get_widget<Gtk::CheckButton>(_builder, id);
        if (btn.get_active()) {
            return mode;
        }
    }

    g_warning("PaintInherited::get_mode - mode has not been set.");
    return PaintDerivedMode::Unset;
}

} // namespace

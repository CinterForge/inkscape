// SPDX-License-Identifier: GPL-2.0-or-later
#include "stack.h"

#include <gtkmm/binlayout.h>
#include "ui/containerize.h"

namespace Inkscape::UI::Widget {

Stack::Stack()
{
    set_name("Stack");
    set_layout_manager(Gtk::BinLayout::create());
    containerize(*this);
}

void Stack::add(Gtk::Widget &widget)
{
    widget.set_sensitive(false);
    widget.set_parent(*this);
}

void Stack::remove(Gtk::Widget &widget)
{
    if (&widget == _active) {
        _active = nullptr;
    }
    widget.unparent();
}

void Stack::setActive(Widget *widget)
{
    if (widget == _active) {
        return;
    }

    if (_active) {
        _active->set_sensitive(false);
    }

    _active = widget;

    if (_active) {
        _active->set_sensitive(true);
    }

    queue_draw();
}

void Stack::snapshot_vfunc(Glib::RefPtr<Gtk::Snapshot> const &snapshot)
{
    if (_active) {
        snapshot_child(*_active, snapshot);
    }
}

} // namespace Inkscape::UI::Widget

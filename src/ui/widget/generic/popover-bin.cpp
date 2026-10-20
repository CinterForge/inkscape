// SPDX-License-Identifier: GPL-2.0-or-later
#include "popover-bin.h"

#include <gtkmm/binlayout.h>
#include "ui/containerize.h"

namespace Inkscape::UI::Widget {

PopoverBin::PopoverBin()
{
    set_name("PopoverBin");
    set_layout_manager(Gtk::BinLayout::create());
    containerize(*this);
}

void PopoverBin::_replace(Gtk::Widget *&holder, Gtk::Widget *widget)
{
    if (widget == holder) {
        return;
    }

    if (holder) {
        holder->unparent();
    }

    holder = widget;

    if (holder) {
        holder->set_parent(*this);
    }
}

} // namespace Inkscape::UI::Widget

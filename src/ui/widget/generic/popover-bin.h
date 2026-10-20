// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_WIDGET_POPOVER_BIN_H
#define INKSCAPE_UI_WIDGET_POPOVER_BIN_H

#include <gtkmm/popover.h>

namespace Inkscape::UI::Widget {

/**
 * Holds a single child widget while allowing a single popover to be displayed over it.
 *
 * Setting another popover displaces (unparents) the old one. If it is managed, this
 * typically also causes it to be deleted.
 *
 * Thus, a useful pattern to deal with popovers that are constructed on-the-fly is to
 * create it using Gtk::make_managed(), then attach it using setPopover().
 */
class PopoverBin : public Gtk::Widget
{
public:
    PopoverBin();

    void setChild(Gtk::Widget *child) { _replace(_child, child); }
    void setPopover(Gtk::Popover *popover) { _replace(_popover, popover); }

private:
    void _replace(Gtk::Widget *&holder, Gtk::Widget *widget);

    Gtk::Widget *_child = nullptr;
    Gtk::Widget *_popover = nullptr;
};

} // namespace Inkscape::UI::Widget

#endif // INKSCAPE_UI_WIDGET_POPOVER_BIN_H

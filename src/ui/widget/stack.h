// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_WIDGET_STACK_H
#define INKSCAPE_UI_WIDGET_STACK_H

#include <gtkmm/widget.h>

namespace Inkscape::UI::Widget {

/// Like Gtk::Stack, but for holding a stack of Inkscape canvases.
///
/// The main difference is that widgets retain their previous allocation on becoming hidden,
/// i.e. their width/height aren't set to zero.
///
/// This is needed to suport generating previews for background tabs.
class Stack : public Gtk::Widget
{
public:
    Stack();

    void add(Gtk::Widget &widget);
    void remove(Gtk::Widget &widget);
    void setActive(Gtk::Widget *widget);

protected:
    void snapshot_vfunc(Glib::RefPtr<Gtk::Snapshot> const &snapshot) override;

private:
    Gtk::Widget *_active = nullptr;
};

} // namespace Inkscape::UI::Widget

#endif // INKSCAPE_UI_WIDGET_STACK_H

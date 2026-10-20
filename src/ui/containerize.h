// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_CONTAINERIZE_H
#define INKSCAPE_UI_CONTAINERIZE_H

#include <gtkmm/widget.h>

namespace Inkscape::UI {

/**
 * Make a custom widget implement sensible memory management for its children.
 *
 * This frees the implementer of a custom widget from having to manually unparent()
 * children added with set_parent() both in the destructor and on signal_destroy(),
 * a memory management detail from C that has no business leaking into C++.
 *
 * Upon destruction, or for managed widgets just before, all children are unparented.
 * Managed children are also deleted if they have no other references.
 *
 * This function is typically called in the constructor of a custom widget that derives
 * from an intrinsically childless Gtk widget, e.g. Gtk::Widget, Gtk::DrawingArea.

 * It must not be used with any intrinsically child-containing Gtk widget, e.g.
 * Gtk::Box, Gtk::SpinButton.
 */
inline void containerize(Gtk::Widget &widget)
{
    g_signal_connect(widget.gobj(), "destroy", G_CALLBACK(+[] (GtkWidget *gobj, void *) {
        for (auto c = gtk_widget_get_first_child(gobj); c; ) {
            auto cnext = gtk_widget_get_next_sibling(c);
            gtk_widget_unparent(c);
            c = cnext;
        }
    }), nullptr);
}

} // namespace Inkscape::UI

#endif // INKSCAPE_UI_CONTAINERIZE_H

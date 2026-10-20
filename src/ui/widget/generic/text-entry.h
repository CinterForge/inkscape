// SPDX-License-Identifier: GPL-2.0-or-later
//
// This is a basic text entry with some light conveniences, like a signal that fires when
// the value in the entry should be committed/saved or reverting to original value upon Escape.

#ifndef INKSCAPE_UI_WIDGET_GENERIC_TEXT_ENTRY_H
#define INKSCAPE_UI_WIDGET_GENERIC_TEXT_ENTRY_H

#include <gtkmm/entry.h>
#include <sigc++/signal.h>

#include "ui/widget/gtk-registry.h"

namespace Gtk {
class EventControllerKey;
} // namespace Gtk

namespace Inkscape::UI::Widget {

class TextEntry : public BuildableWidget<TextEntry, Gtk::Entry> {
public:
    TextEntry();
    explicit TextEntry(GtkEntry* cobject, const Glib::RefPtr<Gtk::Builder>& builder = {});
    ~TextEntry() override = default;

    // Signal fired when the text is "finalized" and is ready to be saved or otherwise commited.
    // Likely, the user pressed Enter or focused out.
    sigc::signal<void ()>& signal_commit() { return _signal_commit; }

private:
    void construct();

    void _on_focus_change();
    bool _on_key_pressed(Gtk::EventControllerKey const &controller,
                         unsigned keyval, unsigned keycode, Gdk::ModifierType state);

    Glib::ustring _saved_value;
    sigc::signal<void ()> _signal_commit;
};

} // namespace Inkscape::UI::Widget

#endif // INKSCAPE_UI_WIDGET_GENERIC_TEXT_ENTRY_H

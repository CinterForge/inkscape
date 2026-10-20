// SPDX-License-Identifier: GPL-2.0-or-later

#include "text-entry.h"

#include <gtkmm/eventcontrollerkey.h>

namespace Inkscape::UI::Widget {

void TextEntry::construct()
{
    set_name("TextEntry");

    signal_activate().connect([this] { _signal_commit.emit(); });
    property_has_focus().signal_changed().connect(sigc::mem_fun(*this, &TextEntry::_on_focus_change));

    auto key = Gtk::EventControllerKey::create();
    key->signal_key_pressed().connect([this, &key = *key](auto &&...args) { return _on_key_pressed(key, args...); }, false);
    add_controller(key);
}

TextEntry::TextEntry()
    : Glib::ObjectBase("TextEntry")
{
    construct();
}

TextEntry::TextEntry(GtkEntry* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
    : Glib::ObjectBase("TextEntry")
    , BuildableWidget(cobject, builder)
{
    construct();
}

void TextEntry::_on_focus_change()
{
    // Gtk::Entry actually has a child Gtk::Text widget with some implementation details (its "delegate").
    // It handles the focus itself. But Gtkmm does not expose this, so we need to drop down to C briefly for it.
    auto delegete_editable = Glib::wrap(gtk_editable_get_delegate(GTK_EDITABLE(gobj())), true);
    auto delegate = std::dynamic_pointer_cast<Gtk::Widget>(delegete_editable);
    if (!delegate) {
        return;
    }

    if (delegate->has_focus()) {
        _saved_value = get_text();
    } else {
        _signal_commit.emit();
    }
}

bool TextEntry::_on_key_pressed(Gtk::EventControllerKey const &controller,
                               unsigned keyval, unsigned keycode, Gdk::ModifierType state)
{
    switch (keyval) {
        case GDK_KEY_Escape:
            set_text(_saved_value);
            return false; // do not steal this event, normal focusing out can continue to happen
    }
    return false;
}

} // namespace Inkscape::UI::Widget

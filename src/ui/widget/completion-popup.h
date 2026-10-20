// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef INKSCAPE_UI_WIDGET_COMPLETION_POPUP_H
#define INKSCAPE_UI_WIDGET_COMPLETION_POPUP_H

#include <gtkmm/box.h>
#include <gtkmm/builder.h>

#include "ui/widget/generic/popover-menu.h"

namespace Gtk {
class EntryCompletion;
class ListStore;
class MenuButton;
class Entry;
} // namespace Gtk

namespace Inkscape::UI::Widget {

class CompletionPopup : public Gtk::Box
{
public:
    CompletionPopup();
    ~CompletionPopup() override;

    PopoverMenu& get_menu();
    Gtk::Entry& get_entry();

    void clear_completion_list();
    void add_to_completion_list(int id, Glib::ustring name, Glib::ustring icon_name, Glib::ustring search_text = {});

    sigc::signal<void (int)>& on_match_selected();
    sigc::signal<void ()>& on_button_press();
    sigc::signal<bool ()>& on_focus();

private:
    bool onPopoverKeyPressed(unsigned keyval, unsigned keycode, Gdk::ModifierType state);
    Glib::RefPtr<Gtk::Builder> _builder;
    Glib::RefPtr<Gtk::ListStore> _list;
    Gtk::Entry& _search;
    Gtk::MenuButton& _button;
    PopoverMenu _popover_menu;
    Glib::RefPtr<Gtk::EntryCompletion> _completion;
    sigc::signal<void (int)> _match_selected;
    sigc::signal<void ()> _button_press;
    sigc::signal<bool ()> _on_focus;
    Glib::ustring _menu_search;
    void clear();
};

} // namespace Inkscape::UI::Widget

#endif // INKSCAPE_UI_WIDGET_COMPLETION_POPUP_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim:filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99:

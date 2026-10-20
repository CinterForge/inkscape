// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief A wrapper for Gtk::Notebook.
 */
 /*
 * Authors: see git history
 *   Tavmjong Bah
 *   Mike Kowalski
 *
 * Copyright (c) 2018 Tavmjong Bah, Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_DIALOG_NOTEBOOK_H
#define INKSCAPE_UI_DIALOG_NOTEBOOK_H

#include <gtkmm/notebook.h>
#include <gtkmm/scrolledwindow.h>

#include "dialog-container.h"
#include "preferences.h"
#include "ui/widget/generic/css-name-class-init.h"
#include "ui/widget/generic/tab-strip.h"
#include "ui/widget/generic/popover-menu.h"

namespace Glib {
class ValueBase;
} // namespace Glib

namespace Gdk {
class ContentProvider;
class Drag;
} // namespace Gdk

namespace Gtk {
class EventController;
class GestureClick;
} // namespace Gtk

namespace Inkscape::UI {

namespace Dialog {

class DialogWindow;

/**
 * A widget that wraps a Gtk::Notebook with dialogs as pages. Its tabs are hidden.
 * We use TabStrip to provide tabs for switching pages.
 *
 * A notebook is fixed to a specific DialogContainer which manages the dialogs inside the notebook.
 */
class DialogNotebook : public Widget::CssNameClassInit, public Gtk::ScrolledWindow
{
public:
    DialogNotebook(DialogContainer *container);
    ~DialogNotebook() override;

    void add_page(Gtk::Widget &page);
    void move_page(Gtk::Widget &page);
    void select_page(Gtk::Widget& page);
    Gtk::Widget* get_page(int position);
    static Gtk::Notebook* get_page_notebook(Gtk::Widget& page);

    // Getters
    Gtk::Notebook *get_notebook() { return &_notebook; }
    DialogContainer *get_container() { return _container; }

    // Notebook callbacks
    void close_tab(Gtk::Widget* page);
    void close_notebook();
    DialogWindow* pop_tab(Gtk::Widget* page);
    void dock_current_tab(DialogContainer::DockLocation location);
    Gtk::ScrolledWindow * get_scrolledwindow(Gtk::Widget &page);
    Gtk::ScrolledWindow * get_current_scrolledwindow(bool skip_scroll_provider);
    void set_requested_height(int height);
    int get_requested_height() const;
    DialogWindow* float_tab(Gtk::Widget& page);

private:
    // Widgets
    DialogContainer *_container;
    UI::Widget::PopoverMenu _menu_dialogs{Gtk::PositionType::BOTTOM, true};
    UI::Widget::PopoverMenu _menu_dock{Gtk::PositionType::BOTTOM};
    UI::Widget::PopoverMenu _menu_tab_ctx{Gtk::PositionType::BOTTOM, true};
    Gtk::Notebook _notebook;
    UI::Widget::TabStrip _tabs;
    void add_notebook_page(Gtk::Widget& page, int position);
    // move page from source notebook to this notebook
    void move_tab_from(DialogNotebook& source, Gtk::Widget& page, int position);
    // build dialog docking popup menu
    void build_docking_menu(UI::Widget::PopoverMenu& menu);
    // build menu listing all dialogs
    void build_dialog_menu(UI::Widget::PopoverMenu& menu);

    // State variables
    bool _detaching_duplicate = false;
    std::vector<sigc::scoped_connection> _conn;
    std::vector<sigc::scoped_connection> _connmenu;
    PrefObserver _label_pref;
    PrefObserver _tabclose_pref;

    static std::list<DialogNotebook *> _instances;
    void add_highlight_header();
    void remove_highlight_header();

    // Signal handlers - notebook
    void on_page_added(Gtk::Widget *page, int page_num);
    void on_page_removed(Gtk::Widget *page, int page_num);
    void size_allocate_vfunc(int width, int height, int baseline) final;
    void on_size_allocate_scroll  (int width);
    void on_page_switch(Gtk::Widget *page, guint page_number);
    bool on_scroll_event(double dx, double dy);
    // Helpers
    bool provide_scroll(Gtk::Widget &page);
    void change_page(size_t pagenum);
    void measure_vfunc(Gtk::Orientation orientation, int for_size, int &min, int &nat, int &min_baseline, int &nat_baseline) const override;
    // helper to correctly restore the height of vertically stacked dialogs
    int _natural_height = 0;
};

Gtk::Widget* find_dialog_page(Widget::TabStrip* tabs, int position);
DialogNotebook* find_dialog_notebook(Widget::TabStrip* tabs);

} // namespace Dialog

} // namespace Inkscape::UI

#endif // INKSCAPE_UI_DIALOG_NOTEBOOK_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

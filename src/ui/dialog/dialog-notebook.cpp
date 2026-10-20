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

#include <cassert>
#include <gtkmm/eventcontrollerscroll.h>
#include <gtkmm/grid.h>

#include "enums.h"
#include "inkscape-window.h"
#include "inkscape.h"
#include "ui/column-menu-builder.h"
#include "ui/dialog/dialog-base.h"
#include "ui/dialog/dialog-data.h"
#include "ui/dialog/dialog-multipaned.h"
#include "ui/dialog/dialog-window.h"
#include "ui/util.h"

namespace Inkscape::UI::Dialog {

std::list<DialogNotebook *> DialogNotebook::_instances;
static const Glib::Quark dialog_notebook_id("dialog-notebook-id");

DialogNotebook* find_dialog_notebook(Widget::TabStrip* tabs) {
    return static_cast<DialogNotebook*>(tabs->get_data(dialog_notebook_id));
}

Gtk::Widget* find_dialog_page(Widget::TabStrip* tabs, int position) {
    if (!tabs) return nullptr;

    if (auto notebook = find_dialog_notebook(tabs)) {
        return notebook->get_page(position);
    }
    return nullptr;
}

/**
 * DialogNotebook constructor.
 *
 * @param container the parent DialogContainer of the notebook.
 */
DialogNotebook::DialogNotebook(DialogContainer* container)
    : CssNameClassInit{"notebook"}
    , _container(container)
{
    set_name("DialogNotebook");
    set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::NEVER);
    set_has_frame(false);
    set_vexpand(true);
    set_hexpand(true);

    // =========== Getting preferences ==========
    auto prefs = Inkscape::Preferences::get();

    _label_pref = prefs->createObserver("/options/notebooklabels/value", [=,this](const auto& entry) {
        auto status = entry.getInt();
        auto labels = UI::Widget::TabStrip::ShowLabels::Never;
        if (status == PREFS_NOTEBOOK_LABELS_AUTO) {
            labels = UI::Widget::TabStrip::ShowLabels::Always;
        }
        else if (status == PREFS_NOTEBOOK_LABELS_ACTIVE) {
            labels = UI::Widget::TabStrip::ShowLabels::ActiveOnly;
        }
        _tabs.set_show_labels(labels);
    });
    _label_pref->call();

    _tabclose_pref = prefs->createObserver("/options/notebooktabs/show-closebutton", [this](const auto& entry) {
        _tabs.set_show_close_button(entry.getBool());
    });
    _tabclose_pref->call();

    // ============= Notebook menu ==============
    _notebook.set_name("DockedDialogNotebook");
    _notebook.set_show_border(false);
    _notebook.set_group_name("InkscapeDialogGroup");
    _notebook.set_scrollable(true);
    _notebook.set_show_tabs(false);

    // Watch for scroll events, to cycle through tabs
    auto scroll_controller = Gtk::EventControllerScroll::create();
    scroll_controller->set_flags(Gtk::EventControllerScroll::Flags::VERTICAL | Gtk::EventControllerScroll::Flags::DISCRETE);
    scroll_controller->signal_scroll().connect(sigc::mem_fun(*this, &DialogNotebook::on_scroll_event), false);
    _tabs.add_controller(scroll_controller);

    build_docking_menu(_menu_dock);
    build_docking_menu(_menu_tab_ctx);
    _tabs.set_tabs_context_popup(&_menu_tab_ctx);

    build_dialog_menu(_menu_dialogs);

    auto const menubtn = Gtk::make_managed<Gtk::MenuButton>();
    menubtn->set_icon_name("pan-down-symbolic");
    menubtn->set_has_frame(false);
    menubtn->set_popover(_menu_dock);
    menubtn->set_visible(true);
    menubtn->set_valign(Gtk::Align::CENTER);
    menubtn->set_halign(Gtk::Align::CENTER);
    menubtn->set_focusable(false);
    menubtn->set_can_focus(false);
    menubtn->set_focus_on_click(false);
    menubtn->set_name("DialogMenuButton");

    // =============== Signals ==================
    _conn.emplace_back(_notebook.signal_page_added().connect(sigc::mem_fun(*this, &DialogNotebook::on_page_added)));
    _conn.emplace_back(_notebook.signal_page_removed().connect(sigc::mem_fun(*this, &DialogNotebook::on_page_removed)));
    _conn.emplace_back(_notebook.signal_switch_page().connect(sigc::mem_fun(*this, &DialogNotebook::on_page_switch)));

    _tabs.set_hexpand();
    _tabs.set_new_tab_popup(&_menu_dialogs);
    _tabs.signal_select_tab().connect([this](auto& tab) {
        _tabs.select_tab(tab);
        int page_pos = _tabs.get_tab_position(tab);
        _notebook.set_current_page(page_pos);
        auto curr_page = _notebook.get_nth_page(page_pos);
        if (auto dialog = dynamic_cast<DialogBase*>(curr_page)) {
            dialog->focus_dialog();
        }
    });
    _tabs.signal_close_tab().connect([this](auto& tab) {
        auto index = _tabs.get_tab_position(tab);
        if (auto page = _notebook.get_nth_page(index)) {
            close_tab(page);
        }
    });
    _tabs.signal_float_tab().connect([this](auto& tab) {
        // make docked dialog float
        auto index = _tabs.get_tab_position(tab);
        if (auto page = _notebook.get_nth_page(index)) {
            float_tab(*page);
        }
    });
    _tabs.signal_move_tab().connect([this](auto& tab, int src_position, auto& source, int dest_position) {
        // move tab from source tabstrip/notebook here
        if (auto notebook = find_dialog_notebook(&source)) {
            if (auto page = notebook->get_page(src_position)) {
                move_tab_from(*notebook, *page, dest_position);
            }
        }
    });
    _tabs.signal_tab_rearranged().connect([this](int from, int to) {
        if (auto page = _notebook.get_nth_page(from)) {
            _notebook.reorder_child(*page, to);
        }
    });
    _tabs.signal_dnd_begin().connect([this] {
        DialogMultipaned::add_drop_zone_highlight_instances();
        for (auto instance : _instances) {
            instance->add_highlight_header();
        }
    });
    _tabs.signal_dnd_end().connect([this](bool) {
        // Remove dropzone highlights
        DialogMultipaned::remove_drop_zone_highlight_instances();
        for (auto instance : _instances) {
            instance->remove_highlight_header();
        }
    });
    // remember tabs owner
    _tabs.set_data(dialog_notebook_id, this);

    // Add wrapper boxes around the tabs widget, to establish the widget hierarchy that css themes
    // will expect - notebook > header > tabs > tab.
    // Manually construct this box with C, to easily add a css-name
    auto content_obj = GTK_BOX(g_object_new(GTK_TYPE_BOX,
                                            "css-name", "notebook",
                                            "orientation", GTK_ORIENTATION_VERTICAL,
                                            NULL));
    auto content = Gtk::manage(Glib::wrap(content_obj));

    // Manually construct this box with C, to easily add a css-name
    auto header_obj = GTK_BOX(g_object_new(GTK_TYPE_BOX, "css-name", "header", NULL));
    auto header = Gtk::manage(Glib::wrap(header_obj));
    header->add_css_class("top");
    header->append(_tabs);
    header->append(*menubtn);
    content->append(*header);

    auto sep = Gtk::make_managed<Gtk::Separator>(Gtk::Orientation::HORIZONTAL);
    sep->set_size_request(-1, 1);
    content->append(*sep);

    content->append(_notebook);
    set_child(*content);

    _instances.push_back(this);
}

void DialogNotebook::build_docking_menu(UI::Widget::PopoverMenu& menu) {
    const auto icon_size = Gtk::IconSize::NORMAL;

    // Docking menu options
    auto grid = Gtk::make_managed<Gtk::Grid>();
    grid->set_name("MenuDockingRect");
    auto make_item = [icon_size](const char* icon, const char* tooltip) {
        auto item = Gtk::make_managed<UI::Widget::PopoverMenuItem>("", true, icon, icon_size);
        item->set_tooltip_text(_(tooltip));
        return item;
    };
    auto dock_lt = make_item("dock-left-top", "Dock current tab at the top left");
    _conn.emplace_back(dock_lt->signal_activate().connect([this]{ dock_current_tab(DialogContainer::TopLeft); }));
    auto dock_rt = make_item("dock-right-top", "Dock current tab at the top right");
    _conn.emplace_back(dock_rt->signal_activate().connect([this]{ dock_current_tab(DialogContainer::TopRight); }));
    auto dock_lb = make_item("dock-left-bottom", "Dock current tab at the bottom left");
    _conn.emplace_back(dock_lb->signal_activate().connect([this]{ dock_current_tab(DialogContainer::BottomLeft); }));
    auto dock_rb = make_item("dock-right-bottom", "Dock current tab at the bottom right");
    _conn.emplace_back(dock_rb->signal_activate().connect([this]{ dock_current_tab(DialogContainer::BottomRight); }));
    // Move to new window
    auto floating = make_item("floating-dialog", "Move current tab to new window");
    floating->set_valign(Gtk::Align::CENTER);
    _conn.emplace_back(floating->signal_activate().connect([this]{ pop_tab(nullptr); }));
    grid->attach(*dock_lt, 0, 0);
    grid->attach(*dock_lb, 0, 1);
    grid->attach(*floating, 1, 0, 1, 2);
    grid->attach(*dock_rt, 2, 0);
    grid->attach(*dock_rb, 2, 1);
    int row = 0;
    menu.attach(*grid, 0, 1, row, row + 1);
    row++;
    auto sep = Gtk::make_managed<Gtk::Separator>(Gtk::Orientation::HORIZONTAL);
    sep->set_size_request(-1, 1);
    menu.attach(*sep, 0, 1, row, row + 1);
    row++;
    // Close tab
    auto new_menu_item = Gtk::make_managed<UI::Widget::PopoverMenuItem>(_("Close Tab"));
    _conn.emplace_back(new_menu_item->signal_activate().connect([this]{ close_tab(nullptr); }));
    menu.attach(*new_menu_item, 0, 1, row, row + 1);
    row++;
    // Close notebook
    new_menu_item = Gtk::make_managed<UI::Widget::PopoverMenuItem>(_("Close Panel"));
    _conn.emplace_back(new_menu_item->signal_activate().connect([this]{ close_notebook(); }));
    menu.attach(*new_menu_item, 0, 1, row, row + 1);

    if (Preferences::get()->getBool("/theme/symbolicIcons", true)) {
        menu.add_css_class("symbolic");
    }
}

void DialogNotebook::build_dialog_menu(UI::Widget::PopoverMenu& menu) {
    // dialogs are already ordered by categories, and that is exactly how they should be arranged in the menu
    auto dialog_data = get_dialog_data_list();
    const auto icon_size = Gtk::IconSize::NORMAL;
    int row = 0;
    auto builder = ColumnMenuBuilder<DialogData::Category>{menu, 2, icon_size, row};
    for (auto const &data : dialog_data) {
        if (data.category == DialogData::Diagnostics) {
            continue; // hide dev dialogs from dialogs menu
        }
        auto callback = [this, key = data.key]{
            // get desktop's container, it may be different than current '_container'!
            if (auto desktop = SP_ACTIVE_DESKTOP) {
                if (auto container = desktop->getContainer()) {
                    // open dialog and dock it here if request comes from the main window and dialog was not floating;
                    // if we are in a floating dialog window, then do not dock new dialog here, it is not useful
                    bool floating = DialogManager::singleton().should_open_floating(key);
                    container->new_dialog(key, container == _container && !floating ? this : nullptr, true);
                }
            }
        };
        builder.add_item(data.label, data.category, {}, data.icon_name, true, false, std::move(callback));
        if (builder.new_section()) {
            builder.set_section(gettext(dialog_categories[data.category]));
        }
    }

    if (Preferences::get()->getBool("/theme/symbolicIcons", true)) {
        menu.add_css_class("symbolic");
    }
}

DialogNotebook::~DialogNotebook()
{
    // disconnect signals first, so no handlers are invoked when removing pages
    _conn.clear();
    _connmenu.clear();

    // Unlink and remove pages
    for (int i = _notebook.get_n_pages(); i >= 0; --i) {
        DialogBase *dialog = dynamic_cast<DialogBase *>(_notebook.get_nth_page(i));
        _container->unlink_dialog(dialog);
        _notebook.remove_page(i);
    }

    _instances.remove(this);
}

void DialogNotebook::add_highlight_header()
{
    _notebook.add_css_class("nb-highlight");
}

void DialogNotebook::remove_highlight_header()
{
    _notebook.remove_css_class("nb-highlight");
}

/**
 * get provide scroll
 */
bool DialogNotebook::provide_scroll(Gtk::Widget &page) {
    auto const &dialog_data = get_dialog_data();
    auto dialogbase = dynamic_cast<DialogBase*>(&page);
    if (dialogbase) {
        auto data = dialog_data.find(dialogbase->get_type());
        if ((*data).second.provide_scroll == ScrollProvider::PROVIDE) {
            return true;
        }
    }
    return false;
}

Gtk::ScrolledWindow* DialogNotebook::get_scrolledwindow(Gtk::Widget &page)
{
    if (auto const child = page.get_first_child()) {
        if (auto const scrolledwindow = dynamic_cast<Gtk::ScrolledWindow *>(child)) {
            return scrolledwindow;
        }
    }
    return nullptr;
}

/**
 * Set provide scroll
 */
Gtk::ScrolledWindow* DialogNotebook::get_current_scrolledwindow(bool const skip_scroll_provider)
{
    auto const pagenum = _notebook.get_current_page();
    if (auto const page = _notebook.get_nth_page(pagenum)) {
        if (skip_scroll_provider && provide_scroll(*page)) {
            return nullptr;
        }
        return get_scrolledwindow(*page);
    }
    return nullptr;
}

/**
 * Adds a widget as a new page with a tab.
 */
void DialogNotebook::add_page(Gtk::Widget &page) {
    page.set_vexpand();

    // TODO: It is not exactly great to replace the passed pageʼs children from under it like this.
    //       But stopping it requires to ensure all references to page elsewhere are right/updated.
    if (auto const box = dynamic_cast<Gtk::Box *>(&page)) {
        auto const wrapper = Gtk::make_managed<Gtk::ScrolledWindow>();
        wrapper->set_vexpand(true);
        wrapper->set_propagate_natural_height(true);
        wrapper->set_overlay_scrolling(false);
        wrapper->get_style_context()->add_class("noborder");

        auto const wrapperbox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL,0);
        wrapperbox->set_vexpand(true);

        // This used to transfer pack-type and child properties, but now those are set on children.
        for (auto child = box->get_first_child(); child; ) {
            auto next = child->get_next_sibling();
            child->reference();
            box->remove(*child);
            wrapperbox->append(*child);
            child->unreference();
            child = next;
        }

        wrapper->set_child(*wrapperbox);
        box->append(*wrapper);

        if (provide_scroll(page)) {
            wrapper->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::EXTERNAL);
        } else {
            wrapper->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
        }
    }

    add_notebook_page(page, -1);
}

void DialogNotebook::add_notebook_page(Gtk::Widget& page, int position) {
    int page_number = _notebook.insert_page(page, position);
    _notebook.set_tab_reorderable(page);
    _notebook.set_tab_detachable(page);
    _notebook.set_current_page(page_number);
}

/**
 * Moves a page from a different notebook to this one.
 */
void DialogNotebook::move_page(Gtk::Widget &page)
{
    // Find old notebook
    auto old_notebook = get_page_notebook(page);
    if (!old_notebook) {
        std::cerr << "DialogNotebook::move_page: page not in notebook!" << std::endl;
        return;
    }
    if (old_notebook == &_notebook) {
        return; // no op
    }

    // Keep references until re-attachment
    page.reference();

    old_notebook->detach_tab(page);
    add_notebook_page(page, -1);
    // Remove unnecessary references
    page.unreference();

    // Set default settings for a new page
    _notebook.set_tab_reorderable(page);
    _notebook.set_tab_detachable(page);
}

void DialogNotebook::select_page(Gtk::Widget& page) {
    auto pos = _notebook.page_num(page);
    _notebook.set_current_page(pos);
}

// ============ Notebook callbacks ==============

/**
 * Callback to close the current active tab.
 */
void DialogNotebook::close_tab(Gtk::Widget* page) {
    int page_number = _notebook.get_current_page();

    if (page) {
        page_number = _notebook.page_num(*page);
    }

    if (dynamic_cast<DialogBase*>(_notebook.get_nth_page(page_number))) {
        // is this a dialog in a floating window?
        if (auto window = dynamic_cast<DialogWindow*>(_container->get_root())) {
            // store state of floating dialog before it gets deleted
            DialogManager::singleton().store_state(*window);
        }
    }

    // Remove page from notebook
    _notebook.remove_page(page_number);

    if (_notebook.get_n_pages() == 0) {
        close_notebook();
        return;
    }

    // Update tab labels by comparing the sum of their widths to the allocation
    on_size_allocate_scroll(get_width());
}

/**
 * Shutdown callback - delete the parent DialogMultipaned before destructing.
 */
void DialogNotebook::close_notebook()
{
    // Search for DialogMultipaned
    DialogMultipaned *multipaned = dynamic_cast<DialogMultipaned *>(get_parent());
    if (multipaned) {
        multipaned->remove(*this);
    } else if (get_parent()) {
        std::cerr << "DialogNotebook::close_notebook: Unexpected parent!" << std::endl;
    }
}

void DialogNotebook::move_tab_from(DialogNotebook& source, Gtk::Widget& page, int position) {
    auto& old_notebook = source._notebook;

    // Keep references until re-attachment
    page.reference();

    old_notebook.detach_tab(page);
    add_notebook_page(page, position);

    page.unreference();

    // Set default settings for a new page
    _notebook.set_tab_reorderable(page);
    _notebook.set_tab_detachable(page);

    if (old_notebook.get_n_pages() == 0) {
        source.close_notebook();
    }
}

Gtk::Widget* DialogNotebook::get_page(int position) {
    return _notebook.get_nth_page(position);
}

Gtk::Notebook* DialogNotebook::get_page_notebook(Gtk::Widget& page) {
    auto parent = page.get_parent();
    auto notebook = dynamic_cast<Gtk::Notebook*>(parent);
    if (!notebook && parent) {
        // page's parent might be a Stack
        notebook = dynamic_cast<Gtk::Notebook*>(parent->get_parent());
    }
    return notebook;
}

DialogWindow* DialogNotebook::float_tab(Gtk::Widget& page) {
    // Move page to notebook in new dialog window (attached to active InkscapeWindow).
    auto inkscape_window = _container->get_inkscape_window();
    auto window = new DialogWindow(inkscape_window, &page);
    window->set_visible(true);

    if (_notebook.get_n_pages() == 0) {
        close_notebook();
        return window;
    }

    // Update tab labels by comparing the sum of their widths to the allocation
    on_size_allocate_scroll(get_width());

    return window;
}

/**
 * Move the current active tab to a floating window.
 */
DialogWindow* DialogNotebook::pop_tab(Gtk::Widget* page) {
    // Find page.
    if (!page) {
        page = _notebook.get_nth_page(_notebook.get_current_page());
    }

    if (!page) {
        std::cerr << "DialogNotebook::pop_tab: page not found!" << std::endl;
        return nullptr;
    }

    return float_tab(*page);
}

void DialogNotebook::dock_current_tab(DialogContainer::DockLocation location) {
    auto page = _notebook.get_nth_page(_notebook.get_current_page());
    if (!page) return;

    // we need to get hold of the dialog container in the main window
    // (this instance may be in a floating dialog window)
    auto wnd = _container->get_inkscape_window();
    if (!wnd) return;
    auto container = wnd->get_desktop()->getContainer();
    if (!container) return;

    container->dock_dialog(*page, *this, location, nullptr, nullptr);
}

// ========= Signal handlers - notebook =========

/**
 * Signal handler to update dialog list when adding a page.
 */
void DialogNotebook::on_page_added(Gtk::Widget *page, int page_num)
{
    auto dialog = dynamic_cast<DialogBase*>(page);

    // Does current container/window already have such a dialog?
    if (dialog && _container->has_dialog_of_type(dialog)) {
        // We already have a dialog of the same type

        // Highlight first dialog
        auto other_dialog = _container->get_dialog(dialog->get_type());
        other_dialog->blink();

        // Remove page from notebook
        _detaching_duplicate = true; // HACK: prevent removing the initial dialog of the same type
        _notebook.detach_tab(*page);
        return;
    } else if (dialog) {
        // We don't have a dialog of this type

        // Add to dialog list
        _container->link_dialog(dialog);
    } else {
        // This is not a dialog
        return;
    }

    auto tab = _tabs.add_tab(dialog->get_name(), dialog->get_icon(), page_num);
    _tabs.select_tab(*tab);

    // Update tab labels by comparing the sum of their widths to the allocation
    on_size_allocate_scroll(get_width());
}

/**
 * Signal handler to update dialog list when removing a page.
 */
void DialogNotebook::on_page_removed(Gtk::Widget *page, int page_num)
{
    /**
     * When adding a dialog in a notebooks header zone of the same type as an existing one,
     * we remove it immediately, which triggers a call to this method. We use `_detaching_duplicate`
     * to prevent reemoving the initial dialog.
     */
    if (_detaching_duplicate) {
        _detaching_duplicate = false;
        return;
    }

    // Remove from dialog list
    DialogBase *dialog = dynamic_cast<DialogBase *>(page);
    if (dialog) {
        _container->unlink_dialog(dialog);
    }

    _tabs.remove_tab_at(page_num);
    _tabs.select_tab_at(_notebook.get_current_page());
}

void DialogNotebook::size_allocate_vfunc(int const width, int const height, int const baseline)
{
    Gtk::ScrolledWindow::size_allocate_vfunc(width, height, baseline);

    on_size_allocate_scroll(width);
}

/**
 * We need to remove the scrollbar to snap a whole DialogNotebook to width 0.
 *
 */
void DialogNotebook::on_size_allocate_scroll(int const width)
{
    // magic number
    static constexpr int MIN_HEIGHT = 60;
    //  set or unset scrollbars to completely hide a notebook
    // because we have a "blocking" scroll per tab we need to loop to avoid
    // other page stop out scroll
    for (auto &page : notebook_pages(_notebook)) {
        if (!provide_scroll(page)) {
            auto const scrolledwindow = get_scrolledwindow(page);
            if (scrolledwindow) {
                double height = scrolledwindow->get_allocation().get_height();
                if (height > 1) {
                    auto property = scrolledwindow->property_vscrollbar_policy();
                    auto const policy = property.get_value();
                    if (height >= MIN_HEIGHT && policy != Gtk::PolicyType::AUTOMATIC) {
                        property.set_value(Gtk::PolicyType::AUTOMATIC);
                    } else if (height < MIN_HEIGHT && policy != Gtk::PolicyType::EXTERNAL) {
                        property.set_value(Gtk::PolicyType::EXTERNAL);
                    } else {
                        // we don't need to update; break
                        break;
                    }
                }
            }
        }
    }
}

// [[nodiscard]] static int measure_min_width(Gtk::Widget const &widget)
// {
//     int min_width, ignore;
//     widget.measure(Gtk::Orientation::HORIZONTAL, -1, min_width, ignore, ignore, ignore);
//     return min_width;
// }

void DialogNotebook::on_page_switch(Gtk::Widget *curr_page, guint page) {
    _tabs.select_tab_at(page);
    if (auto dialog = dynamic_cast<DialogBase*>(curr_page)) {
        dialog->focus_dialog();
    }
}

bool DialogNotebook::on_scroll_event(double dx, double dy)
{
    if (_notebook.get_n_pages() <= 1) {
        return false;
    }

    if (dy < 0) {
        auto current_page = _notebook.get_current_page();
        if (current_page > 0) {
            _notebook.set_current_page(current_page - 1);
        }
    } else if (dy > 0) {
        auto current_page = _notebook.get_current_page();
        if (current_page < _notebook.get_n_pages() - 1) {
            _notebook.set_current_page(current_page + 1);
        }
    }
    return true;
}

/**
 * Helper method that change the page
 */
void DialogNotebook::change_page(size_t pagenum)
{
    _notebook.set_current_page(pagenum);
}

void DialogNotebook::measure_vfunc(Gtk::Orientation orientation, int for_size, int &min, int &nat, int &min_baseline, int &nat_baseline) const
{
    Gtk::ScrolledWindow::measure_vfunc(orientation, for_size, min, nat, min_baseline, nat_baseline);
    if (orientation == Gtk::Orientation::VERTICAL && _natural_height > 0) {
        nat = _natural_height;
        min = std::min(min, _natural_height);
    }
}

void DialogNotebook::set_requested_height(int height) {
    _natural_height = height;
}

int DialogNotebook::get_requested_height() const {
    return _natural_height;
}

} // namespace Inkscape::UI::Dialog

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

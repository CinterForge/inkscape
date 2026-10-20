// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * A replacement for GTK3ʼs Gtk::Menu, as removed in GTK4.
 */
/*
 * Authors:
 *   Daniel Boles <dboles.src+inkscape@gmail.com>
 *
 * Copyright (C) 2023 Daniel Boles
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "popover-menu.h"

#include <giomm/themedicon.h>
#include <glibmm/main.h>
#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/grid.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/separator.h>
#include <gtkmm/window.h>

#include "ui/popup-menu.h"
#include "ui/util.h"

namespace Inkscape::UI::Widget {

// Make our Grid have CSS name `menu` to try to piggyback “real” Menusʼ theming.
// Ditto, we leave Popover as `popover` so we don't lose normal Popover theming.
class PopoverMenuGrid final
    : public CssNameClassInit
    , public Gtk::Grid
{
public:
    [[nodiscard]] PopoverMenuGrid()
        : Glib::ObjectBase{"PopoverMenuGrid"}
        , CssNameClassInit{"menu"}
        , Gtk::Grid{}
    {
        add_css_class("menu");
        set_orientation(Gtk::Orientation::VERTICAL);
    }
};

PopoverMenu::PopoverMenu(Gtk::PositionType const position, bool has_arrow)
    : Glib::ObjectBase{"PopoverMenu"}
    , Gtk::Popover{}
    , _scrolled_window{*Gtk::make_managed<Gtk::ScrolledWindow>()}
    , _grid           {*Gtk::make_managed<PopoverMenuGrid    >()}
{
    add_css_class("popover-menu");
    add_css_class("menu");

    set_position(position);
    set_has_arrow(has_arrow);

    _scrolled_window.set_propagate_natural_width (true);
    _scrolled_window.set_propagate_natural_height(true);
    _scrolled_window.set_child(_grid);
    set_child(_scrolled_window);

    signal_show().connect([this]
    {
        check_child_invariants();

        set_scrolled_window_size();

        // FIXME: Initially focused item is sometimes wrong on first popup. GTK bug?
        // Grabbing focus in ::show does not always work & sometimes even crashes :(
        // For now, just remove possibly wrong, visible selection until hover/keynav
        // This is also nicer for menus with only 1 item, like the ToolToolbar popup
        Glib::signal_idle().connect_once( [this]{ unset_items_focus_hover(nullptr); });
    });
}

void PopoverMenu::attach(Gtk::Widget &item,
                         int const left_attach, int const right_attach,
                         int const top_attach, int const bottom_attach)
{
    check_child_invariants();

    auto const width = right_attach - left_attach;
    auto const height = bottom_attach - top_attach;
    _grid.attach(item, left_attach, top_attach, width, height);
    _items.push_back(&item);
}

void PopoverMenu::append(Gtk::Widget &item)
{
    check_child_invariants();

    _grid.attach_next_to(item, Gtk::PositionType::BOTTOM);
    _items.push_back(&item);
}

void PopoverMenu::prepend(Gtk::Widget &item)
{
    check_child_invariants();

    _grid.attach_next_to(item, Gtk::PositionType::TOP);
    _items.push_back(&item);
}

void PopoverMenu::remove(Gtk::Widget &item)
{
    // Check was added with one of our methods, is not Grid, etc.
    auto const it = std::find(_items.begin(), _items.end(), &item);
    g_return_if_fail(it != _items.end());

    _grid.remove(item);
    _items.erase(it);
}

void PopoverMenu::append_section_label(Glib::ustring const &markup)
{
    auto const label = Gtk::make_managed<Gtk::Label>();
    label->set_markup(markup);
    auto const item = Gtk::make_managed<PopoverMenuItem>();
    item->set_child(*label);
    item->set_sensitive(false);
    append(*item);
}

void PopoverMenu::append_separator()
{
    append(*Gtk::make_managed<Gtk::Separator>(Gtk::Orientation::HORIZONTAL));
}

void PopoverMenu::popup_at(Gtk::Widget &widget,
                           int const x_offset, int const y_offset)
{
    ::Inkscape::UI::popup_at(*this, widget, x_offset, y_offset);
}

void PopoverMenu::popup_at_center(Gtk::Widget &widget)
{
    ::Inkscape::UI::popup_at_center(*this, widget);
}

std::vector<Gtk::Widget *> const &PopoverMenu::get_items()
{
    return _items;
}

void PopoverMenu::check_child_invariants()
{
    // Check no one (accidentally?) removes our Grid or ScrolledWindow.
    // GtkPopover interposes a content widget and ScrolledWindow a Viewport, so:
    g_assert(is_descendant_of(_scrolled_window, *this));
    g_assert(is_descendant_of(_grid, _scrolled_window));
}

void PopoverMenu::set_scrolled_window_size()
{
    static constexpr int padding = 16; // Spare some window size for border etc.
    auto &window = dynamic_cast<Gtk::Window const &>(*get_root());
    _scrolled_window.set_max_content_width (window.get_width () - 2 * padding);
    _scrolled_window.set_max_content_height(window.get_height() - 2 * padding);
}

bool PopoverMenu::activate(Glib::ustring const &search) {
    bool match = false;
    Gtk::Widget *fallback_match = nullptr; 
    for (auto item : _items) {
        if (!_active_search) {
            _active_search = Gtk::make_managed<Gtk::Label>(search);
            _active_search->get_style_context()->add_class("menu_search");
            _active_search->set_xalign(0.1);
            _grid.attach_next_to(*_active_search, Gtk::PositionType::BOTTOM);
        }
        for (auto &widg : UI::children(*item)) {
            item->unset_state_flags(Gtk::StateFlags::FOCUSED | Gtk::StateFlags::PRELIGHT);
            if (!search.empty()) {
                 for (auto &mi : UI::children(widg)) {
                    if (auto label = dynamic_cast<Gtk::Label *>(&mi)) {
                        auto text_data = label->get_text();
                        // if not matched and search == begining of label
                        if (!match && text_data.size() >= search.size()) {
                            if (text_data.substr(0, search.size()).lowercase() == search.lowercase()) {
                                match = true;
                                item->grab_focus();
                                break;
                            }
                            if (!fallback_match && text_data.lowercase().find(search.lowercase()) != Glib::ustring::npos) {
                                fallback_match = item;
                            }
                        }
                    }   
                }
            }
        }
    }
    if (!match && fallback_match) {
        match = true;
        fallback_match->grab_focus();
    }
    if (_active_search) {
        if (search.empty()) {
            _active_search->hide();
        } else {
            auto searchstring = !pango_version_check(1, 50, 0) ?
                "<span size=\"x-large\" line_height=\"0.7\">⌕</span><small> %1</small>" :
                "<span size=\"large\">⌕</span><small> %1</small>";
            _active_search->set_markup(Glib::ustring::compose(searchstring, search));
            _active_search->show();
        }
    }
    return match;
}

void PopoverMenu::unset_items_focus_hover(Gtk::Widget * const except_active)
{
    for (auto const item : _items) {
        if (item != except_active) {
            item->unset_state_flags(Gtk::StateFlags::FOCUSED | Gtk::StateFlags::PRELIGHT);
        }
    }
}

void PopoverMenu::remove_all()
{
    for (auto item : _items) {
        _grid.remove(*item);
    }
    _items.clear();
}

// PopoverMenuItem

PopoverMenuItem::PopoverMenuItem(Glib::ustring const &text,
                                 bool const mnemonic,
                                 Glib::ustring const &icon_name,
                                 Gtk::IconSize const icon_size,
                                 bool const popdown_on_activate)
    : Glib::ObjectBase{"PopoverMenuItem"}
    , CssNameClassInit{"menuitem"}
    , Gtk::Button{}
{
    get_style_context()->add_class("menuitem");
    add_css_class("regular-item");
    set_has_frame(false);

    Gtk::Image *image = nullptr;

    if (!text.empty()) {
        _label = Gtk::make_managed<Gtk::Label>(text, Gtk::Align::START, Gtk::Align::CENTER, mnemonic);
    }

    if (!icon_name.empty()) {
        image = Gtk::make_managed<Gtk::Image>();
        image->set_from_icon_name(icon_name);
        image->set_icon_size(icon_size);
    }

    if (_label && image) {
        auto &hbox = *Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
        hbox.append(*image);
        hbox.append(*_label);
        set_child(hbox);
    } else if (_label) {
        set_child(*_label);
    } else if (image) {
        set_child(*image);
    }

    if (popdown_on_activate) {
        signal_activate().connect([this]
        {
            if (auto const menu = get_menu()) {
                menu->popdown();
            }
        });
    }
}

Glib::SignalProxy<void ()> PopoverMenuItem::signal_activate()
{
    return signal_clicked();
}

PopoverMenu *PopoverMenuItem::get_menu()
{
    for (auto &parent : parent_chain(*this) | std::views::drop(1)) {
        if (auto const menu = dynamic_cast<PopoverMenu *>(&parent)) {
            return menu;
        }
    }
    return nullptr;
}

void PopoverMenuItem::set_label(Glib::ustring const &name)
{
    if (_label) {
        _label->set_text(name);
    } else {
        _label = Gtk::make_managed<Gtk::Label>(name, Gtk::Align::START, Gtk::Align::CENTER);
        set_child(*_label);
    }
}

} // namespace Inkscape::UI::Widget

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

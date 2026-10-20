// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "toolbar.h"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/image.h>
#include <gtkmm/menubutton.h>

#include "desktop.h"
#include "ui/util.h"
#include "ui/widget/canvas.h"

namespace Inkscape::UI::Toolbar {

struct ToolbarWidget::MenuButton
{
    // Constructor to initialize data members
    MenuButton(int priority, int group_size, Gtk::MenuButton *menu_btn,
                      std::stack<std::pair<Gtk::Widget *, Gtk::Widget *>> toolbar_children)
        : priority(priority)
        , group_size(group_size)
        , menu_btn(menu_btn)
        , toolbar_children(std::move(toolbar_children))
    {}

    // Data members
    int priority;
    int group_size;
    Gtk::MenuButton *menu_btn;
    std::stack<std::pair<Gtk::Widget *, Gtk::Widget *>> popover_children;
    std::stack<std::pair<Gtk::Widget *, Gtk::Widget *>> toolbar_children;
};

ToolbarWidget::ToolbarWidget(Gtk::Box &toolbar)
    : _toolbar{toolbar}
{
    set_child(&_toolbar);
}

ToolbarWidget::~ToolbarWidget() = default;

static bool isMatchingPattern(const std::string &str, const std::string &pattern)
{
    // Early exit if string length is less than pattern length (guaranteed mismatch)
    if (str.size() < pattern.size()) {
        return false;
    }

    for (size_t i = 0; i < pattern.size(); ++i) {
        if (std::tolower(str[i]) != std::tolower(pattern[i])) {
            return false; // Mismatch found, stop comparing
        }
    }

    // All characters matched
    return true;
}

void ToolbarWidget::_initMenuBtns()
{
    std::map<std::string, std::pair<int, std::stack<std::pair<Gtk::Widget *, Gtk::Widget *>>>> menu_btn_groups;
    int position = 0;

    // Iterate over all the children of this toolbar.
    for (auto &child : UI::children(_toolbar)) {
        // Find out the CSS classes associated with each child.
        auto css_classes = child.get_css_classes();
        int group_size = 1;

        // Iterate over all the CSS classes and find out
        // the movable children by searching for classes
        // which contains the "priority" prefix(case-insensitive).
        for (const auto &c : css_classes) {
            if (isMatchingPattern(c, "priority")) {
                // Check if the group_size is also defined.
                bool group_size_defined = false;
                for (const auto &cl : css_classes) {
                    if (isMatchingPattern(cl, "groupsize")) {
                        group_size = cl[cl.size() - 1] - '0';
                        group_size_defined = true;
                    }
                }

                // Store this child in the map.
                auto prev_child = child.get_prev_sibling();
                auto it = menu_btn_groups.find(c);

                if (it != menu_btn_groups.end()) {
                    // This group already exists.
                    // Push this child in the vector.
                    it->second.second.emplace(prev_child, &child);
                    if (group_size_defined) {
                        it->second.first = group_size;
                    }
                } else {
                    std::stack<std::pair<Gtk::Widget *, Gtk::Widget *>> toolbar_children;
                    toolbar_children.emplace(prev_child, &child);
                    menu_btn_groups.insert({c, {group_size, toolbar_children}});
                }
            }
        }
        position++;
    }

    // Now, start inserting menu buttons in the toolbar.
    for (auto [key, value] : menu_btn_groups) {
        // The map is lexicographically sorted on the basis of priorities.
        // Step 1: Find out the priority of this group.
        // Assumption: The last character of the class name stores the
        // value of the priority.
        auto priority = key[key.size() - 1] - '0';

        // Add this menu button to the _menu_btns vector.
        _insert_menu_btn(priority, value.first, value.second);

        // The menu button added at the end would be the first to
        // collapse or expand.
        _active_mb_index = _menu_btns.size() - 1;
    }

    // Insert a very large value to prevent the toolbar
    // from expanding when all the menu buttons are in the
    // expanded state.
    _size_needed.push(10000);
}

void ToolbarWidget::_insert_menu_btn(int priority, int group_size,
                                     std::stack<std::pair<Gtk::Widget *, Gtk::Widget *>> toolbar_children)
{
    auto menu_btn = Gtk::make_managed<Gtk::MenuButton>();
    auto popover = Gtk::make_managed<Gtk::Popover>();
    auto box = Gtk::make_managed<Gtk::Box>(_toolbar.get_orientation(), 4);

    if (_toolbar.get_orientation() == Gtk::Orientation::VERTICAL) {
        menu_btn->set_direction(Gtk::ArrowType::LEFT);
    }

    popover->set_child(*box);
    menu_btn->set_popover(*popover);

    // Insert this menu button right next to its topmost toolbar child.
    _toolbar.insert_child_after(*menu_btn, *toolbar_children.top().second);
    menu_btn->set_visible(false);

    // Add this menu button to the _menu_btns vector.
    _menu_btns.push_back(
        std::make_unique<MenuButton>(priority, group_size, menu_btn, std::move(toolbar_children)));
}

void ToolbarWidget::measure_vfunc(Gtk::Orientation orientation, int for_size, int &min, int &nat, int &min_baseline, int &nat_baseline) const
{
    _toolbar.measure(orientation, for_size, min, nat, min_baseline, nat_baseline);

    if (_toolbar.get_orientation() == orientation) {
        // Return too-small value to allow shrinking.
        min = 0;
    }
}

void ToolbarWidget::on_size_allocate(int width, int height, int baseline)
{
    _resize_handler(width, height);
    UI::Widget::Bin::on_size_allocate(width, height, baseline);
}

static int min_dimension(Gtk::Widget const *widget, Gtk::Orientation const orientation)
{
    int min = 0;
    int ignore = 0;
    widget->measure(orientation, -1, min, ignore, ignore, ignore);
    return min;
};

void ToolbarWidget::_resize_handler(int width, int height)
{
    if (_resizing || _active_mb_index < 0) {
        return;
    }

    auto const orientation = _toolbar.get_orientation();
    auto const allocated_size = orientation == Gtk::Orientation::VERTICAL ? height : width;
    int min_size = min_dimension(&_toolbar, orientation);

    _resizing = true;
    if (allocated_size < min_size) {
        // Shrinkage required.
        while (allocated_size < min_size) {
            if (_menu_btns[_active_mb_index]->toolbar_children.empty()) {
                // This menu button can no longer be collapsed. Switch to the next
                // menu button (towards the left of the vector).
                if (_active_mb_index > 0) {
                    _active_mb_index -= 1;
                    continue;
                } else {
                    // Reaching this point indicates that the toolbar cannot be shrunk any further.
                    _resizing = false;
                    return;
                }
            }

            // Now, move the toolbar_children of this menu button to the popover.
            auto mb = _menu_btns[_active_mb_index].get();
            auto popover_box = dynamic_cast<Gtk::Box *>(mb->menu_btn->get_popover()->get_child());
            _move_children(&_toolbar, popover_box, mb->toolbar_children, mb->popover_children, mb->group_size);
            mb->menu_btn->set_visible(true);

            int old = min_size;
            min_size = min_dimension(&_toolbar, orientation);
            int change = old - min_size;
            _size_needed.push(change);
        }
    } else if (allocated_size > min_size) {
        // Once the allocated size of the toolbar is greater than its
        // minimum size, try to re-insert a group of elements back
        // into the toolbar.
        if (!(allocated_size > min_size + _size_needed.top())) {
            // Not enough space, skip.
            _resizing = false;
            return;
        }

        // Expand until there are children left in the popovers.
        while (_active_mb_index < _menu_btns.size()) {
            // Check if the currently active menu button is expandable or not.
            if (_menu_btns[_active_mb_index]->popover_children.empty()) {
                // This menu button can no longer be expanded. Switch to the next
                // menu button (towards the right of the vector).
                if (_active_mb_index < _menu_btns.size() - 1) {
                    _active_mb_index += 1;
                    continue;
                } else {
                    // Reaching this point indicates that the toolbar cannot be expanded any further.
                    // Set this menu button invisible and return.
                    // _menu_btns[_active_mb_index]->set_visible(false);
                    _resizing = false;
                    return;
                }
            }

            auto mb = _menu_btns[_active_mb_index].get();

            // See if we have enough space to expand the topmost collapsed button.
            int req_size = min_size + _size_needed.top();

            if (req_size > allocated_size) {
                // Not enough space - stop.
                break;
            }

            // Move a group of widgets back into the toolbar.
            auto popover_box = dynamic_cast<Gtk::Box *>(mb->menu_btn->get_popover()->get_child());
            _move_children(popover_box, &_toolbar, mb->toolbar_children, mb->popover_children, mb->group_size, true);
            _size_needed.pop();

            if (mb->popover_children.empty()) {
                // Set it invisible only if all the children have moved to the toolbar.
                mb->menu_btn->set_visible(false);
            }

            min_size = min_dimension(&_toolbar, orientation);
        }
    }

    _resizing = false;
}

void ToolbarWidget::_update_menu_btn_image(Gtk::Widget *child)
{
    Glib::ustring icon_name = "go-down";

    if (auto btn = dynamic_cast<Gtk::Button *>(child); btn && _toolbar.get_orientation() == Gtk::Orientation::HORIZONTAL) {
        // Find the icon name from the child image.
        if (auto image = dynamic_cast<Gtk::Image *>(btn->get_child())) {
            auto icon = image->get_icon_name();
            if (icon != "") {
                icon_name = icon;
            }
        } else {
            // Find the icon name from the button itself.
            auto icon = btn->get_icon_name();
            if (icon != "") {
                icon_name = icon;
            }
        }
    }

    auto menu_btn = _menu_btns[_active_mb_index]->menu_btn;
    menu_btn->set_always_show_arrow(!(icon_name == "go-down"));
    menu_btn->set_icon_name(icon_name.c_str());
}

void ToolbarWidget::_move_children(Gtk::Box *src, Gtk::Box *dest,
                             std::stack<std::pair<Gtk::Widget *, Gtk::Widget *>> &tb_children,
                             std::stack<std::pair<Gtk::Widget *, Gtk::Widget *>> &popover_children, int group_size,
                             bool is_expanding)
{
    while (group_size--) {
        Gtk::Widget *child;
        Gtk::Widget *prev_child;

        if (is_expanding) {
            std::tie(prev_child, child) = popover_children.top();
            popover_children.pop();
            tb_children.emplace(prev_child, child);
        } else {
            std::tie(prev_child, child) = tb_children.top();
            tb_children.pop();
            popover_children.emplace(prev_child, child);
        }

        child->reference();

        src->remove(*child);

        // is_expanding will be true when the children are being put back into
        // the toolbar. In that case, insert the children at their previous
        // positions.
        if (is_expanding) {
            if (!prev_child) {
                dest->insert_child_at_start(*child);
            } else {
                dest->insert_child_after(*child, *prev_child);
            }

            if (!popover_children.empty()) {
                _update_menu_btn_image(popover_children.top().second);
            }
        } else {
            dest->prepend(*child);
            _update_menu_btn_image(child);
        }

        child->unreference();
    }
}

Toolbar::~Toolbar()
{
    // Lifecycle model for toolbars requires desktop to be unset before destruction.
    assert(!_desktop);
}

void Toolbar::onDefocus()
{
    if (_desktop) {
        _desktop->getCanvas()->grab_focus();
    }
}

void Toolbar::setDesktop(SPDesktop *desktop)
{
    _desktop = desktop;

    _doc_replaced_conn.disconnect();
    if (_desktop) {
        _doc_replaced_conn = _desktop->connectDocumentReplaced([this](SPDesktop*, SPDocument*) { documentReplaced(); });
    }
    documentReplaced();
}

} // namespace Inkscape::UI::Toolbar

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

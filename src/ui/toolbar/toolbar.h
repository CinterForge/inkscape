// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_TOOLBAR_TOOLBAR_H
#define INKSCAPE_UI_TOOLBAR_TOOLBAR_H

#include <stack>
#include <gtkmm/box.h>
#include <sigc++/scoped_connection.h>

#include "ui/defocus-target.h"
#include "ui/widget/generic/bin.h"

class SPDesktop;

namespace Gtk {
class Box;
class MenuButton;
}

namespace Inkscape::Util { class Unit; }

namespace Inkscape::UI::Toolbar {

/**
 * \brief A toolbar widget providing support for collapsible sections.
 */
class ToolbarWidget : public UI::Widget::Bin
{
public:
    ~ToolbarWidget() override;

protected:
    ToolbarWidget(Gtk::Box &toolbar);

    Gtk::Box &_toolbar;

    void measure_vfunc(Gtk::Orientation orientation, int for_size, int &min, int &nat, int &min_baseline, int &nat_baseline) const override;
    void on_size_allocate(int width, int height, int baseline) override;

    // Required to be called by derived class ctors after construction.
    void _initMenuBtns();

private:
    struct MenuButton;

    std::vector<std::unique_ptr<MenuButton>> _menu_btns;
    std::stack<int> _size_needed;
    int _active_mb_index = -1;
    bool _resizing = false;

    void _insert_menu_btn(int priority, int group_size,
                         std::stack<std::pair<Gtk::Widget *, Gtk::Widget *>> toolbar_children);
    void _resize_handler(int width, int height);
    void _update_menu_btn_image(Gtk::Widget *child);
    void _move_children(Gtk::Box *src, Gtk::Box *dest, std::stack<std::pair<Gtk::Widget *, Gtk::Widget *>> &tb_children,
                        std::stack<std::pair<Gtk::Widget *, Gtk::Widget *>> &popover_children, int group_size,
                        bool is_expanding = false);
};

/**
 * @brief Base class for all tool toolbars.
 */
class Toolbar
    : public ToolbarWidget
    , public Inkscape::UI::DefocusTarget
{
public:
    ~Toolbar() override;

    virtual void setDesktop(SPDesktop *desktop);
    SPDesktop *getDesktop() const { return _desktop; }

    virtual void setActiveUnit(Util::Unit const *unit) {}

    void onDefocus() override;

protected:
    using ToolbarWidget::ToolbarWidget;

    virtual void documentReplaced() {} // convenience hook for when the doc is assigned or replaced

    SPDesktop *_desktop = nullptr;

private:
    sigc::scoped_connection _doc_replaced_conn;
};

} // namespace Inkscape::UI::Toolbar

#endif // INKSCAPE_UI_TOOLBAR_TOOLBAR_H

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

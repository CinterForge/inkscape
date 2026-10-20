// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Inkscape document tabs bar.
 */
/*
 * Authors:
 *   PBS <pbs3141@gmail.com>
 *
 * Copyright (C) 2024 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_WIDGET_TABS_WIDGET_H
#define INKSCAPE_UI_WIDGET_TABS_WIDGET_H

#include <gtkmm/widget.h>
#include <2geom/point.h>

#include "ui/widget/generic/css-name-class-init.h"

namespace Gtk { class Popover; }

class SPDesktop;
class SPDesktopWidget;

namespace Inkscape::UI::Widget {

struct Tab;
class TabDrag;

// Widget that implements the document tab bar. It handles drag and drop between itself or other
// tab rows. It holds the actual visual representation of the tabs and nothing more.
class DesktopTabRow : public CssNameClassInit, public Gtk::Widget
{
public:
    DesktopTabRow(SPDesktopWidget *desktop_widget);
    ~DesktopTabRow() override;

    void addTab(SPDesktop *desktop, int pos = -1);
    void removeTab(SPDesktop *desktop);
    void switchTab(SPDesktop *desktop);
    void refreshTitle(SPDesktop *desktop);

    void switchToPrevDesktop();
    void switchToNextDesktop();
    void switchToDesktop(SPDesktop *desktop);

    int positionOfTab(SPDesktop *desktop) const;
    SPDesktop *tabAtPosition(int i) const;
    Gtk::Widget *widgetAtPosition(int i) const;
    Glib::ustring tabName(int i) const;
    int size() const;

private:
    SPDesktopWidget *const _desktop_widget;
    Gtk::Widget *const _overlay;
    Gtk::Popover *_popover = nullptr;

    std::vector<std::shared_ptr<Tab>> _tabs;
    std::weak_ptr<Tab> _active;
    std::weak_ptr<Tab> _right_clicked;
    std::weak_ptr<Tab> _left_clicked;
    Geom::Point _left_click_pos;

    friend TabDrag;
    std::shared_ptr<TabDrag> _drag_src;
    std::shared_ptr<TabDrag> _drag_dst;

    class Instances;

    Gtk::SizeRequestMode get_request_mode_vfunc() const override;
    void measure_vfunc(Gtk::Orientation orientation, int for_size, int &min, int &nat, int &minb, int &natb) const override;
    void size_allocate_vfunc(int width, int height, int baseline) override;

    void _setTooltip(SPDesktop *desktop, Glib::RefPtr<Gtk::Tooltip> const &tooltip);
    std::pair<std::weak_ptr<Tab>, Geom::Point> _tabAtPoint(Geom::Point const &pos);
    void _reorderTab(int from, int to);
    int _activeTabPosition();
};

} // namespace Inkscape::UI::Widget

#endif // INKSCAPE_UI_WIDGET_TABS_WIDGET_H

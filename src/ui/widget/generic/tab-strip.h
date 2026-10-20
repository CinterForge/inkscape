// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Generic tab strip widget
 */
/*
 * Authors:
 *   PBS <pbs3141@gmail.com>
 *   Mike Kowalski
 *
 * Copyright (C) 2024 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_WIDGET_TAB_STRIP_H
#define INKSCAPE_UI_WIDGET_TAB_STRIP_H

#include <2geom/point.h>
#include <gtkmm/builder.h>
#include <gtkmm/menubutton.h>
#include <gtkmm/orientable.h>

#include "ui/widget/generic/css-name-class-init.h"
#include "ui/widget/gtk-registry.h"

namespace Gtk { class Popover; }

namespace Inkscape::UI::Widget {

struct TabWidget;
class TabWidgetDrag;

/// Widget that implements strip of tabs
class TabStrip : public CssNameClassInit, public Gtk::Orientable, public BuildableWidget<TabStrip, Gtk::Widget>
{
public:
    using BaseObjectType = GtkWidget;

    TabStrip(Gtk::Orientation orientation = Gtk::Orientation::HORIZONTAL);
    explicit TabStrip(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder = {});
    ~TabStrip() override;

    // create a new tab
    Gtk::Widget* add_tab(const Glib::ustring& label, const Glib::ustring& icon, int pos = -1);
    // remove tab from the strip
    void remove_tab(const Gtk::Widget& tab);
    void remove_tab_at(int pos);
    // mark tab as activated; other tabs will be deselected
    void select_tab(const Gtk::Widget& tab);
    void select_tab_at(int pos);
    // manually move tabs to a position
    void set_tabs_order(std::vector<Gtk::Widget *> sorted);
    // Get a vector of the tab widgets
    std::vector<Gtk::Widget *> get_tabs() const;
    // find position of the tab in a strip
    int get_tab_position(const Gtk::Widget& tab) const;
    // get tab at specified position or null if there's none there
    Gtk::Widget* get_tab_at(int i) const;
    // add a popup to the plus (+) button
    void set_new_tab_popup(Gtk::Popover* popover);
    // add a context popup to all tabs
    void set_tabs_context_popup(Gtk::Popover* popover);
    // enable/disable rearranging tabs by draggin them to new position
    enum class Rearrange { Never, Internally, Externally };
    void set_rearranging_tabs(Rearrange enable);
    // set label behavior
    enum class ShowLabels { Never, Always, ActiveOnly };
    void set_show_labels(ShowLabels labels);
    // return true if tab is active
    bool is_tab_active(const Gtk::Widget& tab) const;
    // show/hide close button in individual tabs
    void set_show_close_button(bool show);
    // tabs support drag&drop; this is the type used by drop source, so clients can check it
    static GType get_dnd_source_type();
    // given drop source value unpack it to the source TabStrip and tab index
    static std::optional<std::pair<TabStrip*, int>> unpack_drop_source(const Glib::ValueBase& value);
    // if true, show drag handles in the tabs
    void set_draw_handle(bool show = true);

    // user attempts to select given tab
    sigc::signal<void (Gtk::Widget&)> signal_select_tab() { return _signal_select_tab; }
    // user attempts to close given tab
    sigc::signal<void (Gtk::Widget&)> signal_close_tab() { return _signal_close_tab; }
    // user drops floating tab outside of any tab strip
    sigc::signal<void (Gtk::Widget&)> signal_float_tab() { return _signal_float_tab; }
    // user moves tab to this tab strip from a different tab strip; tab position in source and destination is provided
    sigc::signal<void (Gtk::Widget&, int, TabStrip&, int)> signal_move_tab() { return _signal_move_tab; }
    // user moved tab within this tab strip to a new position
    sigc::signal<void (int, int)> signal_tab_rearranged() { return _signal_tab_rearranged; }
    // user started dragging a tab outside of the tabstrip
    sigc::signal<void ()> signal_dnd_begin() { return _signal_dnd_begin; }
    // tab d&d has ended; bool argument is true if it was cancelled
    sigc::signal<void (bool)> signal_dnd_end() { return _signal_dnd_end; }

private:
    void construct();

    Gtk::Widget *const _overlay;
    Gtk::Popover *_popover = nullptr;
    Gtk::MenuButton _plus_btn;
    std::vector<std::shared_ptr<TabWidget>> _tabs;
    std::weak_ptr<TabWidget> _active;
    std::weak_ptr<TabWidget> _right_clicked;
    std::weak_ptr<TabWidget> _left_clicked;
    Geom::Point _left_click_pos;
    sigc::signal<void (Gtk::Widget&)> _signal_select_tab;
    sigc::signal<void (Gtk::Widget&)> _signal_close_tab;
    sigc::signal<void (Gtk::Widget&)> _signal_float_tab;
    sigc::signal<void (Gtk::Widget&, int, TabStrip&, int)> _signal_move_tab;
    sigc::signal<void (int, int)> _signal_tab_rearranged;
    sigc::signal<void ()> _signal_dnd_begin;
    sigc::signal<void (bool)> _signal_dnd_end;
    Rearrange _rearrange = Rearrange::Externally;
    ShowLabels _show_labels = ShowLabels::Never;
    bool _show_close_btn = true;
    bool _show_drag_handles = false;

    friend TabWidgetDrag;
    std::shared_ptr<TabWidgetDrag> _drag_src;
    std::shared_ptr<TabWidgetDrag> _drag_dst;

    void _update_new_tab();
    void _updateVisibility();
    TabWidget* find_tab(Gtk::Widget& tab);
    Gtk::SizeRequestMode get_request_mode_vfunc() const override;
    void measure_vfunc(Gtk::Orientation orientation, int for_size, int &min, int &nat, int &minb, int &natb) const override;
    void size_allocate_vfunc(int width, int height, int baseline) override;

    void _setTooltip(const TabWidget& tab, Glib::RefPtr<Gtk::Tooltip> const &tooltip);
    std::pair<std::weak_ptr<TabWidget>, Geom::Point> _tabAtPoint(Geom::Point const &pos);
    bool _reorderTab(int from, int to);
    sigc::scoped_connection _finish_conn; // Used to defer finishing d&d
};

} // namespace Inkscape::UI::Widget

#endif // INKSCAPE_UI_WIDGET_TAB_STRIP_H

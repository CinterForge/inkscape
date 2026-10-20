// SPDX-License-Identifier: GPL-2.0-or-later

/** @file
 * @brief A widget with multiple panes. Agnostic to type what kind of widgets panes contain.
 *
 * Authors: see git history
 *   Tavmjong Bah
 *
 * Copyright (c) 2020 Tavmjong Bah, Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_DIALOG_MULTIPANED_H
#define INKSCAPE_UI_DIALOG_MULTIPANED_H

#include <gtkmm/droptarget.h>
#include <gtkmm/gesture.h>

#include "dialog-notebook.h"

namespace Glib {
class ValueBase;
} // namespace Glib

namespace Gtk {
class DropTarget;
class GestureDrag;
} // namespace Gtk

namespace Inkscape::UI::Dialog {

/*
 * A widget with multiple panes. Agnostic to type what kind of widgets panes contain.
 * Handles allow a user to resize children widgets. Drop zones allow adding widgets
 * at either end.
 */
class DialogMultipaned final
    : public Gtk::Orientable
    , public Gtk::Widget
{
public:
    DialogMultipaned(Gtk::Orientation orientation = Gtk::Orientation::HORIZONTAL);
    ~DialogMultipaned() final;

    void prepend(std::unique_ptr<Gtk::Widget> child);
    void append (std::unique_ptr<Gtk::Widget> child);
    void remove (Gtk::Widget &child);

    // Getters and setters
    Gtk::Widget *get_first_widget();
    Gtk::Widget *get_last_widget ();
    void get_children() = delete; ///< We manage our own child list. Call get_multipaned_children()
    std::vector<std::unique_ptr<Gtk::Widget>> const &get_multipaned_children() const { return _children; }
    void set_drop_gtypes(std::vector<GType> const &gtypes);
    bool has_empty_widget() const { return static_cast<bool>(_empty_widget); }

    // Signals
    sigc::signal<void ()> signal_now_empty();
    sigc::signal<bool (Gtk::Widget&, DialogNotebook&, DialogContainer::DockLocation, DialogNotebook*)> signal_dock_dialog() {
        return _signal_dock_dialog;
    }
    sigc::signal<bool (Gtk::Widget&, DialogNotebook&)> signal_float_dialog() {
        return _signal_float_dialog;
    }

    // UI functions
    void set_dropzone_sizes(int start, int end);
    void toggle_multipaned_children(bool show);
    void children_toggled();
    void ensure_multipaned_children();
    void set_restored_width(int width);

    static void    add_drop_zone_highlight_instances();
    static void remove_drop_zone_highlight_instances();

private:
    // Overrides
    Gtk::SizeRequestMode get_request_mode_vfunc() const override;
    void measure_vfunc(Gtk::Orientation orientation, int for_size,
                       int &minimum, int &natural,
                       int &minimum_baseline, int &natural_baseline) const final;
    void size_allocate_vfunc(int width, int height, int baseline) final;

    // Signals
    sigc::signal<bool (Gtk::Widget&, DialogNotebook&, DialogContainer::DockLocation, DialogNotebook*)> _signal_dock_dialog;
    sigc::signal<bool (Gtk::Widget&, DialogNotebook&)> _signal_float_dialog;
    sigc::signal<void ()> _signal_now_empty;

    // We must manage children ourselves.
    std::vector<std::unique_ptr<Gtk::Widget>> _children;

    const Glib::RefPtr<Gtk::DropTarget> _drop_target{Gtk::DropTarget::create(G_TYPE_INVALID, Gdk::DragAction::MOVE)};

    // Values used when dragging handle.
    int _handle = -1; // Child number of active handle
    int _drag_handle = -1;
    Gtk::Widget* _resizing_widget1 = nullptr;
    Gtk::Widget* _resizing_widget2 = nullptr;
    Gtk::Widget* _hide_widget1 = nullptr;
    Gtk::Widget* _hide_widget2 = nullptr;
    Gtk::Allocation start_allocation1;
    Gtk::Allocation start_allocationh;
    Gtk::Allocation start_allocation2;
    Gtk::Allocation allocation1;
    Gtk::Allocation allocationh;
    Gtk::Allocation allocation2;

    // drag on handle/separator
    Gtk::EventSequenceState on_drag_begin (double  start_x, double  start_y);
    Gtk::EventSequenceState on_drag_end   (double offset_x, double offset_y);
    Gtk::EventSequenceState on_drag_update(double offset_x, double offset_y);
    // drag+drop data
    bool on_drag_data_drop   (Glib::ValueBase const &value, double x, double y);
    bool on_prepend_drag_data(Glib::ValueBase const &value, double x, double y);
    bool on_append_drag_data (Glib::ValueBase const &value, double x, double y);

    // Others
    Gtk::Widget* _empty_widget = nullptr; // placeholder in an empty container
    void insert(int pos, std::unique_ptr<Gtk::Widget> child);
    void add_empty_widget();
    void remove_empty_widget();
    std::vector<sigc::scoped_connection> _connections;
    int _natural_width = 0;
};

} // namespace Inkscape::UI::Dialog

#endif // INKSCAPE_UI_DIALOG_MULTIPANED_H

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

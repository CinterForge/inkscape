// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 5/20/25.
//
// Simple scale widget that shows the range in discrete blocks.

#ifndef GENERIC_WIDGET_SCALE_BAR_H
#define GENERIC_WIDGET_SCALE_BAR_H

#include <gtkmm/widget.h>
#include <gtkmm/gestureclick.h>

#include "ui/widget-vfuncs-class-init.h"

namespace Gtk {
class EventControllerScroll;
class EventControllerMotion;
}

namespace Inkscape::UI::Widget {

class ScaleBar : public WidgetVfuncsClassInit, public Gtk::Widget {
public:
    ScaleBar();

    void set_adjustment(Glib::RefPtr<Gtk::Adjustment> adj);

    void set_max_block_count(int n);

    // set the height of scale blocks; by default, it is approximately the size of the UI font capital letters
    void set_block_height(int height);

private:
    void snapshot_vfunc(const Glib::RefPtr<Gtk::Snapshot>& snapshot) override;
    void draw_scale(const Glib::RefPtr<Gtk::Snapshot>& snapshot);
    void set_adjustment_value(double x);
    Gtk::EventSequenceState on_click_pressed(const Gtk::GestureClick& click, int n_press, double x, double y);
    void on_motion(const Gtk::EventControllerMotion& motion, double x, double y);
    void css_changed(GtkCssStyleChange*) override;
    void on_scroll_begin();
    bool on_scroll(Gtk::EventControllerScroll& scroll, double dx, double dy);
    void on_scroll_end();

    int _block_count = 0; // No blocks

    // height in pixels
    int _block_height = 10;
    Glib::RefPtr<Gtk::Adjustment> _adjustment;
    sigc::scoped_connection _connection;
    Gdk::RGBA _selected;
    Gdk::RGBA _unselected;
};

} // namespace

#endif //GENERIC_WIDGET_SCALE_BAR_H

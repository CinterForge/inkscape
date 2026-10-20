// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * A slider with colored background - implementation.
 *//*
 * Authors:
 *   see git history
 *
 * Copyright (C) 2018-2024 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLOR_SLIDER_H
#define SEEN_COLOR_SLIDER_H

#include <gtkmm/drawingarea.h>

#include "colors/spaces/components.h"

namespace Gtk {
class GestureDrag;
class Builder;
class EventControllerMotion;
class GestureClick;
} // namespace Gtk

namespace Inkscape::Colors {
class ColorSet;
} // namespace Inkscape::Colors

namespace Inkscape::UI::Widget {

/*
 * A slider with colored background
 */
class ColorSlider : public Gtk::DrawingArea {
public:
    ColorSlider(std::shared_ptr<Colors::ColorSet> color, Colors::Space::Component component);
    ColorSlider(
        BaseObjectType *cobject,
        Glib::RefPtr<Gtk::Builder> const &builder,
        std::shared_ptr<Colors::ColorSet> color,
        Colors::Space::Component component);
    ~ColorSlider() override;

    double getScaled() const;
    void setScaled(double value);
protected:
    friend class ColorPageChannel;

    std::shared_ptr<Colors::ColorSet> _colors;
    Colors::Space::Component _component;
private:
    void construct();
    void draw_func(Cairo::RefPtr<Cairo::Context> const &cr, int width, int height);

    void on_click_pressed(Gtk::GestureClick const &click, int n_press, double x, double y);
    void on_motion(Gtk::EventControllerMotion const &motion, double x, double y);
    void update_component(double x, double y, Gdk::ModifierType const state);
    void on_drag(Gdk::EventSequence* sequence);

    sigc::scoped_connection _changed_connection;
    sigc::signal<void ()> signal_value_changed;
    bool _dragging = false;
    Glib::RefPtr<Gtk::GestureDrag> _drag;
    // Memory buffers for the painted gradient
    std::vector<unsigned int> _gr_buffer;
    Glib::RefPtr<Gdk::Pixbuf> _gradient;
    bool _hover = false;
    unsigned int _tick_callback = 0;
    double _ring_size = 0;
    double _ring_thickness = 0;
    gint64 _last_time = 0;
};

} // namespace Inkscape::UI::Widget

#endif // SEEN_COLOR_SLIDER_H

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

// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 9/3/24.

#ifndef STROKE_OPTIONS_H
#define STROKE_OPTIONS_H
#include <gtkmm/grid.h>
#include <gtkmm/togglebutton.h>
#include <ui/operation-blocker.h>

#include "generic/spin-button.h"
#include "style/paint-order.h"

class SPStyle;

namespace Inkscape::UI::Widget {

class StrokeOptions : public Gtk::Grid {
public:
    StrokeOptions();

    // update UI to reflect the item's style
    void update_widgets(SPStyle& style);

    sigc::signal<void (const char*)> _join_changed;
    sigc::signal<void (const char*)> _cap_changed;
    sigc::signal<void (const char*)> _order_changed;
    sigc::signal<void (double)> _miter_changed;
private:
    Gtk::ToggleButton _join_bevel;
    Gtk::ToggleButton _join_round;
    Gtk::ToggleButton _join_miter;
    InkSpinButton _miter_limit;
    Gtk::ToggleButton _cap_butt;
    Gtk::ToggleButton _cap_round;
    Gtk::ToggleButton _cap_square;
    PaintOrderWidget _paint_order;
    OperationBlocker _update;
};

} // namespace

#endif //STROKE_OPTIONS_H

// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 6/27/24.
//

#include "resizing-separator.h"

#include <gtkmm/cssprovider.h>
#include <gtkmm/gesturedrag.h>

namespace Inkscape::UI::Widget {

// language=CSS
auto resizing_separator_css = R"=====(
#ResizingSeparator {
    border: 1px solid @unfocused_borders;
    border-radius: 1px;
    background-color: alpha(@unfocused_borders, 0.4);
}
)=====";

ResizingSeparator::ResizingSeparator(Orientation orientation) {
    construct(orientation);
}

ResizingSeparator::ResizingSeparator(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
    : Widget(cobject) {
    construct(Orientation::Both);
}

void ResizingSeparator::resize(Gtk::Widget* widget, Geom::Point max) {
    _resize = widget;
    _max_size = max;
}

void ResizingSeparator::set_orientation(Orientation orientation) {
    _orientation = orientation;

    switch (orientation) {
    case Orientation::Horizontal:
        set_cursor("se-resize");
        break;
    case Orientation::Vertical:
        set_cursor("ns-resize");
        break;
    case Orientation::Both:
        set_cursor("nwse-resize");
        break;
    }
}

void ResizingSeparator::construct(Orientation orientation) {
    set_name("ResizingSeparator");
    set_size_request(_size.x(), _size.y());

    static Glib::RefPtr<Gtk::CssProvider> provider;
    if (!provider) {
        provider = Gtk::CssProvider::create();
        provider->load_from_data(resizing_separator_css);
        auto const display = Gdk::Display::get_default();
        Gtk::StyleContext::add_provider_for_display(display, provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 10);
    }

    set_orientation(orientation);

    _drag = Gtk::GestureDrag::create();
    _drag->signal_begin().connect(sigc::mem_fun(*this, &ResizingSeparator::on_drag_begin));
    _drag->signal_update().connect(sigc::mem_fun(*this, &ResizingSeparator::on_drag_update));
    _drag->signal_end().connect(sigc::mem_fun(*this, &ResizingSeparator::on_drag_end));
    _drag->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    add_controller(_drag);
}

void ResizingSeparator::on_drag_begin(Gdk::EventSequence* sequence) {
    _initial_size.x() = _resize ? _resize->get_width() : 0;
    _initial_size.y() = _resize ? _resize->get_height() : 0;
    double x, y;
    _drag->get_start_point(x, y);
    auto start = compute_point(*get_parent(), Gdk::Graphene::Point(x, y));
    if (start) {
        _initial_position.x() = start->get_x();
        _initial_position.y() = start->get_y();
    }
}

void ResizingSeparator::on_drag_update(Gdk::EventSequence* sequence) {
    if (!_resize) return;

    double dx = 0.0;
    double dy = 0.0;
    double x = 0, y = 0;
    if (_drag->get_offset(dx, dy) && _drag->get_start_point(x, y)) {
        auto end = compute_point(*get_parent(), Gdk::Graphene::Point(x + dx, y + dy));
        if (end.has_value()) {
            auto dist = Geom::Point(end->get_x(), end->get_y()) - _initial_position;
            auto size = dist + _initial_size;
            auto width = std::clamp(size.x(), 0.0, std::max(0.0, _max_size.x()));
            auto height = std::clamp(size.y(), 0.0, std::max(0.0, _max_size.y()));
            if (_orientation == Orientation::Horizontal) {
                _resize->set_size_request(-1, width);
            }
            else if (_orientation == Orientation::Vertical) {
                _resize->set_size_request(-1, height);
            }
            else {
                _resize->set_size_request(width, height);
            }
            _signal_resized.emit(Geom::Point(width, height));
        }
    }
}

void ResizingSeparator::on_drag_end(Gdk::EventSequence* sequence) {
    // no op
}

} // namespace

// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 6/27/24.
//

#ifndef RESIZING_SEPARATOR_H
#define RESIZING_SEPARATOR_H

#include <gtkmm/gesturedrag.h>
#include <gtkmm/widget.h>

#include "2geom/point.h"

namespace Gtk {
class Builder;
}

namespace Inkscape::UI::Widget {

// This separator can be dragged vertically to resize the associated widget, typically a sibling

class ResizingSeparator : public Gtk::Widget {
public:
    enum class Orientation {
        Horizontal,
        Vertical,
        Both
    };
    ResizingSeparator(Orientation orientation = Orientation::Vertical);
    ResizingSeparator(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
    ~ResizingSeparator() override = default;

    // Use this separator to resize the given widget and set max size.
    void resize(Gtk::Widget* widget, Geom::Point max);

    // Set resizing separator orientation to decide in which direction can widget resizing occur.
    void set_orientation(Orientation orientation);

    sigc::signal<void (Geom::Point)>& get_signal_resized() { return _signal_resized; }

private:
    void construct(Orientation orientation);
    Orientation _orientation = Orientation::Vertical;
    Geom::Point _size{4, 4};
    Geom::Point _initial_position;
    Geom::Point _initial_size;
    Geom::Point _max_size;
    Gtk::Widget* _resize = nullptr;
    sigc::signal<void (Geom::Point)> _signal_resized;
    Glib::RefPtr<Gtk::GestureDrag> _drag;
    void on_drag_begin(Gdk::EventSequence* sequence);
    void on_drag_update(Gdk::EventSequence* sequence);
    void on_drag_end(Gdk::EventSequence* sequence);
};

} // namespace

#endif //RESIZING_SEPARATOR_H

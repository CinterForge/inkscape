// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Bin: widget that can hold one child, useful as a base class of custom widgets
 */
/*
 * Authors:
 *   Daniel Boles <dboles.src+inkscape@gmail.com>
 *
 * Copyright (C) 2023 Daniel Boles
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "bin.h"

#include <cassert>

#include "ui/containerize.h"

namespace Inkscape::UI::Widget {

void Bin::_construct()
{
    set_name("Bin");
    set_overflow(Gtk::Overflow::HIDDEN);
    containerize(*this);
}

Bin::Bin(Gtk::Widget *child)
{
    _construct();
    set_child(child);
}

Bin::Bin(BaseObjectType *cobject, Glib::RefPtr<Gtk::Builder> const &)
    : Gtk::Widget(cobject)
{
    _construct();
    // Add child from builder file. (For custom types, C++ wrapper must already be instantiated.)
    _child = get_first_child();
    assert(!_child || !_child->get_next_accessible_sibling());
}

void Bin::set_child(Gtk::Widget *child)
{
    if (child == _child || (child && child->get_parent())) {
        return;
    }

    if (_child) {
        _child->unparent();
    }

    _child = child;

    if (_child) {
        _child->set_parent(*this);
    }
}

void Bin::on_size_allocate(int width, int height, int baseline)
{
    Gtk::Widget::size_allocate_vfunc(width, height, baseline);

    if (_child && _child->get_visible()) {
        int min_width, min_height, ignore;
        _child->measure(Gtk::Orientation::HORIZONTAL, -1, min_width, ignore, ignore, ignore);
        _child->measure(Gtk::Orientation::VERTICAL, -1, min_height, ignore, ignore, ignore);
        _child->size_allocate({0, 0, std::max(width, min_width), std::max(height, min_height)}, baseline);
    }
}

Gtk::SizeRequestMode Bin::get_request_mode_vfunc() const
{
    return _child ? _child->get_request_mode() : Gtk::SizeRequestMode::CONSTANT_SIZE;
}

void Bin::measure_vfunc(Gtk::Orientation orientation, int for_size, int &min, int &nat, int &min_baseline, int &nat_baseline) const
{
    if (_child && _child->get_visible()) {
        _child->measure(orientation, for_size, min, nat, min_baseline, nat_baseline);
    } else {
        min = nat = min_baseline = nat_baseline = 0;
    }
}

void Bin::size_allocate_vfunc(int width, int height, int baseline)
{
    _signal_before_resize.emit(width, height, baseline);
    on_size_allocate(width, height, baseline);
    _signal_after_resize.emit(width, height, baseline);
}

} // namespace Inkscape::UI::Widget

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

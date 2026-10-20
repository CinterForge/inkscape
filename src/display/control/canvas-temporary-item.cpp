// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Provides a class that can contain active TemporaryItem's on a desktop
 * When the object is deleted, it also deletes the canvasitem it contains!
 * This object should be created/managed by a TemporaryItemList.
 * After its lifetime, it fires the timeout signal, afterwards *it deletes itself*.
 *
 * (part of code inspired by message-stack.cpp)
 *
 * Authors:
 *   Johan Engelen
 *
 * Copyright (C) Johan Engelen 2008 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/main.h>

#include "canvas-temporary-item.h"
#include "canvas-item.h"

namespace Inkscape {
namespace Display {

TemporaryItem::TemporaryItem(CanvasItem *item, int lifetime_msecs)
    : canvasitem(std::move(item))
{
    // Zero lifetime means stay forever, so do not add timeout event.
    if (lifetime_msecs > 0) {
        timeout_conn = Glib::signal_timeout().connect([this] {
            signal_timeout.emit(this);
            delete this;
            return false;
        }, lifetime_msecs);
    }
}

TemporaryItem::~TemporaryItem() = default;

} // namespace Display
} // namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

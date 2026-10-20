// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_CANVAS_TEMPORARY_ITEM_H
#define INKSCAPE_CANVAS_TEMPORARY_ITEM_H

/*
 * Authors:
 *   Johan Engelen
 *
 * Copyright (C) Johan Engelen 2008 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <sigc++/signal.h>
#include <sigc++/connection.h>
#include "display/control/canvas-item-ptr.h"
#include <sigc++/scoped_connection.h>

namespace Inkscape {

class CanvasItem;

namespace Display {

/**
 * Provides a class to put a canvasitem temporarily on-canvas.
 */
class TemporaryItem final
{
public:
    TemporaryItem(CanvasItem *item, int lifetime_msecs);
    TemporaryItem(TemporaryItem const &) = delete;
    TemporaryItem &operator=(TemporaryItem const &) = delete;
    ~TemporaryItem();

    sigc::signal<void (TemporaryItem *)> signal_timeout;

protected:
    friend class TemporaryItemList;

    CanvasItemPtr<CanvasItem> canvasitem; ///< The item we are holding on to.
    sigc::scoped_connection timeout_conn;
};

} //namespace Display
} //namespace Inkscape

#endif // INKSCAPE_CANVAS_TEMPORARY_ITEM_H

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

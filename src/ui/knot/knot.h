// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * Declarations for SPKnot: Desktop-bound visual control object.
 */
/*
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_SP_KNOT_H
#define SEEN_SP_KNOT_H

#include <glibmm/ustring.h>
#include <sigc++/scoped_connection.h>
#include <sigc++/signal.h>
#include <2geom/point.h>

#include "display/control/canvas-item-ctrl.h"
#include "display/control/canvas-item-ptr.h"
#include "knot-enums.h"

namespace Gdk {
class Cursor;
}

class SPDesktop;
class SPItem;

namespace Inkscape {
class CanvasItemCtrl;
struct CanvasEvent;
struct MotionEvent;
} // namespace Inkscape

/**
 * Desktop-bound visual control object.
 *
 * A knot is a draggable object, with callbacks to change something by
 * dragging it, visually represented by a canvas item (mostly square).
 *
 * See also KnotHolderEntity.
 * See also ControlPoint (which does the same kind of things).
 */
class SPKnot
{
public:
    SPKnot(SPDesktop *desktop, char const *tip, Inkscape::CanvasItemCtrlType type, Glib::ustring const &name = "unknown");
    virtual ~SPKnot();

    SPKnot(SPKnot const &) = delete;
    SPKnot &operator=(SPKnot const &) = delete;

    SPDesktop *desktop  = nullptr;                  /**< Desktop we are on. */
    CanvasItemPtr<Inkscape::CanvasItemCtrl> ctrl;   /**< Our CanvasItemCtrl. */
    SPItem *owner       = nullptr;                  /**< Optional Owner Item */
    SPItem *sub_owner   = nullptr;                  /**< Optional SubOwner Item */
    unsigned int flags  = SP_KNOT_VISIBLE;

    Inkscape::HandleSize _size = Inkscape::HandleSize::NORMAL; /**< Always square. Must be odd. */
    bool size_set       = false;                    /**< Use default size unless explicitly set. */
    double angle        = 0.0;                      /**< Angle of mesh handle. */
    bool is_lpe         = false;                    /**< is lpe knot. */
    Geom::Point pos;                                /**< Our desktop coordinates. */
    Geom::Point grabbed_rel_pos;                    /**< Grabbed relative position. */
    Geom::Point drag_origin;                        /**< Origin of drag. */
    SPAnchorType anchor = SP_ANCHOR_CENTER;         /**< Anchor. */

    bool grabbed        = false;
    bool moved          = false;
    Geom::IntPoint xyp;                             /**< Where drag started */
    int  tolerance      = 0;
    bool within_tolerance = false;
    bool transform_escaped = false; // true iff resize or rotate was cancelled by esc.

    Inkscape::CanvasItemCtrlShape shape = Inkscape::CANVAS_ITEM_CTRL_SHAPE_SQUARE;   /**< Shape type. */
    bool shape_set      = false;                    /**< Use default shape unless explicitly set. */

    unsigned char *image[SP_KNOT_VISIBLE_STATES];
    Glib::RefPtr<Gdk::Cursor> _cursors[SP_KNOT_VISIBLE_STATES];

    sigc::scoped_connection _event_connection;

    double pressure         = 0.0;    /**< The tablet pen pressure when the knot is being dragged. */

    // FIXME: signals should NOT need to emit the object they came from, the callee should
    // be able to figure that out
    sigc::signal<void(SPKnot *, unsigned int)> click_signal;
    sigc::signal<void (SPKnot*, unsigned int)> doubleclicked_signal;
    sigc::signal<void (SPKnot*, unsigned int)> mousedown_signal;
    sigc::signal<void (SPKnot*, unsigned int)> grabbed_signal;
    sigc::signal<void (SPKnot *, unsigned int)> ungrabbed_signal;
    sigc::signal<void (SPKnot *, Geom::Point const &, unsigned int)> moved_signal;
    sigc::signal<bool (SPKnot*, Inkscape::CanvasEvent const &)> event_signal;

    sigc::signal<bool (SPKnot*, Geom::Point*, unsigned int)> request_signal;

    // TODO: all the members above should eventually become private, accessible via setters/getters
    void setSize(Inkscape::HandleSize size);
    void setAnchor(unsigned int i);
    void setAngle(double i);

    void setImage(unsigned char* normal, unsigned char* mouseover, unsigned char* dragging, unsigned char* selected);

    void setCursor(SPKnotStateType type, Glib::RefPtr<Gdk::Cursor> cursor);
    void setTip(Glib::ustring &&tip);

    /**
     * Show knot on its canvas.
     */
    void show();

    /**
     * Hide knot on its canvas.
     */
    void hide();

    /**
     * Set flag in knot, with side effects.
     */
    void setFlag(unsigned int flag, bool set);

    /**
     * Update knot's control state.
     */
    void updateCtrl();

    /**
     * Request or set new position for knot.
     */
    void requestPosition(Geom::Point const &pos, unsigned int state);

    /**
     * Update knot for dragging and tell canvas an item was grabbed.
     */
    void startDragging(Geom::Point const &p, Geom::IntPoint const &xy, uint32_t etime);

    /**
     * Move knot to new position and emits "moved" signal.
     */
    void setPosition(Geom::Point const &p, unsigned int state);

    /**
     * Move knot to new position, without emitting a MOVED signal.
     */
    void moveto(Geom::Point const &p);
    /**
     * Select knot.
     */
    void selectKnot(bool select);

    /**
     * Returns position of knot.
     */
    Geom::Point position() const { return pos; }

    /**
     * Event handler (from CanvasItems).
     */
    bool eventHandler(Inkscape::CanvasEvent const &event);

    bool is_visible()   const { return (flags & SP_KNOT_VISIBLE)   != 0; }
    bool is_selected()  const { return (flags & SP_KNOT_SELECTED)  != 0; }
    bool is_mouseover() const { return (flags & SP_KNOT_MOUSEOVER) != 0; }
    bool is_dragging()  const { return (flags & SP_KNOT_DRAGGING)  != 0; }
    bool is_grabbed()   const { return (flags & SP_KNOT_GRABBED)   != 0; }

    void handler_request_position(Inkscape::MotionEvent const &event);

    static void ref(SPKnot *knot) { knot->ref_count++; }
    static void unref(SPKnot *knot);

private:
    Glib::ustring _tip;

    int ref_count = 1;

    /**
     * Set knot control state (dragging/mouseover/normal).
     */
    void _setCtrlState();
};

#endif // SEEN_SP_KNOT_H

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

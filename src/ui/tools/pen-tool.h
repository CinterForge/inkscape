// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_TOOLS_PEN_TOOl_H
#define INKSCAPE_UI_TOOLS_PEN_TOOl_H

/** \file
 * PenTool: a context for pen tool events.
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "display/control/canvas-item-enums.h"
#include "live_effects/effect.h"
#include "ui/tools/freehand-base.h"

#define SP_PEN_CONTEXT(obj) (dynamic_cast<Inkscape::UI::Tools::PenTool*>((Inkscape::UI::Tools::ToolBase*)obj))
#define SP_IS_PEN_CONTEXT(obj) (dynamic_cast<const Inkscape::UI::Tools::PenTool*>((const Inkscape::UI::Tools::ToolBase*)obj) != NULL)

namespace Inkscape {
class CanvasItemCtrl;
class CanvasItemCurve;

struct ButtonPressEvent;
struct MotionEvent;
struct ButtonReleaseEvent;
struct KeyPressEvent;
struct KeyReleaseEvent;
} // namespace Inkscape

namespace Inkscape::UI::Tools {

/**
 * PenTool: a context for pen tool events.
 */
class PenTool : public FreehandBase
{
public:
    PenTool(SPDesktop *desktop,
        std::string &&prefs_path = "/tools/freehand/pen",
        std::string &&cursor_filename = "pen.svg");
    ~PenTool() override;

    enum Mode {
        MODE_CLICK,
        MODE_DRAG
    };

    enum State {
        POINT,
        CONTROL,
        CLOSE,
        STOP,
        DEAD
    };

    Geom::Point p_array[5];
    Geom::Point previous;
    /** \invar npoints in {0, 2, 5}. */
    // npoints somehow determines the type of the node (what does it mean, exactly? the number of Bezier handles?)
    gint npoints = 0;

    Mode mode = MODE_CLICK;
    State state = POINT;
    bool polylines_only = false;
    bool polylines_paraxial = false;
    Geom::Point paraxial_angle;

    bool spiro = false;  // Spiro mode active?
    bool bspline = false; // BSpline mode active?

    unsigned int expecting_clicks_for_LPE = 0; // if positive, finish the path after this many clicks
    Inkscape::LivePathEffect::Effect *waiting_LPE = nullptr; // if NULL, waiting_LPE_type in SPDrawContext is taken into account
    SPLPEItem *waiting_item = nullptr;

    CanvasItemPtr<CanvasItemCtrl> ctrl[4]; // Origin, Start, Center, End point of path.
    static constexpr std::array<CanvasItemCtrlType, 4> ctrl_types = {
        CANVAS_ITEM_CTRL_TYPE_NODE_SMOOTH, CANVAS_ITEM_CTRL_TYPE_ROTATE,
        CANVAS_ITEM_CTRL_TYPE_ROTATE, CANVAS_ITEM_CTRL_TYPE_NODE_SMOOTH};

    CanvasItemPtr<CanvasItemCurve> cl0;
    CanvasItemPtr<CanvasItemCurve> cl1;
    
    bool events_disabled = false;

    void nextParaxialDirection(Geom::Point const &pt, Geom::Point const &origin, guint state);
    void setPolylineMode();
    bool hasWaitingLPE();
    void waitForLPEMouseClicks(Inkscape::LivePathEffect::EffectType effect_type, unsigned int num_clicks, bool use_polylines = true);

protected:
    void set(Inkscape::Preferences::Entry const &val) override;
    bool root_handler(CanvasEvent const &event) override;
    bool item_handler(SPItem* item, CanvasEvent const &event) override;

private:
    bool _handleButtonPress(ButtonPressEvent const &event);
    bool _handle2ButtonPress(ButtonPressEvent const &event);
    bool _handleMotionNotify(MotionEvent const &event);
    bool _handleButtonRelease(ButtonReleaseEvent const &event);
    bool _handleKeyPress(KeyPressEvent const &event);

    //this function changes the colors red, green and blue making them transparent or not depending on if the function uses spiro
    void _bsplineSpiroColor();
    //creates a node in bspline or spiro modes
    void _bsplineSpiro(bool shift);
    //creates a node in bspline or spiro modes
    void _bsplineSpiroOn();
    //creates a CUSP node
    void _bsplineSpiroOff();
    //continues the existing curve in bspline or spiro mode
    void _bsplineSpiroStartAnchor(bool cusp_node);
    //continues the existing curve with the union node in bspline or spiro modes
    void _bsplineSpiroStartAnchorOn();
    //continues an existing curve with the union node in CUSP mode
    void _bsplineSpiroStartAnchorOff();
    //modifies the "red_curve" when it detects movement
    void _bsplineSpiroMotion(bool cusp_node, bool move_prev);
    //closes the curve with the last node in bspline or spiro mode
    void _bsplineSpiroEndAnchorOn();
    //closes the curve with the last node in CUSP mode
    void _bsplineSpiroEndAnchorOff();
    //apply the effect
    void _bsplineSpiroBuild();

    void _setInitialPoint(Geom::Point const p);
    void _setSubsequentPoint(Geom::Point const p, bool statusbar, guint status = 0);
    void _setCtrl(Geom::Point const p, guint state);
    void _finishSegment(Geom::Point p, guint state);
    bool _undoLastPoint(bool user_undo = false);
    bool _redoLastPoint();

    void _finish(bool closed);

    void _resetColors();

    void _disableEvents();
    void _enableEvents();

    void _setToNearestHorizVert(Geom::Point &pt, guint const state) const;

    void _setAngleDistanceStatusMessage(Geom::Point const p, int pc_point_to_compare, Glib::ustring const &message);

    void _lastpointToLine();
    void _lastpointToCurve();
    void _lastpointMoveScreen(gdouble x, gdouble y);
    void _lastpointMove(gdouble x, gdouble y);
    void _redrawAll();

    void _endpointSnapHandle(Geom::Point &p, guint const state);
    void _endpointSnap(Geom::Point &p, guint const state);

    void _cancel();

    sigc::connection _desktop_destroy;
    // NOTE: undoing work in progress always deletes the last added point,
    // so there's no need for an undo stack.
    std::vector<Geom::PathVector> _redo_stack; ///< History of undone events
    bool _did_redo = false;

    Util::ActionAccel _acc_to_line;
    Util::ActionAccel _acc_to_curve;
    Util::ActionAccel _acc_to_guides;

    Modifiers::Modifier *mod_freehand_angle_snapping;
    Modifiers::Modifier *mod_freehand_dot;
    Modifiers::Modifier *mod_move_no_snapping;
    Modifiers::Modifier *mod_pen_cusp_node;
    Modifiers::Modifier *mod_pen_move_prev;
    Modifiers::Modifier *mod_pen_switch_axis;
    Modifiers::Modifier *mod_select_add_to;
};

} // namespace Inkscape:UI::Tools

#endif // INKSCAPE_UI_TOOLS_PEN_TOOl_H

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

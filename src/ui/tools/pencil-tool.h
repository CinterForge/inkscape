// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_TOOLS_PENCIL_TOOl_H
#define INKSCAPE_UI_TOOLS_PENCIL_TOOl_H

/** \file
 * PencilTool: a context for pencil tool events
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "freehand-base.h"

class SPShape;

#define SP_IS_PENCIL_CONTEXT(obj) (dynamic_cast<const Inkscape::UI::Tools::PencilTool*>((const Inkscape::UI::Tools::ToolBase*)obj) != NULL)

namespace Inkscape {
struct ButtonPressEvent;
struct MotionEvent;
struct ButtonReleaseEvent;
struct KeyPressEvent;
struct KeyReleaseEvent;
struct ExtendedInput;
} // namespace Inkscape

namespace Inkscape::UI::Tools {

enum PencilState {
    SP_PENCIL_CONTEXT_IDLE,
    SP_PENCIL_CONTEXT_ADDLINE,
    SP_PENCIL_CONTEXT_FREEHAND,
    SP_PENCIL_CONTEXT_SKETCH
};

/**
 * PencilTool: a context for pencil tool events
 */
class PencilTool : public FreehandBase
{
public:
    PencilTool(SPDesktop *desktop);
    ~PencilTool() override;

    Geom::Point p_array[16];
    std::vector<Geom::Point> ps;
    std::vector<Geom::Point> points;
    void addPowerStrokePencil();
    void powerStrokeInterpolate(Geom::Path const path);
    Geom::Piecewise<Geom::D2<Geom::SBasis> > sketch_interpolation; // the current proposal from the sketched paths
    unsigned sketch_n = 0; // number of sketches done

protected:
    bool root_handler(CanvasEvent const &event) override;

private:
    bool _handleButtonPress(ButtonPressEvent const &event);
    bool _handleMotionNotify(MotionEvent const &event);
    bool _handleButtonRelease(ButtonReleaseEvent const &event);
    bool _handleKeyPress(KeyPressEvent const &event);
    bool _handleKeyRelease(KeyReleaseEvent const &event);
    void _setStartpoint(Geom::Point const &p);
    void _setEndpoint(Geom::Point const &p);
    void _finishEndpoint();
    void _addFreehandPoint(Geom::Point const &p, guint state, bool last);
    void _fitAndSplit();
    void _interpolate();
    void _sketchInterpolate();
    void _extinput(ExtendedInput const &ext);
    void _cancel();
    void _endpointSnap(Geom::Point &p, guint const state);
    std::vector<Geom::Point> _wps;
    Geom::PathVector _pressure_curve;
    Geom::Point _req_tangent;
    bool _is_drawing = false;
    PencilState _state = SP_PENCIL_CONTEXT_IDLE;
    int _npoints = 0;

    Modifiers::Modifier *mod_freehand_angle_snapping;
    Modifiers::Modifier *mod_freehand_dot;
    Modifiers::Modifier *mod_move_no_snapping;
    Modifiers::Modifier *mod_pencil_sketch;
    Modifiers::Modifier *mod_select_add_to;
};

} // namespace Inkscape::UI::Tools

#endif // INKSCAPE_UI_TOOLS_PENCIL_TOOl_H

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

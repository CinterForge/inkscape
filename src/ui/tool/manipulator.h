// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Manipulator - edits something on-canvas
 */
/* Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *
 * Copyright (C) 2009 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_TOOL_MANIPULATOR_H
#define INKSCAPE_UI_TOOL_MANIPULATOR_H

#include "ui/tools/tool-base.h"

class SPDesktop;
namespace Inkscape {
namespace UI {

class ControlPointSelection;

/**
 * @brief Tool component that processes events and does something in response to them.
 * Note: this class is probably redundant.
 */
class Manipulator
{
public:
    Manipulator(SPDesktop *d)
        : _desktop(d)
    {}
    virtual ~Manipulator() = default;
    
    /// Handle input event. Returns true if handled.
    virtual bool event(Inkscape::UI::Tools::ToolBase *tool, CanvasEvent const &event) = 0;
    SPDesktop *const _desktop;
};

/**
 * @brief Tool component that edits something on the canvas using selectable control points.
 * Note: this class is probably redundant.
 */
class PointManipulator
    : public Manipulator
    , public sigc::trackable
{
public:
    PointManipulator(SPDesktop *d, ControlPointSelection &sel)
        : Manipulator(d)
        , _selection(sel)
    {}

    /// Type of extremum points to add in PathManipulator::insertNodeAtExtremum
    enum ExtremumType
    {
        EXTR_MIN_X,
        EXTR_MAX_X,
        EXTR_MIN_Y,
        EXTR_MAX_Y
    };

protected:
    ControlPointSelection &_selection;
};

} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_TOOL_MANIPULATOR_H

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

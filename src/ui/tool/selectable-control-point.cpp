// SPDX-License-Identifier: GPL-2.0-or-later
/* Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2009 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "ui/tool/selectable-control-point.h"
#include "ui/tool/control-point-selection.h"
#include "ui/widget/events/canvas-event.h"

namespace Inkscape {
namespace UI {

SelectableControlPoint::SelectableControlPoint(SPDesktop *d, Geom::Point const &initial_pos, SPAnchorType anchor,
                                               Inkscape::CanvasItemCtrlType type,
                                               ControlPointSelection &sel,
                                               Inkscape::CanvasItemGroup *group)
    : ControlPoint(d, initial_pos, anchor, type, group)
    , _selection(sel)
{
    _canvas_item_ctrl->set_name("CanvasItemCtrl:SelectableControlPoint");
    _selection.allPoints().insert(this);
}

SelectableControlPoint::~SelectableControlPoint()
{
    _selection.erase(this);
    _selection.allPoints().erase(this);
}

bool SelectableControlPoint::grabbed(MotionEvent const &)
{
    // if a point is dragged while not selected, it should select itself
    if (!selected()) {
        _takeSelection();
    }
    _selection._pointGrabbed(this);
    return false;
}

void SelectableControlPoint::dragged(Geom::Point &new_pos, MotionEvent const &event)
{
    _selection._pointDragged(new_pos, event);
}

void SelectableControlPoint::ungrabbed(ButtonReleaseEvent const *)
{
    _selection._pointUngrabbed();
}

bool SelectableControlPoint::clicked(ButtonReleaseEvent const &event)
{
    if (_selection._pointClicked(this, event)) {
        return true;
    }

    if (event.button != 1) return false;
    if (mod_shift(event)) {
        if (selected()) {
            _selection.erase(this);
        } else {
            _selection.insert(this);
        }
    } else {
        _takeSelection();
    }
    return true;
}

void SelectableControlPoint::select(bool toselect)
{
    if (toselect) {
        _selection.insert(this);
    } else {
        _selection.erase(this);
    }
}

void SelectableControlPoint::_takeSelection()
{
    _selection.clear();
    _selection.insert(this);
}

bool SelectableControlPoint::selected() const
{
    SelectableControlPoint *p = const_cast<SelectableControlPoint*>(this);
    return _selection.find(p) != _selection.end();
}

void SelectableControlPoint::_setState(State state)
{
    if (!selected()) {
        ControlPoint::_setState(state);
    } else {
        _canvas_item_ctrl->set_normal(true);
        switch (state) {
            case STATE_NORMAL:
                break;
            case STATE_MOUSEOVER:
                _canvas_item_ctrl->set_hover();
                break;
            case STATE_CLICKED:
                _canvas_item_ctrl->set_click();
                break;
        }
        _state = state;
    }
}

} // namespace UI
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

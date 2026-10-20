// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_RUBBERBAND_H
#define SEEN_RUBBERBAND_H
/*
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Carl Hetherington <inkscape@carlh.net>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cairomm/pattern.h>
#include <2geom/point.h>
#include <2geom/path.h>
#include <2geom/rect.h>
#include <vector>
#include "display/control/canvas-item-enums.h"
#include "display/control/canvas-item-ptr.h"

/* fixme: do multidocument safe */

class SPDesktop;

namespace Inkscape {

class CanvasItemBpath;
class CanvasItemRect;

/**
 * Rubberbanding selector.
 */
class Rubberband
{
public:
    enum class Mode {
        RECT,
        TOUCHPATH,
        TOUCHRECT
    };
    enum class Operation {
        ADD,
        INVERT,
        REMOVE
    };

    void start(SPDesktop *desktop, Geom::Point const &p, bool tolerance = false);
    void move(Geom::Point const &p);
    Geom::OptRect getRectangle() const;
    void stop();
    bool isStarted() const { return _started; }
    bool isMoved() const { return _moved; }

    Rubberband::Mode getMode() const { return _mode; }
    std::vector<Geom::Point> getPoints() const;
    Geom::Path getPath() const;

    static constexpr auto default_mode = Rubberband::Mode::RECT;
    static constexpr auto default_operation = Rubberband::Operation::ADD;
    static constexpr auto default_handle = CanvasItemCtrlType::RUBBERBAND_RECT;
    static constexpr auto default_deselect_handle = CanvasItemCtrlType::RUBBERBAND_DESELECT;
    static constexpr auto default_invert_handle = CanvasItemCtrlType::RUBBERBAND_INVERT;

    void setMode(Rubberband::Mode mode) { _mode = mode; };
    void setOperation(Rubberband::Operation operation) { _operation = operation; };
    void setHandle(CanvasItemCtrlType handle) { _handle = handle; _invert_handle = _get_invert_handle(handle); _deselect_handle = _get_deselect_handle(handle); };

    static Rubberband* get(SPDesktop *desktop);

private:
    Rubberband(SPDesktop *desktop);
    static Rubberband* _instance;
    
    SPDesktop *_desktop;
    Geom::Point _start;
    Geom::Point _end;
    Geom::Path _path;

    CanvasItemPtr<CanvasItemRect> _rect;
    CanvasItemPtr<CanvasItemBpath> _touchpath;
    CanvasItemCtrlType _handle = default_handle; // Used for styling through css
    CanvasItemCtrlType _invert_handle = default_invert_handle;
    CanvasItemCtrlType _deselect_handle = default_deselect_handle;
    Geom::Path _touchpath_curve;

    CanvasItemCtrlType _get_deselect_handle(CanvasItemCtrlType handle) {
        // use default deselect mechanism unless it's a freehand path
        if (handle == CanvasItemCtrlType::RUBBERBAND_TOUCHPATH_SELECT) {
            return CanvasItemCtrlType::RUBBERBAND_TOUCHPATH_DESELECT;
        } else {
            return CanvasItemCtrlType::RUBBERBAND_DESELECT;
        }
    }

    CanvasItemCtrlType _get_invert_handle(CanvasItemCtrlType handle) {
        // use default invert mechanism unless it's a freehand path
        return handle == CanvasItemCtrlType::RUBBERBAND_TOUCHPATH_SELECT ? CanvasItemCtrlType::RUBBERBAND_TOUCHPATH_INVERT : CanvasItemCtrlType::RUBBERBAND_INVERT;
    }

    void delete_canvas_items();

    bool _started = false;
    bool _moved = false;
    Rubberband::Mode _mode = default_mode;
    Rubberband::Operation _operation = default_operation;
    double _tolerance = 0.0;
};

} // namespace Inkscape

#endif // SEEN_RUBBERBAND_H

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

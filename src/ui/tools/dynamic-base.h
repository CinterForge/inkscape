// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_TOOLS_DYNAMIC_BASE_H
#define INKSCAPE_UI_TOOLS_DYNAMIC_BASE_H

/*
 * Common drawing mode. Base class of Eraser and Calligraphic tools.
 *
 * Authors:
 *   Mitsuru Oka <oka326@parkcity.ne.jp>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * The original dynadraw code:
 *   Paul Haeberli <paul@sgi.com>
 *
 * Copyright (C) 1998 The Free Software Foundation
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 * Copyright (C) 2008 Jon A. Cruz
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <2geom/path.h>

#include "ui/tools/tool-base.h"
#include "display/control/canvas-item-ptr.h"

inline constexpr int SAMPLING_SIZE = 8;

namespace Inkscape {

class CanvasItemBpath;
namespace XML { class Node; }

namespace UI {
namespace Tools {

class DynamicBase : public ToolBase
{
public:
    DynamicBase(SPDesktop *desktop, std::string &&prefs_path, std::string &&cursor_filename);
    ~DynamicBase() override;

    void set(Preferences::Entry const &val) override;

protected:
    /** accumulated shape which ultimately goes in svg:path */
    Geom::Path accumulated;

    /** canvas items for "committed" segments */
    std::vector<CanvasItemPtr<CanvasItemBpath>> segments;

    /** canvas item for red "leading" segment */
    CanvasItemPtr<CanvasItemBpath> currentshape;

    /** shape of red "leading" segment */
    Geom::Path currentcurve;

    /** left edge of the stroke; combined to get accumulated */
    Geom::Path cal1;

    /** right edge of the stroke; combined to get accumulated */
    Geom::Path cal2;

    /** left edge points for this segment */
    Geom::Point point1[SAMPLING_SIZE];

    /** right edge points for this segment */
    Geom::Point point2[SAMPLING_SIZE];

    /** number of edge points for this segment */
    int npoints = 0;

    /* repr */
    XML::Node *repr = nullptr;

    /* common */
    Geom::Point cur;
    Geom::Point vel;
    double vel_max = 0.0;
    Geom::Point acc;
    Geom::Point ang;
    Geom::Point last;
    Geom::Point del;

    /* extended input data */
    double pressure = 1.0;
    double xtilt = 0.0;
    double ytilt = 0.0;

    /* attributes */
    bool usepressure = false;
    bool usetilt = false;
    double mass = 0.3;
    double drag = 1.0;
    double angle = 30.0;
    double width = 0.2;

    double vel_thin = 0.1;
    double flatness = 0.9;
    double tremor = 0.0;
    double cap_rounding = 0.0;

    bool is_drawing = false;

    /** uses absolute width independent of zoom */
    bool abs_width = false;

    Geom::Point getViewPoint(Geom::Point const &n) const;
    Geom::Point getNormalizedPoint(Geom::Point const &v) const;
};

} // namespace Tools
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_TOOLS_DYNAMIC_BASE_H

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

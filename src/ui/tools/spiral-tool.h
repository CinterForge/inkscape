// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_TOOLS_SPIRAL_TOOl_H
#define INKSCAPE_UI_TOOLS_SPIRAL_TOOl_H

/** \file
 * Spiral drawing context
 */
/*
 * Authors:
 *   Mitsuru Oka
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2001 Lauris Kaplinski
 * Copyright (C) 2001-2002 Mitsuru Oka
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "ui/tools/tool-base.h"
#include "object/weakptr.h"

class SPSpiral;

namespace Inkscape { class Selection; }

namespace Inkscape::UI::Tools {

class SpiralTool : public ToolBase
{
public:
    SpiralTool(SPDesktop *desktop);
    ~SpiralTool() override;

    void set(Preferences::Entry const &val) override;
    bool root_handler(CanvasEvent const &event) override;

private:
    SPWeakPtr<SPSpiral> spiral;
    Geom::Point center;
    double revo = 3.0;
    double exp = 1.0;
    double t0 = 0.0;

    sigc::connection sel_changed_connection;

    Modifiers::Modifier *mod_freehand_angle_snapping;
    Modifiers::Modifier *mod_select_add_to;

    void drag(Geom::Point const &p, unsigned state);
    void finishItem();
    void cancel();
    void selection_changed(Selection *selection);
};

} // namespace Inkscape::UI::Tools

#endif // INKSCAPE_UI_TOOLS_ARC_TOOl_H

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

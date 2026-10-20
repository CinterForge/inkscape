// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_TOOLS_STAR_TOOL_H
#define INKSCAPE_UI_TOOLS_STAR_TOOL_H

/*
 * Star drawing context
 *
 * Authors:
 *   Mitsuru Oka <oka326@parkcity.ne.jp>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2001-2002 Mitsuru Oka
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "ui/tools/tool-base.h"
#include "object/weakptr.h"

#include "2geom/affine.h"

class SPStar;

namespace Inkscape { class Selection; }

namespace Inkscape::UI::Tools {

class StarTool : public ToolBase
{
public:
    StarTool(SPDesktop *desktop);
    ~StarTool() override;

    void set(Preferences::Entry const &val) override;
    bool root_handler(CanvasEvent const &event) override;

private:
    SPWeakPtr<SPStar> star;

    Geom::Point center;

    /* Number of corners */
    int magnitude = 5;

    /* Outer/inner radius ratio */
    double proportion = 0.5;

    /* flat sides or not? */
    bool isflatsided = false;

    /* rounded corners ratio */
    double rounded = 0.0;

    // randomization
    double randomized = 0.0;

    // Scale of the new object
    Geom::Affine _tr = Geom::identity();

    sigc::connection sel_changed_connection;

    Modifiers::Modifier *mod_freehand_angle_snapping;
    Modifiers::Modifier *mod_select_add_to;

    void drag(Geom::Point p, unsigned state);
    void finishItem();
    void cancel();
    void selection_changed(Selection *selection);
};

} // namespace Inkscape::UI::Tools

#endif // INKSCAPE_UI_TOOLS_STAR_TOOL_H

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

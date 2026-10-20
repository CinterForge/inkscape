// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_TOOLS_RECT_TOOL_H
#define INKSCAPE_UI_TOOLS_RECT_TOOL_H

/*
 * Rectangle drawing context
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "ui/tools/tool-base.h"
#include "object/weakptr.h"

class SPRect;

namespace Inkscape { class Selection; }

namespace Inkscape::UI::Tools {

class RectTool : public ToolBase
{
public:
    RectTool(SPDesktop *desktop);
    ~RectTool() override;

    void set(Preferences::Entry const &val) override;
    bool root_handler(CanvasEvent const &event) override;
    bool item_handler(SPItem *item, CanvasEvent const &event) override;

private:
    SPWeakPtr<SPRect> rect;
    Geom::Point center;

    double rx;	/* roundness radius (x direction) */
    double ry;	/* roundness radius (y direction) */

    sigc::connection sel_changed_connection;

    Modifiers::Modifier *mod_select_add_to;
    Modifiers::Modifier *mod_select_force_drag;
    Modifiers::Modifier *mod_trans_confine;
    Modifiers::Modifier *mod_trans_off_center;

    void drag(Geom::Point const pt, unsigned state);
    void finishItem();
    void cancel();
    void selection_changed(Selection *selection);
};

} // namespace Inkscape::UI::Tools

#endif // INKSCAPE_UI_TOOLS_RECT_TOOL_H

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

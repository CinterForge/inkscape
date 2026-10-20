// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_TOOLS_BOX3D_TOOL_H
#define INKSCAPE_UI_TOOLS_BOX3D_TOOL_H

/*
 * 3D box drawing tool
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2002 Lauris Kaplinski
 * Copyright (C) 2007 Maximilian Albert <Anhalter42@gmx.de>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "object/weakptr.h"
#include "proj_pt.h"
#include "vanishing-point.h"

#include "ui/modifiers.h"
#include "ui/tools/tool-base.h"

class SPItem;
class SPBox3D;
namespace Box3D { struct VPDrag; }
namespace Inkscape { class Selection; }

namespace Inkscape::UI::Tools {

class Box3dTool : public ToolBase
{
public:
    Box3dTool(SPDesktop *desktop);
    ~Box3dTool() override;

    std::unique_ptr<Box3D::VPDrag> _vpdrag;

    bool root_handler(CanvasEvent const &event) override;
    bool item_handler(SPItem *item, CanvasEvent const &event) override;

private:
    SPWeakPtr<SPBox3D> box3d;
    Geom::Point center;

    /**
     * save three corners while dragging:
     * 1) the starting point (already done by the event_context)
     * 2) drag_ptB --> the opposite corner of the front face (before pressing shift)
     * 3) drag_ptC --> the "extruded corner" (which coincides with the mouse pointer location
     *    if we are ctrl-dragging but is constrained to the perspective line from drag_ptC
     *    to the vanishing point Y otherwise)
     */
    Geom::Point drag_origin;
    Geom::Point drag_ptB;
    Geom::Point drag_ptC;

    Proj::Pt3 drag_origin_proj;
    Proj::Pt3 drag_ptB_proj;
    Proj::Pt3 drag_ptC_proj;

    bool two_dimension_dragged = false; ///< whether we are ctrl-dragging (extruding in two dimensions)
    bool extruded = false; ///< whether shift-dragging already occurred (i.e. the box is already extruded)

    sigc::scoped_connection sel_changed_connection;

    Modifiers::Modifier *mod_box3d_extrude_one;
    Modifiers::Modifier *mod_box3d_extrude_two;
    Modifiers::Modifier *mod_select_add_to;
    Modifiers::Modifier *mod_select_force_drag;
    Modifiers::Modifier *mod_select_in_groups;

    void selection_changed(Selection *selection);

    void drag();
	void finishItem();
    void cancel();
};

} // namespace Inkscape::UI::Tools

#endif // INKSCAPE_UI_TOOLS_BOX3D_TOOL_H

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

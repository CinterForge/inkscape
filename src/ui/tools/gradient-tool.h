// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Gradient drawing and editing tool
 */
/*
 * Authors:
 *   bulia byak <buliabyak@users.sf.net>
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *   Jon A. Cruz <jon@joncruz.org.
 *
 * Copyright (C) 2007 Johan Engelen
 * Copyright (C) 2005,2010 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_TOOLS_GRADIENT_TOOL_H
#define INKSCAPE_UI_TOOLS_GRADIENT_TOOL_H

#include "ui/tools/tool-base.h"

namespace Inkscape { class Selection; }

namespace Inkscape::UI::Tools {

class GradientTool : public ToolBase
{
public:
    GradientTool(SPDesktop *desktop);
    ~GradientTool() override;

    void add_stops_between_selected_stops();

protected:
    bool root_handler(CanvasEvent const &event) override;

private:
    Geom::Point mousepoint_doc; // stores mousepoint when over_line in doc coords
    Geom::Point origin;
    bool cursor_addnode = false;

    sigc::scoped_connection selcon;
    sigc::scoped_connection subselcon;

    Modifiers::Modifier *mod_freehand_angle_snapping;
    Modifiers::Modifier *mod_gradient_create;
    Modifiers::Modifier *mod_node_insert;
    Modifiers::Modifier *mod_select_add_to;
    Modifiers::Modifier *mod_select_force_drag;

    void select_next();
    void select_prev();

    void selection_changed();
    void simplify(double tolerance);
    void add_stop_near_point(SPItem *item, Geom::Point const &mouse_p);
    void drag(Geom::Point const &pt, uint32_t etime);
    SPItem *is_over_curve(Geom::Point const &event_p);
};

} // namespace Inkscape::UI::Tools

#endif // INKSCAPE_UI_TOOLS_GRADIENT_TOOL_H

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

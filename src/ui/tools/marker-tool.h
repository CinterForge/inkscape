// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_TOOLS_MARKER_TOOl_H
#define INKSCAPE_UI_TOOLS_MARKER_TOOl_H

/** @file
 * Marker edit mode - onCanvas marker editing of marker orientation, position, scale
 *//*
 * Authors:
 * see git history
 * Rachana Podaralla <rpodaralla3@gatech.edu>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#include "ui/tools/tool-base.h"
#include "ui/tool/shape-record.h"

namespace Inkscape { class Selection; }

namespace Inkscape::UI::Tools {

class MarkerTool : public ToolBase
{
public:
    MarkerTool(SPDesktop *desktop);
    ~MarkerTool() override;

    void selection_changed(Inkscape::Selection *selection);

    bool root_handler(CanvasEvent const &event) override;
    std::map<SPItem *, std::unique_ptr<ShapeEditor>> _shape_editors;

    int editMarkerMode = -1;

private:
    Modifiers::Modifier *mod_select_force_drag;

    sigc::connection sel_changed_connection;
    ShapeRecord get_marker_transform(SPShape *shape, SPItem *parent_item, SPMarker *sp_marker, SPMarkerLoc marker_type);
};

} // namespace Inkscape::UI::Tools

#endif // INKSCAPE_UI_TOOLS_MARKER_TOOl_H


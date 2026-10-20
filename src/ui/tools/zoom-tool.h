// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_TOOLS_ZOOM_TOOL_H
#define INKSCAPE_UI_TOOLS_ZOOM_TOOL_H

/*
 * Handy zooming tool
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *
 * Copyright (C) 1999-2002 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "ui/tools/tool-base.h"

namespace Inkscape::UI::Tools {

class ZoomTool : public ToolBase
{
public:
    ZoomTool(SPDesktop *desktop);
    ~ZoomTool() override;

    bool root_handler(CanvasEvent const &event) override;

private:
    bool escaped = false;

    Modifiers::Modifier *mod_canvas_zoom_invert;
};

} // namespace Inkscape::UI::Tools

#endif // INKSCAPE_UI_TOOLS_ZOOM_TOOL_H

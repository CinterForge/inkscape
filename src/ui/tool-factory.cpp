// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Factory for ToolBase tree
 *
 * Authors:
 *   Markus Engel
 *
 * Copyright (C) 2013 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "tool-factory.h"

#include "ui/tools/arc-tool.h"
#include "ui/tools/box3d-tool.h"
#include "ui/tools/calligraphic-tool.h"
#include "ui/tools/connector-tool.h"
#include "ui/tools/dropper-tool.h"
#include "ui/tools/eraser-tool.h"
#include "ui/tools/flood-tool.h"
#include "ui/tools/gradient-tool.h"
#include "ui/tools/lpe-tool.h"
#include "ui/tools/measure-tool.h"
#include "ui/tools/mesh-tool.h"
#include "ui/tools/node-tool.h"
#include "ui/tools/object-picker-tool.h"
#include "ui/tools/pages-tool.h"
#include "ui/tools/pencil-tool.h"
#include "ui/tools/rect-tool.h"
#include "ui/tools/marker-tool.h"
#include "ui/tools/select-tool.h"
#include "ui/tools/booleans-tool.h"
#include "ui/tools/spiral-tool.h"
#include "ui/tools/spray-tool.h"
#include "ui/tools/star-tool.h"
#include "ui/tools/text-tool.h"
#include "ui/tools/tweak-tool.h"
#include "ui/tools/zoom-tool.h"

using namespace Inkscape::UI::Tools;

ToolBase *ToolFactory::createObject(SPDesktop *desktop, std::string const &id)
{
    ToolBase *tool = nullptr;

    if (id == "/tools/shapes/arc")
        tool = new ArcTool(desktop);
    else if (id == "/tools/shapes/3dbox")
        tool = new Box3dTool(desktop);
    else if (id == "/tools/calligraphic")
        tool = new CalligraphicTool(desktop);
    else if (id == "/tools/connector")
        tool = new ConnectorTool(desktop);
    else if (id == "/tools/dropper")
        tool = new DropperTool(desktop);
    else if (id == "/tools/eraser")
        tool = new EraserTool(desktop);
    else if (id == "/tools/paintbucket")
        tool = new FloodTool(desktop);
    else if (id == "/tools/gradient")
        tool = new GradientTool(desktop);
    else if (id == "/tools/lpetool")
        tool = new LpeTool(desktop);
    else if (id == "/tools/marker")
        tool = new MarkerTool(desktop);
    else if (id == "/tools/measure")
        tool = new MeasureTool(desktop);
    else if (id == "/tools/mesh")
        tool = new MeshTool(desktop);
    else if (id == "/tools/nodes")
        tool = new NodeTool(desktop);
    else if (id == "/tools/booleans")
        tool = new InteractiveBooleansTool(desktop);
    else if (id == "/tools/pages")
        tool = new PagesTool(desktop);
    else if (id == "/tools/freehand/pencil")
        tool = new PencilTool(desktop);
    else if (id == "/tools/freehand/pen")
        tool = new PenTool(desktop);
    else if (id == "/tools/shapes/rect")
        tool = new RectTool(desktop);
    else if (id == "/tools/select")
        tool = new SelectTool(desktop);
    else if (id == "/tools/shapes/spiral")
        tool = new SpiralTool(desktop);
    else if (id == "/tools/spray")
        tool = new SprayTool(desktop);
    else if (id == "/tools/shapes/star")
        tool = new StarTool(desktop);
    else if (id == "/tools/text")
        tool = new TextTool(desktop);
    else if (id == "/tools/tweak")
        tool = new TweakTool(desktop);
    else if (id == "/tools/zoom")
        tool = new ZoomTool(desktop);
    else if (id == "/tools/picker")
        tool = new ObjectPickerTool(desktop);
    else {
        fprintf(stderr, "WARNING: unknown tool: %s", id.c_str());
        // Backup tool prevents crashes in signals that expect a tool to exist.
        tool = new SelectTool(desktop);
    }

    return tool;
}

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

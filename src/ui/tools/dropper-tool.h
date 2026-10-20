// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_TOOLS_DROPPER_TOOL_H
#define INKSCAPE_UI_TOOLS_DROPPER_TOOL_H

/*
 * Tool for picking colors from drawing
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "display/control/canvas-item-ptr.h"
#include "ui/modifiers.h"
#include "ui/tools/tool-base.h"

namespace Inkscape { class CanvasItemBpath; }
namespace Inkscape::UI::Tools {

class DropperTool : public ToolBase
{
public:
    DropperTool(SPDesktop *desktop);
    ~DropperTool() override;

    std::optional<Colors::Color> get_color(bool invert = false, bool non_dropping = false) const;
    sigc::signal<void (Colors::Color const &)> onetimepick_signal;

protected:
    bool root_handler(CanvasEvent const &event) override;

private:
    unsigned _translucency_key;

    // Stored color.
    std::optional<Colors::Color> stored_color;

    // Stored color taken from canvas. Used by clipboard.
    // Identical to stored_color if dropping disabled.
    std::optional<Colors::Color> non_dropping_color;

    bool invert = false;   ///< Set color to inverse rgb value
    bool stroke = false;   ///< Set to stroke color. In dropping mode, set from stroke color
    bool dropping = false; ///< When true, get color from selected objects instead of canvas
    // bool dragging = false; ///< When true, get average color for region on canvas, instead of a single point

    double radius = 0.0;                       ///< Size of region under dragging mode
    CanvasItemPtr<CanvasItemBpath> area;       ///< Circle depicting region's borders in dragging mode
    Geom::Point centre;                        ///< Center of region in dragging mode

    Modifiers::Modifier *mod_dropper_dropping;
    Modifiers::Modifier *mod_dropper_invert;
    Modifiers::Modifier *mod_dropper_stroke;
    Modifiers::Modifier *mod_select_force_drag;
};

} // namespace Inkscape::UI::Tools

#endif // INKSCAPE_UI_TOOLS_DROPPER_TOOL_H

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

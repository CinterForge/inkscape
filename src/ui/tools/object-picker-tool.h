// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_OBJECT_PICKER_TOOL_H
#define SEEN_OBJECT_PICKER_TOOL_H

#include "display/control/canvas-item-ptr.h"
#include "display/control/canvas-item-rect.h"
#include "display/control/canvas-item-text.h"
#include "ui/tools/tool-base.h"

namespace Inkscape::UI::Tools {

class ObjectPickerTool : public ToolBase {
public:
    ObjectPickerTool(SPDesktop* desktop);
    ~ObjectPickerTool() override;

    sigc::signal<bool (SPObject*)> signal_object_picked;
    sigc::signal<void()> signal_tool_switched;

private:
    bool root_handler(const CanvasEvent& event) override;
    void show_text(const Geom::Point& cursor, const char* text);
    CanvasItemPtr<CanvasItemText> _label;
    CanvasItemPtr<CanvasItemRect> _frame;
    sigc::scoped_connection _zoom;
};

} // namespaces

#endif

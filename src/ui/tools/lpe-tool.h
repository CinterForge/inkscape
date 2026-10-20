// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * LPETool: a generic tool composed of subtools that are given by LPEs
 */
/* Authors:
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *
 * Copyright (C) 1998 The Free Software Foundation
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 * Copyright (C) 2008 Maximilian Albert
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_TOOLS_LPE_TOOL_H
#define INKSCAPE_UI_TOOLS_LPE_TOOL_H

#include "ui/tools/pen-tool.h"

// This is the list of subtools from which the toolbar of the LPETool is built automatically.
extern int const num_subtools;

struct SubtoolEntry
{
    Inkscape::LivePathEffect::EffectType type;
    char const *icon_name;
};

extern SubtoolEntry const lpesubtools[];

class ShapeEditor;

namespace Inkscape {
class CanvasItemText;
class CanvasItemRect;
class Selection;
} // namespace Inkscape

namespace Inkscape::UI::Tools {

class LpeTool : public PenTool
{
public:
    LpeTool(SPDesktop *desktop);
    ~LpeTool() override;

    void switch_mode(LivePathEffect::EffectType type);
    void reset_limiting_bbox();
    void create_measuring_items(Selection *selection = nullptr);
    void delete_measuring_items();
    void update_measuring_items();
    void show_measuring_info(bool show = true);

    LivePathEffect::EffectType mode = LivePathEffect::BEND_PATH;

protected:
    void set(Preferences::Entry const &val) override;
    bool root_handler(CanvasEvent const &event) override;
    bool item_handler(SPItem *item, CanvasEvent const &event) override;

private:
    void selection_changed(Selection *selection);

    std::unique_ptr<ShapeEditor> shape_editor;
    CanvasItemPtr<CanvasItemRect> canvas_bbox;

    std::unordered_map<SPPath*, CanvasItemPtr<CanvasItemText>> measuring_items;

    sigc::scoped_connection sel_changed_connection;
};

int lpetool_mode_to_index(LivePathEffect::EffectType type);
int lpetool_item_has_construction(SPItem *item);
bool lpetool_try_construction(SPDesktop *desktop, LivePathEffect::EffectType type);
std::pair<Geom::Point, Geom::Point> lpetool_get_limiting_bbox_corners(SPDocument const *document);

} // namespace Inkscape::UI::Tools

inline auto SP_LPETOOL_CONTEXT(Inkscape::UI::Tools::ToolBase *tool) { return dynamic_cast<Inkscape::UI::Tools::LpeTool*>(tool); }

#endif // INKSCAPE_UI_TOOLS_LPE_TOOL_H

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

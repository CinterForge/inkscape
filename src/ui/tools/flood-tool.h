// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_TOOLS_FLOOD_TOOL_H
#define INKSCAPE_UI_TOOLS_FLOOD_TOOL_H

/*
 * Flood fill drawing context
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   John Bintz <jcoswell@coswellproductions.org>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "ui/modifiers.h"
#include "ui/tools/tool-base.h"

namespace Inkscape { class Selection; }

namespace Inkscape::UI::Tools {

class FloodTool : public ToolBase
{
public:
    FloodTool(SPDesktop *desktop);
    ~FloodTool() override;

    SPItem *item = nullptr;

	sigc::connection sel_changed_connection;

    bool root_handler(CanvasEvent const &event) override;
    bool item_handler(SPItem *item, CanvasEvent const &event) override;

    static void set_channels(int channels);

    static std::vector<char const *> const channel_list;
    static std::vector<char const *> const gap_list;

private:
    void selection_changed(Selection *selection);
    void finishItem();

    Modifiers::Modifier *mod_flood_item;
    Modifiers::Modifier *mod_flood_touch_fill;
    Modifiers::Modifier *mod_select_add_to;
};

enum PaintBucketChannels
{
    FLOOD_CHANNELS_RGB,
    FLOOD_CHANNELS_R,
    FLOOD_CHANNELS_G,
    FLOOD_CHANNELS_B,
    FLOOD_CHANNELS_H,
    FLOOD_CHANNELS_S,
    FLOOD_CHANNELS_L,
    FLOOD_CHANNELS_ALPHA
};

} // namespace Inkscape::UI::Tools

#endif // INKSCAPE_UI_TOOLS_FLOOD_TOOL_H

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

// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * A tool for building shapes.
 */
/* Authors:
 *   Martin Owens
 *
 * Copyright (C) 2022 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_TOOLS_BOOLEANS_TOOL_H
#define INKSCAPE_UI_TOOLS_BOOLEANS_TOOL_H

#include <vector>

#include "object/object-set.h"
#include "ui/tools/tool-base.h"

class SPDesktop;
class SPItem;

namespace Inkscape {
class BooleanBuilder;
struct ButtonPressEvent;
struct ButtonReleaseEvent;
struct MotionEvent;
struct KeyPressEvent;

namespace UI {
namespace Tools {

class InteractiveBooleansTool : public ToolBase
{
public:
    InteractiveBooleansTool(SPDesktop *desktop);
    ~InteractiveBooleansTool() override;

    void switching_away(std::string const &new_tool) override;

    // Preferences set
    void set(Preferences::Entry const &val) override;

    // Catch empty selections
    bool is_ready() const override;

    // Event functions
    bool root_handler(CanvasEvent const &event) override;

    void shape_commit();
    void shape_cancel();
    void set_opacity(double opacity = 1.0);
private:
    void update_status();
    void hide_selected_objects(bool hide = true);
    bool should_add(unsigned state) const;

    bool event_button_press_handler(ButtonPressEvent const &event);
    bool event_button_release_handler(ButtonReleaseEvent const &event);
    bool event_motion_handler(MotionEvent const &event);
    bool event_key_press_handler(KeyPressEvent const &event);

    std::unique_ptr<BooleanBuilder> boolean_builder;

    sigc::scoped_connection _sel_modified;
    sigc::scoped_connection _sel_changed;

    bool to_commit = false;
    unsigned _translucency_key;
    std::optional<Geom::Point> last_cursor_position;

    ObjectSet _items_to_manage;
};

} // namespace Tools
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_TOOLS_BOOLEANS_TOOL_H

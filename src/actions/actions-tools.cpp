// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for switching tools.
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include <giomm.h>  // Not <gtkmm.h>! To eventually allow a headless version!
#include <glibmm/i18n.h>

#include "actions-tools.h"
#include "actions-helper.h"

#include "inkscape-application.h"
#include "inkscape-window.h"
#include "message-context.h"

#include "object/box3d.h"
#include "object/sp-ellipse.h"
#include "object/sp-flowtext.h"
#include "object/sp-offset.h"
#include "object/sp-path.h"
#include "object/sp-rect.h"
#include "object/sp-spiral.h"
#include "object/sp-star.h"
#include "object/sp-text.h"
#include "object/sp-marker.h"

#include "ui/dialog/dialog-container.h"
#include "ui/dialog/dialog-manager.h"
#include "ui/dialog/inkscape-preferences.h"
#include "ui/tools/connector-tool.h"
#include "ui/tools/text-tool.h"
#include "ui/tools/tool-data.h"

Glib::ustring
get_active_tool(InkscapeWindow *win)
{
    Glib::ustring state;

    auto action = win->lookup_action("tool-switch");
    if (!action) {
        show_output("get_active_tool: action 'tool-switch' missing!");
        return state;
    }

    auto saction = std::dynamic_pointer_cast<Gio::SimpleAction>(action);
    if (!saction) {
        show_output("get_active_tool: action 'tool-switch' not SimpleAction!");
        return state;
    }

    saction->get_state(state);

    return state;
}

int
get_active_tool_enum(InkscapeWindow *win)
{
    return get_tool_data().at(get_active_tool(win)).tool;
}

void tool_switch(Glib::ustring const &tool, InkscapeWindow *win);

void
set_active_tool(InkscapeWindow *win, Glib::ustring const &tool)
{
    // Seems silly to have a function to just flip argument order... but it's consistent with other
    // external functions.
    tool_switch(tool, win);
}

void
open_tool_preferences(InkscapeWindow *win, Glib::ustring const &tool)
{
    tool_preferences(tool, win);
}

/**
 * Set tool to appropriate one to edit 'item'.
 */
void
set_active_tool(InkscapeWindow *win, SPItem *item, Geom::Point const p)
{
    if (is<SPRect>(item)) {
        tool_switch("Rect", win);
    } else if (is<SPGenericEllipse>(item)) {
        tool_switch("Arc", win);
    } else if (is<SPStar>(item)) {
        tool_switch("Star", win);
    } else if (is<SPBox3D>(item)) {
        tool_switch("3DBox", win);
    } else if (is<SPSpiral>(item)) {
        tool_switch("Spiral", win);
    } else if (is<SPMarker>(item)) {
        tool_switch("Marker", win);
    } else if (is<SPPath>(item)) {
        if (Inkscape::UI::Tools::cc_item_is_connector(item)) {
            tool_switch("Connector", win);
        }
        else {
            tool_switch("Node", win);
        }
    } else if (is<SPText>(item) || is<SPFlowtext>(item))  {
        tool_switch("Text", win);
        SPDesktop* dt = win->get_desktop();
        if (!dt) {
            show_output("set_active_tool: no desktop!");
            return;
        }
        SP_TEXT_CONTEXT(dt->getTool())->placeCursorAt(item, p);
    } else if (is<SPOffset>(item))  {
        tool_switch("Node", win);
    }
}

/**
 * Set display mode. Callback for 'tool-switch' action.
 */
void
tool_switch(Glib::ustring const &tool, InkscapeWindow *win)
{
    auto const &tool_data = get_tool_data();
    // Valid tool?
    auto tool_it = tool_data.find(tool);
    if (tool_it == tool_data.end()) {
        show_output(Glib::ustring("tool-switch: invalid tool name: ") + tool.raw());
        return;
    }

    // Have desktop?
    SPDesktop* dt = win->get_desktop();
    if (!dt) {
        show_output("tool_switch: no desktop!");
        return;
    }

    auto action = win->lookup_action("tool-switch");
    if (!action) {
        show_output("tool-switch: action 'tool-switch' missing!");
        return;
    }

    auto saction = std::dynamic_pointer_cast<Gio::SimpleAction>(action);
    if (!saction) {
        show_output("tool-switch: action 'tool-switch' not SimpleAction!");
        return;
    }

    // Gtk sometimes fires multiple actions at us, including when switch 'away' from
    // an option. So we catch duplications here and don't switch to ourselves.
    Glib::ustring current_tool;
    saction->get_state(current_tool);
    if (current_tool == tool)
        return;

    // Update button states.
    saction->set_enabled(false); // Avoid infinite loop when called by tool_toogle().
    saction->change_state(tool);
    saction->set_enabled(true);


    // Switch to new tool. TODO: Clean this up. This should be one window function.
    // Setting tool via preference path is a bit strange.
    dt->tipsMessageContext()->set(Inkscape::NORMAL_MESSAGE, get_tool_msg().at(tool).c_str());
    dt->setTool(tool_data.at(tool).pref_path);

    if (auto new_tool = dt->getTool()) {
        new_tool->set_last_active_tool(current_tool);
    }
}

/**
 * Open preferences page for tool. Could be turned into actions if need be.
 */
void
tool_preferences(Glib::ustring const &tool, InkscapeWindow *win)
{
    auto const &tool_data = get_tool_data();
    // Valid tool?
    auto tool_it = tool_data.find(tool);
    if (tool_it == tool_data.end()) {
        show_output(Glib::ustring("tool-preferences: invalid tool name: ") + tool.raw());
        return;
    }

    // Have desktop?
    SPDesktop* dt = win->get_desktop();
    if (!dt) {
        show_output("tool-preferences: no desktop!");
        return;
    }

    auto prefs = Inkscape::Preferences::get();
    prefs->setInt("/dialogs/preferences/page", tool_it->second.pref);
    Inkscape::UI::Dialog::DialogContainer* container = dt->getContainer();

    // Create dialog if it doesn't exist (also sets page if dialog not already in opened tab).
    container->new_floating_dialog("Preferences");

    // Find dialog and explicitly set page (in case not set in previous line).
    auto dialog = Inkscape::UI::Dialog::DialogManager::singleton().find_floating_dialog("Preferences");
    if (dialog) {
        auto pref_dialog = dynamic_cast<Inkscape::UI::Dialog::InkscapePreferences *>(dialog);
        if (pref_dialog) {
            pref_dialog->showPage(); // Switch to page indicated in preferences file (set above).
        }
    }
}

/**
 * Toggle between "Selector" and last used tool.
 */
void
tool_toggle(Glib::ustring const &tool, InkscapeWindow *win)
{
    SPDesktop* dt = win->get_desktop();
    if (!dt) {
        show_output("tool_toggle: no desktop!");
        return;
    }

    auto action = win->lookup_action("tool-switch");
    if (!action) {
        show_output("tool_toggle: action 'tool_switch' missing!");
        return;
    }

    auto saction = std::dynamic_pointer_cast<Gio::SimpleAction>(action);
    if (!saction) {
        show_output("tool_toogle: action 'tool_switch' not SimpleAction!");
        return;
    }

    static Glib::ustring old_tool = "Select";

    Glib::ustring current_tool;
    saction->get_state(current_tool);
    if (current_tool == tool) {
        current_tool = old_tool;
    } else {
        old_tool = current_tool;
        current_tool = tool;
    }

    tool_switch(current_tool, win);
}

Glib::ustring get_active_tool(SPDesktop *desktop)
{
    InkscapeWindow* win = desktop->getInkscapeWindow();
    return get_active_tool(win);
}

int get_active_tool_enum(SPDesktop *desktop)
{
    InkscapeWindow* win = desktop->getInkscapeWindow();
    return get_active_tool_enum(win);
}

void set_active_tool(SPDesktop *desktop, Glib::ustring const &tool)
{
    InkscapeWindow* win = desktop->getInkscapeWindow();
    set_active_tool(win, tool);
}

void set_active_tool(SPDesktop *desktop, SPItem *item, Geom::Point const p)
{
    InkscapeWindow* win = desktop->getInkscapeWindow();
    set_active_tool(win, item, p);
}

const Glib::ustring SECTION = NC_("Action Section", "Tool Switch");

std::vector<std::vector<Glib::ustring>> raw_data_tools =
{
    // clang-format off
    {"win.tool-switch('Select')",       N_("Selector Tool"),        SECTION, N_("Select and transform objects")                  },
    {"win.tool-switch('Node')",         N_("Node Tool"),            SECTION, N_("Edit paths by nodes")                           },
    {"win.tool-switch('Booleans')",     N_("Shape Builder Tool"),   SECTION, N_("Build shapes with the Boolean tool")           },

    {"win.tool-switch('Rect')",         N_("Rectangle Tool"),       SECTION, N_("Create rectangles and squares")                 },
    {"win.tool-switch('Arc')",          N_("Ellipse/Arc Tool"),     SECTION, N_("Create circles, ellipses and arcs")             },
    {"win.tool-switch('Star')",         N_("Star/Polygon Tool"),    SECTION, N_("Create stars and polygons")                     },
    {"win.tool-switch('3DBox')",        N_("3D Box Tool"),          SECTION, N_("Create 3D Boxes")                               },
    {"win.tool-switch('Spiral')",       N_("Spiral Tool"),          SECTION, N_("Create spirals")                                },
    {"win.tool-switch('Marker')",       N_("Marker Tool"),          SECTION, N_("Edit markers")                                  },

    {"win.tool-switch('Pen')",          N_("Pen Tool"),             SECTION, N_("Draw Bezier curves and straight lines")         },
    {"win.tool-switch('Pencil')",       N_("Pencil Tool"),          SECTION, N_("Draw freehand lines")                           },
    {"win.tool-switch('Calligraphic')", N_("Calligraphy Tool"),     SECTION, N_("Draw calligraphic or brush strokes")            },
    {"win.tool-switch('Text')",         N_("Text Tool"),            SECTION, N_("Create and edit text objects")                  },

    {"win.tool-switch('Gradient')",     N_("Gradient Tool"),        SECTION, N_("Create and edit gradients")                     },
    {"win.tool-switch('Mesh')",         N_("Mesh Tool"),            SECTION, N_("Create and edit meshes")                        },
    {"win.tool-switch('Dropper')",      N_("Dropper Tool"),         SECTION, N_("Pick colors from image")                        },
    {"win.tool-switch('PaintBucket')",  N_("Paint Bucket Tool"),    SECTION, N_("Fill bounded areas")                            },

    {"win.tool-switch('Tweak')",        N_("Tweak Tool"),           SECTION, N_("Tweak objects by sculpting or painting")        },
    {"win.tool-switch('Spray')",        N_("Spray Tool"),           SECTION, N_("Spray copies or clones of objects")             },
    {"win.tool-switch('Eraser')",       N_("Eraser Tool"),          SECTION, N_("Erase objects or paths")                        },
    {"win.tool-switch('Connector')",    N_("Connector Tool"),       SECTION, N_("Create diagram connectors")                     },
    {"win.tool-switch('LPETool')",      N_("LPE Tool"),             SECTION, N_("Do geometric constructions")                    },

    {"win.tool-switch('Zoom')",         N_("Zoom Tool"),            SECTION, N_("Zoom in or out")                                },
    {"win.tool-switch('Measure')",      N_("Measure Tool"),         SECTION, N_("Measure objects")                               },
    {"win.tool-switch('Pages')",        N_("Pages Tool"),           SECTION, N_("Create and edit document pages")                },

    {"win.tool-toggle('Select')",       N_("Toggle Selector Tool"), SECTION, N_("Toggle between Selector tool and last used tool") },
    {"win.tool-toggle('Dropper')",      N_("Toggle Dropper"),       SECTION, N_("Toggle between Dropper tool and last used tool")},
    // clang-format on
};


void
add_actions_tools(InkscapeWindow* win)
{
    // clang-format off
    win->add_action_radio_string ( "tool-switch",        sigc::bind(sigc::ptr_fun(&tool_switch),  win), "Select");
    win->add_action_radio_string ( "tool-toggle",        sigc::bind(sigc::ptr_fun(&tool_toggle),  win), "Select");
    // clang-format on

    auto app = InkscapeApplication::instance();
    if (!app) {
        show_output("add_actions_tools: no app!");
        return;
    }

    app->get_action_extra_data().add_data(raw_data_tools);
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

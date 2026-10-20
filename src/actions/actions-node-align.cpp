// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for aligning and distributing objects without GUI.
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Some code and ideas from src/ui/dialogs/align-and-distribute.cpp
 *   Authors: Bryce Harrington
 *            Martin Owens
 *            John Smith
 *            Patrick Storz
 *            Jabier Arraiza
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 * To do: Remove GUI dependency!
 */

#include "actions-node-align.h"
#include "actions-helper.h"

#include <iostream>
#include <limits>

#include <giomm.h>  // Not <gtkmm.h>! To eventually allow a headless version!
#include <glibmm/i18n.h>

#include <2geom/coord.h>

#include "desktop.h"
#include "inkscape-application.h"
#include "inkscape-window.h"
#include "ui/tool/node-types.h"
#include "ui/tool/multi-path-manipulator.h" // Node align/distribute
#include "ui/tools/node-tool.h"             // Node align/distribute

using Inkscape::UI::AlignTargetNode;

void
node_align(const Glib::VariantBase& value, InkscapeWindow* win, Geom::Dim2 direction)
{
    auto const tool = win->get_desktop()->getTool();
    auto node_tool = dynamic_cast<Inkscape::UI::Tools::NodeTool*>(tool);
    if (!node_tool) {
        show_output("node_align: tool is not Node tool!");
        return;
    }

    Glib::Variant<Glib::ustring> s = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(value);
    std::vector<Glib::ustring> tokens = Glib::Regex::split_simple(" ", s.get());
    if (tokens.size() > 1) {
        show_output("node_align: too many arguments!");
        return;
    }

    // clang-format off
    auto target = AlignTargetNode::MID_NODE;
    if (tokens.size() == 1) {
        std::string token = tokens[0];
        if (token == "pref") {
            Inkscape::Preferences *prefs = Inkscape::Preferences::get();
            token = prefs->getString("/dialogs/align/nodes-align-to", "first");
        }
        if      (token == "last"   ) target = AlignTargetNode::LAST_NODE;
        else if (token == "first"  ) target = AlignTargetNode::FIRST_NODE;
        else if (token == "middle" ) target = AlignTargetNode::MID_NODE;
        else if (token == "min"    ) target = AlignTargetNode::MIN_NODE;
        else if (token == "max"    ) target = AlignTargetNode::MAX_NODE;
    }
    // clang-format on
    node_tool->_multipath->alignNodes(direction, target);
} 

void
node_distribute(InkscapeWindow* win, Geom::Dim2 direction)
{
    auto const tool = win->get_desktop()->getTool();
    auto node_tool = dynamic_cast<Inkscape::UI::Tools::NodeTool*>(tool);
    if (!node_tool) {
        show_output("node_distribute: tool is not Node tool!");
        return;
    }

    node_tool->_multipath->distributeNodes(direction);
} 

const Glib::ustring SECTION = NC_("Action Section", "Node");

std::vector<std::vector<Glib::ustring>> raw_data_node_align =
{
    // clang-format off
    {"win.node-align-horizontal",       N_("Align nodes horizontally"),      SECTION, N_("Align selected nodes horizontally; usage [last|first|middle|min|max|pref]" )},
    {"win.node-align-vertical",         N_("Align nodes vertically"),        SECTION, N_("Align selected nodes vertically; usage [last|first|middle|min|max|pref]"   )},
    {"win.node-distribute-horizontal",  N_("Distribute nodes horizontally"), SECTION, N_("Distribute selected nodes horizontally"                              )},
    {"win.node-distribute-vertical",    N_("Distribute nodes vertically"),   SECTION, N_("Distribute selected nodes vertically"                                )}
    // clang-format on
};

// These are window actions as the require the node tool to be active and nodes to be selected.
void
add_actions_node_align(InkscapeWindow* win)
{
    Glib::VariantType String(Glib::VARIANT_TYPE_STRING);

    // clang-format off
    win->add_action_with_parameter( "node-align-horizontal",      String, sigc::bind(sigc::ptr_fun(&node_align),      win, Geom::X));
    win->add_action_with_parameter( "node-align-vertical",        String, sigc::bind(sigc::ptr_fun(&node_align),      win, Geom::Y));
    win->add_action(                "node-distribute-horizontal",         sigc::bind(sigc::ptr_fun(&node_distribute), win, Geom::X));
    win->add_action(                "node-distribute-vertical",           sigc::bind(sigc::ptr_fun(&node_distribute), win, Geom::Y));
    // clang-format on

    auto app = InkscapeApplication::instance();
    if (!app) {
        show_output("add_actions_node_align: no app!");
        return;
    }
    app->get_action_extra_data().add_data(raw_data_node_align);
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

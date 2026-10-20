// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for operating on nodes without GUI.
 *
 * Requires nodes to be selected which currently can only be done with node tool.
 *
 * Copyright (C) 2026 Tavmjong Bah
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

#include "actions-node-tool.h"
#include "actions-helper.h"

#include <iostream>
#include <limits>

#include <giomm.h>  // Not <gtkmm.h>! To eventually allow a headless version!
#include <glibmm/i18n.h>

#include <2geom/coord.h>

#include "desktop.h"
#include "inkscape-application.h"
#include "inkscape-window.h"
#include "preferences.h"
#include "ui/tool/node-types.h"
#include "ui/tool/manipulator.h"            // EXTR_MAX_X, etc.
#include "ui/tool/path-manipulator.h"       // NodeDeletMode
#include "ui/tool/multi-path-manipulator.h" // Node insert/deletion/etc.
#include "ui/tools/node-tool.h"             // Node tool access

using Inkscape::UI::PointManipulator;
using Inkscape::UI::SegmentType;

void
node_nodes_insert(InkscapeWindow* win)
{
    auto const tool = win->get_desktop()->getTool();
    auto node_tool = dynamic_cast<Inkscape::UI::Tools::NodeTool*>(tool);
    if (!node_tool) {
        show_output("node_nodes_insert: tool is not Node tool!");
        return;
    }

    node_tool->_multipath->insertNodes();
}

void
node_nodes_insert_extrema(InkscapeWindow* win, PointManipulator::ExtremumType type)
{
    auto const tool = win->get_desktop()->getTool();
    auto node_tool = dynamic_cast<Inkscape::UI::Tools::NodeTool*>(tool);
    if (!node_tool) {
        show_output("node_nodes_insert_extrema: tool is not Node tool!");
        return;
    }

    if (!win->get_desktop()->yaxisdown()) {
        // Flip y coordinate
        if (type == PointManipulator::EXTR_MIN_Y) {
            type =  PointManipulator::EXTR_MAX_Y;
        } else if (type == PointManipulator::EXTR_MAX_Y) {
            type = PointManipulator::EXTR_MIN_Y;
        }
    }

    node_tool->_multipath->insertNodesAtExtrema(type);
}

void
node_nodes_delete(InkscapeWindow* win)
{
    auto const tool = win->get_desktop()->getTool();
    auto node_tool = dynamic_cast<Inkscape::UI::Tools::NodeTool*>(tool);
    if (!node_tool) {
        show_output("node_nodes_delete: tool is not Node tool!");
        return;
    }

    auto prefs = Preferences::get();
    node_tool->_multipath->deleteNodes((Inkscape::UI::NodeDeleteMode)prefs->getInt("/tools/node/delete-mode-default", (int)Inkscape::UI::NodeDeleteMode::automatic));
}

void node_simplify(InkscapeWindow* win)
{
    auto const tool = win->get_desktop()->getTool();
    auto node_tool = dynamic_cast<Inkscape::UI::Tools::NodeTool*>(tool);
    if (!node_tool) {
        show_output("node_simplify: tool is not Node tool!");
        return;
    }

    double epsilon = 2.0 / win->get_desktop()->current_zoom();
    node_tool->_multipath->simplifyInvisible(epsilon);
}

void
node_nodes_join(InkscapeWindow* win)
{
    auto const tool = win->get_desktop()->getTool();
    auto node_tool = dynamic_cast<Inkscape::UI::Tools::NodeTool*>(tool);
    if (!node_tool) {
        show_output("node_nodes_join: tool is not Node tool!");
        return;
    }

    node_tool->_multipath->joinNodes();
}

void
node_nodes_break(InkscapeWindow* win)
{
    auto const tool = win->get_desktop()->getTool();
    auto node_tool = dynamic_cast<Inkscape::UI::Tools::NodeTool*>(tool);
    if (!node_tool) {
        show_output("node_nodes_break: tool is not Node tool!");
        return;
    }

    node_tool->_multipath->breakNodes();
}

void
node_segments_join(InkscapeWindow* win)
{
    auto const tool = win->get_desktop()->getTool();
    auto node_tool = dynamic_cast<Inkscape::UI::Tools::NodeTool*>(tool);
    if (!node_tool) {
        show_output("node_segments_join: tool is not Node tool!");
        return;
    }

    node_tool->_multipath->joinSegments();
}

void
node_segments_delete(InkscapeWindow* win)
{
    auto const tool = win->get_desktop()->getTool();
    auto node_tool = dynamic_cast<Inkscape::UI::Tools::NodeTool*>(tool);
    if (!node_tool) {
        show_output("node_segements_delete: tool is not Node tool!");
        return;
    }

    node_tool->_multipath->deleteSegments();
}

void
node_set_node_type(InkscapeWindow* win, Inkscape::UI::NodeType type)
{
    auto const tool = win->get_desktop()->getTool();
    auto node_tool = dynamic_cast<Inkscape::UI::Tools::NodeTool*>(tool);
    if (!node_tool) {
        show_output("node_set_node_type: tool is not Node tool!");
        return;
    }

    node_tool->_multipath->setNodeType(type);
}

void
node_set_segment_type(InkscapeWindow* win, SegmentType type)
{
    auto const tool = win->get_desktop()->getTool();
    auto node_tool = dynamic_cast<Inkscape::UI::Tools::NodeTool*>(tool);
    if (!node_tool) {
        show_output("node_set_segment_type: tool is not Node tool!");
        return;
    }

    node_tool->_multipath->setSegmentType(type);
}

const Glib::ustring SECTION = NC_("Action Section", "Node");

std::vector<std::vector<Glib::ustring>> raw_data_node_tool =
{
    // clang-format off
    {"win.node-nodes-insert",            N_("Nodes insert"),                   SECTION, N_("Insert nodes in selected segments"                       )},
    {"win.node-nodes-delete",            N_("Nodes delete"),                   SECTION, N_("Delete selected nodes"                                   )},

    {"win.node-nodes-join",              N_("Nodes join"),                     SECTION, N_("Join selected nodes"                                     )},
    {"win.node-nodes-break",             N_("Nodes break"),                    SECTION, N_("Break path between selected nodes"                       )},
    {"win.node-segments-join",           N_("Segments join"),                  SECTION, N_("Join selected end nodes with a new segment"              )},
    {"win.node-segments-delete",         N_("Segments break"),                 SECTION, N_("Delete segment between two non-end nodes"                )},
    {"win.node-simplify",                N_("Simplify path"),                  SECTION, N_("Join selected nodes which are very close together"       )},

    {"win.node-nodes-to-cusp",           N_("Nodes to cusp"),                  SECTION, N_("Convert selected nodes to corner nodes"                  )},
    {"win.node-nodes-to-smooth",         N_("Nodes to smooth"),                SECTION, N_("Convert selected nodes to smooth nodes"                  )},
    {"win.node-nodes-to-symmetric",      N_("Nodes to symmetrical"),           SECTION, N_("Convert selected nodes to symmetrical nodes"             )},
    {"win.node-nodes-to-auto",           N_("Nodes to auto"),                  SECTION, N_("Convert selected nodes to auto smooth nodes"             )},

    {"win.node-segments-to-curves",      N_("Segments to curves"),             SECTION, N_("Convert selected path segments to curves"                )},
    {"win.node-segments-to-lines",       N_("Segments to lines"),              SECTION, N_("Convert selected path segments to lines"                 )}
    // clang-format on
};

// These are window actions as the require the node tool to be active and nodes to be selected.
void
add_actions_node_tool(InkscapeWindow* win)
{
    Glib::VariantType String(Glib::VARIANT_TYPE_STRING);

    // clang-format off
    win->add_action(   "node-nodes-insert",         sigc::bind(sigc::ptr_fun(&node_nodes_insert),         win)                                     );
    win->add_action(   "node-nodes-delete",         sigc::bind(sigc::ptr_fun(&node_nodes_delete),         win)                                     );

    win->add_action(   "node-nodes-insert-left",    sigc::bind(sigc::ptr_fun(&node_nodes_insert_extrema), win, PointManipulator::EXTR_MIN_X)       );
    win->add_action(   "node-nodes-insert-right",   sigc::bind(sigc::ptr_fun(&node_nodes_insert_extrema), win, PointManipulator::EXTR_MAX_X)       );
    win->add_action(   "node-nodes-insert-top",     sigc::bind(sigc::ptr_fun(&node_nodes_insert_extrema), win, PointManipulator::EXTR_MIN_Y)       );
    win->add_action(   "node-nodes-insert-bottom",  sigc::bind(sigc::ptr_fun(&node_nodes_insert_extrema), win, PointManipulator::EXTR_MAX_Y)       );

    win->add_action(   "node-nodes-join",           sigc::bind(sigc::ptr_fun(&node_nodes_join),           win)                                     );
    win->add_action(   "node-nodes-break",          sigc::bind(sigc::ptr_fun(&node_nodes_break),          win)                                     );
    win->add_action(   "node-segments-join",        sigc::bind(sigc::ptr_fun(&node_segments_join),        win)                                     );
    win->add_action(   "node-segments-delete",      sigc::bind(sigc::ptr_fun(&node_segments_delete),      win)                                     );
    win->add_action(   "node-simplify",             sigc::bind(sigc::ptr_fun(&node_simplify),             win)                                     );

    win->add_action(   "node-nodes-to-cusp",        sigc::bind(sigc::ptr_fun(&node_set_node_type),        win, Inkscape::UI::NODE_CUSP)            );
    win->add_action(   "node-nodes-to-smooth",      sigc::bind(sigc::ptr_fun(&node_set_node_type),        win, Inkscape::UI::NODE_SMOOTH)          );
    win->add_action(   "node-nodes-to-symmetric",   sigc::bind(sigc::ptr_fun(&node_set_node_type),        win, Inkscape::UI::NODE_SYMMETRIC)       );
    win->add_action(   "node-nodes-to-auto",        sigc::bind(sigc::ptr_fun(&node_set_node_type),        win, Inkscape::UI::NODE_AUTO)            );

    win->add_action(   "node-segments-to-lines",    sigc::bind(sigc::ptr_fun(&node_set_segment_type),     win, Inkscape::UI::SEGMENT_STRAIGHT)     );
    win->add_action(   "node-segments-to-curves",   sigc::bind(sigc::ptr_fun(&node_set_segment_type),     win, Inkscape::UI::SEGMENT_CUBIC_BEZIER) );
    // clang-format on

    auto app = InkscapeApplication::instance();
    if (!app) {
        show_output("add_actions_node_tool: no app!");
        return;
    }
    app->get_action_extra_data().add_data(raw_data_node_tool);
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

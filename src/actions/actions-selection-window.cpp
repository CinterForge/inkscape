// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 *
 * Actions related to selection which require desktop.
 *
 * These are linked to the desktop as they operate differently
 * depending on if the node tool is in use or not.
 *
 * To do: Rewrite select_same_fill_and_stroke to remove desktop dependency.
 *
 * Authors:
 *   Sushant A A <sushant.co19@gmail.com>
 *
 * Copyright (C) 2021 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <giomm.h>
#include <glibmm/i18n.h>

#include "actions-selection-window.h"
#include "actions-helper.h"
#include "inkscape-application.h"
#include "inkscape-window.h"
#include "desktop.h"
#include "ui/dialog/dialog-container.h"
#include "actions/actions-tools.h"
#include "selection-chemistry.h"

void
select_all(InkscapeWindow* win)
{
    SPDesktop* dt = win->get_desktop();

    // Select All
    Inkscape::SelectionHelper::selectAll(dt);
}

void
select_all_layers(InkscapeWindow* win)
{
    SPDesktop* dt = win->get_desktop();

    // Select All in All Layers
    Inkscape::SelectionHelper::selectAllInAll(dt);
}

void
select_same_fill_and_stroke(InkscapeWindow* win)
{
    SPDesktop* dt = win->get_desktop();

    // Fill and Stroke
    Inkscape::SelectionHelper::selectSameFillStroke(dt);
}

void
select_same_fill(InkscapeWindow* win)
{
    SPDesktop* dt = win->get_desktop();

    // Fill Color
    Inkscape::SelectionHelper::selectSameFillColor(dt);
}

void
select_same_stroke_color(InkscapeWindow* win)
{
    SPDesktop* dt = win->get_desktop();

    // Stroke Color
    Inkscape::SelectionHelper::selectSameStrokeColor(dt);
}

void
select_same_stroke_style(InkscapeWindow* win)
{
    SPDesktop* dt = win->get_desktop();

    // Stroke Style
    Inkscape::SelectionHelper::selectSameStrokeStyle(dt);
}

void
select_same_object_type(InkscapeWindow* win)
{
    SPDesktop* dt = win->get_desktop();

    // Object Type
    Inkscape::SelectionHelper::selectSameObjectType(dt);
}

void
select_invert(InkscapeWindow* win)
{
    SPDesktop* dt = win->get_desktop();

    // Invert Selection
    Inkscape::SelectionHelper::invert(dt);
}

void
select_invert_all(InkscapeWindow* win)
{
    SPDesktop* dt = win->get_desktop();

    // Invert Selection
    Inkscape::SelectionHelper::invertAllInAll(dt);
}

void
select_none(InkscapeWindow* win)
{
    SPDesktop* dt = win->get_desktop();

    // Deselect
    Inkscape::SelectionHelper::selectNone(dt);
}

const Glib::ustring SECTION = NC_("Action Section", "Select");

std::vector<std::vector<Glib::ustring>> raw_selection_dekstop_data =
{
    // clang-format off
    {"win.select-all",                   N_("Select All"),                  SECTION, N_("Select all objects or all nodes")},
    {"win.select-all-layers",            N_("Select All in All Layers"),    SECTION, N_("Select all objects in all visible and unlocked layers")},
    {"win.select-same-fill-and-stroke",  N_("Fill and Stroke"),             SECTION, N_("Select all objects with the same fill and stroke as the selected objects")},
    {"win.select-same-fill",             N_("Fill Color"),                  SECTION, N_("Select all objects with the same fill as the selected objects")},
    {"win.select-same-stroke-color",     N_("Stroke Color"),                SECTION, N_("Select all objects with the same stroke as the selected objects")},
    {"win.select-same-stroke-style",     N_("Stroke Style"),                SECTION, N_("Select all objects with the same stroke style (width, dash, markers) as the selected objects")},
    {"win.select-same-object-type",      N_("Object Type"),                 SECTION, N_("Select all objects with the same object type (rect, arc, text, path, bitmap etc) as the selected objects")},
    {"win.select-invert",                N_("Invert Selection"),            SECTION, N_("Invert selection (unselect what is selected and select everything else)")},
    {"win.select-invert-all",            N_("Invert in All Layers"),        SECTION, N_("Invert selection in all visible and unlocked layers")},
    {"win.select-none",                  N_("Deselect"),                    SECTION, N_("Deselect any selected objects or nodes")},
    // DO NOT ADD select-next or select-previous here as their default keys conflict with Gtk's widget navigation.

    // clang-format on
};

void
add_actions_select_window(InkscapeWindow* win)
{
    // clang-format off
    win->add_action( "select-all",                      sigc::bind(sigc::ptr_fun(&select_all), win));
    win->add_action( "select-all-layers",               sigc::bind(sigc::ptr_fun(&select_all_layers), win));
    win->add_action( "select-same-fill-and-stroke",     sigc::bind(sigc::ptr_fun(&select_same_fill_and_stroke), win));
    win->add_action( "select-same-fill",                sigc::bind(sigc::ptr_fun(&select_same_fill), win));
    win->add_action( "select-same-stroke-color",        sigc::bind(sigc::ptr_fun(&select_same_stroke_color), win));
    win->add_action( "select-same-stroke-style",        sigc::bind(sigc::ptr_fun(&select_same_stroke_style), win));
    win->add_action( "select-same-object-type",         sigc::bind(sigc::ptr_fun(&select_same_object_type), win));
    win->add_action( "select-invert",                   sigc::bind(sigc::ptr_fun(&select_invert), win));
    win->add_action( "select-invert-all",               sigc::bind(sigc::ptr_fun(&select_invert_all), win));
    win->add_action( "select-none",                     sigc::bind(sigc::ptr_fun(&select_none), win));
    // clang-format on

    auto app = InkscapeApplication::instance();
    if (!app) {
        show_output("add_actions_edit: no app!");
        return;
    }
    app->get_action_extra_data().add_data(raw_selection_dekstop_data);
}

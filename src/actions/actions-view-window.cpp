// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 *
 * Gio::Actions for window handling that are not useful from the command line (thus tied to window map).
 * Found under the "View" menu.
 *
 * Authors:
 *   Sushant A A <sushant.co19@gmail.com>
 *
 * Copyright (C) 2021 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "actions-view-window.h"
#include "actions-helper.h"

#include <giomm.h>
#include <glibmm/i18n.h>

#include "inkscape-application.h"
#include "inkscape-window.h"
#include "inkscape.h"      // previous/next window
#include "actions/actions-extra-data.h"
#include "ui/widget/desktop-widget.h"

namespace {

void window_previous(InkscapeWindow *win)
{
    INKSCAPE.switch_desktops_prev();
}

void window_next(InkscapeWindow *win)
{
    INKSCAPE.switch_desktops_next();
}

void window_new(InkscapeWindow *win)
{
    auto app = InkscapeApplication::instance();
    auto doc = app->get_active_document();
    if (!doc) {
        return;
    }
    app->desktopOpen(doc);
}

void tab_previous(InkscapeWindow *win)
{
    win->get_desktop_widget()->advanceTab(-1);
}

void tab_next(InkscapeWindow *win)
{
    win->get_desktop_widget()->advanceTab(1);
}

const Glib::ustring SECTION = NC_("Action Section", "View");

auto const raw_data_view_window = std::vector<std::vector<Glib::ustring>>
{
    // clang-format off
    {"win.window-new",      N_("Duplicate Window"), SECTION, N_("Open a new window with the same document")},
    {"win.window-previous", N_("Previous Window"),  SECTION, N_("Switch to the previous document window")},
    {"win.window-next",     N_("Next Window"),      SECTION, N_("Switch to the next document window")},
    {"win.tab-next",        N_("Next Tab"),         SECTION, N_("Switch to the next document tab")},
    {"win.tab-previous",    N_("Previous Tab"),     SECTION, N_("Switch to the previous document tab")},
    // clang-format on
};

} // namespace

void add_actions_view_window(InkscapeWindow* win)
{
    // clang-format off
    win->add_action("window-new",                  sigc::bind(sigc::ptr_fun(&window_new),       win));
    win->add_action("window-previous",             sigc::bind(sigc::ptr_fun(&window_previous),  win));
    win->add_action("window-next",                 sigc::bind(sigc::ptr_fun(&window_next),      win));
    win->add_action("tab-next",                    sigc::bind(sigc::ptr_fun(&tab_next),         win));
    win->add_action("tab-previous",                sigc::bind(sigc::ptr_fun(&tab_previous),     win));
    // clang-format on

    // Check if there is already an application instance (GUI or non-GUI).
    auto app = InkscapeApplication::instance();
    if (!app) {
        show_output("add_actions_view_window: no app!");
        return;
    }
    app->get_action_extra_data().add_data(raw_data_view_window);
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

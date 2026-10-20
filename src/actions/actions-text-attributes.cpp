// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 *
 * Actions Related to Text Attributes (decoration, etc.)
 *
 * Authors:
 *   Tavmjong Bah
 *
 * Copyright (C) 2026 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "actions-text-attributes.h"

#include <vector>

#include <giomm.h>
#include <glibmm/i18n.h>

#include "inkscape-application.h"
#include "inkscape-window.h"

#include "actions-helper.h"
#include "desktop.h"
#include "desktop-style.h"
#include "document-undo.h"
#include "style.h"

#include "ui/icon-names.h"
#include "util-string/context-string.h"
#include "xml/sp-css-attr.h" // SPCSSAttr
#include "xml/repr.h"        // sp_repr_css_xxxx

const Glib::ustring SECTION = NC_("Action Section", "Text");

std::vector<Glib::ustring> text_decoration_line_values
{
    "underline",   
    "line-through",
    "overline"
};

/*
 * Set text-decoration for current selection.
 * Eventually to be tied to an App level action for use from command line (requires validating string).
 * Setting style needs to be removed from SPDesktop.
 */
static void
text_decoration_set_css(InkscapeApplication *app, Glib::ustring attribute, Glib::ustring value)
{
    auto desktop = app->get_active_desktop();
    if (!desktop) {
        return;
    }

    SPCSSAttr *css = sp_repr_css_attr_new();
    if (!value.empty()) {
        sp_repr_css_set_property (css, attribute.c_str(), value.c_str());
    } else {
        sp_repr_css_set_property (css, attribute.c_str(), "none");
    }

    sp_desktop_set_style(desktop, css, true, false);

    // Save for undo
    DocumentUndo::maybeDone(desktop->getDocument(), "text-decoration", RC_("Undo", "Text: Change text-decoration"), INKSCAPE_ICON("draw-text"));
}

void
text_decoration_line_set(InkscapeWindow *win, SPStyle* query)
{
    for (auto value : text_decoration_line_values) {
        Glib::ustring action_name = "text-decoration-line-" + value;
        auto action = win->lookup_action(action_name);
        if (!action) {
            show_output(action_name + ": action missing!");
            continue;
        }

        auto saction = std::dynamic_pointer_cast<Gio::SimpleAction>(action);
        if (!saction) {
            show_output(action_name + ": action not SimpleAction!");
            continue;
        }

        if (value == "underline")    saction->change_state(query->text_decoration_line.underline);
        if (value == "line-through") saction->change_state(query->text_decoration_line.line_through);
        if (value == "overline")     saction->change_state(query->text_decoration_line.overline);
    }
}

/*
 * Window level action tied to buttons.
 */
void
text_decoration_line(InkscapeWindow *win, const int& position)
{
    Glib::ustring out;
    int pos = 0;
    for (auto value : text_decoration_line_values) {
        Glib::ustring action_name = "text-decoration-line-" + value;
        auto action = win->lookup_action(action_name);
        if (!action) {
            show_output(action_name + ": action missing!");
            ++pos;
            continue;
        }

        auto saction = std::dynamic_pointer_cast<Gio::SimpleAction>(action);
        if (!saction) {
            show_output(action_name + ": action not SimpleAction!");
            ++pos;
            continue;
        }

        bool state = false;
        saction->get_state(state);

        if (pos == position) {
            state = !state;
            saction->change_state(state);
        }

        if (state) {
            out += text_decoration_line_values[pos] + " ";
        }

        ++pos;
    }

    if (!out.empty()) {
        out.erase(--out.end());
    }

    auto app = InkscapeApplication::instance();
    text_decoration_set_css(app, "text-decoration-line", out);
}

std::vector<std::vector<Glib::ustring>> raw_data_text_attributes =
{
    // clang-format off
    {"win.text-decoration-line-underline",             N_("Text Underline"),             SECTION, N_("Toggle text underline")    },
    {"win.text-decoration-line-line-through",          N_("Text Strikethrough"),         SECTION, N_("Toggle text strikethrough")},
    {"win.text-decoration-line-overline",              N_("Text Overline"),              SECTION, N_("Toggle text overline")     },
    // clang-format on
};

void
add_actions_text_attributes(InkscapeWindow* win)
{
    // clang-format off
    win->add_action_bool( "text-decoration-line-underline",    sigc::bind(sigc::ptr_fun(&text_decoration_line),   win, 0), false);
    win->add_action_bool( "text-decoration-line-line-through", sigc::bind(sigc::ptr_fun(&text_decoration_line),   win, 1), false);
    win->add_action_bool( "text-decoration-line-overline",     sigc::bind(sigc::ptr_fun(&text_decoration_line),   win, 2), false);
    // clang-format on

    auto app = InkscapeApplication::instance();
    if (!app) {
        show_output("add_actions_text_attributes: no app!");
        return;
    }
    app->get_action_extra_data().add_data(raw_data_text_attributes);
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

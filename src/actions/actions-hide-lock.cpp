// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 *
 * Actions related to hide and lock
 *
 * Authors:
 *   Sushant A A <sushant.co19@gmail.com>
 *   Tavmjong Bah
 *
 * Copyright (C) 2021-2022 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "actions-hide-lock.h"
#include "actions-helper.h"

#include <giomm.h>  // Not <gtkmm.h>! To eventually allow a headless version!
#include <glibmm/i18n.h>

#include "inkscape-application.h"
#include "document.h"
#include "document-undo.h"
#include "selection.h"

#include "object/sp-root.h"

// Helper to unlock/unhide everything. (Could also be used to lock/hide everything but that isn't very useful.)
static bool
hide_lock_recurse(bool (*f)(SPItem*, bool), SPItem *item, bool hide_or_lock)
{
    bool changed = false;

    if (f(item, hide_or_lock)) {
        changed = true;
    }

    for (auto& child : item->children) {
        auto item = cast<SPItem>(&child);
        if (item && hide_lock_recurse(f, item, hide_or_lock)) {
            changed = true;
        }
    }

    return changed;
}

// Helper to hide/unhide one item.
bool
hide_lock_hide(SPItem* item, bool hide)
{
    bool changed = false;
    if (item->isHidden() != hide) {
        item->setHidden(hide);
        changed = true;
    }
    return changed;
}

// Helper to lock/unlock one item.
bool
hide_lock_lock(SPItem* item, bool lock)
{
    bool changed = false;
    if (item->isLocked() != lock) {
        item->setLocked(lock);
        changed = true;
    }
    return changed;
}

// Unhide all
void
hide_lock_unhide_all(InkscapeApplication* app)
{
    auto document = app->get_active_document();
    auto root = document->getRoot();

    bool changed = hide_lock_recurse(&hide_lock_hide, root, false); // Unhide

    if (changed) {
        Inkscape::DocumentUndo::done(document, RC_("Undo", "Unhid all objects in the current layer"), "");
    }
}

// Unlock all
void
hide_lock_unlock_all(InkscapeApplication* app)
{
    auto document = app->get_active_document();
    auto root = document->getRoot();

    bool changed = hide_lock_recurse(&hide_lock_lock, root, false); // Unlock

    if (changed) {
        Inkscape::DocumentUndo::done(document, RC_("Undo", "Unlocked all objects in the current layer"), "");
    }
}

// Unhide selected items and their descendents.
void
hide_lock_unhide_below(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();
    if (!selection) {
        show_output("hide_lock_unhide_below: no selection!");
        return;
    }

    bool changed = false;
    for (auto item : selection->items()) {
        if (hide_lock_recurse(&hide_lock_hide, item, false)) {
            changed = true;
        }
    }

    if (changed) {
        auto document = app->get_active_document();
        Inkscape::DocumentUndo::done(document, RC_("Undo", "Unhid selected items and their descendents."), "");
    }
}

// Unlock selected items and their descendents.
void
hide_lock_unlock_below(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();
    if (!selection) {
        show_output("hide_lock_unhide_below: no selection!");
        return;
    }

    bool changed = false;
    for (auto item : selection->items()) {
        if (hide_lock_recurse(&hide_lock_lock, item, false)) {
            changed = true;
        }
    }

    if (changed) {
        auto document = app->get_active_document();
        Inkscape::DocumentUndo::done(document, RC_("Undo", "Unlocked selected items and their descendents."), "");
    }
}

// Hide/unhide selected items.
void
hide_lock_hide_selected(InkscapeApplication* app, bool hide)
{
    auto selection = app->get_active_selection();
    if (!selection) {
        show_output("hide_lock_hide_selected: no selection!");
        return;
    }

    bool changed = false;
    for (auto item : selection->items()) {
        if (hide_lock_hide(item, hide)) {
            changed = true;
        }
    }

    if (changed) {
        auto document = app->get_active_document();
        Inkscape::DocumentUndo::done(document, (hide ? RC_("Undo", "Hid selected items.") : RC_("Undo", "Unhid selected items.")), "");
        selection->clear();
    }
}

// Lock/Unlock selected items.
void
hide_lock_lock_selected(InkscapeApplication* app, bool lock)
{
    auto selection = app->get_active_selection();
    if (!selection) {
        show_output("hide_lock_lock_selected: no selection!");
        return;
    }

    bool changed = false;
    for (auto item : selection->items()) {
        if (hide_lock_lock(item, lock)) {
            changed = true;
        }
    }

    if (changed) {
        auto document = app->get_active_document();
        Inkscape::DocumentUndo::done(document, (lock ? RC_("Undo", "Locked selected items.") : RC_("Undo", "Unlocked selected items.")), "");
        selection->clear();
    }
}

const Glib::ustring SECTION = NC_("Action Section", "Hide and Lock");

std::vector<std::vector<Glib::ustring>> raw_data_hide_lock =
{
    // clang-format off
    {"app.unhide-all",              N_("Unhide All"),         SECTION, N_("Unhide all objects")                           },
    {"app.unlock-all",              N_("Unlock All"),         SECTION, N_("Unlock all objects")                           },

    {"app.selection-hide",          N_("Hide selection"),     SECTION, N_("Hide all selected objects")                    },
    {"app.selection-unhide",        N_("Unhide selection"),   SECTION, N_("Unhide all selected objects")                  },
    {"app.selection-unhide-below",  N_("Unhide descendents"), SECTION, N_("Unhide all items inside selected objects")     },

    {"app.selection-lock",          N_("Lock selection"),     SECTION, N_("Lock all selected objects")                    },
    {"app.selection-unlock",        N_("Unlock selection"),   SECTION, N_("Unlock all selected objects")                  },
    {"app.selection-unlock-below",  N_("Unlock descendents"), SECTION, N_("Unlock all items inside selected objects")     },
    // clang-format on
};

void
add_actions_hide_lock(InkscapeApplication* app)
{
    auto *gapp = app->gio_app();

    // clang-format off
    gapp->add_action( "unhide-all",             sigc::bind(      sigc::ptr_fun(&hide_lock_unhide_all),    app));
    gapp->add_action( "unlock-all",             sigc::bind(      sigc::ptr_fun(&hide_lock_unlock_all),    app));

    gapp->add_action( "selection-hide",         sigc::bind(sigc::ptr_fun(&hide_lock_hide_selected), app, true ));
    gapp->add_action( "selection-unhide",       sigc::bind(sigc::ptr_fun(&hide_lock_hide_selected), app, false));
    gapp->add_action( "selection-unhide-below", sigc::bind(      sigc::ptr_fun(&hide_lock_unhide_below),  app));

    gapp->add_action( "selection-lock",         sigc::bind(sigc::ptr_fun(&hide_lock_lock_selected), app, true ));
    gapp->add_action( "selection-unlock",       sigc::bind(sigc::ptr_fun(&hide_lock_lock_selected), app, false));
    gapp->add_action( "selection-unlock-below", sigc::bind(      sigc::ptr_fun(&hide_lock_unlock_below),  app));
    // clang-format on

    app->get_action_extra_data().add_data(raw_data_hide_lock);
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

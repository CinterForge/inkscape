// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for to change selection, tied to the application and without GUI.
 *
 * Copyright (C) 2018 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include <iostream>

#include <giomm.h>  // Not <gtkmm.h>! To eventually allow a headless version!
#include <glibmm/i18n.h>

#include "actions-selection.h"
#include "actions-helper.h"

#include "document.h"
#include "inkscape-application.h"
#include "selection.h"            // Selection

#include "object/sp-root.h"       // select_all: document->getRoot();
#include "object/sp-item-group.h" // select_all

void
select_clear(InkscapeApplication* app)
{
    SPDocument* document = nullptr;
    Inkscape::Selection* selection = nullptr;
    if (!get_document_and_selection(app, &document, &selection)) {
        return;
    }
    selection->clear();
}

void
select_by_id(Glib::ustring ids, InkscapeApplication* app)
{
    SPDocument* document = nullptr;
    Inkscape::Selection* selection = nullptr;
    if (!get_document_and_selection(app, &document, &selection)) {
        return;
    }

    auto tokens = Glib::Regex::split_simple("\\s*,\\s*", ids);
    for (auto id : tokens) {
        SPObject* object = document->getObjectById(id);
        if (object) {
            selection->add(object);
        } else {
            show_output(Glib::ustring("select_by_id: Did not find object with id: ") + id.raw());
        }
    }
}

void
unselect_by_id(Glib::ustring ids, InkscapeApplication* app)
{
    SPDocument* document = nullptr;
    Inkscape::Selection* selection = nullptr;
    if (!get_document_and_selection(app, &document, &selection)) {
        return;
    }

    auto tokens = Glib::Regex::split_simple("\\s*,\\s*", ids);
    for (auto id : tokens) {
        SPObject* object = document->getObjectById(id);
        if (object) {
            selection->remove(object);
        } else {
            show_output(Glib::ustring("unselect_by_id: Did not find object with id: ") + id.raw());
        }
    }
}

void
select_by_class(Glib::ustring klass, InkscapeApplication* app)
{
    SPDocument* document = nullptr;
    Inkscape::Selection* selection = nullptr;
    if (!get_document_and_selection(app, &document, &selection)) {
        return;
    }

    auto objects = document->getObjectsByClass(klass);
    selection->add(objects.begin(), objects.end());
}

void
select_by_element(Glib::ustring element, InkscapeApplication* app)
{
    SPDocument* document = nullptr;
    Inkscape::Selection* selection = nullptr;
    if (!get_document_and_selection(app, &document, &selection)) {
        return;
    }
    auto objects = document->getObjectsByElement(element);
    selection->add(objects.begin(), objects.end());
}

void
select_by_selector(Glib::ustring selector, InkscapeApplication* app)
{
    SPDocument* document = nullptr;
    Inkscape::Selection* selection = nullptr;
    if (!get_document_and_selection(app, &document, &selection)) {
        return;
    }

    auto objects = document->getObjectsBySelector(selector);
    selection->add(objects.begin(), objects.end());
}


// Helper
void
get_all_items_recursive(std::vector<SPObject *> &objects, SPObject *object, Glib::ustring &condition)
{
    for (auto &o : object->childList(false)) {
        if (is<SPItem>(o)) {
            auto group = cast<SPGroup>(o);
            if (condition == "layers") {
                if (group && group->layerMode() == SPGroup::LAYER) {
                    objects.emplace_back(o);
                    continue; // Layers cannot contain layers.
                }
            } else if (condition == "no-layers") {
                if (group && group->layerMode() == SPGroup::LAYER) {
                    // recurse one level
                } else {
                    objects.emplace_back(o);
                    continue;
                }
            } else if (condition == "groups") {
                if (group) {
                    objects.emplace_back(o);
                }
            } else if (condition == "all") {
                objects.emplace_back(o);
            } else {
                // no-groups, default
                if (!group) {
                    objects.emplace_back(o);
                    continue; // Non-groups cannot contain items.
                }
            }
            get_all_items_recursive(objects, o, condition);
        }
    }
}


/*
 * 'layers':            All layers.
 * 'groups':            All groups (including layers).
 * 'no-layers':         All top level objects in all layers (matches GUI "Select All in All Layers").
 * 'no-groups':         All objects other than groups (and layers).
 * 'all':               All objects including groups and their descendents.
 *
 * Note: GUI "Select All" requires knowledge of selected layer, which is a desktop property.
 */
void
select_all(Glib::ustring condition, InkscapeApplication* app)
{
    if (condition != "" && condition != "layers" && condition != "no-layers" &&
        condition != "groups" && condition != "no-groups" && condition != "all") {
        show_output( "select_all: allowed options are '', 'all', 'layers', 'no-layers', 'groups', and 'no-groups'" );
        return;
    }

    SPDocument* document = nullptr;
    Inkscape::Selection* selection = nullptr;
    if (!get_document_and_selection(app, &document, &selection)) {
        return;
    }

    std::vector<SPObject *> objects;
    get_all_items_recursive(objects, document->getRoot(), condition);

    selection->setList(objects);
}

void 
select_invert(Glib::ustring condition, InkscapeApplication* app)
{
    if (condition != "" && condition != "layers" && condition != "no-layers" &&
        condition != "groups" && condition != "no-groups" && condition != "all") {
        show_output( "select_all: allowed options are '', 'all', 'layers', 'no-layers', 'groups', and 'no-groups'" );
        return;
    }

    SPDocument* document = nullptr;
    Inkscape::Selection* selection = nullptr;
    if (!get_document_and_selection(app, &document, &selection)) {
        return;
    }

    // Find all objects that match condition.
    std::vector<SPObject *> objects;
    get_all_items_recursive(objects, document->getRoot(), condition);

    // Get current selection.
    auto current = selection->items_vector();

    // Remove current selection from object vector.
    std::erase_if(objects, [&current] (SPObject const *x) {
        return std::find(current.begin(), current.end(), x) != current.end();
    });

    // Set selection to object vector.
    selection->setList(objects);
}

// Debug... print selected items
void
select_list(InkscapeApplication* app)
{
    SPDocument* document = nullptr;
    Inkscape::Selection* selection = nullptr;
    if (!get_document_and_selection(app, &document, &selection)) {
        return;
    }

    for (auto obj : selection->objects()) {
        std::stringstream buffer;
        buffer << *obj;
        show_output(buffer.str(), false);
    }
}

const Glib::ustring SECTION = NC_("Action Section", "Select");

std::vector<std::vector<Glib::ustring>> raw_data_selection =
{
    // clang-format offs
    {"app.select-clear",             N_("Clear Selection"),          SECTION, N_("Clear selection")},
    {"app.select",                   N_("Select"),                   SECTION, N_("Select by ID (deprecated)")},
    {"app.unselect",                 N_("Deselect"),                 SECTION, N_("Deselect by ID (deprecated)")},
    {"app.select-by-id",             N_("Select by ID"),             SECTION, N_("Select by ID")},
    {"app.unselect-by-id",           N_("Deselect by ID"),           SECTION, N_("Deselect by ID")},
    {"app.select-by-class",          N_("Select by Class"),          SECTION, N_("Select by class")},
    {"app.select-by-element",        N_("Select by Element"),        SECTION, N_("Select by SVG element (e.g. 'rect')")},
    {"app.select-by-selector",       N_("Select by Selector"),       SECTION, N_("Select by CSS selector")},
    {"app.select-all",               N_("Select All Objects"),       SECTION, N_("Select all; options: 'all' (every object including groups), 'layers', 'no-layers' (top level objects in layers), 'groups' (all groups including layers), 'no-groups' (all objects other than groups and layers, default)")},
    {"app.select-list",              N_("List Selection"),           SECTION, N_("Print a list of objects in current selection")},
    // clang-format on
};

void
add_actions_selection(InkscapeApplication* app)
{
    auto *gapp = app->gio_app();

    // clang-format off
    gapp->add_action(               "select-clear",                 sigc::bind(sigc::ptr_fun(&select_clear),             app)        );
    gapp->add_action_radio_string(  "select",                       sigc::bind(sigc::ptr_fun(&select_by_id),             app), "null"); // Backwards compatible.
    gapp->add_action_radio_string(  "unselect",                     sigc::bind(sigc::ptr_fun(&unselect_by_id),           app), "null"); // Match select.
    gapp->add_action_radio_string(  "select-by-id",                 sigc::bind(sigc::ptr_fun(&select_by_id),             app), "null");
    gapp->add_action_radio_string(  "unselect-by-id",               sigc::bind(sigc::ptr_fun(&unselect_by_id),           app), "null");
    gapp->add_action_radio_string(  "select-by-class",              sigc::bind(sigc::ptr_fun(&select_by_class),          app), "null");
    gapp->add_action_radio_string(  "select-by-element",            sigc::bind(sigc::ptr_fun(&select_by_element),        app), "null");
    gapp->add_action_radio_string(  "select-by-selector",           sigc::bind(sigc::ptr_fun(&select_by_selector),       app), "null");
    gapp->add_action_radio_string(  "select-all",                   sigc::bind(sigc::ptr_fun(&select_all),               app), "null");
    gapp->add_action_radio_string(  "select-invert",                sigc::bind(sigc::ptr_fun(&select_invert),            app), "null");
    gapp->add_action(               "select-list",                  sigc::bind(sigc::ptr_fun(&select_list),              app)        );
    // clang-format on

    app->get_action_extra_data().add_data(raw_data_selection);
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

// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 *
 * Actions Related to Editing which require document
 *
 * Authors:
 *   Sushant A A <sushant.co19@gmail.com>
 *
 * Copyright (C) 2021 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "actions-edit-document.h"

#include <glibmm/i18n.h>

#include "document.h"
#include "document-undo.h"
#include "inkscape-application.h"
#include "selection-chemistry.h"

#include "actions/actions-extra-data.h"
#include "object/sp-guide.h"
#include "object/sp-namedview.h"
#include "xml/node.h"

void
create_guides_around_page(SPDocument* document)
{
    // Create Guides Around the Page
    sp_guide_create_guides_around_page(document);
}

void
lock_all_guides(SPDocument *document)
{
    document->getNamedView()->toggleLockGuides();
}

void
show_all_guides(SPDocument *document)
{
    document->getNamedView()->toggleShowGuides();
}

void
delete_all_guides(SPDocument* document)
{
    // Delete All Guides
    sp_guide_delete_all_guides(document);
}

void
fit_canvas_drawing(SPDocument* document)
{
    // Fit Page to Drawing
    if (fit_canvas_to_drawing(document)) {
        Inkscape::DocumentUndo::done(document, RC_("Undo", "Fit Page to Drawing"), "");
    }
}

void
set_display_unit(Glib::ustring abbr, SPDocument* document)
{
    // This does not modify the scale of the document, just the units
    Inkscape::XML::Node *repr = document->getNamedView()->getRepr();
    repr->setAttribute("inkscape:document-units", abbr);
    document->setModifiedSinceSave();
    Inkscape::DocumentUndo::done(document, RC_("Undo", "Changed default display unit"), "");
}

void toggle_clip_to_page(SPDocument* document) {
    if (!document || !document->getNamedView()) return;

    auto clip = !document->getNamedView()->clip_to_page;
    document->getNamedView()->change_bool_setting(SPAttr::INKSCAPE_CLIP_TO_PAGE_RENDERING, clip);
    document->setModifiedSinceSave();
    Inkscape::DocumentUndo::done(document, RC_("Undo", "Clip to page"), "");
}

void
show_grids(SPDocument *document)
{
    document->getNamedView()->toggleShowGrids();
}

const Glib::ustring SECTION = NC_("Action Section", "Edit Document");

std::vector<std::vector<Glib::ustring>> raw_data_edit_document = {
    // clang-format off
    {"doc.create-guides-around-page", N_("Create Guides Around the Current Page"), SECTION, N_("Create four guides aligned with the page borders of the current page")},
    {"doc.lock-all-guides",           N_("Lock All Guides"),                       SECTION, N_("Toggle lock of all guides in the document")},
    {"doc.show-all-guides",           N_("Show All Guides"),                       SECTION, N_("Toggle visibility of all guides in the document")},
    {"doc.delete-all-guides",         N_("Delete All Guides"),                     SECTION, N_("Delete all the guides in the document")},
    {"doc.fit-canvas-to-drawing",     N_("Fit Page to Drawing"),                   SECTION, N_("Fit the page to the drawing")},
    {"doc.clip-to-page",              N_("Toggle Clip to Page"),                   SECTION, N_("Toggle between clipped to page and complete rendering")},
    {"doc.show-grids",                N_("Show Grids"),                            SECTION, N_("Toggle the visibility of grids")},
    // clang-format on
};

void
add_actions_edit_document(SPDocument* document)
{
    Glib::RefPtr<Gio::SimpleActionGroup> map = document->getActionGroup();

    // clang-format off
    map->add_action( "create-guides-around-page",           sigc::bind(sigc::ptr_fun(&create_guides_around_page),  document));
    map->add_action( "delete-all-guides",                   sigc::bind(sigc::ptr_fun(&delete_all_guides),  document));
    map->add_action( "fit-canvas-to-drawing",               sigc::bind(sigc::ptr_fun(&fit_canvas_drawing),  document));
    map->add_action_bool( "lock-all-guides",                sigc::bind(sigc::ptr_fun(&lock_all_guides),   document));
    map->add_action_bool( "show-all-guides",                sigc::bind(sigc::ptr_fun(&show_all_guides),   document));
    map->add_action_bool( "show-grids",                     sigc::bind(sigc::ptr_fun(&show_grids),   document));

    map->add_action_radio_string("set-display-unit",        sigc::bind(sigc::ptr_fun(&set_display_unit), document), "px");
    map->add_action("clip-to-page",                         [=](){ toggle_clip_to_page(document); });
    // clang-format on

    // Check if there is already an application instance (GUI or non-GUI).
    auto app = InkscapeApplication::instance();
    if (!app) { // i.e. Inkview
      return;
    }
    app->get_action_extra_data().add_data(raw_data_edit_document);
}

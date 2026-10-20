// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 *
 *  Actions for Editing an object
 *
 * Authors:
 *   Sushant A A <sushant.co19@gmail.com>
 *
 * Copyright (C) 2021 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "actions-edit.h"

#include <giomm.h>
#include <glibmm/i18n.h>

#include "actions-helper.h"
#include "desktop.h"
#include "inkscape-application.h"
#include "document-undo.h"
#include "selection.h"
#include "selection-chemistry.h"

#include "object/sp-guide.h"
#include "ui/icon-names.h"
#include "ui/tools/node-tool.h"
#include "ui/tools/text-tool.h"

#include "actions/actions-svg-processing.h"

namespace ActionsEdit {

void
object_to_pattern(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    //  Objects to Pattern
    selection->tile();
}

void
pattern_to_object(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    //  Pattern to Objects
    selection->untile();
}

void
object_to_marker(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    //  Objects to Marker
    selection->toMarker();
}

void
object_to_guides(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    //  Objects to Guides
    selection->toGuides();
}

void
cut(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    // Cut
    selection->cut();
}

void
copy(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    //  Copy
    selection->copy();
}

void
paste_style(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    //  Paste Style
    selection->pasteStyle();
}

void
paste_size(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    //  Paste Size
    selection->pasteSize(true,true);
}

void
paste_width(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    //  Paste Width
    selection->pasteSize(true, false);
}

void
paste_height(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    //  Paste Height
    selection->pasteSize(false, true);
}

void
paste_size_separately(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    //  Paste Size Separately
    selection->pasteSizeSeparately(true, true);
}

void
paste_width_separately(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    //  Paste Width Separately
    selection->pasteSizeSeparately(true, false);
}

void
paste_height_separately(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    //  Paste Height Separately
    selection->pasteSizeSeparately(false, true);
}

void
duplicate(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    //  Duplicate
    selection->duplicate();
}

void duplicate_transform(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();
    selection->duplicate(true);
    selection->reapplyAffine();
    Inkscape::DocumentUndo::done(app->get_active_document(), RC_("Undo", "Duplicate and Transform"),
                                 INKSCAPE_ICON("edit-duplicate"));
}

void
clone(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    //  Create Clone
    selection->clone();
}

void
clone_unlink(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    //  Unlink Clone
    selection->unlink();
}

void
clone_unlink_recursively(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    //  Unlink Clones recursively
    selection->unlinkRecursive(false, true);
}

void
clone_link(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    //  Relink to Copied
    selection->relink();
}

void
select_original(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    //  Select Original
    selection->cloneOriginal();
}

void
clone_link_lpe(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    //  Clone original path (LPE)
    selection->cloneOriginalPathLPE();
}

void
edit_delete(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    // For text and node too special handling.
    if (auto desktop = selection->desktop()) {
        if (auto const text_tool = dynamic_cast<Inkscape::UI::Tools::TextTool *>(desktop->getTool())) {
            text_tool->deleteSelected();
            return;
        }
        if (auto const node_tool = dynamic_cast<Inkscape::UI::Tools::NodeTool *>(desktop->getTool())) {
            // This means we delete items is no nodes are selected.
            if (node_tool->_selected_nodes) {
                node_tool->deleteSelected();
                return;
            }
        }
    }

    //  Delete select objects only.
    selection->deleteItems();
}

void
edit_delete_selection(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();
    selection->deleteItems();
}

void
paste_path_effect(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    //  Paste Path Effect
    selection->pastePathEffect();
}

void
remove_path_effect(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    //  Remove Path Effect
    selection->removeLPE();
}

void
swap_fill_and_stroke(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    // Swap fill and Stroke
    selection->swapFillStroke();
}

void
fit_canvas_to_selection(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    // Fit Page to Selection
    selection->fitCanvas(true);
}

void
chameleon_fill(InkscapeApplication *app)
{
    app->get_active_selection()->chameleonFill();
}

const Glib::ustring SECTION = NC_("Action Section", "Edit");

std::vector<std::vector<Glib::ustring>> raw_data_edit = {
    // clang-format off
    {"app.object-to-pattern",        N_("Objects to Pattern"),        SECTION, N_("Convert selection to a rectangle with tiled pattern fill")},
    {"app.pattern-to-object",        N_("Pattern to Objects"),        SECTION, N_("Extract objects from a tiled pattern fill")},
    {"app.object-to-marker",         N_("Objects to Marker"),         SECTION, N_("Convert selection to a line marker")},
    {"app.object-to-guides",         N_("Objects to Guides"),         SECTION, N_("Convert selected objects to a collection of guidelines aligned with their edges")},
    {"app.cut",                      N_("Cut"),                       SECTION, N_("Cut selection to clipboard")},
    {"app.copy",                     N_("Copy"),                      SECTION, N_("Copy selection to clipboard")},
    {"app.paste-style",              N_("Paste Style"),               SECTION, N_("Apply the style of the copied object to selection")},
    {"app.paste-size",               N_("Paste Size"),                SECTION, N_("Scale selection to match the size of the copied object")},
    {"app.paste-width",              N_("Paste Width"),               SECTION, N_("Scale selection horizontally to match the width of the copied object")},
    {"app.paste-height",             N_("Paste Height"),              SECTION, N_("Scale selection vertically to match the height of the copied object")},
    {"app.paste-size-separately",    N_("Paste Size Separately"),     SECTION, N_("Scale each selected object to match the size of the copied object")},
    {"app.paste-width-separately",   N_("Paste Width Separately"),    SECTION, N_("Scale each selected object horizontally to match the width of the copied object")},
    {"app.paste-height-separately",  N_("Paste Height Separately"),   SECTION, N_("Scale each selected object vertically to match the height of the copied object")},
    {"app.duplicate",                N_("Duplicate"),                 SECTION, N_("Duplicate Selected Objects")},
    {"app.duplicate-transform",      N_("Duplicate and Transform"),   SECTION, N_("Duplicate selected objects and reapply last transformation")},
    {"app.clone",                    N_("Create Clone"),              SECTION, N_("Create a clone (a copy linked to the original) of selected object")},
    {"app.clone-unlink",             N_("Unlink Clone"),              SECTION, N_("Cut the selected clones' links to the originals, turning them into standalone objects")},
    {"app.clone-unlink-recursively", N_("Unlink Clones recursively"), SECTION, N_("Unlink all clones in the selection, even if they are in groups.")},
    {"app.clone-link",               N_("Relink to Copied"),          SECTION, N_("Relink the selected clones to the object currently on the clipboard")},
    {"app.select-original",          N_("Select Original"),           SECTION, N_("Select the object to which the selected clone is linked")},
    {"app.clone-link-lpe",           N_("Clone original path (LPE)"), SECTION, N_("Creates a new path, applies the Clone original LPE, and refers it to the selected path")},
    {"app.delete",                   N_("Delete"),                    SECTION, N_("Delete selected items, nodes or text.")},
    {"app.delete-selection",         N_("Delete Items"),              SECTION, N_("Delete selected items")},
    {"app.paste-path-effect",        N_("Paste Path Effect"),         SECTION, N_("Apply the path effect of the copied object to selection")},
    {"app.remove-path-effect",       N_("Remove Path Effect"),        SECTION, N_("Remove any path effects from selected objects")},
    {"app.swap-fill-and-stroke",     N_("Swap fill and stroke"),      SECTION, N_("Swap fill and stroke of an object")},
    {"app.fit-canvas-to-selection",  N_("Fit Page to Selection"),     SECTION, N_("Fit the page to the current selection")},
    {"app.chameleon-fill",           N_("Chameleon Fill"),            SECTION, N_("Set each object's color to the average of all colors inside that shape.")}
    // clang-format on
};

} // namespace ActionsEdit

using namespace ActionsEdit;

void
add_actions_edit(InkscapeApplication* app)
{
    auto *gapp = app->gio_app();

    // clang-format off
    gapp->add_action( "object-to-pattern",               sigc::bind(sigc::ptr_fun(&object_to_pattern), app));
    gapp->add_action( "pattern-to-object",               sigc::bind(sigc::ptr_fun(&pattern_to_object), app));
    gapp->add_action( "object-to-marker",                sigc::bind(sigc::ptr_fun(&object_to_marker), app));
    gapp->add_action( "object-to-guides",                sigc::bind(sigc::ptr_fun(&object_to_guides), app));
    gapp->add_action( "cut",                             sigc::bind(sigc::ptr_fun(&cut), app));
    gapp->add_action( "copy",                            sigc::bind(sigc::ptr_fun(&copy), app));
    gapp->add_action( "paste-style",                     sigc::bind(sigc::ptr_fun(&paste_style), app));
    gapp->add_action( "paste-size",                      sigc::bind(sigc::ptr_fun(&paste_size), app));
    gapp->add_action( "paste-width",                     sigc::bind(sigc::ptr_fun(&paste_width), app));
    gapp->add_action( "paste-height",                    sigc::bind(sigc::ptr_fun(&paste_height), app));
    gapp->add_action( "paste-size-separately",           sigc::bind(sigc::ptr_fun(&paste_size_separately), app));
    gapp->add_action( "paste-width-separately",          sigc::bind(sigc::ptr_fun(&paste_width_separately), app));
    gapp->add_action( "paste-height-separately",         sigc::bind(sigc::ptr_fun(&paste_height_separately), app));
    gapp->add_action( "duplicate",                       sigc::bind(sigc::ptr_fun(&duplicate), app));
    gapp->add_action( "duplicate-transform",             sigc::bind(sigc::ptr_fun(&duplicate_transform), app));
    // explicit namespace reference added because NetBSD provides a conflicting clone() function in its libc headers
    gapp->add_action( "clone",                           sigc::bind(sigc::ptr_fun(&ActionsEdit::clone), app));
    gapp->add_action( "clone-unlink",                    sigc::bind(sigc::ptr_fun(&clone_unlink), app));
    gapp->add_action( "clone-unlink-recursively",        sigc::bind(sigc::ptr_fun(&clone_unlink_recursively), app));
    gapp->add_action( "clone-link",                      sigc::bind(sigc::ptr_fun(&clone_link), app));
    gapp->add_action( "select-original",                 sigc::bind(sigc::ptr_fun(&select_original), app));
    gapp->add_action( "clone-link-lpe",                  sigc::bind(sigc::ptr_fun(&clone_link_lpe), app));
    gapp->add_action( "delete",                          sigc::bind(sigc::ptr_fun(&edit_delete), app));
    gapp->add_action( "delete-selection",                sigc::bind(sigc::ptr_fun(&edit_delete_selection), app));
    gapp->add_action( "paste-path-effect",               sigc::bind(sigc::ptr_fun(&paste_path_effect), app));
    gapp->add_action( "remove-path-effect",              sigc::bind(sigc::ptr_fun(&remove_path_effect), app));
    gapp->add_action( "swap-fill-and-stroke",            sigc::bind(sigc::ptr_fun(&swap_fill_and_stroke), app));
    gapp->add_action( "fit-canvas-to-selection",         sigc::bind(sigc::ptr_fun(&fit_canvas_to_selection), app));
    gapp->add_action( "chameleon-fill",                  sigc::bind(sigc::ptr_fun(&chameleon_fill), app));
    // clang-format on

    if (!app) {
        show_output("add_actions_edit: no app!");
        return;
    }
    app->get_action_extra_data().add_data(raw_data_edit);
}

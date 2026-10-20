// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Context menu
 */
/* Authors:
 *   Tavmjong Bah
 *
 * Rewrite of code authored by:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *   Kris De Gussem <Kris.DeGussem@gmail.com>
 *
 * Copyright (C) 2022 Tavmjong Bah
 * Copyright (C) 2012 Kris De Gussem
 * Copyright (C) 2010 authors
 * Copyright (C) 1999-2005 authors
 * Copyright (C) 2004 David Turner
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "contextmenu.h"

#include <glibmm/i18n.h>
#include <gtkmm/image.h>

#include "desktop.h"
#include "document-undo.h"
#include "inkscape-window.h"
#include "layer-manager.h"
#include "object/sp-anchor.h"
#include "object/sp-image.h"
#include "object/sp-page.h"
#include "object/sp-shape.h"
#include "object/sp-text.h"
#include "object/sp-use.h"
#include "preferences.h"
#include "selection.h"
#include "ui/desktop/menu-set-tooltips-shift-icons.h"
#include "ui/util.h"

static void
AppendItemFromAction(Glib::RefPtr<Gio::Menu> const &menu,
                     Glib::ustring const &action,
                     Glib::ustring const &label,
                     Glib::ustring const &icon_name = {})
{
    auto prefs = Inkscape::Preferences::get();
    int use_icons = prefs->getInt("/theme/menuIcons", 0);

    auto menu_item = Gio::MenuItem::create(label, action);
    if (!icon_name.empty() && use_icons >= 1) {
        auto icon = Gio::Icon::create(icon_name);
        menu_item->set_icon(icon);
    }
    menu->append_item(menu_item);
}

/** @brief Create a menu section containing the standard editing actions:
 *         Cut, Copy, Paste, Paste... (in place, on page, style, size, width, height, size separately, width separately, height separately).
 *
 *  @param paste_only If true, only the Paste action will be included.
 *  @return A new menu containing the requested actions.
 */
static Glib::RefPtr<Gio::Menu> create_clipboard_actions(bool const paste_only = false)
{
    auto result = Gio::Menu::create();
    if (!paste_only) {
        AppendItemFromAction(result, "app.cut",  _("Cu_t"),  "edit-cut");
        AppendItemFromAction(result, "app.copy", _("_Copy"), "edit-copy");
    }
    AppendItemFromAction(result, "win.paste", _("_Paste"), "edit-paste");

    /// Also appending special paste options
    /// (in place, paste on page, paste style, paste size, paste width, paste height, paste size separately,
    /// paste width separately, paste height separately), to increase discoverability.
    auto gmenu_paste_section = Gio::Menu::create();
    auto gmenu_paste_submenu = Gio::Menu::create();
    AppendItemFromAction(gmenu_paste_submenu, "win.paste-in-place", _("Paste _in Place"), "edit-paste-in-place");
    AppendItemFromAction(gmenu_paste_submenu, "win.paste-on-page", _("Paste _on Page"), "");
    AppendItemFromAction(gmenu_paste_submenu, "app.paste-style", _("Paste _Style"), "edit-paste-style");
    AppendItemFromAction(gmenu_paste_submenu, "app.paste-size", _("Paste Si_ze Onto Selection"), "edit-paste-size");
    AppendItemFromAction(gmenu_paste_submenu, "app.paste-width", _("Paste _Width Onto Selection"), "edit-paste-width");
    AppendItemFromAction(gmenu_paste_submenu, "app.paste-height", _("Paste _Height Onto Selection"), "edit-paste-height");
    AppendItemFromAction(gmenu_paste_submenu, "app.paste-size-separately", _("Paste Size Onto Selection Separately"), "edit-paste-size-separately");
    AppendItemFromAction(gmenu_paste_submenu, "app.paste-width-separately", _("Paste Width Onto Selection Separately"), "edit-paste-width-separately");
    AppendItemFromAction(gmenu_paste_submenu, "app.paste-height-separately", _("Paste Height Onto Selection Separately"), "edit-paste-height-separately");
    gmenu_paste_section->append_submenu(_("Paste Options"), gmenu_paste_submenu);
    result->append_section(gmenu_paste_section);

    return result;
}

/// Recursively force all Image descendants with :storage-type other than EMPTY to :visible = TRUE.
/// We have to do this this if using Gio::Menu with icons as GTK, in its vast genius, doesnʼt think
/// those should ever actually be visible in the majority of cases. So, we just have to fight it 🤷
/// We donʼt show images if :storage-type == EMPTY so that shift_icons() can select on :only-child.
void show_all_images(Gtk::Widget &parent)
{
    Inkscape::UI::for_each_descendant(parent, [=](Gtk::Widget &child)
    {
        if (auto const image = dynamic_cast<Gtk::Image *>(&child);
            image && image->get_storage_type() != Gtk::Image::Type::EMPTY)
        {
            image->set_visible(true);
        }
        return Inkscape::UI::ForEachResult::_continue;
    });
}

/// Check if the item is a clone of an image.
bool is_clone_of_image(SPItem const *item)
{
    if (auto const *clone = cast<SPUse>(item)) {
        return is<SPImage>(clone->trueOriginal());
    }
    return false;
}

static bool childrenIncludedInSelection(SPItem *item, Inkscape::Selection &selection)
{
    return std::any_of(item->children.begin(), item->children.end(), [&selection](auto &child) {
        if (auto childItem = cast<SPItem>(&child)) {
            return selection.includes(childItem) || childrenIncludedInSelection(childItem, selection);
        }
        return false;
    });
}

ContextMenu::ContextMenu(SPDesktop *desktop, SPObject *object, std::vector<SPItem*> const &items, bool hide_layers_and_objects_menu_item)
{
    set_name("ContextMenu");

    auto item = cast<SPItem>(object);

    // std::cout << "ContextMenu::ContextMenu: " << (item ? item->getId() : "no item") << std::endl;
    action_group = Gio::SimpleActionGroup::create();
    insert_action_group("ctx", action_group);
    auto document = desktop->getDocument();
    action_group->add_action("unhide-objects-below-cursor", sigc::bind(sigc::mem_fun(*this, &ContextMenu::unhide_or_unlock), document, true));
    action_group->add_action("unlock-objects-below-cursor", sigc::bind(sigc::mem_fun(*this, &ContextMenu::unhide_or_unlock), document, false));

    auto gmenu         = Gio::Menu::create(); // Main menu
    auto gmenu_section = Gio::Menu::create(); // Section (used multiple times)

    auto layer = Inkscape::LayerManager::asLayer(item);  // Layers have their own context menu in the Object and Layers dialog.
    auto root = desktop->layerManager().currentRoot();

    // Save the items in context
    items_under_cursor = items;

    bool has_hidden_below_cursor = false;
    bool has_locked_below_cursor = false;
    for (auto item : items_under_cursor) {
        if (item->isHidden()) {
            has_hidden_below_cursor = true;
        }
        if (item->isLocked()) {
            has_locked_below_cursor = true;
        }
    }
    // std::cout << "Items below cursor: " << items_under_cursor.size()
    //           << "  hidden: " << std::boolalpha << has_hidden_below_cursor
    //           << "  locked: " << std::boolalpha << has_locked_below_cursor
    //           << std::endl;

    // clang-format off

    // Undo/redo
    // gmenu_section = Gio::Menu::create();
    // AppendItemFromAction(gmenu_section, "doc.undo",      _("Undo"),       "edit-undo");
    // AppendItemFromAction(gmenu_section, "doc.redo",      _("Redo"),       "edit-redo");
    // gmenu->append_section(gmenu_section);

    if (auto page = cast<SPPage>(object)) {
        auto &page_manager = document->getPageManager();
        page_manager.selectPage(page);

        gmenu_section = Gio::Menu::create();
        AppendItemFromAction(gmenu_section, "doc.page-new", _("_New Page"), "pages-add");
        AppendItemFromAction(gmenu_section, "doc.page-duplicate", _("Duplicate Page"), "pages-duplicate");
        gmenu->append_section(gmenu_section);

        gmenu_section = Gio::Menu::create();
        AppendItemFromAction(gmenu_section, "doc.page-delete", _("_Delete Page"), "pages-remove");
        AppendItemFromAction(gmenu_section, "doc.page-move-backward", _("Move Page _Backward"), "pages-order-backwards");
        AppendItemFromAction(gmenu_section, "doc.page-move-forward", _("Move Page _Forward"), "pages-order-forwards");
        gmenu->append_section(gmenu_section);

    } else {
        auto only_layer = layer && !desktop->getSelection()->includes(layer);

        if (!only_layer) {
            // "item" is the object that was under the mouse when right-clicked. It determines what is shown
            // in the menu thus it makes the most sense that it is either selected or part of the current
            // selection.
            auto &selection = *desktop->getSelection();

            // Do not include this object in the selection if any of its
            // children have been selected separately.
            if (object && !selection.includes(object) && item && !childrenIncludedInSelection(item, selection)) {
                selection.set(object);
            }

            if (!item) {
                // Even when there's no item, we should still have the Paste action on top
                // (see https://gitlab.com/inkscape/inkscape/-/issues/4150)
                gmenu->append_section(create_clipboard_actions(true));

                gmenu_section = Gio::Menu::create();
                AppendItemFromAction(gmenu_section, "win.dialog-open('DocumentProperties')", _("Document Properties..."), "document-properties");
                gmenu->append_section(gmenu_section);
            } else {
                // When an item is selected, show all three of Cut, Copy and Paste.
                gmenu->append_section(create_clipboard_actions());

                gmenu_section = Gio::Menu::create();
                AppendItemFromAction(gmenu_section, "app.duplicate", _("Duplic_ate"), "edit-duplicate");
                AppendItemFromAction(gmenu_section, "app.clone", _("_Clone"), "edit-clone");
                AppendItemFromAction(gmenu_section, "app.delete-selection", _("_Delete"), "edit-delete");
                if (auto text = cast<SPText>(item); text && text->has_shape_inside()) {
                    AppendItemFromAction(gmenu_section, "app.text-unflow-and-keep-shape", _("_Unflow and keep shape"), "select_text_unflow_and_keep_shape");
                }
                gmenu->append_section(gmenu_section);

                // Dialogs
                auto gmenu_dialogs = Gio::Menu::create();
                if (!hide_layers_and_objects_menu_item) { // Hidden when context menu is popped up in Layers and Objects dialog!
                    AppendItemFromAction(gmenu_dialogs, "win.dialog-open('Objects')",                   _("Layers and Objects..."), "dialog-objects"             );
                }
                AppendItemFromAction(gmenu_dialogs,     "win.dialog-open('ObjectProperties')",          _("_Object Properties..."), "dialog-object-properties"   );

                if (is<SPShape>(item) || is<SPText>(item) || is<SPGroup>(item)) {
                    AppendItemFromAction(gmenu_dialogs, "win.dialog-open('FillStroke')",                _("_Fill and Stroke..."),   "dialog-fill-and-stroke"     );
                }

                // Image dialogs (mostly).
                if (auto image = cast<SPImage>(item)) {
                    AppendItemFromAction(     gmenu_dialogs, "win.dialog-open('Trace')",                     _("_Trace Bitmap..."),      "bitmap-trace"          );

                    if (image->getClipObject()) {
                        AppendItemFromAction( gmenu_dialogs, "app.element-image-crop",                       _("Crop Image to Clip"),    ""                      );
                    }
                    if (strncmp(image->href, "data", 4) == 0) {
                        // Image is embedded.
                        AppendItemFromAction( gmenu_dialogs, "app.org.inkscape.filter.extract-image",        _("Extract Image..."),      ""                      );
                    } else {
                        // Image is linked.
                        AppendItemFromAction( gmenu_dialogs, "app.org.inkscape.filter.selected.embed-image", _("Embed Image"),           ""                      );
                        AppendItemFromAction( gmenu_dialogs, "app.element-image-edit",                       _("Edit Externally..."),    ""                      );
                    }
                }

                // A clone of an image supports "Edit Externally" as well
                if (is_clone_of_image(item)) {
                    AppendItemFromAction(gmenu_dialogs, "app.element-image-edit", _("Edit Externally..."), "");
                }

                // Text dialogs.
                if (is<SPText>(item)) {
                    AppendItemFromAction(     gmenu_dialogs, "win.dialog-open('Text')",                      _("_Text and Font..."),     "dialog-text-and-font"  );
                    AppendItemFromAction(     gmenu_dialogs, "win.dialog-open('Spellcheck')",                _("Check Spellin_g..."),    "tools-check-spelling"  );
                }
                gmenu->append_section(gmenu_dialogs); // We might add to it later...

                if (!is<SPAnchor>(item)) {
                    // Item menu

                    // Selection
                    gmenu_section = Gio::Menu::create();
                    auto gmenu_submenu = Gio::Menu::create();
                    AppendItemFromAction(     gmenu_submenu, "win.select-same-fill-and-stroke",     _("Fill _and Stroke"),      "edit-select-same-fill-and-stroke");
                    AppendItemFromAction(     gmenu_submenu, "win.select-same-fill",                _("_Fill Color"),           "edit-select-same-fill"           );
                    AppendItemFromAction(     gmenu_submenu, "win.select-same-stroke-color",        _("_Stroke Color"),         "edit-select-same-stroke-color"   );
                    AppendItemFromAction(     gmenu_submenu, "win.select-same-stroke-style",        _("Stroke St_yle"),         "edit-select-same-stroke-style"   );
                    AppendItemFromAction(     gmenu_submenu, "win.select-same-object-type",         _("_Object Type"),          "edit-select-same-object-type"    );
                    gmenu_section->append_submenu(_("Select Sa_me"), gmenu_submenu);
                    gmenu->append_section(gmenu_section);

                    // Groups and Layers
                    gmenu_section = Gio::Menu::create();
                    AppendItemFromAction(     gmenu_section, "win.selection-move-to-layer",         _("_Move to Layer..."),     ""                                );
                    AppendItemFromAction(     gmenu_section, "app.selection-link",                  _("Create Anchor (Hyperlink)"),   ""                          );
                    AppendItemFromAction(     gmenu_section, "app.selection-group",                 _("_Group"),                ""                                );
                    if (is<SPGroup>(item)) {
                        AppendItemFromAction( gmenu_section, "app.selection-ungroup",               _("_Ungroup"),              ""                                );
                        Glib::ustring label = Glib::ustring::compose(_("Enter Group %1"), item->defaultLabel());
                        AppendItemFromAction( gmenu_section, "win.selection-group-enter",           label,                      ""                                );
                        if (!layer && (item->getParentGroup()->isLayer() || item->getParentGroup() == root)) {
                            // A layer should be a child of root or another layer.
                            AppendItemFromAction( gmenu_section, "win.layer-from-group",            _("Group to Layer"),        ""                                );
                        }
                    }
                    auto group = cast<SPGroup>(item->parent);
                    if (group && !group->isLayer()) {
                        AppendItemFromAction( gmenu_section, "win.selection-group-exit",            _("Exit Group"),            ""                                );
                        AppendItemFromAction( gmenu_section, "app.selection-ungroup-pop",           _("_Pop Selection out of Group"), ""                          );
                    }
                    gmenu->append_section(gmenu_section);

                    // Clipping and Masking
                    gmenu_section = Gio::Menu::create();
                    if (selection.size() > 1) {
                        AppendItemFromAction( gmenu_section, "app.object-set-clip",                 _("Set Cl_ip"),             ""                                );
                    }
                    if (item->getClipObject()) {
                        AppendItemFromAction( gmenu_section, "app.object-release-clip",             _("Release C_lip"),         ""                                );
                    } else {
                        AppendItemFromAction( gmenu_section, "app.object-set-clip-group",           _("Set Clip G_roup"),       ""                                );
                    }
                    if (selection.size() > 1) {
                        AppendItemFromAction( gmenu_section, "app.object-set-mask",                 _("Set Mask"),              ""                                );
                    }
                    if (item->getMaskObject()) {
                        AppendItemFromAction( gmenu_section, "app.object-release-mask",             _("Release Mask"),          ""                                );
                    }
                    gmenu->append_section(gmenu_section);

                    // Hide and Lock
                    gmenu_section = Gio::Menu::create();
                    AppendItemFromAction(     gmenu_section, "app.selection-hide",                  _("Hide Selected Objects"), ""                                );
                    AppendItemFromAction(     gmenu_section, "app.selection-lock",                  _("Lock Selected Objects"), ""                                );
                    gmenu->append_section(gmenu_section);

                } else {
                    // Anchor menu
                    gmenu_section = Gio::Menu::create();
                    AppendItemFromAction(     gmenu_section, "app.element-a-open-link",             _("_Open Link in Browser"), ""                                );
                    AppendItemFromAction(     gmenu_section, "app.selection-ungroup",               _("_Remove Link"),          ""                                );
                    AppendItemFromAction(     gmenu_section, "win.selection-group-enter",           _("Enter Group"),           ""                                );
                    gmenu->append_section(gmenu_section);
                }
            }

            // Hidden or locked beneath cursor
            gmenu_section = Gio::Menu::create();
            if (has_hidden_below_cursor) {
                AppendItemFromAction( gmenu_section, "ctx.unhide-objects-below-cursor",     _("Unhide Objects Below Cursor"),  ""                         );
            }
            if (has_locked_below_cursor) {
                AppendItemFromAction( gmenu_section, "ctx.unlock-objects-below-cursor",     _("Unlock Objects Below Cursor"),  ""                         );
            }
            gmenu->append_section(gmenu_section);
        }

        if (layer) {
            // Layers: Only used in "Layers and Objects" dialog.

            gmenu_section = Gio::Menu::create();
            AppendItemFromAction(gmenu_section,     "win.layer-new",                _("_Add Layer..."),              "layer-new");
            AppendItemFromAction(gmenu_section,     "win.layer-duplicate",          _("D_uplicate Layer"),           "layer-duplicate");
            AppendItemFromAction(gmenu_section,     "win.layer-delete",             _("_Delete Layer"),              "layer-delete");
            AppendItemFromAction(gmenu_section,     "win.layer-rename",             _("Re_name Layer..."),           "layer-rename");
            AppendItemFromAction(gmenu_section,     "win.layer-to-group",           _("Layer to _Group"),            "dialog-objects");
            gmenu->append_section(gmenu_section);

            gmenu_section = Gio::Menu::create();
            AppendItemFromAction(gmenu_section,     "win.layer-raise",              _("_Raise Layer"),               "layer-raise");
            AppendItemFromAction(gmenu_section,     "win.layer-lower",              _("_Lower Layer"),               "layer-lower");
            gmenu->append_section(gmenu_section);

            gmenu_section = Gio::Menu::create();
            AppendItemFromAction(gmenu_section,     "win.layer-hide-toggle-others", _("_Hide/Show Other Layers"),    "");
            AppendItemFromAction(gmenu_section,     "win.layer-hide-all",           _("_Hide All Layers"),           "");
            AppendItemFromAction(gmenu_section,     "win.layer-unhide-all",         _("_Show All Layers"),           "");
            gmenu->append_section(gmenu_section);

            gmenu_section = Gio::Menu::create();
            AppendItemFromAction(gmenu_section,     "win.layer-lock-toggle-others", _("_Lock/Unlock Other Layers"),  "");
            AppendItemFromAction(gmenu_section,     "win.layer-lock-all",           _("_Lock All Layers"),           "");
            AppendItemFromAction(gmenu_section,     "win.layer-unlock-all",         _("_Unlock All Layers"),         "");
            gmenu->append_section(gmenu_section);
        }
    }
    // clang-format on

    set_menu_model(gmenu);
    set_position(Gtk::PositionType::BOTTOM);
    set_halign(Gtk::Align::START);
    set_has_arrow(false);
    show_all_images(*this);
    set_flags(Gtk::PopoverMenu::Flags::NESTED);

    // Do not install this CSS provider; it messes up menus with icons (like popup menu with all dialogs).
    // It doesn't work well with context menu either, introducing disturbing visual glitch
    // where menu shifts upon opening.
    show_icons_and_tooltips(*this);
    // Set the style and icon theme of the new menu based on the desktop
    if (auto const window = desktop->getInkscapeWindow()) {
        if (window->has_css_class("dark")) {
            add_css_class("dark");
        } else {
            add_css_class("bright");
        }
        auto prefs = Inkscape::Preferences::get();
        if (prefs->getBool("/theme/symbolicIcons", false)) {
            add_css_class("symbolic");
        } else {
            add_css_class("regular");
        }
    }
}

void
ContextMenu::unhide_or_unlock(SPDocument* document, bool unhide)
{
    for (auto item : items_under_cursor) {
        if (unhide) {
            if (item->isHidden()) {
                item->setHidden(false);
            }
        } else {
            if (item->isLocked()) {
                item->setLocked(false, true);
            }
        }
    }

    // We wouldn't be here if we didn't make a change.
    Inkscape::DocumentUndo::done(document, (unhide ? RC_("Undo", "Unhid objects") : RC_("Undo", "Unlocked objects")), "");
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

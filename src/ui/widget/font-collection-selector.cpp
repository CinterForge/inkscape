// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * This file contains the definition of the FontCollectionSelector class. This widget
 * defines a treeview to provide the interface to create, read, update and delete font
 * collections and their respective fonts. This class contains all the code related to
 * population of collections and their fonts in the TreeStore.
 */
/*
 * Author:
 *   Vaibhav Malik <vaibhavmalik2018@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "font-collection-selector.h"

#include <gtkmm/droptarget.h>
#include <gtkmm/eventcontrollerkey.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/treestore.h>

#include "libnrtype/font-lister.h"
#include "ui/dialog-run.h"
#include "ui/tools/tool-base.h"
#include "ui/widget/iconrenderer.h"
#include "util/font-collections.h"

namespace Inkscape::UI::Widget {

FontCollectionSelector::FontCollectionSelector()
{
    treeview = Gtk::make_managed<Gtk::TreeView>();
    setup_tree_view(treeview);
    store = Gtk::TreeStore::create(FontCollection);
    treeview->set_model(store);

    setup_signals();
}

FontCollectionSelector::~FontCollectionSelector() = default;

// Setup the treeview of the widget.
void FontCollectionSelector::setup_tree_view(Gtk::TreeView *tv)
{
    cell_text = Gtk::make_managed<Gtk::CellRendererText>();
    cell_font_count = Gtk::make_managed<Gtk::CellRendererText>();
    del_icon_renderer = Gtk::make_managed<IconRenderer>();
    del_icon_renderer->add_icon("edit-delete");

    text_column.pack_start (*cell_text, true);
    text_column.add_attribute (*cell_text, "text", TEXT_COLUMN);
    text_column.set_expand(true);

    font_count_column.pack_start(*cell_font_count, true);
    font_count_column.add_attribute(*cell_font_count, "text", FONT_COUNT_COLUMN);

    del_icon_column.pack_start(*del_icon_renderer, false);

    treeview->set_headers_visible (false);
    treeview->enable_model_drag_dest (Gdk::DragAction::MOVE);

    // Append the columns to the treeview.
    treeview->append_column(text_column);
    treeview->append_column(font_count_column);
    treeview->append_column(del_icon_column);

    scroll.set_policy (Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
    scroll.set_overlay_scrolling(false);
    scroll.set_child(*treeview);

    frame.set_hexpand (true);
    frame.set_vexpand (true);
    frame.set_child(scroll);

    // Grid
    set_name("FontCollection");
    set_row_spacing(4);
    set_column_spacing(1);

    // Add extra columns to the "frame" to change space distribution
    attach (frame,  0, 0, 1, 2);
}

void FontCollectionSelector::change_frame_name(const Glib::ustring& name)
{
    frame.set_label(name);
}

void FontCollectionSelector::setup_signals()
{
    cell_text->signal_edited().connect([this](const Glib::ustring &current_path, const Glib::ustring &new_text) {
        // Store the expanded collections in a map.
        std::set<Glib::ustring> expanded_collections;

        store->foreach ([&](Gtk::TreeModel::Path const &path, Gtk::TreeModel::const_iterator const &it) {
            auto collection = it->get_value(FontCollection.name);
            if (treeview->row_expanded(path)) {
                expanded_collections.insert(collection);
            }
            return false;
        });

        bool current_collection_expanded = false;
        if (auto iter = store->get_iter(current_path)) {
            auto path = store->get_path(iter);
            if (treeview->row_expanded(path)) {
                current_collection_expanded = true;
            }
        }

        bool updated = on_rename_collection(current_path, new_text);
        Gtk::TreeModel::Path updated_path;
        if (updated && current_collection_expanded) {
            expanded_collections.insert(new_text);
        } else if (!updated && new_entry) {
            // Delete this row if the new collection couldn't be created.
            if (auto iter = store->get_iter(current_path)) {
                store->erase(iter);
            }
        }

        // Now start expanding the collections to restore the state as before.
        store->foreach ([&](Gtk::TreeModel::Path const &path, Gtk::TreeModel::const_iterator const &it) {
            auto collection = it->get_value(FontCollection.name);

            if (expanded_collections.contains(collection)) {
                treeview->expand_row(path, false);
            }

            if (updated && (collection == new_text)) {
                updated_path = path;
            }

            return false;
        });

        auto tree_sel = treeview->get_selection();
        if (updated && updated_path) {
            // tree_sel->select_path(updated_path);
            tree_sel->select(updated_path);
            // treeview->scroll_to_row(updated_path);
        }

        new_entry = false;
    });

    treeview->set_row_separator_func(sigc::mem_fun(*this, &FontCollectionSelector::row_separator_func));
    text_column.set_cell_data_func(*cell_text, sigc::mem_fun(*this, &FontCollectionSelector::text_cell_data_func));
    font_count_column.set_cell_data_func(*cell_font_count,
                                         sigc::mem_fun(*this, &FontCollectionSelector::font_count_cell_data_func));
    del_icon_column.set_cell_data_func(*del_icon_renderer,
                                       sigc::mem_fun(*this, &FontCollectionSelector::icon_cell_data_func));

    del_icon_renderer->signal_activated().connect(sigc::mem_fun(*this, &FontCollectionSelector::on_delete_icon_clicked));

    auto const key = Gtk::EventControllerKey::create();
    key->signal_key_pressed().connect([this, &key = *key](auto &&...args) { return on_key_pressed(key, args...); }, true);
    treeview->add_controller(key);

    // Signals for drag and drop.
    auto const drop = Gtk::DropTarget::create(G_TYPE_STRING, Gdk::DragAction::COPY);
    drop->signal_motion().connect(sigc::mem_fun(*this, &FontCollectionSelector::on_drop_motion), false); // before
    drop->signal_drop().connect(sigc::mem_fun(*this, &FontCollectionSelector::on_drop_drop), false); // before
    drop->signal_leave().connect(sigc::mem_fun(*this, &FontCollectionSelector::on_drop_leave));
    treeview->add_controller(drop);

    treeview->get_selection()->signal_changed().connect([this]{ on_selection_changed(); });
}

// This function manages the visibility of the font count column.
void FontCollectionSelector::text_cell_data_func(Gtk::CellRenderer *const renderer,
                                                 Gtk::TreeModel::const_iterator const &iter)
{
    // Only font collections should have a font count next to their names.
    bool const is_collection = !iter->parent();
    cell_text->property_editable() = is_collection ? true : false;
}

// This function manages the visibility of the font count column.
void FontCollectionSelector::font_count_cell_data_func(Gtk::CellRenderer *const renderer,
                                                       Gtk::TreeModel::const_iterator const &iter)
{
    // Only font collections should have a font count next to their names.
    bool const is_collection = !iter->parent();
    cell_font_count->set_visible(is_collection);

    Glib::ustring const markup = "<span alpha='50%'>" + std::to_string((*iter)[FontCollection.font_count]) + "</span>";
    renderer->set_property("markup", markup);
}

// This function will TURN OFF the visibility of the delete icon for system collections.
void FontCollectionSelector::icon_cell_data_func(Gtk::CellRenderer * const renderer,
                                                 Gtk::TreeModel::const_iterator const &iter)
{
    // Add the delete icon only if the collection is editable(user-collection).
    if (auto const parent = iter->parent()) {
        // Case: It is a font.
        bool is_user = (*parent)[FontCollection.is_editable];
        del_icon_renderer->set_visible(is_user);
    } else if((*iter)[FontCollection.is_editable]) {
        // Case: User font collection.
        del_icon_renderer->set_visible(true);
    } else {
        // Case: System font collection.
        del_icon_renderer->set_visible(false);
    }
}

// This function will TURN OFF the visibility of checkbuttons for children in the TreeStore.
void FontCollectionSelector::check_button_cell_data_func(Gtk::CellRenderer * const renderer,
                                                         Gtk::TreeModel::const_iterator const &iter)
{
    renderer->set_visible(false);
    /*
    // Append the checkbutton column only if the iterator have some children.
    Gtk::TreeModel::Row row = *iter;
    auto parent = row->parent();

    if(parent) {
        renderer->set_visible(false);
    }
    else {
        renderer->set_visible(true);
    }
    */
}

bool FontCollectionSelector::row_separator_func(Glib::RefPtr<Gtk::TreeModel> const &/*model*/,
                                                Gtk::TreeModel::const_iterator const &iter)
{
    return iter->get_value(FontCollection.name) == "#";
}

void FontCollectionSelector::populate_collections()
{
    store->clear();
    populate_user_collections();
}

// This function will keep the collections_list updated after any event.
void FontCollectionSelector::populate_user_collections()
{
    // Get the list of all the user collections.
    FontCollections *font_collections = Inkscape::FontCollections::get();
    auto collections = font_collections->get_collections();

    // Now insert these collections one by one into the treeview.
    store->freeze_notify();
    Gtk::TreeModel::iterator iter;

    for(const auto &col: collections) {
        iter = store->append();
        (*iter)[FontCollection.name] = col;

        // User collections are editable.
        (*iter)[FontCollection.is_editable] = true;

        // Alright, now populate the fonts of this collection.
        populate_fonts(col);
    }
    store->thaw_notify();
}

void FontCollectionSelector::populate_fonts(const Glib::ustring& collection_name)
{
    // Get the FontLister instance to get the list of all the collections.
    FontCollections *font_collections = Inkscape::FontCollections::get();
    std::set <Glib::ustring> fonts = font_collections->get_fonts(collection_name);

    // First find the location of this collection_name in the map.
    int index = font_collections->get_user_collection_location(collection_name);

    store->freeze_notify();

    // Generate the iterator path.
    Gtk::TreePath path;
    path.push_back(index);
    Gtk::TreeModel::iterator iter = store->get_iter(path);

    if (!iter) {
        store->thaw_notify();
        return;
    }

    // Update the font count.
    (*iter)[FontCollection.font_count] = fonts.size();

    // auto child_iter = iter->children();
    auto size = iter->children().size();

    // Clear the previously stored fonts at this path.
    while(size--) {
        Gtk::TreeModel::iterator child = iter->children().begin();
        store->erase(child);
    }

    for(auto const &font: fonts) {
        Gtk::TreeModel::iterator child = store->append((*iter).children());
        (*child)[FontCollection.name] = font;
        (*child)[FontCollection.is_editable] = false;
    }

    store->thaw_notify();
}

void FontCollectionSelector::on_delete_icon_clicked(Glib::ustring const &path)
{
    auto collections = Inkscape::FontCollections::get();
    auto iter = store->get_iter(path);
    if (auto const parent = iter->parent()) {
        // It is a font.
        collections->remove_font((*parent)[FontCollection.name], (*iter)[FontCollection.name]);

        // Update the font count of the parent iter.
        (*parent)[FontCollection.font_count] = (*parent)[FontCollection.font_count] - 1;

        store->erase(iter);
    } else {
        // It is a collection.
        // No need to confirm in case of empty collections.
        if (collections->get_fonts((*iter)[FontCollection.name]).empty()) {
            collections->remove_collection((*iter)[FontCollection.name]);
            store->erase(iter);
            return;
        }

        // Warn the user and then proceed.
        deletion_warning_message_dialog((*iter)[FontCollection.name], [this, iter] (int response) {
            if (response == Gtk::ResponseType::YES) {
                auto collections = Inkscape::FontCollections::get();
                collections->remove_collection((*iter)[FontCollection.name]);
                store->erase(iter);
            }
        });
    }
}

void FontCollectionSelector::on_create_collection()
{
    new_entry = true;
    Gtk::TreeModel::iterator iter = store->append();
    (*iter)[FontCollection.is_editable] = true;
    (*iter)[FontCollection.font_count] = 0;

    Gtk::TreeModel::Path path = (Gtk::TreeModel::Path)iter;
    treeview->set_cursor(path, text_column, true);
    grab_focus();
}

bool FontCollectionSelector::on_rename_collection(const Glib::ustring &path, const Glib::ustring &new_text)
{
    // Fetch the collections.
    FontCollections *collections = Inkscape::FontCollections::get();

    // Check if the same collection is already present.
    bool is_system = collections->find_collection(new_text, true);
    bool is_user = collections->find_collection(new_text, false);

    // Return if the new name is empty.
    // Do not allow user collections to be named as system collections.
    if (new_text == "" || is_system || is_user) {
        return false;
    }

    Gtk::TreeModel::iterator iter = store->get_iter(path);

    // Return if it is not a valid iter.
    if(!iter) {
        return false;
    }

    // To check if it's a font-collection or a font.
    if (auto const parent = iter->parent(); !parent) {
        // Call the rename_collection function
        collections->rename_collection((*iter)[FontCollection.name], new_text);
    } else {
        collections->rename_font((*parent)[FontCollection.name], (*iter)[FontCollection.name], new_text);
    }

    (*iter)[FontCollection.name] = new_text;
    populate_collections();

    return true;
}

void FontCollectionSelector::on_delete_button_pressed()
{
    // Get the current collection.
    Glib::RefPtr<Gtk::TreeSelection> selection = treeview->get_selection();
    Gtk::TreeModel::iterator iter = selection->get_selected();
    Gtk::TreeModel::Row row = *iter;
    auto const parent = iter->parent();

    auto collections = Inkscape::FontCollections::get();

    if (!parent) {
        // It is a collection.
        // Check if it is a system collection.
        bool is_system = collections->find_collection((*iter)[FontCollection.name], true);
        if (is_system) {
            return;
        }

        // Warn the user and then proceed.
        deletion_warning_message_dialog((*iter)[FontCollection.name], [this, iter] (int response) {
            if (response == Gtk::ResponseType::YES) {
                auto collections = Inkscape::FontCollections::get();
                collections->remove_collection((*iter)[FontCollection.name]);
                store->erase(iter);
            }
        });
    } else {
        // It is a font.
        // Check if it belongs to a system collection.
        bool is_system = collections->find_collection((*parent)[FontCollection.name], true);
        if (is_system) {
            return;
        }

        collections->remove_font((*parent)[FontCollection.name], row[FontCollection.name]);

        store->erase(iter);
    }
}

// Function to edit the name of the collection or font.
void FontCollectionSelector::on_edit_button_pressed()
{
    Glib::RefPtr<Gtk::TreeSelection> selection = treeview->get_selection();

    if(selection) {
        Gtk::TreeModel::iterator iter = selection->get_selected();
        if(!iter) {
            return;
        }

        auto const parent = iter->parent();
        bool is_system = Inkscape::FontCollections::get()->find_collection((*iter)[FontCollection.name], true);

        if (!parent && !is_system) {
            // It is a collection.
            treeview->set_cursor(Gtk::TreePath(iter), text_column, true);
        }
    }
}

void FontCollectionSelector::deletion_warning_message_dialog(Glib::ustring const &collection_name, sigc::slot<void(int)> onresponse)
{
    auto message = Glib::ustring::compose(_("Are you sure want to delete the \"%1\" font collection?\n"), collection_name);
    auto dialog = std::make_unique<Gtk::MessageDialog>(message, false, Gtk::MessageType::WARNING, Gtk::ButtonsType::YES_NO, true);
    dialog->signal_response().connect(onresponse);
    dialog_show_modal_and_selfdestruct(std::move(dialog), get_root());
}

bool FontCollectionSelector::on_key_pressed(Gtk::EventControllerKey const &controller,
                                            unsigned keyval, unsigned keycode, Gdk::ModifierType state)
{
    switch (Inkscape::UI::Tools::get_latin_keyval(controller, keyval, keycode, state)) {
        case GDK_KEY_Delete:
            on_delete_button_pressed();
            return true;
    }

    return false;
}

Gdk::DragAction FontCollectionSelector::on_drop_motion(double x, double y)
{
    Gtk::TreeModel::Path path;
    Gtk::TreeView::DropPosition pos;
    treeview->get_dest_row_at_pos(x, y, path, pos);
    treeview->unset_state_flags(Gtk::StateFlags::DROP_ACTIVE);

    auto tree_sel = treeview->get_selection();
    if (path) {
        if (auto iter = store->get_iter(path)) {
            if (auto parent = iter->parent()) {
                tree_sel->select(parent);
            } else {
                tree_sel->select(iter);
            }
            return Gdk::DragAction::COPY;
        }
    }

    tree_sel->unselect_all();
    return {};
}

void FontCollectionSelector::on_drop_leave()
{
    treeview->get_selection()->unselect_all();
}

bool FontCollectionSelector::on_drop_drop(Glib::ValueBase const &, double x, double y)
{
    // 1. Get the row at which the data is dropped.
    Gtk::TreePath path;
    int bx{}, by{};
    treeview->convert_widget_to_bin_window_coords(x, y, bx, by);
    if (!treeview->get_path_at_pos(bx, by, path)) {
        return false;
    }
    Gtk::TreeModel::iterator iter = store->get_iter(path);
    // Case when the font is dragged in the empty space.
    if(!iter) {
        return false;
    }

    Glib::ustring collection_name = (*iter)[FontCollection.name];

    bool is_expanded = false;
    if (auto const parent = iter->parent()) {
        is_expanded = true;
        collection_name = (*parent)[FontCollection.name];
    } else {
        is_expanded = treeview->row_expanded(path);
    }

    auto const collections = Inkscape::FontCollections::get();

    bool const is_system = collections->find_collection(collection_name, true);
    if (is_system) {
        // The font is dropped in a system collection.
        return false;
    }

    // 2. Get the data that is sent by the source.
    // std::cout << "Received: " << selection_data.get_data() << std::endl;
    // std::cout << (*iter)[FontCollection.name] << std::endl;
    // Add the font into the collection.
    auto const font_name = Inkscape::FontLister::get_instance()->get_dragging_family();
    collections->add_font(collection_name, font_name);

    // Re-populate the collection.
    populate_fonts(collection_name);

    // Re-expand this row after re-population.
    if(is_expanded) {
        treeview->expand_to_path(path);
    }

    return true;
}

void FontCollectionSelector::on_selection_changed()
{
    Glib::RefPtr <Gtk::TreeSelection> selection = treeview->get_selection();
    if (!selection) return;

    FontCollections *font_collections = Inkscape::FontCollections::get();
    Gtk::TreeModel::iterator iter = selection->get_selected();
    if (!iter) return;

    auto parent = iter->parent();

    // We use 3 states to adjust the sensitivity of the edit and
    // delete buttons in the font collections manager dialog.
    int state = 0;

    // State -1: Selection is a system collection or a system
    // collection font.(Neither edit nor delete)

    // State 0: It's not a system collection or it's font. But it is
    // a user collection.(Both edit and delete).

    // State 1: It is a font that belongs to a user collection.
    // (Only delete)

    if(parent) {
        // It is a font, and thus it is not editable.
        // Now check if it's parent is a system collection.
        bool is_system = font_collections->find_collection((*parent)[FontCollection.name], true);
        state = (is_system) ? SYSTEM_COLLECTION: USER_COLLECTION_FONT;
    } else {
        // Check if it is a system collection.
        bool is_system = font_collections->find_collection((*iter)[FontCollection.name], true);
        state = (is_system) ? SYSTEM_COLLECTION: USER_COLLECTION;
    }

    signal_changed.emit(state);
}

} // namespace Inkscape::UI::Widget

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :

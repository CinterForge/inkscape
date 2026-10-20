// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This file defines the Font Collections class. A map takes care of all the collections.
 * On the hard disk the font collections are stored in the user profile path under the
 * "fontcollections" directory. Each collection file is a plain text file which is named
 * as "collection_name.txt" and contains the fonts contained in that collection. On
 * initializing the collections, it loads the font collections stored in the files and
 * their respective fonts.
 *
 * This file further contains all the necessary functions to create a new font collection,
 * update the fonts stored in a collection, rename a collection and deletion of collections
 * and their fonts.
 *
 * Authors:
 *   Vaibhav Malik <vaibhavmalik2018@gmail.com>
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#ifndef INK_COLLECTIONS_H
#define INK_COLLECTIONS_H

#include <set>
#include <string>
#include <vector>

#include <glib/gi18n.h>
#include <glibmm/ustring.h>
#include <sigc++/connection.h>
#include <sigc++/signal.h>

namespace Inkscape {

inline const std::string RECENTLY_USED_FONTS = _("Recently Used Fonts");
inline const std::string DOCUMENT_FONTS = _("Document Fonts");

struct FontCollection {
    Glib::ustring name;
    std::set <Glib::ustring> fonts;
    bool is_system;

    bool operator == (const FontCollection& ft) const { return name == ft.name;}
    bool operator < (const FontCollection& ft) const { return name < ft.name;}

    FontCollection(const Glib::ustring name, bool is_system): name(name), is_system(is_system) {}

    FontCollection(const Glib::ustring name, const std::set <Glib::ustring> fonts, bool is_system): name(name), fonts(fonts), is_system(is_system) {}

    void insert_font(const Glib::ustring &font_name)
    {
        fonts.insert(font_name);
    }
};

// The FontCollections class is a singleton class.
class FontCollections {
public:
    enum What {
        All,
        System,
        User
    };

    static FontCollections* get();
    ~FontCollections() = default;

    void init();
    void clear();
    // void print_collection_font_map();

    void read(std::vector<std::string> const &files, bool is_system = false);
    void write_collection(const Glib::ustring& collection_name, const std::set <Glib::ustring>& fonts, bool is_system = false);

    // System collections.
    // We just store the names of system collections here.
    // The logic to manage the system collections is written in separate files.
    void add_system_collections();

    // Add/remove user collections
    void add_collection(const Glib::ustring& collection_name, bool is_system = false);
    void remove_collection(const Glib::ustring& collection_name);
    void rename_collection(const Glib::ustring& old_name, const Glib::ustring& new_name);
    void rename_font(const Glib::ustring& collection_name, const Glib::ustring& old_name, const Glib::ustring& new_name);
    void add_font(const Glib::ustring& collection_name, const Glib::ustring& font_name);
    void remove_font(const Glib::ustring& collection_name, const Glib::ustring& font_name);
    void update_selected_collections(const Glib::ustring& collection_name);
    bool is_collection_selected(const Glib::ustring& collection_name);
    void clear_selected_collections();

    // Utility
    std::string& trim_left_and_right(std::string& s, const char* t = " \t\n\r\f\v");
    int get_user_collection_location(const Glib::ustring& collection_name);
    std::string generate_filename_from_collection(const Glib::ustring &collection_name, bool is_system);
    int get_collections_count(bool is_system = false) const;
    int get_selected_collections_count() const;
    bool find_collection(Glib::ustring const &collection_name, bool is_system = false) const;
    bool is_font_in_collection(const Glib::ustring& collection_name, const Glib::ustring& font_name) const;
    std::vector<Glib::ustring> get_collections(bool is_system = false) const;
    std::vector<Glib::ustring> get_all_collections() const;
    std::set<Glib::ustring> const &get_fonts(Glib::ustring const &name, bool is_system = false) const;
    const std::set<Glib::ustring>& get_selected_collections() const;

    // This signal will be emitted whenever there's a change in the font collections
    // This includes: Creating/deleting collection, and adding/deleting fonts.
    sigc::connection connect_update(sigc::slot <void ()> slot) {
        return update_signal.connect(slot);
    }

    // This signal will be emitted whenever the user selects or
    // un-selects a font collection.
    sigc::connection connect_selection_update(sigc::slot <void ()> slot) {
        return selection_update_signal.connect(slot);
    }

private:
    FontCollections();

    std::set <FontCollection> _system_collections;
    std::set <FontCollection> _user_collections;
    std::set <Glib::ustring> _selected_collections;

    void _read(std::string const &file, bool is_system = false);

    sigc::signal <void ()> update_signal;
    sigc::signal <void ()> selection_update_signal;
};

} // Namespace Inkscape

#endif // INK_COLLECTIONS_H

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

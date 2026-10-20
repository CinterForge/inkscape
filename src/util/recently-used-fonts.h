// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Recently used fonts are stored in a separate file in the fontcollections directory under the SYSTEM
 * path. Recently used fonts are managed as a list with the help of a list.
 *
 * Authors:
 *   Vaibhav Malik <vaibhavmalik2018@gmail.com>
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#ifndef INK_RECENTLY_USED_FONTS_H
#define INK_RECENTLY_USED_FONTS_H

#include <list>
#include <set>
#include <glibmm/ustring.h>
#include <sigc++/connection.h>
#include <sigc++/signal.h>

#include "io/dir-util.h"
#include "io/resource.h"

namespace Inkscape {

inline const char *RECENTFONTS_FILENAME = "recently_used_fonts.log";

class RecentlyUsedFonts {
public:
    enum What {
        All,
        System,
        User
    };

    static RecentlyUsedFonts* get();
    ~RecentlyUsedFonts() = default;

    // To load the last saved recent font list.
    void init();
    void clear();

    // void print_recently_used_fonts();

    // Read recently used fonts from file.
    void read(const Glib::ustring& file_name);
    void write_recently_used_fonts();

    void change_max_list_size(const int& max_size);
    void prepend_to_list(const Glib::ustring& font_name);
    void pop_front();
    bool get_continuous_streak() const;
    void set_continuous_streak(bool streak);

    std::list<Glib::ustring> const &get_fonts() const;
    int get_count() const;

    sigc::connection connectUpdate(sigc::slot <void ()> slot) {
        return update_signal.connect(slot);
    }

private:
    RecentlyUsedFonts();
    void _read(const Glib::ustring& file_name);
    void _write_recently_used_fonts();

    // This list will contain the recently used fonts queue.
    std::list <Glib::ustring> _recent_list;

    // Defines the maximum size the recently_used_font_list can have.
    // TODO: Add an option in the preferences to change the maximum size of
    // the recently used font list.
    int _max_size{};
    bool continuous_streak = false;

    sigc::signal <void ()> update_signal;
};

} // Namespace Inkscape

#endif // INK_RECENTLY_USED_FONTS_H

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

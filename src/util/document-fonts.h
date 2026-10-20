// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Header file that defines the singleton DocumentFonts class.
 * This is a singleton class.
 *
 * Authors:
 *   Vaibhav Malik <vaibhavmalik2018@gmail.com>
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 */

#ifndef INKSCAPE_UTIL_DOCUMENT_FONTS_H
#define INKSCAPE_UTIL_DOCUMENT_FONTS_H

#include <vector>
#include <map>
#include <set>
#include <glibmm/ustring.h>
#include <sigc++/connection.h>
#include <sigc++/signal.h>

namespace Inkscape {

class DocumentFonts
{
public:
    static DocumentFonts *get();

    void clear();
    // void print_document_fonts();
    void update_document_fonts(std::map<Glib::ustring, std::set<Glib::ustring>> const &font_data);
    std::set<Glib::ustring> const &get_fonts() const;

    // Signals
    sigc::connection connectUpdate(sigc::slot<void ()> slot) {
        return update_signal.connect(slot);
    }

private:
    DocumentFonts() = default;

    std::set<Glib::ustring> _document_fonts;

    // Signals
    sigc::signal<void ()> update_signal;
};

} // namespace Inkscape

#endif // INKSCAPE_UTIL_DOCUMENT_FONTS_H

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

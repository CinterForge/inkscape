// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This file contains document used fonts related logic. The functions to manage the
 * document fonts are defined in this file.
 *
 * Authors:
 *   Vaibhav Malik <vaibhavmalik2018@gmail.com>
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include "document-fonts.h"

// #include <iostream>

namespace Inkscape {

DocumentFonts *DocumentFonts::get()
{
    static DocumentFonts instance;
    return &instance;
}

void DocumentFonts::clear()
{
    _document_fonts.clear();
}

/*
void DocumentFonts::print_document_fonts()
{
    std::cout << std::endl << "********************" << std::endl;

    for(auto const& font: _document_fonts) {
        std::cout << font << std::endl;
    }

    std::cout << std::endl << "********************" << std::endl;
}
*/

void DocumentFonts::update_document_fonts(std::map<Glib::ustring, std::set<Glib::ustring>> const &font_data)
{
    // Clear the old fonts and then insert latest set.
    clear();

    // Iterate over all the fonts in this map,
    // and insert these fonts into the document_fonts.
    for(auto const &ele: font_data) {
        _document_fonts.insert(ele.first);
    }

    // Emit the update signal to keep everything consistent.
    update_signal.emit();
}

// Returns the fonts used in the document.
std::set<Glib::ustring> const &DocumentFonts::get_fonts() const
{
    return _document_fonts;
}

} // namespace Inkscape

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

// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file Object properties dialog.
 */
/*
 * Inkscape, an Open Source vector graphics editor
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Copyright (C) 2012 Kris De Gussem <Kris.DeGussem@gmail.com>
 * c++version based on former C-version (GPL v2+) with authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Johan Engelen <goejendaagh@zonnet.nl>
 *   Abhishek Sharma
 */

#ifndef SEEN_INKSCAPE_UI_DIALOG_OBJECTPROPERTIES_H
#define SEEN_INKSCAPE_UI_DIALOG_OBJECTPROPERTIES_H

#include "widgets/sp-attribute-widget.h"

class SPAttributeTable;
class SPItem;

namespace Gtk {
class Grid;
} // namespace Gtk

namespace Inkscape::UI::Dialog {

/**
 * A subdialog widget to show object "interactive" properties. Those are JavaScript event handlers.
 *
 * Note: This component is embedded in the ObjectAttributes dialog and not used on its own.
 *
 */
class ObjectProperties {
public:
    ObjectProperties();
    ~ObjectProperties() = default;

    SPAttributeTable* get_attr_table() { return _attr_table; }
    Gtk::Grid& get_grid() { return _attr_table->get_grid(); }

private:
    bool _blocked;
    SPItem *_current_item; //to store the current item, for not wasting resources
    std::vector<Glib::ustring> _int_attrs;
    std::vector<Glib::ustring> _int_labels;
    SPAttributeTable *_attr_table; //the widget for showing the on... names at the bottom
};

} // namespace Inkscape::UI::Dialog

#endif // SEEN_INKSCAPE_UI_DIALOG_OBJECTPROPERTIES_H

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

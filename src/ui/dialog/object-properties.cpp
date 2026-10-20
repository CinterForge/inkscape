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
 * c++ version based on former C-version (GPL v2+) with authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Johan Engelen <goejendaagh@zonnet.nl>
 *   Abhishek Sharma
 */

#include "object-properties.h"

#include <gtkmm/object.h>

#include "document.h"
#include "object/sp-item.h"
#include "ui/syntax.h"
#include "widgets/sp-attribute-widget.h"

namespace Inkscape::UI::Dialog {

ObjectProperties::ObjectProperties() :
     _blocked(false)
    , _current_item(nullptr)
    , _attr_table(Gtk::make_managed<SPAttributeTable>(Syntax::SyntaxMode::JavaScript))
{
    //initialize labels for the table at the bottom of the dialog
    _int_attrs.emplace_back("onclick");
    _int_attrs.emplace_back("onmouseover");
    _int_attrs.emplace_back("onmouseout");
    _int_attrs.emplace_back("onmousedown");
    _int_attrs.emplace_back("onmouseup");
    _int_attrs.emplace_back("onmousemove");
    _int_attrs.emplace_back("onfocusin");
    _int_attrs.emplace_back("onfocusout");
    _int_attrs.emplace_back("onload");

    _int_labels.emplace_back("Click");
    _int_labels.emplace_back("MouseOver");
    _int_labels.emplace_back("MouseOut");
    _int_labels.emplace_back("MouseDown");
    _int_labels.emplace_back("MouseUp");
    _int_labels.emplace_back("MouseMove");
    _int_labels.emplace_back("FocusIn");
    _int_labels.emplace_back("FocusOut");
    _int_labels.emplace_back("Load");

    _attr_table->create(_int_labels, _int_attrs);
}

} // namespace Inkscape::UI::Dialog

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

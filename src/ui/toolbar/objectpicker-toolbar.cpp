// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Object picker toolbar
 *//*
 * Authors:
 * see git history
 * Vaibhav Malik <vaibhavmalik2018@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "objectpicker-toolbar.h"

#include <gtkmm/box.h>

#include "ui/builder-utils.h"

namespace Inkscape::UI::Toolbar {

ObjectPickerToolbar::ObjectPickerToolbar()
    : Toolbar{get_widget<Gtk::Box>(create_builder("toolbar-objectpicker.ui"), "objectpicker-toolbar")}
{
    _initMenuBtns();
}

} // namespace Inkscape::UI::Toolbar

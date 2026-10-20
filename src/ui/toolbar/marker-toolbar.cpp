// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Marker edit mode toolbar - onCanvas marker editing of marker orientation, position, scale
 *//*
 * Authors:
 * see git history
 * Rachana Podaralla <rpodaralla3@gatech.edu>
 * Vaibhav Malik <vaibhavmalik2018@gmail.com>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "marker-toolbar.h"

#include <gtkmm/box.h>

#include "ui/builder-utils.h"

namespace Inkscape::UI::Toolbar {

MarkerToolbar::MarkerToolbar()
    : Toolbar{get_widget<Gtk::Box>(create_builder("toolbar-marker.ui"), "marker-toolbar")}
{
    _initMenuBtns();
}

} // namespace Inkscape::UI::Toolbar

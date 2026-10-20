// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Separator widget for extensions
 *//*
 * Authors:
 *   Patrick Storz <eduard.braun2@gmx.de>
 *
 * Copyright (C) 2019 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "widget-separator.h"

#include <gtkmm/separator.h>

#include "xml/node.h"
#include "extension/extension.h"

namespace Inkscape {
namespace Extension {


WidgetSeparator::WidgetSeparator(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext)
    : InxWidget(xml, ext)
{
}

/** \brief  Create a label for the description */
Gtk::Widget *WidgetSeparator::get_widget(sigc::signal<void ()> *changeSignal)
{
    if (_hidden) {
        return nullptr;
    }

    auto const separator = Gtk::make_managed<Gtk::Separator>();
    separator->set_visible(true);
    return separator;
}

}  /* namespace Extension */
}  /* namespace Inkscape */

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief New From Template main dialog
 */
/* Authors:
 *   Jan Darowski <jan.darowski@gmail.com>, supervised by Krzysztof Kosiński
 *
 * Copyright (C) 2013 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_SEEN_UI_DIALOG_NEW_FROM_TEMPLATE_H
#define INKSCAPE_SEEN_UI_DIALOG_NEW_FROM_TEMPLATE_H

#include <gtkmm/dialog.h>
#include <gtkmm/button.h>
#include <glibmm/i18n.h>

#include "ui/widget/document-templates.h"

namespace Inkscape::UI {

class NewFromTemplate : public Gtk::Dialog
{
public:
    static void load_new_from_template(Gtk::Window& parent);
    ~NewFromTemplate() override = default;

private:
    NewFromTemplate(Gtk::Window& parent);
    UI::Widget::DocumentTemplates _list;
    Gtk::Button _create_template_button{_("Create from template")};
    Gtk::Button _cancel{_("Cancel")};

    void _createFromTemplate();
    void _onClose(int responseCode);
};

} // namespace

#endif

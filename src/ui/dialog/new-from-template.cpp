// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief New From Template main dialog - implementation
 */
/* Authors:
 *   Jan Darowski <jan.darowski@gmail.com>, supervised by Krzysztof Kosiński
 *   Martin Owens, Mike Kowalski
 *
 * Copyright (C) 2013 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "new-from-template.h"

#include "desktop.h"
#include "inkscape-application.h"
#include "inkscape.h"
#include "preferences.h"
#include "ui/dialog-run.h"

namespace Inkscape::UI {

NewFromTemplate::NewFromTemplate(Gtk::Window& parent) : Dialog(_("New From Template"), parent, true)
{
    auto size = Preferences::get()->getPoint("/dialogs/now-from-template/size", Geom::Point(750, 500));
    set_default_size(size.x(), size.y());

    auto& templates = _list.templates();
    templates.init(Extension::TEMPLATE_NEW_FROM, UI::Widget::TemplateList::All);
    set_child(_list);

    set_default_widget(_create_template_button);

    _cancel.add_css_class("dialog-cmd-button");
    _list.add_button(_cancel, UI::Widget::DocumentTemplates::End);
    _list.add_button(_create_template_button, UI::Widget::DocumentTemplates::End);

    _create_template_button.signal_clicked().connect([this]{ _createFromTemplate(); });
    // deal with disabling "create" button as selection changes
    _create_template_button.set_sensitive(_list.templates().has_selected_preset());
    templates.connectItemSelected([this](int pos) { _create_template_button.set_sensitive(pos >= 0); });
    templates.connectItemActivated(sigc::mem_fun(*this, &NewFromTemplate::_createFromTemplate));
    templates.signal_switch_page().connect([this](auto& name) {
        _create_template_button.set_sensitive(_list.templates().has_selected_preset());
    });

    // remember dialog size
    signal_response().connect([this](int i) {
        int width = 0, height = 0;
        get_default_size(width, height);
        if (width > 0 && height > 0) {
            Preferences::get()->setPoint("/dialogs/now-from-template/size", Geom::Point(width, height));
        }
        // todo: remember current page and current template
        //
    });

    _cancel.signal_clicked().connect([this]{ _onClose(Gtk::ResponseType::CANCEL); });

    set_transient_for(parent);
    set_visible();
    _list.templates().focus();
}

void NewFromTemplate::_createFromTemplate()
{
    SPDesktop *old_desktop = SP_ACTIVE_DESKTOP;

    auto doc = _list.templates().new_document();

    // Cancel button was pressed.
    if (!doc)
        return;

    auto app = InkscapeApplication::instance();
    app->desktopOpen(doc);

    if (old_desktop)
        old_desktop->clearWaitingCursor();

    _onClose(0);
}

void NewFromTemplate::_onClose(int responseCode)
{
    response(responseCode);
}

void NewFromTemplate::load_new_from_template(Gtk::Window& parent)
{
    NewFromTemplate dl(parent);
    UI::dialog_run(dl);
}

} // namespace

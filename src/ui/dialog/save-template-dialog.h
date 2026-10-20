// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * "Save document as template" dialog
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2017 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef INKSCAPE_SEEN_UI_DIALOG_SAVE_TEMPLATE_H
#define INKSCAPE_SEEN_UI_DIALOG_SAVE_TEMPLATE_H

#include <glibmm/i18n.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/dialog.h>
#include <gtkmm/sizegroup.h>
#include <ui/operation-blocker.h>

#include "ui/widget/document-templates.h"
#include "ui/widget/generic/spin-button.h"

namespace Gtk {
class SizeGroup;
}
namespace Inkscape::UI::Dialog {

class SaveTemplate : Gtk::Dialog {
public:
    SaveTemplate(Gtk::Window& parent);

    static void save_document_as_template(Gtk::Window &parentWindow);

private:
    void update_save_widgets();
    void save_template(Gtk::Window &parent);

    Gtk::Grid _content;
    Gtk::CheckButton _set_as_default{_("Set as default template")};
    Gtk::Label _name{_("_Name"), true};
    Gtk::Entry _filename;
    Gtk::Label _desc_label{_("_Description"), true};
    Gtk::Entry _description;
    UI::Widget::DocumentTemplates _list;
    Gtk::Button _save{_("Save")};
    Gtk::Button _cancel{_("Cancel")};
    Glib::RefPtr<Gtk::SizeGroup> _btn_group = Gtk::SizeGroup::create(Gtk::SizeGroup::Mode::HORIZONTAL);
    OperationBlocker _update;
    std::shared_ptr<Extension::TemplatePreset> _current_preset;
};

} // namespace Inkscape:UI::Dialog

#endif // INKSCAPE_SEEN_UI_DIALOG_SAVE_TEMPLATE_H

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

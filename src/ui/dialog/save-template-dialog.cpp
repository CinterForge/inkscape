// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * "Save document as template" dialog
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "save-template-dialog.h"

#include "file.h"
#include "ui/dialog-run.h"

namespace Inkscape::UI::Dialog {

SaveTemplate::SaveTemplate(Gtk::Window& parent) : Dialog(_("Save as Template"), parent, true) {
    set_default_size(600, 400);

    _name.set_halign(Gtk::Align::END);
    _desc_label.set_halign(Gtk::Align::END);
    _content.set_column_spacing(4);
    _content.set_row_spacing(4);
    _content.set_margin_top(16);
    _content.set_margin_bottom(8);
    // lame attempt to align content optically:
    _content.set_margin_start(60);
    _content.set_margin_end(100);
    _content.attach(_name, 0, 0);
    _content.attach(_filename, 1, 0);
    _content.attach(_desc_label, 0, 1);
    _content.attach(_description, 1, 1);
    _filename.set_hexpand();
    _filename.set_max_length(250);
    _filename.set_input_hints(Gtk::InputHints::NO_SPELLCHECK);
    _filename.signal_changed().connect([this]{ update_save_widgets(); });
    _description.set_hexpand();

    auto& templates = _list.templates();
    templates.init(Extension::TEMPLATE_NEW_FROM, UI::Widget::TemplateList::Custom);
    _list.show_page_selector(false);
    _list.show_header(false);
    _list.set_content(_content);
    set_child(_list);

    _save.add_css_class("dialog-cmd-button");
    _btn_group->add_widget(_cancel);
    _btn_group->add_widget(_save);
    _set_as_default.set_tooltip_text(_("Base every other new document on this template"));
    _list.add_button(_set_as_default, UI::Widget::DocumentTemplates::Start);
    _list.add_button(_cancel, UI::Widget::DocumentTemplates::End);
    _list.add_button(_save, UI::Widget::DocumentTemplates::End);

    templates.connectItemSelected([this](int pos) { update_save_widgets(); });
    _cancel.signal_clicked().connect([this]{ response(Gtk::ResponseType::CANCEL); });
    _save.signal_clicked().connect([this]{ response(Gtk::ResponseType::OK); });

    update_save_widgets();
    set_default_widget(_save);
    set_transient_for(parent);
    set_visible();
    _filename.grab_focus();
}

void SaveTemplate::update_save_widgets() {
    if (_update.pending()) return;

    auto scope = _update.block();

    bool enable_save = false;
    bool enable_name = false;

    auto preset = _list.templates().get_selected_preset();

    if (_current_preset != preset) {
        _current_preset = preset;

        if (_list.templates().has_selected_new_template()) {
            _filename.set_text("");
            _description.set_text("");
        }
        else if (preset) {
            _filename.set_text(preset->get_name());
            _description.set_text(preset->get_description());
        }
    }

    if (_list.templates().has_selected_new_template()) {
        // saving a new template?
        bool has_text = _filename.get_text_length() > 0;
        enable_name = true;
        enable_save = has_text;
    }
    else if (preset) {
        // overwriting existing template
        enable_name = false;
        enable_save = true;
    }

    _save.set_label(preset ? _("Overwrite") : _("Save"));
    _save.set_sensitive(enable_save);
    _filename.set_sensitive(enable_name);
    _name.set_sensitive(enable_name);
}

void SaveTemplate::save_template(Gtk::Window &parent) {
    auto fname = _filename.get_text();
    auto description = _description.get_text();
    auto set_default_template = _set_as_default.get_active();
    // not used currently:
    Glib::ustring author, keywords;

    if (auto preset = _list.templates().get_selected_preset()) {
        sp_file_save_template(parent, preset->get_name(), author, description, keywords, set_default_template);
    }
    else if (!fname.empty()) {
        sp_file_save_template(parent, fname, author, description, keywords, set_default_template);
    }
}

void SaveTemplate::save_document_as_template(Gtk::Window &parent) {

    SaveTemplate dialog(parent);
    int response = UI::dialog_run(dialog);

    switch (response) {
    case Gtk::ResponseType::OK:
        dialog.save_template(parent);
        break;
    default:
        break;
    }

    dialog.close();
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

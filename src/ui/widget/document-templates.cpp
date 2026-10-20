// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 1/23/2025.
//

#include "document-templates.h"

#include <glib/gi18n.h>
#include <gtkmm/separator.h>

namespace Inkscape::UI::Widget {

DocumentTemplates::DocumentTemplates() {
    set_expand();

    _header.set_hexpand();
    attach(_header, 0, 0, 3);
    _hint.set_valign(Gtk::Align::CENTER);
    _hint.set_text(_("Select a template"));
    _hint.set_margin_start(8);
    _search.set_halign(Gtk::Align::END);
    _search.set_margin(8);
    _search.signal_search_changed().connect([this](){
        _templates.filter(_search.get_text());
    });
    _find.set_text_with_mnemonic(_("_Find"));
    _find.set_use_underline();
    _find.set_halign(Gtk::Align::END);
    _find.set_hexpand();
    _find.set_mnemonic_widget(_search);
    _header.append(_hint);
    _header.append(_find);
    _header.append(_search);

    attach(*Gtk::make_managed<Gtk::Separator>(), 0, 1, 3);

    attach(_categories, 0, 2);
    _templates.set_expand();
    attach(_templates, 2, 2);
    _categories.set_stack(_templates);
    _categories.add_css_class("compact-stack-sidebar");

    attach(*Gtk::make_managed<Gtk::Separator>(), 0, 3, 3);

    _footer.set_margin(8);
    _footer.set_start_widget(_start);
    _start.set_spacing(8);
    _footer.set_end_widget(_end);
    _end.set_spacing(8);
    _footer.set_hexpand();
    attach(_footer, 0, 5, 3);
}

void DocumentTemplates::add_button(Gtk::Widget& button, ButtonLocation pos) {
    if (pos == Start) {
        _start.append(button);
    }
    else if (pos == End) {
        _end.append(button);
    }
}

void DocumentTemplates::show_page_selector(bool show) {
    _categories.set_visible(show);
}

void DocumentTemplates::show_header(bool show) {
    _header.set_visible(show);
}

void DocumentTemplates::set_content(Gtk::Widget& widget) {
    attach(widget, 0, 4, 3);
}

} // namespace

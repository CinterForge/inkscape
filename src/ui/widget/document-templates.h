// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 1/23/2025.
//

#ifndef DOCUMENT_TEMPLATES_H
#define DOCUMENT_TEMPLATES_H

#include <gtkmm/box.h>
#include <gtkmm/grid.h>
#include <gtkmm/searchentry2.h>
#include <gtkmm/stacksidebar.h>

#include "template-list.h"

namespace Inkscape::UI::Widget {

class DocumentTemplates : public Gtk::Grid {
public:
    DocumentTemplates();

    TemplateList& templates() { return _templates; }
    enum ButtonLocation {Start, Center, End};
    void add_button(Gtk::Widget& button, ButtonLocation pos);
    void show_page_selector(bool show);
    void show_header(bool show);
    void set_content(Gtk::Widget& widget);

private:
    Gtk::Box _header;
    Gtk::Label _hint;
    Gtk::Label _find;
    Gtk::SearchEntry2 _search;
    Gtk::StackSidebar _categories;
    TemplateList _templates;
    // Gtk::Box _footer;
    Gtk::Box _start;
    Gtk::Box _end;
    Gtk::CenterBox _footer;
};

} // namespace

#endif //DOCUMENT_TEMPLATES_H

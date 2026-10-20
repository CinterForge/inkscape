// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 8/6/25.
//

#ifndef CHARACTER_VIEWER_H
#define CHARACTER_VIEWER_H

#include <glibmm/refptr.h>
#include <gtkmm/box.h>
#include <gtkmm/builder.h>

#include "libnrtype/font-instance.h"

namespace Inkscape::UI::Widget {
class DropDownList;
}

namespace Gtk {
class Scale;
class Label;
class DrawingArea;
class SearchEntry2;
}

namespace Inkscape::UI::Widget {
class SimpleGrid;

class CharacterViewer : public Gtk::Box {
public:
    CharacterViewer();

    void set_font(FontInstance* font, const Glib::ustring& name);

    sigc::signal<void (const Glib::ustring&)>& signal_insert_text() {
        return _signal_insert_text;
    }

private:
    void refresh();
    void show_characters(std::uint32_t from, std::uint32_t to, Glib::ustring filter);
    Glib::RefPtr<Gtk::Builder> _builder;
    SimpleGrid& _char_grid;
    FontInstance* _font = nullptr;
    std::vector<FontInstance::CharInfo> _characters;
    int _cell_size = 30;
    Gtk::DrawingArea& _glyph_image;
    int _current_cell = -1;
    Gtk::Label& _char_name;
    Gtk::Label& _font_name;
    Gtk::SearchEntry2& _search;
    DropDownList& _range_selector;
    Gtk::Scale& _char_size_scale;
    sigc::signal<void (const Glib::ustring&)> _signal_insert_text;
};

} // namespace

#endif //CHARACTER_VIEWER_H

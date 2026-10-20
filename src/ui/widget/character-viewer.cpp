// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 8/6/25.
//

#include "character-viewer.h"

#include <format>
#include <cairomm/surface.h>
#include <giomm/file.h>
#include <gtkmm/grid.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/label.h>
#include <gtkmm/scale.h>
#include <gtkmm/searchentry2.h>
#include <gtkmm/snapshot.h>

#include "drop-down-list.h"
#include "preferences.h"
#include "io/resource.h"
#include "libnrtype/font-instance.h"
#include "ui/builder-utils.h"
#include "ui/util.h"
#include "ui/widget/generic/simple-grid.h"
#include "util/drawing-utils.h"
#include "util/glyph-draw.h"
#include "util/unicode.h"

namespace Inkscape::UI::Widget {

static std::array char_sizes = {
    20, 25, 30, 35, 40, 50, 60
};

CharacterViewer::CharacterViewer():
    _builder(create_builder("character-viewer.ui")),
    _char_grid(get_derived_widget<SimpleGrid>(_builder, "cmap-glyphs")),
    _glyph_image(get_widget<Gtk::DrawingArea>(_builder, "glyph-image")),
    _char_name(get_widget<Gtk::Label>(_builder, "char-name")),
    _font_name(get_widget<Gtk::Label>(_builder, "font-name")),
    _search(get_widget<Gtk::SearchEntry2>(_builder, "search-entry")),
    _range_selector(get_derived_widget<DropDownList>(_builder, "ranges")),
    _char_size_scale(get_widget<Gtk::Scale>(_builder, "char-size-scale"))
{
    _search.signal_changed().connect([this] { refresh(); });

    for (int i = 0; i < char_sizes.size(); i++) {
        _char_size_scale.add_mark(i, Gtk::PositionType::TOP, {});
    }
    _char_size_scale.set_format_value_func([](double value) {
        return Glib::ustring::format(char_sizes[static_cast<int>(value)]);
    });
    auto char_size = Preferences::get()->getInt("/options/charmap/char-size", _cell_size);
    if (auto pos = std::ranges::find(char_sizes, char_size); pos != char_sizes.end()) {
        _char_size_scale.set_value(pos - char_sizes.begin());
    }
    else {
        _char_size_scale.set_value(2);
        char_size = char_sizes[2];
    }
    _cell_size = char_size;
    _char_size_scale.signal_value_changed().connect([this] {
        auto size = char_sizes[_char_size_scale.get_value()];
        _char_grid.set_cell_size(size, size);
        Preferences::get()->setInt("/options/charmap/char-size", size);
    });

    _range_selector.set_button_max_chars(20); // limit how wide dropdown can get
    _range_selector.set_ellipsize_button(true);
    _range_selector.enable_search();
    for (auto& range : Util::get_unicode_ranges()) {
        _range_selector.append(range.name);
    }
    _range_selector.set_selected(0);
    _range_selector.signal_changed().connect([this] { refresh(); });

    _glyph_image.set_draw_func([this](const Cairo::RefPtr<Cairo::Context>& ctx, int width, int height) {
        if (!_font || _current_cell < 0 || _characters.empty()) return;

        auto [unicode, glyph_index] = _characters.at(_current_cell);
        auto fg = get_color();
        Gdk::RGBA line{fg.get_red(), fg.get_green(), fg.get_blue(), 0.15};

        Util::draw_glyph({
            .font = _font,
            .font_size = 0, // auto
            .glyph_index = glyph_index,
            .ctx = ctx,
            .rect = Geom::IntRect::from_xywh(0, 0, width, height),
            .glyph_color = fg,
            .line_color = line,
            .draw_metrics = true,
            .draw_background = false
        });
    });

    _char_grid.set_cell_size(_cell_size, _cell_size);
    _char_grid.set_gap(1, 1);
    _char_grid.set_can_focus();
    _char_grid.set_focusable();
    _char_grid.set_focus_on_click();
    _char_grid.connect_cell_selected([this](int index) {
        _current_cell = index;
        auto [unicode, glyph_index] = _characters.at(index);
        auto result = std::format("\nU+{:04X}\n\n{:s}", unicode, Util::get_unicode_name(unicode));
        _char_name.set_text(result);
        _glyph_image.queue_draw();
    });
    _char_grid.connect_cell_open([this](int index) {
        if (!_font) return;

        auto [unicode, glyph_index] = _characters.at(index);
        if (unicode) {
            char u[10];
            auto const len = g_unichar_to_utf8(unicode, u);
            u[len] = '\0';
            _signal_insert_text.emit(u);
        }
    });
    _char_grid.set_draw_func([this](const Glib::RefPtr<Gtk::Snapshot>& snapshot, std::uint32_t index, const Geom::IntRect& rect, bool selected) {
        if (!_font) return;

        Gdk::RGBA bg(0.05, 0.43, 1);
        auto fg = get_color();
        if (selected) {
            auto style = get_style_context();
            fg = Util::lookup_selected_foreground_color(style).value_or(fg);
            bg = Util::lookup_selected_background_color(style).value_or(bg);
        }

        int scale = get_scale_factor();
        auto surface = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, rect.width() * scale, rect.height() * scale);
        surface->set_device_scale(scale);
        auto ctx = Cairo::Context::create(surface);
        // draw single glyph
        auto [unicode, glyph_index] = _characters.at(index);
        Util::draw_glyph({
            .font = _font,
            .glyph_index = glyph_index,
            .ctx = ctx,
            .rect = Geom::IntRect::from_xywh(0, 0, rect.width(), rect.height()),
            .glyph_color = fg,
            .background_color = bg,
            .draw_metrics = false,
            .draw_background = selected
        });
        snapshot->append_texture(to_texture(surface), Gdk::Graphene::Rect(rect.left(), rect.top(), rect.width(), rect.height()));
    });

    _char_grid.connect_tooltip([this](int index) {
        if (_font) {
            auto [unicode, glyph_index] = _characters.at(index);
            return Glib::ustring(Util::get_unicode_name(unicode));
        }
        return Glib::ustring();
    });

    append(get_widget<Gtk::Grid>(_builder, "main-grid"));
}

void CharacterViewer::set_font(FontInstance* font, const Glib::ustring& name) {
    _font = font;
    auto n = _range_selector.get_selected();
    auto& ranges = Util::get_unicode_ranges();
    if (n >= ranges.size()) {
        n = 0;
    }
    show_characters(ranges[n].from, ranges[n].to, _search.get_text());

    _font_name.set_text({});
    _font_name.set_tooltip_text({});
    if (font) {
        _font_name.set_text(name);
        _font_name.set_tooltip_text(name);
    }
    else {
        _char_grid.clear();
    }
}

void CharacterViewer::refresh() {
    // filter by range and Unicode name
    auto n = _range_selector.get_selected();
    auto& ranges = Util::get_unicode_ranges();
    if (n >= ranges.size()) {
        n = 0;
    }
    show_characters(ranges[n].from, ranges[n].to, _search.get_text());
}

void CharacterViewer::show_characters(std::uint32_t from, std::uint32_t to, Glib::ustring filter) {
    _characters.clear();
    _current_cell = -1;
    _char_grid.set_cell_count(0);
    _char_name.set_text({});
    _glyph_image.queue_draw();

    if (_font) {
        auto characters = _font->find_all_characters(from, to);

        if (!filter.empty()) {
            filter = filter.uppercase();
            std::copy_if(characters.begin(), characters.end(), std::back_inserter(_characters), [&filter](const FontInstance::CharInfo& info) {
                return Util::get_unicode_name(info.unicode).find(filter) != std::string::npos;
            });
        }
        else {
            _characters = std::move(characters);
        }
        _char_grid.set_cell_count(_characters.size());
    }
}

} // namespace


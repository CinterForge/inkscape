// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Entry widget for typing color value in css form
 *//*
 * Authors:
 *   Tomasz Boczkowski <penginsbacon@gmail.com>
 *
 * Copyright (C) 2014 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/i18n.h>

#include "color-entry.h"

#include "colors/spaces/gamut.h"
#include "colors/spaces/base.h"
#include "svg/css-ostringstream.h"

namespace Inkscape::UI::Widget {

ColorEntry::ColorEntry(std::shared_ptr<Colors::ColorSet> colors)
    : _colors(std::move(colors))
    , _updating(false)
    , _updatingrgba(false)
    , _prevpos(0)
{
    set_name("ColorEntry");

    _color_changed_connection = _colors->signal_changed.connect(sigc::mem_fun(*this, &ColorEntry::_onColorChanged));
    signal_activate().connect(sigc::mem_fun(*this, &ColorEntry::_onColorChanged));
    get_buffer()->signal_inserted_text().connect(sigc::mem_fun(*this, &ColorEntry::_inputCheck));
    _onColorChanged();

    // add extra character for pasting a hash, '#11223344'
    set_max_length(9);
    set_max_width_chars(9);
    set_width_chars(8);
    set_tooltip_text(_("Hexadecimal RGB value of the color"));
}

ColorEntry::~ColorEntry()
{
    _color_changed_connection.disconnect();
}

void ColorEntry::_inputCheck(guint pos, const gchar * /*chars*/, guint n_chars)
{
    // remember position of last character, so we can remove it.
    // we only overflow by 1 character at most.
    _prevpos = pos + n_chars - 1;
}

void ColorEntry::on_changed()
{
    if (_updating || _updatingrgba) {
        return;
    }

    auto text = get_text();
    // If it looks like a plain hex string (e.g., "ff00ff") add a '#' 
    // so both "ff00ff" and "#ff00ff" are accepted.
    if (looksLikeHex(text)) {
        text = "#" + text;
    }

    auto new_color = Colors::Color::parse(text);

    _updatingrgba = true;
    if (new_color) {
        if (auto color = _colors->get()) {
            new_color->setOpacity(color->getOpacity());
        }
        _colors->setAll(*new_color);
    }
    _updatingrgba = false;
}


void ColorEntry::_onColorChanged()
{
    if (_updatingrgba) {
        return;
    }

    if (_colors->isEmpty()) {
        set_text(_("N/A"));
        return;
    }

    auto color = *_colors->getAverage().converted(Colors::Space::Type::RGB);
    if (Colors::out_of_gamut(color, color.getSpace())) {
        // out of sRGB gamut warning
        auto r = color[0];
        auto g = color[1];
        auto b = color[2];
        CSSOStringStream rgb;
        // high precision, so we show values just barely above/below limits
        rgb.precision(2);
        rgb << "rgb(" << 100 * r << "% " << 100 * g << "% " << 100 * b << "%)";
        _signal_out_of_gamut.emit(Glib::ustring::compose(_("Color %1 is out of sRGB gamut.\nIt has been mapped to sRGB gamut."), rgb.str().c_str()));
        _warning = true;
        color = Colors::to_gamut_css(color, color.getSpace());
    }
    else if (_warning) {
        // clear warning
        _warning = false;
        _signal_out_of_gamut.emit({});
    }
    auto text = color.toString(false);

    if (get_text().raw() != text) {
        _updating = true;
        set_text(text);
        _updating = false;
    }
}

// Checks if a text looks like a hex color code
// for example, returns true for "fff" or "ff00ff",
// but false for actual hex codes (like "#fff" or "#ff00ff")
// or any invalid hex strings.
bool ColorEntry::looksLikeHex(const Glib::ustring &text) const
{
    if (text.empty() || text[0] == '#') {
        return false;
    }

    auto len = text.size();
    if (len != 3 && len != 4 && len != 6 && len != 8) {
        return false;
    }

    return std::all_of(text.begin(), text.end(), [](unsigned char c) {
        return std::isxdigit(c);
    });
}

} // namespace

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

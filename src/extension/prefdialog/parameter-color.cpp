// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2005-2007 Authors:
 *   Ted Gould <ted@gould.cx>
 *   Johan Engelen <johan@shouraizou.nl>
 *   Christopher Brown <audiere@gmail.com>
 *   Jon A. Cruz <jon@joncruz.org>
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstdio>
#include <cstdlib>
#include <string>
#include <gtkmm/box.h>
#include <gtkmm/colorbutton.h>
#include <gtkmm/label.h>

#include "parameter-color.h"

#include "colors/manager.h"
#include "extension/extension.h"
#include "preferences.h"
#include "ui/pack.h"
#include "ui/util.h"
#include "ui/widget/color-notebook.h"
#include "xml/node.h"

namespace Inkscape::Extension {

ParamColor::ParamColor(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext)
    : InxParameter(xml, ext)
    , _colors(std::make_shared<Colors::ColorSet>())
{
    if (xml->firstChild()) {
        if (auto parsed = Colors::Color::parse(xml->firstChild()->content())) {
            _colors->set(*parsed);
        }
    }
    if (_colors->isEmpty()) {
        auto prefs = Preferences::get();
        _colors->set(prefs->getColor(pref_name()));
    }

    _color_changed = _colors->signal_changed.connect(sigc::mem_fun(*this, &ParamColor::_onColorChanged));

    // parse appearance
    if (_appearance) {
        if (!strcmp(_appearance, "colorbutton")) {
            _mode = COLOR_BUTTON;
        } else {
            g_warning("Invalid value ('%s') for appearance of parameter '%s' in extension '%s'",
                      _appearance, _name, _extension->get_id());
        }
    }
}

ParamColor::~ParamColor()
{
    _color_changed.disconnect();
}

Gtk::Widget *ParamColor::get_widget(sigc::signal<void ()> *changeSignal)
{
    if (_hidden) {
        return nullptr;
    }

    if (changeSignal) {
        _changeSignal = std::make_unique<sigc::signal<void ()>>(*changeSignal);
    }

    auto const hbox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, GUI_PARAM_WIDGETS_SPACING);
    if (_mode == COLOR_BUTTON) {
        auto const label = Gtk::make_managed<Gtk::Label>(_text, Gtk::Align::START);
        // to ensure application of alignment
        // for some reason set_align is not enough
        label->set_xalign(0);
        label->set_visible(true);
        UI::pack_start(*hbox, *label, true, true);

        // TODO: It would be nicer to have a custom Gtk::ColorButton() implementation here,
        //       that wraps an Inkscape::UI::Widget::ColorNotebook into a new dialog
        auto color = _colors->get();
        if (!color)
            color = Colors::Color::parse("magenta");
        _color_button = Gtk::make_managed<Gtk::ColorButton>(color_to_rgba(*color));
        _color_button->set_title(_text);
        _color_button->set_use_alpha();
        _color_button->set_visible(true);
        UI::pack_end(*hbox, *_color_button, false, false);

        _color_button->signal_color_set().connect(sigc::mem_fun(*this, &ParamColor::_onColorButtonChanged));
    } else {
        Gtk::Widget *selector = Gtk::make_managed<Inkscape::UI::Widget::ColorNotebook>(_colors);
        UI::pack_start(*hbox, *selector, true, true);
        selector->set_visible(true);
    }

    hbox->set_visible(true);
    return hbox;

}

void ParamColor::_onColorChanged()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (auto color = _colors->get()) {
        prefs->setColor(pref_name(), *color);

        if (_changeSignal)
            _changeSignal->emit();
    }
}

void ParamColor::_onColorButtonChanged()
{
    set(Colors::Color(to_guint32(_color_button->get_rgba())));
}

std::string ParamColor::value_to_string() const
{
    return get().toString(true);
}

void ParamColor::string_to_value(const std::string &in)
{
    // Parse color. If parsing fails, return black.
    static const auto black = Colors::Color(0, false);
    set(Colors::Color::parse(in).value_or(black));
}

} // namespace Inkscape::Extension

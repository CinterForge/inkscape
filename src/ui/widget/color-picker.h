// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief Color picker button and window.
 */
/* Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Ralf Stephan <ralf@ark.in-berlin.de>
 *
 * Copyright (C) Authors 2000-2005
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_COLOR_PICKER_H
#define SEEN_INKSCAPE_COLOR_PICKER_H

#include "labelled.h"

#include "colors/color-set.h"
#include "ui/widget/color-preview.h"
#include <gtkmm/menubutton.h>

namespace Gtk {
class Builder;
}

namespace Inkscape::UI::Widget {

class ColorNotebook;

class ColorPicker : public Gtk::MenuButton {
public:
    [[nodiscard]] ColorPicker(Glib::ustring title,
                              Glib::ustring const &tip,
                              Colors::Color const &initial,
                              bool undo,
                              bool use_transparency = true);

    ColorPicker(BaseObjectType *cobject, Glib::RefPtr<Gtk::Builder> const &,
                Glib::ustring title, bool use_transparency = true);

    // custom popup content
    ColorPicker(Gtk::Widget& popup_content, const Glib::ustring& tip);

    ~ColorPicker() override;

    void setColor(Colors::Color const &);
    void open();
    void close();
    void setTitle(Glib::ustring title);

    sigc::connection connectChanged(sigc::slot<void (Colors::Color const &)> slot)
    {
        return _changed_signal.connect(std::move(slot));
    }

    [[nodiscard]] Colors::Color get_current_color() const {
        if (_colors->isEmpty())
            return Colors::Color(0x0);
        return _colors->getAverage();
    }

    sigc::signal<void (void)> signal_open_popup() { return _signal_open; }

    void set_icon(const Glib::ustring& icon_name);

private:
    void _onSelectedColorChanged();
    virtual void on_changed(Colors::Color const &);
    void _construct(Gtk::Widget* content);
    void set_preview(std::uint32_t rgba);

    std::unique_ptr<ColorPreview> _preview;
    Glib::ustring _title;
    sigc::signal<void (Colors::Color const &)> _changed_signal;
    bool          _undo     = false;
    bool          _updating = false;
    std::shared_ptr<Colors::ColorSet> _colors;
    Gtk::Popover _popup;
    ColorNotebook* _color_selector = nullptr;
    sigc::signal<void (void)> _signal_open;
};


class LabelledColorPicker : public Labelled {
public:
    [[nodiscard]] LabelledColorPicker(Glib::ustring const &label,
                                      Glib::ustring const &title,
                                      Glib::ustring const &tip,
                                      Colors::Color const &initial,
                                      bool undo)
        : Labelled(label, tip, new ColorPicker(title, tip, initial, undo)) {
        property_sensitive().signal_changed().connect([this](){
            getWidget()->set_sensitive(is_sensitive());
        });
    }

    void setColor(Colors::Color const &color)
    {
        static_cast<ColorPicker *>(getWidget())->setColor(color);
    }

    void closeWindow()
    {
        static_cast<ColorPicker *>(getWidget())->close();
    }

    sigc::connection connectChanged(sigc::slot<void (Colors::Color const &)> slot)
    {
        return static_cast<ColorPicker*>(getWidget())->connectChanged(std::move(slot));
    }
};

} // namespace Inkscape::UI::Widget

#endif // SEEN_INKSCAPE_COLOR_PICKER_H

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

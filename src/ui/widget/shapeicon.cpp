// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Martin Owens
 *
 * Copyright (C) 2020 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

 #include <gtkmm/icontheme.h>
 #include <gtkmm/snapshot.h>

#include "ui/widget/shapeicon.h"

#include "ui/icon-loader.h"
#include "ui/util.h"

namespace Inkscape::UI::Widget {

static const int ICON_SIZE = 16;

void CellRendererItemIcon::update_color()
{
    _icon_color = _property_color.get_value();
    if (!_icon_color && _widget_color) {
        _icon_color = _widget_color;
    }
}

void CellRendererItemIcon::update_shape()
{
    std::string shape_type = _property_shape_type.get_value();
    if (shape_type == "-") { // "-" is an explicit request not to draw any icon
        _shape.reset();
        _overlay.reset();
        return;
    }

    auto icon_theme = Gtk::IconTheme::get_for_display(Gdk::Display::get_default());
    auto icon_name = get_shape_icon(shape_type, 0).icon_name;
    _shape = icon_theme->lookup_icon(icon_name, ICON_SIZE);
}

void CellRendererItemIcon::update_overlay()
{
    auto icon_theme = Gtk::IconTheme::get_for_display(Gdk::Display::get_default());
    int clipmask = _property_clipmask.get_value();
    if (clipmask == OVERLAY_CLIP) {
        _overlay = icon_theme->lookup_icon("overlay-clip-symbolic", ICON_SIZE);
    } else if (clipmask == OVERLAY_MASK) {
        _overlay = icon_theme->lookup_icon("overlay-mask-symbolic", ICON_SIZE);
    } else if (clipmask == OVERLAY_BOTH) {
        _overlay = icon_theme->lookup_icon("overlay-clipmask-symbolic", ICON_SIZE);
    } else {
        _overlay.reset();
    }
}

void CellRendererItemIcon::snapshot_vfunc(Glib::RefPtr<Gtk::Snapshot> const &snapshot,
                                          Gtk::Widget &widget,
                                          const Gdk::Rectangle &background_area,
                                          const Gdk::Rectangle &cell_area,
                                          Gtk::CellRendererState flags)
{
    // CSS color might have changed, so refresh if so:
    if (auto const color = to_guint32(widget.get_color());
        _widget_color != color)
    {
        _widget_color = color;
        update_color();
    }

    paint_icon(_shape.get(), _icon_color, snapshot.get(), cell_area);
    paint_icon(_overlay.get(), _widget_color, snapshot.get(), cell_area);
}

void CellRendererItemIcon::paint_icon(Gtk::IconPaintable *icon,
                                      std::uint32_t color,
                                      Gtk::Snapshot *snapshot,
                                      const Gdk::Rectangle &area)
{
    // Directly paint the icons ourselves, since we want to be dynamic based on the widget colors,
    // but we can't change properties or add/remove css classes to the widget dynamically in this
    // method or GTK will crash.
    // Note: this approach doesn't easily let us handle any state flags (like focused, insensitive).
    // The long term fix for rendering these icons with colors the "right way" is porting to
    // GtkListView and using real widgets.

    if (!icon) {
        return;
    }

    auto offset_x = area.get_x() + (area.get_width() - ICON_SIZE) / 2;
    auto offset_y = area.get_y() + (area.get_height() - ICON_SIZE) / 2;
    auto rgba = to_rgba(color);

    snapshot->save();
    snapshot->translate(Gdk::Graphene::Point(offset_x, offset_y));

    gtk_symbolic_paintable_snapshot_symbolic(
        GTK_SYMBOLIC_PAINTABLE(icon->gobj()),
        GDK_SNAPSHOT(snapshot->gobj()),
        ICON_SIZE, ICON_SIZE,
        rgba.gobj(), 1
    );

    snapshot->restore();
}

bool CellRendererItemIcon::activate_vfunc(Glib::RefPtr<Gdk::Event const> const &event,
                                          Gtk::Widget &widget,
                                          const Glib::ustring &path,
                                          const Gdk::Rectangle &background_area,
                                          const Gdk::Rectangle &cell_area,
                                          Gtk::CellRendererState flags) {
    _signal_activated.emit(path);
    return true;
}

} // namespace Inkscape::UI::Widget

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

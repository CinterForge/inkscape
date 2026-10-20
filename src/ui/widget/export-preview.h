// SPDX-License-Identifier: GPL-2.0-or-later
/* Authors:
 *   Anshudhar Kumar Singh <anshudhar2001@gmail.com>
 *
 * Copyright (C) 2021 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_WIDGET_EXPORT_PREVIEW_H
#define INKSCAPE_UI_WIDGET_EXPORT_PREVIEW_H

#include <2geom/rect.h>
#include <gtkmm/picture.h>

namespace Gtk {
class Builder;
} // namespace Gtk

class SPDocument;
class SPObject;
class SPItem;

namespace Inkscape {

class Drawing;

namespace UI::Dialog {

class ExportPreview;

class PreviewDrawing final
{
public:
    PreviewDrawing(SPDocument *document);
    ~PreviewDrawing();

    bool render(ExportPreview *widget, std::uint32_t bg, SPItem const *item, unsigned size, Geom::OptRect const &dboxIn, bool only_item = false);
    void set_shown_items(std::vector<SPItem const *> &&list = {});

private:
    void destruct();
    void construct();

    SPDocument *_document = nullptr;
    std::shared_ptr<Inkscape::Drawing> _drawing;
    unsigned _visionkey = 0;
    bool _to_destruct = false;

    std::vector<SPItem const *> _shown_items;
    sigc::scoped_connection _construct_idle;
};

class ExportPreview final : public Gtk::Picture
{
public:
    ExportPreview() = default;
    ExportPreview(BaseObjectType *cobj, Glib::RefPtr<Gtk::Builder> const &) : Gtk::Picture(cobj) {}

    ~ExportPreview() override;

    void setDrawing(std::shared_ptr<PreviewDrawing> drawing);
    void setItem(SPItem const *item, bool is_layer = false);
    void setBox(Geom::Rect const &bbox);
    void queueRefresh();
    void resetPixels(bool new_size = false);
    void setSize(int newSize);
    void setPreview(Cairo::RefPtr<Cairo::ImageSurface>);
    void setBackgroundColor(std::uint32_t bg_color);

    static std::shared_ptr<Inkscape::Drawing> makeDrawing(SPDocument *doc);

private:
    int size = 128; // size of preview image
    sigc::connection refresh_conn;

    bool _is_layer = false;
    SPItem const *_item = nullptr;
    Geom::OptRect _dbox;

    std::shared_ptr<PreviewDrawing> _drawing;
    std::uint32_t _bg_color = 0;

    sigc::scoped_connection _render_idle;
};

} // namespace UI::Dialog

} // namespace Inkscape

#endif // INKSCAPE_UI_WIDGET_EXPORT_PREVIEW_H

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

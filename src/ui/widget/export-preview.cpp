// SPDX-License-Identifier: GPL-2.0-or-later
/* Authors:
 *   Anshudhar Kumar Singh <anshudhar2001@gmail.com>
 *
 * Copyright (C) 2021 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "export-preview.h"

#include <glibmm/convert.h>

#include "document.h"
#include "io/resource.h"
#include "object/sp-root.h"
#include "util/preview.h"

namespace Inkscape::UI::Dialog {

/**
 * A preview drawing object is responsible for constructing a drawing and showing it's contents
 *
 * On destruction it will gracefully invoke hide itself. You should destroy this object when
 * you need to change the document object being used for the preview.
 */
PreviewDrawing::PreviewDrawing(SPDocument *doc)
    : _document{doc}
{
}

PreviewDrawing::~PreviewDrawing()
{
    destruct();
}

void PreviewDrawing::destruct()
{
    if (!_visionkey)
        return;

    // On exiting the document root might have gone already.
    if (auto root = _document->getRoot()) {
        root->invoke_hide(_visionkey);
    }
    _drawing.reset();
    _visionkey = 0;
}

/**
 * Construct the drawing, when needed
 */
void PreviewDrawing::construct()
{
    auto drawing = std::make_shared<Inkscape::Drawing>();
    _visionkey = SPItem::display_key_new(1);
    if (auto di = _document->getRoot()->invoke_show(*drawing, _visionkey, SP_ITEM_SHOW_DISPLAY)) {
        drawing->setRoot(di);
    } else {
        drawing.reset();
    }

    if (!_shown_items.empty()) {
        _document->getRoot()->invoke_hide_except(_visionkey, _shown_items);
    }

    _drawing = std::move(drawing);
}

/**
  * Render the drawing into a cairo image surface.
  */
bool PreviewDrawing::render(ExportPreview *widget, uint32_t bg, SPItem const *item, unsigned size, Geom::OptRect const &dbox, bool only_item)
{
    if (!_drawing || _to_destruct) {
        if (!_construct_idle.connected()) {
            _construct_idle = Glib::signal_timeout().connect([this]() {
                _to_destruct = false;
                destruct();
                construct();
                return false;
            }, 100);
        }
        return false;
    }

    Geom::OptRect bbox = dbox;
    DrawingItem *di = nullptr;

    if (item) {
        bbox = item->documentVisualBounds();
        if (only_item)
            di = item->get_arenaitem(_visionkey);
    } else if (!dbox)
        bbox = _document->getRoot()->documentVisualBounds();

    if (!bbox)
        return true; // Force quit

    // Use a callback to set the preview rendering;
    widget->setPreview(UI::Preview::render_preview(_document, _drawing, bg, di, size, size, *bbox));
    return true;
}

/**
 * Limit the preview to just these items.
 *
 * You must call refresh after this for the change to take effect.
 */
void PreviewDrawing::set_shown_items(std::vector<SPItem const *> &&list)
{
    _shown_items = std::move(list);
    _to_destruct = true;
}

void ExportPreview::resetPixels(bool new_size)
{
    // An icon to use when the preview hasn't loaded yet
    static Glib::RefPtr<Gdk::Pixbuf> preview_loading;
    if (!preview_loading || new_size) {
        using namespace Inkscape::IO::Resource;
        auto path_utf8 = (Glib::ustring)Inkscape::IO::Resource::get_path(SYSTEM, UIS, "resources", "preview-loading.svg");
        auto path = Glib::filename_from_utf8(path_utf8);
        preview_loading = Gdk::Pixbuf::create_from_file(path, size, size);
    }
    if (preview_loading) {
        set_pixbuf(preview_loading);
    }
    set_visible(true);
}

void ExportPreview::setSize(int newSize)
{
    size = newSize;
    resetPixels(true);
}

ExportPreview::~ExportPreview()
{
    refresh_conn.disconnect();
}

void ExportPreview::setItem(SPItem const *item, bool is_layer)
{
    _is_layer = is_layer;
    _item = item;
    _dbox = {};
}

void ExportPreview::setBox(Geom::Rect const &bbox)
{
    if (bbox.hasZeroArea())
        return;

    _item = nullptr;
    _dbox = bbox;
}

void ExportPreview::setDrawing(std::shared_ptr<PreviewDrawing> drawing)
{
    _drawing = std::move(drawing);
}

/*
 * This is the main function which finally renders the preview.
 * If dbox is given it will use it.
 * if item is given and not dbox then item is used.
 * If both are not given then we simply do nothing.
 */
void ExportPreview::queueRefresh()
{
    if (!_drawing || _render_idle.connected())
        return;

    _render_idle = Glib::signal_timeout().connect([this]() {
        return !_drawing->render(this, _bg_color, _item, size, _dbox, _is_layer);
    }, 100);
}

/**
 * Callback when the rendering is complete.
 */
void ExportPreview::setPreview(Cairo::RefPtr<Cairo::ImageSurface> surface)
{
    if (surface) {
        set_pixbuf(Gdk::Pixbuf::create(surface, 0, 0, surface->get_width(), surface->get_height()));
        set_visible(true);
    }
}

void ExportPreview::setBackgroundColor(uint32_t bg_color)
{
    _bg_color = bg_color;
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

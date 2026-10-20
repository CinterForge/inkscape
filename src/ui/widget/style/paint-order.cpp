// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Widget for paint order styles
 *//*
 * Copyright (C) 2025 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/i18n.h>

#include "paint-order.h"

#include "style-internal.h"

namespace Inkscape::UI::Widget {

void PaintOrderWidget::construct()
{
    set_orientation(Gtk::Orientation::VERTICAL);
    add_option(_("Marker"), "paint-order-markers", _("Arrows, markers and points"),       SP_CSS_PAINT_ORDER_MARKER);
    add_option(_("Stroke"), "paint-order-stroke",  _("The border line around the shape"), SP_CSS_PAINT_ORDER_STROKE);
    add_option(_("Fill"),   "paint-order-fill",    _("The content of the shape"),         SP_CSS_PAINT_ORDER_FILL);
}

PaintOrderWidget::PaintOrderWidget()
    : Glib::ObjectBase("PaintOrderWidget")
{
    construct();
}

PaintOrderWidget::PaintOrderWidget(GtkWidget* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
    : Glib::ObjectBase("PaintOrderWidget")
    , BuildableWidget(cobject, builder)
{
    construct();
}

void PaintOrderWidget::setValue(SPIPaintOrder &po, bool has_markers)
{
    // array to vector
    auto values = po.get_layers();
    // Note: what's painted first is presented at the bottom of the stack.
    std::vector<int> vec = {values[2], values[1], values[0]};
    setValues(vec);

    // Hide the markers if the style has no markers
    setVisible((int)SP_CSS_PAINT_ORDER_MARKER, has_markers);
}

SPIPaintOrder PaintOrderWidget::getValue()
{
    SPIPaintOrder po;
    auto values = getValues();
    for (auto i = 0; i < 3; i++) {
        // Note the reversed order to match setValue()
        po.layer[i] = (SPPaintOrderLayer)values[2 - i];
        po.layer_set[i] = true;
    }
    po.set = true;
    return po;
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
// vim:filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99:

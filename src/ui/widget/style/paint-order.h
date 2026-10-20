// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Widget for paint order styles
 *//*
 * Copyright (C) 2025 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_UI_WIDGET_PAINT_ORDER_H
#define SEEN_UI_WIDGET_PAINT_ORDER_H

#include "ui/widget/generic/reorderable-stack.h"

class SPIPaintOrder;

namespace Inkscape::UI::Widget {

class PaintOrderWidget : public BuildableWidget<PaintOrderWidget, ReorderableStack>
{
public:
    PaintOrderWidget();
    explicit PaintOrderWidget(GtkWidget* cobject, const Glib::RefPtr<Gtk::Builder>& builder = {});

    void setValue(SPIPaintOrder &po, bool has_markers);
    SPIPaintOrder getValue();

private:
    void construct();
};

} // namespace Inkscape::UI::Widget

#endif /* !SEEN_UI_WIDGET_PAINT_ORDER_H */

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

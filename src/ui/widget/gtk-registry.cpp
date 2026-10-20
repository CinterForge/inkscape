// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "gtk-registry.h"

#include "generic/reorderable-stack.h"
#include "generic/spin-button.h"
#include "generic/tab-strip.h"
#include "generic/text-entry.h"
#include "style/paint-order.h"
#include "ui/widget/font-size-selector.h"

namespace Inkscape::UI::Widget {

void register_all()
{
    // Add generic and reusable widgets here
    InkSpinButton::register_type();
    TextEntry::register_type();
    TabStrip::register_type();
    ReorderableStack::register_type();

    // Specific widgets
    FontSizeSelector::register_type();
    PaintOrderWidget::register_type();
}

} // namespace Dialog::UI::Widget

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

// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_DRAG_AND_DROP_H
#define INKSCAPE_UI_DRAG_AND_DROP_H

/**
 * @file
 * Drag and drop of drawings onto canvas.
 */

/* Authors:
 *
 * Copyright (C) Tavmjong Bah 2019
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/ustring.h>

namespace Gtk { class Widget; }
class SPDesktopWidget;
class SPDocument;

struct DnDSymbol
{
    // symbol's ID; may be reused in different symbol sets
    Glib::ustring id;
    // symbol's unique key (across symbol sets known to Inkscape at runtime)
    std::string unique_key;
    // symbol's document
    SPDocument* document = nullptr;
};

void ink_drag_setup(SPDesktopWidget *dtw, Gtk::Widget *widget);

#endif // INKSCAPE_UI_DRAG_AND_DROP_H

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

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Johan Engelen <goejendaagh@zonnet.nl>
 *
 * Copyright (C) 2006 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_UI_WIDGET_ENTRY_H
#define SEEN_INKSCAPE_UI_WIDGET_ENTRY_H

#include <gtkmm/entry.h>

#include "labelled.h"

namespace Inkscape::UI::Widget {

/**
 * Helperclass for Gtk::Entry widgets.
 */
class Entry : public Labelled
{
public:
    Entry(Glib::ustring const &label,
          Glib::ustring const &tooltip,
          Glib::ustring const &icon = {},
          bool mnemonic = true);

    Gtk::Entry const * getEntry() const { return static_cast<Gtk::Entry const *>(getWidget()); }
    Gtk::Entry       * getEntry()       { return static_cast<Gtk::Entry       *>(getWidget()); }
};

} // namespace Inkscape::UI::Widget

#endif // SEEN_INKSCAPE_UI_WIDGET_ENTRY_H

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

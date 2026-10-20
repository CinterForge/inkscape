// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Johan Engelen <goejendaagh@zonnet.nl>
 *
 * Copyright (C) 2006 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "entry.h"

namespace Inkscape::UI::Widget {

Entry::Entry(Glib::ustring const &label, Glib::ustring const &tooltip,
             Glib::ustring const &icon,
             bool const mnemonic)
    : Labelled{label, tooltip, new Gtk::Entry{}, icon, mnemonic}
{    
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

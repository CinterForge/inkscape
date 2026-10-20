// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_EVENT_H
#define INKSCAPE_EVENT_H

/*
 * Inkscape::Event -- Container for an XML::Event along with some additional information
 *                     describing it.
 *
 * Author:
 *   Gustav Broberg <broberg@kth.se>
 *
 * Copyright (c) 2006 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#include <glibmm/ustring.h>

#include <utility>

#include "xml/event-fns.h"

namespace Inkscape {
namespace XML {
class Event;
}
}

namespace Inkscape {

class Event {

public:

    Event(XML::Event *_event, Glib::ustring _description="", Glib::ustring _icon_name="")
        : event (_event), description (std::move(_description)), icon_name (std::move(_icon_name))  { }

    virtual ~Event() { sp_repr_free_log (event); }

    XML::Event *event;
    unsigned int type = 0;
    Glib::ustring description; // The description to use in the Undo dialog.
    Glib::ustring icon_name;   // The icon to use in the Undo dialog.
};

} // namespace Inkscape

#endif // INKSCAPE_EVENT_H

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

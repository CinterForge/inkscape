// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::SelectionDescriber - shows messages describing selection
 *
 * Authors:
 *   MenTaLguY <mental@rydia.net>
 *
 * Copyright (C) 2004 MenTaLguY
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_SELECTION_DESCRIBER_H
#define INKSCAPE_SELECTION_DESCRIBER_H

#include <sigc++/scoped_connection.h>
#include "message-context.h"

namespace Inkscape {

class MessageStack;
class Selection;

class SelectionDescriber : public sigc::trackable
{
public:
    SelectionDescriber(Inkscape::Selection *selection, MessageStack &stack, char *when_selected, char *when_nothing);
    ~SelectionDescriber();

    void updateMessage(Inkscape::Selection *selection);

private:
    sigc::scoped_connection _selection_changed_connection;

    MessageContext _context;

    char *_when_selected;
    char *_when_nothing;
};

} // namespace Inkscape

#endif // INKSCAPE_SELECTION_DESCRIBER_H

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

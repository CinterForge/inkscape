// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Entry points for event distribution
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_DESKTOP_EVENTS_H
#define INKSCAPE_DESKTOP_EVENTS_H

class SPDesktop;
class SPGuide;

namespace Inkscape {
class CanvasItemGuideLine;
struct CanvasEvent;
}

/* Item handlers */

bool sp_desktop_root_handler(Inkscape::CanvasEvent const &event, SPDesktop *desktop);

/* Guides */

bool sp_dt_guide_event(Inkscape::CanvasEvent const &event, Inkscape::CanvasItemGuideLine *guide_item, SPGuide *guide);

#endif // INKSCAPE_DESKTOP_EVENTS_H

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

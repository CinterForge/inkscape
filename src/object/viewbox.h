// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_INKSCAPE_VIEWBOX_H
#define SEEN_INKSCAPE_VIEWBOX_H

/*
 * viewBox helper class, common code used by root, symbol, marker, pattern, image, view
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com> (code extracted from sp-symbol.h)
 *   Tavmjong Bah
 *   Johan Engelen
 *
 * Copyright (C) 2013-2014 Tavmjong Bah, authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 *
 */

#include <2geom/rect.h>
#include <glib.h>

namespace Inkscape::XML {
class Node;
} // namespace Inkscape::XML

class SPItemCtx;

class SPViewBox {

public:
  SPViewBox();

  /* viewBox; */
  bool viewBox_set;
  Geom::Rect viewBox;  // Could use optrect

  /* preserveAspectRatio */
  bool aspect_set;
  unsigned int aspect_align;  // enum
  unsigned int aspect_clip;   // enum

  /* Child to parent additional transform */
  Geom::Affine c2p;

  void set_viewBox(const gchar* value);
  void set_preserveAspectRatio(const gchar* value);
  void write_viewBox(Inkscape::XML::Node *repr) const;
  void write_preserveAspectRatio(Inkscape::XML::Node *repr) const;

  /* Adjusts c2p for viewbox */
  void apply_viewbox(const Geom::Rect& in, double scale_none = 1.0);

  SPItemCtx get_rctx( const SPItemCtx* ictx, double scale_none = 1.0);

  Geom::OptRect get_paintbox(double width, double height, Geom::OptRect const &size) const;
};

#endif

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-basic-offset:2
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=2:tabstop=8:softtabstop=2:fileencoding=utf-8:textwidth=99 :

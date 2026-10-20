// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_HELPER_PIXBUF_OPS_H
#define INKSCAPE_HELPER_PIXBUF_OPS_H

/*
 * Helpers for SPItem -> gdk_pixbuf related stuff
 *
 * Authors:
 *   John Cliff <simarilius@yahoo.com>
 *
 * Copyright (C) 2008 John Cliff
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <vector>
#include <cstdint>
#include <2geom/forward.h>
#include "display/drawing-item.h"

class SPDocument;
class SPItem;
namespace Inkscape { class Pixbuf; }

Inkscape::Pixbuf *sp_generate_internal_bitmap(SPDocument *document,
                                              Geom::Rect const &area,
                                              double dpi,
                                              std::vector<SPItem const *> items = {},
                                              bool set_opaque = false,
                                              uint32_t const *checkerboard_color = nullptr,
                                              double device_scale = 1.0,
                                              std::optional<Inkscape::Antialiasing> antialias = {});
#endif // INKSCAPE_HELPER_PIXBUF_OPS_H

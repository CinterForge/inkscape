// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2013 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef INKSCAPE_TRACE_IMAGEMAP_GDK_H
#define INKSCAPE_TRACE_IMAGEMAP_GDK_H

#include <gdkmm/pixbuf.h>
#include "imagemap.h"

namespace Inkscape {
namespace Trace {

GrayMap gdkPixbufToGrayMap(Glib::RefPtr<Gdk::Pixbuf> const &);
Glib::RefPtr<Gdk::Pixbuf> grayMapToGdkPixbuf(GrayMap const &);
RgbMap gdkPixbufToRgbMap(Glib::RefPtr<Gdk::Pixbuf> const &);
Glib::RefPtr<Gdk::Pixbuf> indexedMapToGdkPixbuf(IndexedMap const &);

} // namespace Trace
} // namespace Inkscape

#endif // INKSCAPE_TRACE_IMAGEMAP_GDK_H

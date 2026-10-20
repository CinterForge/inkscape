// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Some filters for Potrace in Inkscape
 *
 * Authors:
 *   Bob Jamison <rjamison@titan.com>
 *   Stéphane Gimenez <dev@gim.name>
 *
 * Copyright (C) 2004-2006 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef INKSCAPE_TRACE_FILTERSET_H
#define INKSCAPE_TRACE_FILTERSET_H

#include <gdk-pixbuf/gdk-pixbuf.h>
#include "imagemap.h"

namespace Inkscape {
namespace Trace {

/**
 * Apply gaussian blur to an GrayMap.
 */
GrayMap grayMapGaussian(GrayMap const &gmap);

/**
 * Apply gaussian blur to an RgbMap.
 */
RgbMap rgbMapGaussian(RgbMap const &rgbmap);

GrayMap grayMapCanny(GrayMap const &gmap, double lowThreshold, double highThreshold);

GrayMap quantizeBand(RgbMap const &rgbmap, int nrColors);

} // namespace Trace
} // namespace Inkscape

#endif // INKSCAPE_TRACE_FILTERSET_H

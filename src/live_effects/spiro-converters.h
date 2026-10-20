// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef INKSCAPE_SPIRO_CONVERTERS_H
#define INKSCAPE_SPIRO_CONVERTERS_H

#include <2geom/forward.h>

namespace Spiro {

/**
 * Converts Spiro to 2Geom's Path
 */
class ConverterPath
{
public:
    ConverterPath(Geom::Path &path);
    ConverterPath(ConverterPath const &) = delete;
    ConverterPath &operator=(ConverterPath const &) = delete;

    void moveto(double x, double y);
    void lineto(double x, double y, bool close_last);
    void quadto(double x1, double y1, double x2, double y2, bool close_last);
    void curveto(double x1, double y1, double x2, double y2, double x3, double y3, bool close_last);

private:
    Geom::Path &_path;
};

} // namespace Spiro

#endif

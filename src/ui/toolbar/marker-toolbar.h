// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Marker edit mode toolbar - onCanvas marker editing of marker orientation, position, scale
 *//*
 * Authors:
 * see git history
 * Rachana Podaralla <rpodaralla3@gatech.edu>
 * Vaibhav Malik <vaibhavmalik2018@gmail.com>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_TOOLBAR_SEEN_MARKER_TOOLBAR_H
#define INKSCAPE_UI_TOOLBAR_SEEN_MARKER_TOOLBAR_H

#include "toolbar.h"

namespace Inkscape::UI::Toolbar {

class MarkerToolbar : public Toolbar
{
public:
    MarkerToolbar();
};

} // namespace Inkscape::UI::Toolbar

#endif // INKSCAPE_UI_TOOLBAR_SEEN_MARKER_TOOLBAR_H

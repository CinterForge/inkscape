// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef INKSCAPE_UI_WIDGET_RECOLOR_ART_MANAGER_H
#define INKSCAPE_UI_WIDGET_RECOLOR_ART_MANAGER_H
/*
 * Authors:
 *   Fatma Omara <ftomara647@gmail.com>
 *
 * Copyright (C) 2025 authors
 */

#include <gtkmm/popover.h>

#include "ui/widget/recolor-art.h"
#include "selection.h"

namespace Gtk { class MenuButton; }

namespace Inkscape::UI::Widget {

class RecolorArtManager
{
public:
    static RecolorArtManager &get();

    RecolorArt widget;
    Gtk::Popover popover;

    void reparentPopoverTo(Gtk::MenuButton &button);

    static bool checkSelection(Inkscape::Selection *selection);
    static bool checkMeshObject(Inkscape::Selection *selection);
    static bool checkMarkerObject(SPMarker *marker);

private:
    RecolorArtManager();
};

} // namespace Inkscape::UI::Widget

#endif // INKSCAPE_UI_WIDGET_RECOLOR_ART_MANAGER_H

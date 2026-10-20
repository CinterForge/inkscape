// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief Dialog for creating grid type arrangements of selected objects
 */
/* Authors:
 *   Bob Jamison ( based off trace dialog)
 *   John Cliff
 *   Other dudes from The Inkscape Organization
 *   Abhishek Sharma
 *   Declara Denis
 *
 * Copyright (C) 2004 Bob Jamison
 * Copyright (C) 2004 John Cliff
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_UI_DIALOG_TILE_H
#define SEEN_UI_DIALOG_TILE_H

#include "ui/dialog/dialog-base.h"

namespace Gtk {
class Box;
class Button;
class Notebook;
} // namespace Gtk

namespace Inkscape::UI::Dialog {

class AlignAndDistribute;
class ArrangeTab;
class GridArrangeTab;
class PolarArrangeTab;

class ArrangeDialog final : public DialogBase
{
public:
    ArrangeDialog();

    void desktopReplaced() final;

    void update_arrange_btn();

    /**
     * Callback from Apply
     */
    void _apply();

private:
    Gtk::Box           *_arrangeBox      = nullptr;
    Gtk::Notebook      *_notebook        = nullptr;
    AlignAndDistribute *_align_tab       = nullptr;
    GridArrangeTab     *_gridArrangeTab  = nullptr;
    PolarArrangeTab    *_polarArrangeTab = nullptr;
    Gtk::Button        *_arrangeButton   = nullptr;
};

} // namespace Inkscape::UI::Dialog

#endif // SEEN_UI_DIALOG_TILE_H

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

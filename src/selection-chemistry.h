// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SELECTION_CHEMISTRY_H
#define SEEN_SELECTION_CHEMISTRY_H

/*
 * Miscellaneous operations on selected items
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 1999-2012 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <vector>
#include <2geom/forward.h>
#include <glibmm/ustring.h>

#include "selection.h"

class SPCSSAttr;
class SPDesktop;
class SPDocument;
class SPItem;
class SPObject;

namespace Inkscape {
namespace Util {
class Unit;
}

class Selection;
class ObjectSet;
namespace LivePathEffect { class PathParam; }

namespace SelectionHelper {

void selectAll(SPDesktop *desktop);
void selectAllInAll(SPDesktop *desktop);
void selectNone(SPDesktop *desktop);
void selectSameFillStroke(SPDesktop *dt);
void selectSameFillColor(SPDesktop *dt);
void selectSameStrokeColor(SPDesktop *dt);
void selectSameStrokeStyle(SPDesktop *dt);
void selectSameObjectType(SPDesktop *dt);
void invert(SPDesktop *desktop);
void invertAllInAll(SPDesktop *desktop);
void reverse(SPDesktop *dt);
void fixSelection(SPDesktop *desktop);

} // namespace SelectionHelper

} // namespace Inkscape

void sp_edit_clear_all(Inkscape::Selection *selection);

void sp_edit_select_all(SPDesktop *desktop);
void sp_edit_select_all_in_all_layers (SPDesktop *desktop);
void sp_edit_invert (SPDesktop *desktop);
void sp_edit_invert_in_all_layers (SPDesktop *desktop);


SPCSSAttr *take_style_from_item(SPObject *object);

void sp_selection_paste(SPDesktop *desktop, bool in_place, bool on_page = false);

void sp_set_style_clipboard(SPCSSAttr *css);


void sp_selection_item_next(SPDesktop *desktop);
void sp_selection_item_prev(SPDesktop *desktop);

void sp_selection_next_patheffect_param(SPDesktop *dt);

enum SPSelectStrokeStyleType
{
    SP_FILL_COLOR = 0,
    SP_STROKE_COLOR = 1,
    SP_STROKE_STYLE_WIDTH = 2,
    SP_STROKE_STYLE_DASHES = 3,
    SP_STROKE_STYLE_MARKERS = 4,
    SP_STROKE_STYLE_ALL = 5,
    SP_STYLE_ALL = 6
};

void sp_select_same_fill_stroke_style(SPDesktop *desktop, gboolean fill, gboolean strok, gboolean style);
void sp_select_same_object_type(SPDesktop *desktop);

std::vector<SPItem*> sp_get_same_style(SPItem *sel, std::vector<SPItem*> &src, SPSelectStrokeStyleType type = SP_STYLE_ALL);
std::vector<SPItem*> sp_get_same_object_type(SPItem *sel, std::vector<SPItem*> &src);

void scroll_to_show_item(SPDesktop *desktop, SPItem *item);

bool fit_canvas_to_drawing(SPDocument *, bool with_margins = false);
void fit_canvas_to_drawing(SPDesktop *);

void unlock_all(SPDesktop *dt);
void unlock_all_in_all_layers(SPDesktop *dt);
void unhide_all(SPDesktop *dt);
void unhide_all_in_all_layers(SPDesktop *dt);

std::vector<SPItem*> get_all_items(SPObject *from, SPDesktop *desktop, bool onlyvisible, bool onlysensitive, bool ingroups, std::vector<SPItem*> const &exclude = {});

/* selection cycling */
enum SPCycleType
{
    SP_CYCLE_SIMPLE,
    SP_CYCLE_VISIBLE, // cycle only visible items
    SP_CYCLE_FOCUS // readjust visible area to view selected item
};

// TODO FIXME: This should be moved into preference repr
extern SPCycleType SP_CYCLING;

//
void sp_transform_selected_items(
    SPDesktop* desktop,
    const Geom::Rect& rect,
    const Inkscape::Util::Unit* unit,
    const std::string& action_prefix,
    bool transform_stroke,
    bool preserve_transform,
    bool use_visual_box);

//
Geom::Rect sp_selection_get_xywh(
    SPDesktop* desktop,
    const Inkscape::Util::Unit* unit,
    bool use_visual_box);

#endif // SEEN_SELECTION_CHEMISTRY_H

// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_TOOLS_FREEHAND_BASE_H
#define INKSCAPE_UI_TOOLS_FREEHAND_BASE_H

/*
 * Generic drawing context
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <2geom/pathvector.h>

#include "ui/tools/tool-base.h"
#include "live_effects/effect-enum.h"
#include "display/control/canvas-item-ptr.h"

struct SPDrawAnchor;

namespace Inkscape {
class CanvasItemBpath;
class Selection;
} // namespace Inkscape

namespace Inkscape::UI::Tools {

enum ShapeType
{
    NONE,
    TRIANGLE_IN,
    TRIANGLE_OUT,
    ELLIPSE,
    CLIPBOARD,
    BEND_CLIPBOARD,
    LAST_APPLIED
};

class FreehandBase : public ToolBase
{
public:
    FreehandBase(SPDesktop *desktop, std::string &&prefs_path, std::string &&cursor_filename);
    ~FreehandBase() override;

    Selection *selection = nullptr;

protected:
    uint32_t red_color = 0xff00007f;
    uint32_t blue_color = 0x0000ff7f;
    uint32_t green_color = 0x00ff007f;
    uint32_t highlight_color = 0x0000007f;

public:
    // Red - Last segment as it's drawn.
    CanvasItemPtr<CanvasItemBpath> red_bpath;
    Geom::PathVector red_curve;
    std::optional<Geom::Point> red_curve_get_last_point() const;

    // Blue - New path after LPE as it's drawn.
    CanvasItemPtr<CanvasItemBpath> blue_bpath;
    Geom::PathVector blue_curve;

    // Green - New path as it's drawn.
    std::vector<CanvasItemPtr<CanvasItemBpath>> green_bpaths;
    std::shared_ptr<Geom::PathVector> green_curve;
    std::unique_ptr<SPDrawAnchor> green_anchor;
    bool green_closed = false; // a flag meaning we hit the green anchor, so close the path on itself

    // White
    SPItem *white_item = nullptr;
    std::vector<std::shared_ptr<Geom::PathVector>> white_curves;
    std::vector<std::unique_ptr<SPDrawAnchor>> white_anchors;

    // Temporary modified curve when start anchor
    std::shared_ptr<Geom::PathVector> sa_overwrited;

    // Start anchor
    SPDrawAnchor *sa = nullptr;

    // End anchor
    SPDrawAnchor *ea = nullptr;

    // Type of the LPE that is to be applied automatically to a finished path (if any)
    LivePathEffect::EffectType waiting_LPE_type = LivePathEffect::INVALID_LPE;

    sigc::connection sel_changed_connection;
    sigc::connection sel_modified_connection;

    bool red_curve_is_valid = false;

    bool anchor_statusbar = false;
    
    bool tablet_enabled = false;
    bool is_tablet = false;

    double pressure = 1.0;

    void onSelectionModified();

protected:
    bool root_handler(CanvasEvent const &event) override;
    void _attachSelection();
};

/**
 * Returns FIRST active anchor (the activated one).
 */
SPDrawAnchor *spdc_test_inside(FreehandBase *dc, Geom::Point const &p);

/**
 * Concats red, blue and green.
 * If any anchors are defined, process these, optionally removing curves from white list
 * Invoke _flush_white to write result back to object.
 */
void spdc_concat_colors_and_flush(FreehandBase *dc, bool forceclosed);

/**
 *  Snaps node or handle to PI/rotationsnapsperpi degree increments.
 *
 *  @param dc draw context.
 *  @param p cursor point (to be changed by snapping).
 *  @param o origin point.
 *  @param state  keyboard state to check if ctrl or shift was pressed.
 */
void spdc_endpoint_snap_rotation(ToolBase *tool, Geom::Point &p, Geom::Point const &o, unsigned state);

void spdc_endpoint_snap_free(ToolBase *tool, Geom::Point &p, std::optional<Geom::Point> &start_of_line);

/**
 * If we have an item and a waiting LPE, apply the effect to the item
 * (spiro spline mode is treated separately).
 */
void spdc_check_for_and_apply_waiting_LPE(FreehandBase *dc, SPItem *item);

/**
 * Create a single dot represented by a circle.
 */
void spdc_create_single_dot(ToolBase *tool, Geom::Point const &pt, char const *path, unsigned event_state);

} // namespace Inkscape::UI::Tools

#endif // INKSCAPE_UI_TOOLS_FREEHAND_BASE_H

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

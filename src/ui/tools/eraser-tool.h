// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_TOOLS_ERASER_TOOL_H
#define INKSCAPE_UI_TOOLS_ERASER_TOOL_H

/*
 * Handwriting-like drawing mode
 *
 * Authors:
 *   Mitsuru Oka <oka326@parkcity.ne.jp>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * The original dynadraw code:
 *   Paul Haeberli <paul@sgi.com>
 *
 * Copyright (C) 1998 The Free Software Foundation
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 * Copyright (C) 2008 Jon A. Cruz
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "message.h"
#include "object/weakptr.h"
#include "style-enums.h"
#include "ui/tools/dynamic-base.h"
#include "object/sp-use.h"

namespace Inkscape {
struct KeyPressEvent;
struct ExtendedInput;
} // namespace Inkscape

namespace Inkscape::UI::Tools {

enum class EraserToolMode
{
    DELETE,
    CUT,
    CLIP
};
static inline constexpr auto DEFAULT_ERASER_MODE = EraserToolMode::CUT;

/** Represents an item to erase */
struct EraseTarget
{
    SPItem *item = nullptr;    ///< Pointer to the item to be erased
    bool was_selected = false; ///< Whether the item was part of selection

    EraseTarget(SPItem *ptr, bool sel)
        : item{ptr}
        , was_selected{sel}
    {}
    inline bool operator==(EraseTarget const &other) const noexcept { return item == other.item; }
};

class EraserTool : public DynamicBase
{
public:
    explicit EraserTool(SPDesktop *desktop);

    bool root_handler(CanvasEvent const &event) override;

private:
    // non-static data:
    EraserToolMode mode = DEFAULT_ERASER_MODE;
    bool nowidth = false;
    std::vector<MessageId> _our_messages;
    SPItem *_acid = nullptr;
    std::vector<SPWeakPtr<SPItem>> _survivors;
    Pref<bool> _break_apart;
    Pref<int> _mode_int;

    // static data:
    static constexpr uint32_t trace_color_rgba  = 0xff0000ff; // RGBA red
    static constexpr SPWindRule trace_wind_rule = SP_WIND_RULE_EVENODD;

    static constexpr double tolerance = 0.1;

    static constexpr double epsilon       = 0.5e-6;
    static constexpr double epsilon_start = 0.5e-2;
    static constexpr double vel_start     = 1e-5;

    static constexpr double drag_default = 1.0;
    static constexpr double drag_min     = 0.0;
    static constexpr double drag_max     = 1.0;

    static constexpr double min_pressure     = 0.0;
    static constexpr double max_pressure     = 1.0;
    static constexpr double default_pressure = 1.0;

    static constexpr double min_tilt     = -1.0;
    static constexpr double max_tilt     = 1.0;
    static constexpr double default_tilt = 0.0;

    using Error = std::uint64_t;
    static constexpr Error ALL_GOOD     = 0x0;
    static constexpr Error NON_EXISTENT = 0x1 << 1;
    static constexpr Error NO_AREA_PATH = 0x1 << 2;
    static constexpr Error RASTER_IMAGE = 0x1 << 3;
    static constexpr Error ERROR_GROUP  = 0x1 << 4;

    // private member functions
    void _accumulate();
    bool _apply(Geom::Point const &p);
    bool _booleanErase(EraseTarget target, bool store_survivors);
    void _brush();
    void _cancel();
    void _clearCurrent();
    void _clearStatusBar();
    void _clipErase(SPItem *item) const;
    void _completeBezier(double tolerance_sq, bool releasing);
    bool _cutErase(EraseTarget target, bool store_survivors);
    bool _doWork();
    void _drawTemporaryBox();
    void _extinput(ExtendedInput const &ext);
    void _failedBezierFallback();
    std::vector<EraseTarget> _filterByCollision(std::vector<EraseTarget> const &items, SPItem *with) const;
    std::vector<EraseTarget> _filterCutEraseables(std::vector<EraseTarget> const &items, bool silent = false);
    std::vector<EraseTarget> _findItemsToErase();
    void _fitAndSplit(bool releasing);
    void _fitDrawLastPoint();
    bool _handleKeypress(KeyPressEvent const &key);
    void _handleStrokeStyle(SPItem *item) const;
    SPItem *_insertAcidIntoDocument(SPDocument *document);
    bool _performEraseOperation(std::vector<EraseTarget> const &items_to_erase, bool store_survivors);
    void _reset(Geom::Point p);
    void _setStatusBarMessage(char *message);
    void _updateMode();

    static void _generateNormalDist2(double &r1, double &r2);
    static void _addCap(Geom::Path &curve, Geom::Point const &pre, Geom::Point const &from, Geom::Point const &to,
                        Geom::Point const &post, double rounding);
    static bool _isStraightSegment(SPItem *path);
    static Error _uncuttableItemType(SPItem *item);
    bool _probeUnlinkCutClonedGroup(EraseTarget &original_target, SPUse* clone, SPGroup* cloned_group,
                                    bool store_survivors = true);
};

} // namespace Inkscape::UI::Tools

#endif // INKSCAPE_UI_TOOLS_ERASER_TOOL_H

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

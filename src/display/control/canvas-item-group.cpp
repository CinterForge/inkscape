// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * A CanvasItem that contains other CanvasItem's.
 */
/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Rewrite of SPCanvasGroup
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "canvas-item-group.h"

#include <ranges>

constexpr bool DEBUG_LOGGING = false;

namespace Inkscape {

CanvasItemGroup::CanvasItemGroup(CanvasItemGroup *group)
    : CanvasItem(group)
{
    _name = "CanvasItemGroup";
    _pickable = true; // For now all groups are pickable... look into turning this off for some groups (e.g. temp).
}

CanvasItemGroup::CanvasItemGroup(CanvasItemContext *context)
    : CanvasItem(context)
{
    _name = "CanvasItemGroup:Root";
    _pickable = true; // see above
}

CanvasItemGroup::~CanvasItemGroup()
{
    items.clear_and_dispose([] (auto c) { delete c; });
}

void CanvasItemGroup::_update(bool propagate)
{
    _bounds = {};

    // Update all children and calculate new bounds.
    for (auto &item : items) {
        item.update(propagate);
        _bounds |= item.get_bounds();
    }
}

void CanvasItemGroup::_mark_net_invisible()
{
    if (!_net_visible) {
        return;
    }
    _net_visible = false;
    _need_update = false;
    for (auto &item : items) {
        item._mark_net_invisible();
    }
    _bounds = {};
}

void CanvasItemGroup::visit_page_rects(std::function<void(Geom::Rect const &)> const &f) const
{
    for (auto &item : items) {
        if (!item.is_visible()) continue;
        item.visit_page_rects(f);
    }
}

void CanvasItemGroup::_render(Inkscape::CanvasItemBuffer &buf) const
{
    for (auto &item : items) {
        item.render(buf);
    }
}

void CanvasItemGroup::_invalidate_ctrl_handles()
{
    for (auto &c : items) {
        c._invalidate_ctrl_handles();
    }
}

// Return last visible and pickable item that contains point.
// SPCanvasGroup returned distance but it was not used.
CanvasItem *CanvasItemGroup::pick_item(Geom::Point const &p)
{
    if constexpr (DEBUG_LOGGING) {
        std::cout << "CanvasItemGroup::pick_item:" << std::endl;
        std::cout << "  PICKING: In group: " << _name << "  bounds: " << _bounds << std::endl;
    }

    for (auto &item : items | std::views::reverse) {
        if constexpr (DEBUG_LOGGING) std::cout << "    PICKING: Checking: " << item.get_name() << "  bounds: " << item.get_bounds() << std::endl;

        if (item.is_visible() && item.is_pickable() && item.contains(p)) {
            if (auto group = dynamic_cast<CanvasItemGroup*>(&item)) {
                if (auto ret = group->pick_item(p)) {
                    return ret;
                }
            } else {
                return &item;
            }
        }
    }

    return nullptr;
}

} // namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

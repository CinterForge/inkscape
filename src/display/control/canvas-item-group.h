// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_CANVAS_ITEM_GROUP_H
#define SEEN_CANVAS_ITEM_GROUP_H

/**
 * A CanvasItem that contains other CanvasItems.
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

#include "canvas-item.h"

namespace Inkscape {

class CanvasItemGroup final : public CanvasItem
{
public:
    CanvasItemGroup(CanvasItemGroup *group);
    CanvasItemGroup(CanvasItemContext *context);

    // Geometry
    void visit_page_rects(std::function<void(Geom::Rect const &)> const &) const override;

    // Selection
    CanvasItem *pick_item(Geom::Point const &p);

protected:
    friend class CanvasItem; // access to items
    friend class CanvasItemContext; // access to destructor

    ~CanvasItemGroup() override;

    void _update(bool propagate) override;
    void _mark_net_invisible() override;
    void _render(Inkscape::CanvasItemBuffer &buf) const override;
    void _invalidate_ctrl_handles() override;

    /**
     * Type for linked list storing CanvasItems.
     * Used to speed deletion when a group contains a large number of items (as in nodes for a complex path).
     */
    using CanvasItemList = boost::intrusive::list<
        Inkscape::CanvasItem,
        boost::intrusive::member_hook<Inkscape::CanvasItem, boost::intrusive::list_member_hook<>,
                                      &Inkscape::CanvasItem::member_hook>>;

    CanvasItemList items;
};

} // namespace Inkscape

#endif // SEEN_CANVAS_ITEM_H

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

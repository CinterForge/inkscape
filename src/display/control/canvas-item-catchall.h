// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * A class to catch events after everyone else has had a go.
 */
/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Rewrite of SPCanvasAcetate.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_CANVAS_ITEM_CATCHALL_H
#define SEEN_CANVAS_ITEM_CATCHALL_H

#include "canvas-item.h"

namespace Inkscape {

class CanvasItemCatchall final : public CanvasItem
{
public:
    CanvasItemCatchall(CanvasItemGroup *group);

    // Selection
    bool contains(Geom::Point const &p, double tolerance) override;

protected:
    ~CanvasItemCatchall() override = default;

    void _update(bool propagate) override;
    void _render(Inkscape::CanvasItemBuffer &buf) const override;
};

} // namespace Inkscape

#endif // SEEN_CANVAS_ITEM_CATCHALL_H

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

// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_CANVAS_TEMPORARY_ITEM_LIST_H
#define INKSCAPE_CANVAS_TEMPORARY_ITEM_LIST_H

/*
 * Authors:
 *   Johan Engelen
 *
 * Copyright (C) Johan Engelen 2008 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <list>

namespace Inkscape {

class CanvasItem;

namespace Display {

class TemporaryItem;

/**
 * Provides a class that can contain active TemporaryItems on a desktop.
 */
class TemporaryItemList final
{
public:
    TemporaryItemList() = default;
    TemporaryItemList(TemporaryItemList const &) = delete;
    TemporaryItemList &operator=(TemporaryItemList const &) = delete;
    ~TemporaryItemList();

    TemporaryItem* add_item(CanvasItem *item, int lifetime_msecs);
    void           delete_item(TemporaryItem *tempitem);

protected:
    std::list<TemporaryItem *> itemlist; ///< List of temp items.
};

} // namespace Display
} // namespace Inkscape

#endif // INKSCAPE_CANVAS_TEMPORARY_ITEM_LIST_H

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

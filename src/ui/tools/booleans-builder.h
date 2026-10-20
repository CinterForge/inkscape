// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 *//*
 * Authors:
 *   Martin Owens
 *
 * Copyright (C) 2022 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_TOOLS_BOOLEANS_BUILDER_H
#define INKSCAPE_UI_TOOLS_BOOLEANS_BUILDER_H

#include "booleans-subitems.h"
#include <sigc++/scoped_connection.h>
#include "display/control/canvas-item-ptr.h"

class SPDesktop;
class SPDocument;
class SPObject;
class SPImage;

namespace Inkscape {

class CanvasItemGroup;
class CanvasItemBpath;
class ObjectSet;
class Drawing;

using VisualItem = CanvasItemPtr<CanvasItemBpath>;
struct ItemPair
{
    WorkItem work;
    VisualItem vis;
    bool visible;
};

enum class TaskType
{
    NONE,
    ADD,
    DELETE
};

class BooleanBuilder
{
public:
    BooleanBuilder(ObjectSet *obj, bool flatten = false);
    ~BooleanBuilder();

    void undo();
    void redo();

    std::vector<SPObject *> shape_commit(bool all = false, bool replace = true);
    ItemPair *get_item(const Geom::Point &point);
    bool task_select(const Geom::Point &point, bool add_task = true);
    bool task_add(const Geom::Point &point);
    void task_cancel();
    void task_commit();
    bool has_items() const { return !_work_items.empty(); }
    bool has_task() const { return (bool)_work_task; }
    bool has_changes() const { return !_undo.empty(); }
    bool highlight(const Geom::Point &point, bool add_task = true);
    bool contains_image(SPItem *root) const;

private:
    ObjectSet *_set;
    CanvasItemPtr<CanvasItemGroup> _group;

    std::vector<WorkItem> _work_items;
    std::vector<ItemPair> _screen_items;
    WorkItem _work_task;
    VisualItem _screen_task;
    bool _add_task;
    bool _dark = false;

    // Lists of _work_items which can be brought back.
    std::vector<std::vector<WorkItem>> _undo;
    std::vector<std::vector<WorkItem>> _redo;

    sigc::scoped_connection desk_modified_connection;

    void redraw_item(CanvasItemBpath &bpath, bool selected, TaskType task, bool image);
    void redraw_items();
};

} // namespace Inkscape

#endif // INKSCAPE_UI_TOOLS_BOOLEANS_BUILDER_H

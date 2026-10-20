// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 *
 *//*
 * Authors:
 *   Martin Owens
 *
 * Copyright (C) 2022 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_TOOLS_BOOLEANS_SUBITEMS_H
#define INKSCAPE_UI_TOOLS_BOOLEANS_SUBITEMS_H

#include <2geom/pathvector.h>

class SPItem;
class SPStyle;

namespace Inkscape {

class SubItem;
using WorkItem = std::shared_ptr<SubItem>;
using WorkItems = std::vector<WorkItem>;

/**
 * When an item is broken, each broken part is represented by
 * the SubItem class. This class hold information such as the
 * original items it originated from and the paths that it
 * consists of.
 **/
class SubItem
{
public:

    SubItem(Geom::PathVector paths, SPItem *root, SPItem *item, SPStyle *style)
        : _paths(std::move(paths))
        , _root(root)
        , _item(item)
        , _style(style)
        , _is_image(_get_is_image(item))
    {}

    SubItem(const SubItem &copy)
        : SubItem(copy._paths, copy._root, copy._item, copy._style)
    {}

    SubItem &operator+=(SubItem const &other);

    bool contains(Geom::Point const &pt) const;

    Geom::PathVector const &get_pathv() const { return _paths; }
    SPItem *get_root() const { return _root; }
    SPItem *get_item() const { return _item; }
    SPStyle *getStyle() const { return _style; }
    bool is_image() const { return _is_image; }

    static WorkItems build_mosaic(std::vector<SPItem*> &&items);
    static WorkItems build_flatten(std::vector<SPItem*> &&items);

    bool getSelected() const { return _selected; }
    void setSelected(bool selected) { _selected = selected; }

private:
    static bool _get_is_image(SPItem const *item);

    Geom::PathVector _paths;
    SPItem *_root;
    SPItem *_item;
    SPStyle *_style;
    bool _selected = false;
    bool _is_image = false;
};

} // namespace Inkscape

#endif // INKSCAPE_UI_TOOLS_BOOLEANS_SUBITEMS_H

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Render some items as translucent in a document rendering stack.
 *
 * Author:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2021-2024 Martin Owens
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "translucency-group.h"

#include "display/drawing-item.h"

#include "document.h"
#include "object/sp-item.h"
#include "object/sp-root.h"
#include "style.h"

namespace Inkscape::Display {

TranslucencyGroups::TranslucencyGroups(unsigned int dkey)
    : _dkey(dkey)
{}

/**
 * Produce a new translucency group, each group overrides any previous
 * group when setting solid items allowing for different tools to set
 * their in-context solid items without resetting each other.
 *
 * Use `removeGroupKey(key)` when the context is finished.
 */
unsigned TranslucencyGroups::createGroupKey(double translucency, bool fallback)
{
    static unsigned count = 0; // Reserve 0 for disabled
    _groups[++count] = Group({
        .translucency = translucency,
        .fallback = fallback
    });
    _update();
    return count;
}

/**
 * Remove the given group_key, allowing us to re-request a key later if needed
 */
void TranslucencyGroups::removeGroupKey(unsigned group_key)
{
    _groups.erase(group_key);
    _update();
}

/**
 * Set a specific item as the solid item, all other items are made translucent.
 *
 * @arg group_key - The unique key produced by createGroupKey()
 * @arg item      - The optional item to make solid. Everything else is translucent.
 *                  Setting nullptr here will disable this translucency group and
 *                   will fallback to the previous group if falback was set to true.
 * @arg invert    - If true the translucency is inversed and this item is solid
 */
void TranslucencyGroups::_set_item(unsigned group_key, SPItem *item, bool invert)
{
    if (auto iter = _groups.find(group_key); iter != _groups.end()) {
        // Set the target item, this prevents rerunning rendering.
        if (iter->second.item != item) {
            iter->second.item = item;
            iter->second.invert = invert;
            _update();
        }
    } else if (group_key != 0) {
        std::cerr << "TranslucencyGroups::_set_item: Invalid TranslucencyGroup Id = " << group_key << "\n";
    }
}


void TranslucencyGroups::setTranslucency(unsigned group_key, double translucency)
{
    if (auto iter = _groups.find(group_key); iter != _groups.end()) {
        // Set the target item, this prevents rerunning rendering.
        if (iter->second.translucency != translucency) {
            iter->second.translucency = translucency;
            _update();
        }
    } else if (group_key != 0) {
        std::cerr << "TranslucencyGroups::setTranslucency: Invalid TranslucencyGroup Id = " << group_key << "\n";
    }
}

void TranslucencyGroups::_update()
{
    // Reset all the items in the list.
    for (auto &item : _translucent_items) {
        if (auto arenaitem = item->get_arenaitem(_dkey)) {
            arenaitem->setOpacityOverride({});
        }
    }
    _translucent_items.clear();

    for (auto iter = _groups.rbegin(); iter != _groups.rend(); ++iter) {
        auto &group = iter->second;
        if (group.item) {
            auto root = group.item->document->getRoot();
            if (group.invert) {
                // Generate a list of the inverse items
                _generateTranslucentItems(group.item, root);
            } else {
                _translucent_items.push_back(group.item);
            }

            for (auto &item : _translucent_items) {
                Inkscape::DrawingItem *arenaitem = item->get_arenaitem(_dkey);
                arenaitem->setOpacityOverride(group.translucency);
            }

            return; // Last added, positive result wins.
        }
        if (!group.fallback) {
            return; // This group asks to disable all previous groups
        }
    }
}

/**
 * Generate a new list of sibling items (recursive)
 */
void TranslucencyGroups::_generateTranslucentItems(SPItem *item, SPItem *parent)
{
    if (parent == item)
        return;
    if (parent->isAncestorOf(item)) {
        for (auto &child: parent->children) {
            if (auto child_item = cast<SPItem>(&child)) {
                _generateTranslucentItems(item, child_item);
            }
        }
    } else {
        _translucent_items.push_back(parent);
    }
}

} /* namespace Inkscape::Display */

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

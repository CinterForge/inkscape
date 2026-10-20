// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Render some items as translucent in a document rendering stack.
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2021-2024 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_DISPLAY_TRANSLUCENCY_GROUPS
#define SEEN_DISPLAY_TRANSLUCENCY_GROUPS

#include <vector>
#include <map>

class SPItem;

namespace Inkscape::Display {

class TranslucencyGroups {
public:

    TranslucencyGroups(unsigned int dkey);

    unsigned createGroupKey(double translucency = 0.2, bool fallback = true);
    void removeGroupKey(unsigned group_key);
    void setTranslucency(unsigned group_key, double translucency = 0.2);

    void setSolidItem(unsigned group_key, SPItem *item) { _set_item(group_key, item, true); }
    void setTranslucentItem(unsigned group_key, SPItem *item) { _set_item(group_key, item, false); }
private:
    unsigned int _dkey;

    struct Group {
        double translucency;
        SPItem *item = nullptr;
        bool invert = true;
        bool fallback = true;
    };

    std::map<unsigned, Group> _groups;
    std::vector<SPItem *> _translucent_items;

    void _set_item(unsigned group_key, SPItem *item, bool invert);
    void _generateTranslucentItems(SPItem *item, SPItem *parent);
    void _update();
};

} /* namespace Inkscape::Display */

#endif // SEEN_DISPLAY_TRANSLUCENCY_GROUPS
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

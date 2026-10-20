// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Create a memory of styles we can use to compare.
 *
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2025 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "remember-styles.h"

namespace Inkscape::Extension::Internal {

StyleMap StyleMemory::get_changes(SPStyle const *style) const
{
    StyleMap map;
    auto const &current = get_state();

    for (auto prop : style->properties()) {
        auto attr = prop->id();
        // get_value ALWAYS gives a value, even if it's from a cascade. This is intentional
        // as we want to know if the effective value has changed, not if the style would write a value out.
        auto value = prop->get_value().raw();

        if (_attrs.contains(attr)) {
            auto const it = current.find(attr);
            if (it == current.end() || it->second != value) {
                map[attr] = std::move(value);
            }
        }
    }
    return map;
}

StyleMap StyleMemory::get_ifset(SPStyle const *style) const
{
    StyleMap map;
    for (auto prop : style->properties()) {
        if (prop->set) {
            map[prop->id()] = prop->get_value();
        }
    }
    return map;
}

} // namespace Inkscape::Extension::Internal

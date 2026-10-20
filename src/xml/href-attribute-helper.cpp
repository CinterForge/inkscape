// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Thomas Holder
 *
 * Copyright (C) 2022 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "href-attribute-helper.h"

namespace Inkscape {

/**
 * Get the 'href' or 'xlink:href' (fallback) attribute from an XML node.
 *
 * @return The observed key and its value
 */
std::pair<char const *, char const *> getHrefAttribute(XML::Node const &node)
{
    if (auto value = node.attribute("href")) {
        return {"href", value};
    }

    if (auto value = node.attribute("xlink:href")) {
        return {"xlink:href", value};
    }

    return {"xlink:href", nullptr};
}

/**
 * If the 'href' attribute already exists for the given node, then set a new
 * value for it. Otherwise set the value for 'xlink:href'.
 */
void setHrefAttribute(XML::Node &node, Util::const_char_ptr value)
{
    if (node.attribute("href")) {
        node.setAttribute("href", value);
    } else {
        node.setAttribute("xlink:href", value);
    }
}

} // namespace Inkscape

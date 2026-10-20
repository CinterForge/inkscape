// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Utility functions related to parsing and validation of XML attributes.
 *//*
 * Authors:
 *   tavmjong
 *
 * Copyright (C) 2016 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_ATTRIBUTE_REL_UTIL_H
#define SEEN_ATTRIBUTE_REL_UTIL_H

#include <glibmm/ustring.h>

#include "xml/sp-css-attr.h"

using Inkscape::XML::Node;

/**
 * Utility functions for cleaning XML tree.
 */

/**
 * Enum for preferences
 */
enum SPAttrClean
{
    // clang-format off
    SP_ATTRCLEAN_ATTR_WARN      =  1,
    SP_ATTRCLEAN_ATTR_REMOVE    =  2,
    SP_ATTRCLEAN_STYLE_WARN     =  4,
    SP_ATTRCLEAN_STYLE_REMOVE   =  8,
    SP_ATTRCLEAN_DEFAULT_WARN   = 16,
    SP_ATTRCLEAN_DEFAULT_REMOVE = 32
    // clang-format on
};

/**
 * Get preferences
 */
unsigned int sp_attribute_clean_get_prefs();

/**
 * Remove or warn about inappropriate attributes and useless style properties.
 * repr: the root node in a document or any other node.
 */
void sp_attribute_clean_tree(Node *repr);

/**
 * Recursively clean.
 * repr: the root node in a document or any other node.
 * pref_attr, pref_style, pref_defaults: ignore, delete, or warn.
 */
void sp_attribute_clean_recursive(Node *repr, unsigned int flags);

/**
 * Clean one element (attributes and style properties).
 */
void sp_attribute_clean_element(Node *repr, unsigned int flags);

/**
 * Clean style properties for one element.
 */
void sp_attribute_clean_style(Node *repr, unsigned int flags);

/**
 * Clean style properties for one style string.
 */
Glib::ustring sp_attribute_clean_style(Node *repr, gchar const *string, unsigned int flags);

/**
 * Clean style properties for one CSS.
 */
void sp_attribute_clean_style(Node *repr, SPCSSAttr *css, unsigned int flags);

/**
 * Remove CSS style properties with default values.
 */
void sp_attribute_purge_default_style(SPCSSAttr *css, unsigned int flags);

/**
 * Check one attribute on an element
 */
bool sp_attribute_check_attribute(Glib::ustring const &element, Glib::ustring const &id, Glib::ustring const &attribute,
                                  bool warn);

/**
 * @brief Check whether the 'd' attribute on a <path> element parses correctly.
 *
 * @param d The value of the 'd' attribute.
 * @return false if the value causes a parser error, true otherwise.
 */
bool sp_is_valid_svg_path_d(Glib::ustring const &d);

#endif // SEEN_ATTRIBUTE_REL_UTIL_H

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

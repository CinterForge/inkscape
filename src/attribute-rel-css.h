// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_ATTRIBUTE_REL_CSS_H
#define SEEN_ATTRIBUTE_REL_CSS_H

/*
 * attribute-rel-css.h
 *
 *  Created on: Jul 25, 2011
 *      Author: abhishek
 */

#include <map>
#include <set>
#include <string>

#include <glibmm/ustring.h>

// This data structure stores the valid (element -> set of CSS properties) pair
typedef std::map<Glib::ustring, std::set<Glib::ustring>> hashList;

/*
 * Utility class that helps check whether a given element -> CSS property is
 * valid or not and whether the value assumed by a CSS property has a default
 * value.
 */
class SPAttributeRelCSS
{
public:
    static bool findIfValid(Glib::ustring const &property, Glib::ustring const &element);
    static bool findIfDefault(Glib::ustring const &property, Glib::ustring const &value);
    static bool findIfInherit(Glib::ustring const &property);
    static bool findIfProperty(Glib::ustring const &property);

private:
    SPAttributeRelCSS();
    SPAttributeRelCSS(const SPAttributeRelCSS &) = delete;
    SPAttributeRelCSS &operator=(const SPAttributeRelCSS &) = delete;
    static SPAttributeRelCSS &getInstance();

private:
    /*
     * Allows checking whether data loading is to be done for element -> CSS properties
     * or CSS property -> default value.
     */
    enum storageType
    {
        prop_element_pair,
        prop_defValue_pair
    };
    static bool foundFileProp;
    static bool foundFileDefault;
    hashList propertiesOfElements;

    // Data structure to store CSS property and default value pair
    std::map<Glib::ustring, Glib::ustring> defaultValuesOfProps;
    std::map<Glib::ustring, gboolean> inheritProps;
    bool readDataFromFileIn(Glib::ustring const &fileName, storageType type);
};

#endif // SEEN_ATTRIBUTE_REL_CSS_H

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

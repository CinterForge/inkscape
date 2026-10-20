// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2011 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
/** @file
 * @brief Some functions relevant to sorting reprs by position within a document.
 */

#ifndef SEEN_XML_REPR_SORTING_H
#define SEEN_XML_REPR_SORTING_H

namespace Inkscape::XML { class Node; }

Inkscape::XML::Node *lowest_common_ancestor(Inkscape::XML::Node *a, Inkscape::XML::Node *b);
Inkscape::XML::Node const *lowest_common_ancestor(Inkscape::XML::Node const *a, Inkscape::XML::Node const *b);

bool is_descendant_of(Inkscape::XML::Node const *descendant, Inkscape::XML::Node const *ancestor);

/**
 * Returns the child of \a ancestor that contains \a descendant, or nullptr if none exists.
 * If either \a ancestor or \a descendant is null, then nullptr is returned.
 */
Inkscape::XML::Node *find_containing_child(Inkscape::XML::Node *descendant, Inkscape::XML::Node *ancestor);
Inkscape::XML::Node const *find_containing_child(Inkscape::XML::Node const *descendant, Inkscape::XML::Node const *ancestor);

#endif // SEEN_XML_REPR_SOTRING_H
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

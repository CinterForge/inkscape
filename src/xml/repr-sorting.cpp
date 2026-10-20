// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "repr-sorting.h"

#include "util/forward-pointer-iterator.h"  // for ForwardPointerIterator
#include "util/longest-common-suffix.h"     // for nearest_common_ancestor
#include "xml/node-iterators.h"             // for NodeParentIteratorStrategy
#include "xml/node.h"                       // for Node, NodeType

Inkscape::XML::Node const *lowest_common_ancestor(Inkscape::XML::Node const *a, Inkscape::XML::Node const *b)
{
    auto ancestor = Inkscape::Algorithms::nearest_common_ancestor<Inkscape::XML::NodeConstParentIterator>(a, b, nullptr);

    if (ancestor && ancestor->type() != Inkscape::XML::NodeType::DOCUMENT_NODE) {
        return ancestor;
    }

    return nullptr;
}

Inkscape::XML::Node *lowest_common_ancestor(Inkscape::XML::Node *a, Inkscape::XML::Node *b)
{
    return const_cast<Inkscape::XML::Node*>(
        lowest_common_ancestor(const_cast<Inkscape::XML::Node const*>(a),
                               const_cast<Inkscape::XML::Node const*>(b))
        );
}

bool is_descendant_of(Inkscape::XML::Node const *descendant, Inkscape::XML::Node const *ancestor)
{
    while (true) {
        if (!descendant) {
            return false;
        } else if (descendant == ancestor) {
            return true;
        } else {
            descendant = descendant->parent();
        }
    }
}

Inkscape::XML::Node const *find_containing_child(Inkscape::XML::Node const *descendant, Inkscape::XML::Node const *ancestor)
{
    while (true) {
        if (!descendant) {
            return nullptr;
        }
        auto parent = descendant->parent();
        if (parent == ancestor) {
            return descendant;
        }
        descendant = parent;
    }
}

Inkscape::XML::Node *find_containing_child(Inkscape::XML::Node *descendant, Inkscape::XML::Node *ancestor)
{
    return const_cast<Inkscape::XML::Node*>(
        find_containing_child(
            const_cast<Inkscape::XML::Node const*>(descendant),
            const_cast<Inkscape::XML::Node const*>(ancestor))
        );
}

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

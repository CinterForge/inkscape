// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UTIL_TREEIFY_H
#define INKSCAPE_UTIL_TREEIFY_H

#include <functional>
#include <vector>

namespace Inkscape::Util {

struct TreeifyResult
{
    std::vector<int> preorder; ///< The preorder traversal of the nodes, a permutation of {0, ..., N - 1}.
    std::vector<int> num_children; ///< For each node, the number of direct children.
};

/**
 * Given a collection of nodes 0 ... N - 1 and a containment function,
 * attempt to organise the nodes into a tree (or forest) such that
 * contains(i, j) is true precisely when i is an ancestor of j.
 */
TreeifyResult treeify(int N, std::function<bool(int, int)> const &contains);

} // namespace Inkscape::Util

#endif // INKSCAPE_UTIL_TREEIFY_H
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

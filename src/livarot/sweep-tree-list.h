// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2010 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
/** @file
 * @brief SweepTreeList definition
 */

#ifndef INKSCAPE_LIVAROT_SWEEP_TREE_LIST_H
#define INKSCAPE_LIVAROT_SWEEP_TREE_LIST_H

class Shape;
class SweepTree;

/**
 * The sweepline tree to store a linear sequence of edges that intersect with the sweepline
 * in the exact order.
 *
 * This could just be a double-linked list but it also an AVL search tree
 * to quickly find edges.
 *
 * In this documentation, a SweepTree instance is referred to as a node.
 *
 * This is a class to store the nodes. Most interesting stuff happens in the class
 * SweepTree or its parent class AVLTree.h This just keeps the list of nodes and the pointer
 * to the root node.
 */
class SweepTreeList {
public:
    int nbTree;          /*!< Number of nodes in the tree. */
    int const maxTree;   /*!< Max number of nodes in the tree. */
    SweepTree *trees;    /*!< The array of nodes. */
    SweepTree *racine;   /*!< Root of the tree. */

    /**
     * Constructor to create a new SweepTreeList.
     *
     * @param s The number of maximum nodes it should be able to hold.
     */
    SweepTreeList(int s);

    /**
     * The destructor. But didn't have to be virtual.
     */
    virtual ~SweepTreeList();

    /**
     * Create a new node and add it. This doesn't do any insertion in tree though. It just
     * creates the node and puts it in the list of nodes. The actual insertion would need
     * to be done by calling SweepTree::Insert or in the speical case SweepTree::InsertAt.
     *
     * @param iSrc A pointer to the shape.
     * @param iBord The edge index.
     * @param iWeight Weight of the of the edge. Weight of 2 is equivalent of two identical edges
     * with same direction on top of each other.
     * @param iStartPoint The point at which this node got added. (the upper endpoint if sweeping
     * top to bottom).
     * @param iDst Supposed to be the destination shape. The Shape on which Shape::ConvertToShape
     * was called. `iSrc` is the parameter that was passed in Shape::ConvertToShape. Useless
     * parameter though, not used.
     * @param The address of the newly added node. Can be used later for Inserting it or anything
     * else.
     */
    SweepTree *add(Shape *iSrc, int iBord, int iWeight, int iStartPoint, Shape *iDst);
};


#endif /* !INKSCAPE_LIVAROT_SWEEP_TREE_LIST_H */

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

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef INKSCAPE_LIVAROT_SWEEP_TREE_H
#define INKSCAPE_LIVAROT_SWEEP_TREE_H

#include "livarot/AVL.h"
#include <2geom/point.h>

class Shape;
class SweepEvent;
class SweepEventQueue;
class SweepTreeList;


/**
 * One node in the AVL tree of edges.
 * Note that these nodes will be stored in a dynamically allocated array, hence the Relocate() function.
 */

/**
 * A node in the sweep tree. For details about the sweep tree, what it is, what we do with it,
 * why it's needed, check out SweepTreeList's documentation.
 *
 * Explanation of what is stored in evt and why:
 * Say you have two edges in the sweepline `left` and `right` and an intersection is detected between
 * the two. An intersection event (of type SweepEvent) is created and that event object stores
 * pointer to the `left` and `right` edges (of type SweepTree). The left edge's evt[RIGHT]/evt[1]
 * stores the pointer to the intersection event and the right edge's evt[LEFT]/evt[0] also stores
 * it. This is done for a very important reason. If any point in time, either the LEFT or the RIGHT
 * edge have to change their position in the sweepline for any reason at all (before the
 * intersection point comes), we need to immediately delete that event from our list, cuz the edges
 * are no longer together.
 */
class SweepTree:public AVLTree
{
public:
    SweepEvent *evt[2];   /*!< Intersection with the edge on the left and right (if any). */

    Shape *src;           /*!< Shape from which the edge comes.  (When doing boolean operation on polygons,
                               edges can come from 2 different polygons.) */
    int bord;             /*!< Edge index in the Shape. */

    SweepTree();
    ~SweepTree() override;

    // Inits a brand new node.

    /**
     * Does what usually a constructor does.
     *
     * @param iSrc The pointer to the shape from which this edge comes from.
     * @param iBord The edge index in the shape.
     * @param iWeight The weight of the edge. Used along with edge's orientation to determine
     * sens.
     * @param iStartPoint Point index in the *result* Shape associated with the upper end of the
     * edge
     */
    void MakeNew(Shape *iSrc, int iBord, int iWeight, int iStartPoint);
    // changes the edge associated with this node
    // goal: reuse the node when an edge follows another, which is the most common case

    /**
     * Reuse this node by just changing the variables.
     *
     * This is useful when you have one edge ending at a point and another one starting at the
     * same point. So instead of deleting one and adding another at exactly the same location,
     * you can just reuse the old one and change the variables.
     *
     * @param iSrc The pointer to the shape from which this edge comes from.
     * @param iBord The edge index in the shape.
     * @param iWeight The weight of the edge. Used along with edge's orientation to determine
     * sens.
     * @param iStartPoint Point index in the *result* Shape associated with the upper end of the
     * edge
     */
    void ConvertTo(Shape *iSrc, int iBord, int iWeight, int iStartPoint);

    // Delete the contents of node.

    /**
     * Delete this node. Make sure to change the pointers in any intersection event (that points to
     * this node).
     */
    void MakeDelete();

    // utilites

    // the find function that was missing in the AVLTrree class
    // the return values are defined in LivarotDefs.h

    /**
     * Find where the new edge needs to go in the sweepline tree.
     *
     * Please check out the documentation of the other version of Find that takes
     * a point as input, this function is exactly identical except one block of code.
     *
     * For that special block of code, see this picture.
     *
     * @image html livarot-images/find-point-same.svg
     *
     * The rest of the function is the same as the other Find function so you're already
     * familiar with how it works. The difference is how the y == 0 case is handled. When we are
     * within that block, it's established that the upper endpoint of the new edge is on our edge.
     * Now we see how the rest of the edge is orientated with respect to our edge so we can decide
     * where to put it. To do this, we get the edge vector of the new edge and make sure it's top
     * to bottom or if horizontal left to right. Then we rotate this new edge vector
     * counter-clockwise by 90 degrees and shift it so that it starts at the same point as
     * bNorm.ccw() does. The picture takes two cases simultaneously, one with edge red and the
     * other being edge magenta. Their normals are shown with dotted lines and the shifted versions
     * are lighter in color.
     *
     * Now if sweepSens is false, we take a cross product of new edge's normal with this edge's
     * normal or cross(nNorm, bNorm). To figure out the direction of a cross product in SVG
     * coordinates use this variation of right hand rule. Let index finger point to vector A. Let
     * middle finger point to vector B. If thumb points out of page, cross product is negative, if
     * it points inside the page, cross product is positive. Now you can see how when sweepSens is
     * false, the cross product checks out with the orientation of the edges. If the cross product
     * turns out to be zero, then we take dot product and let that decide. TODO: Which orientation
     * will do what though?
     *
     * What about the other condition of sweepSens? When would sweepSens be true and how would that
     * be useful. I think it has to do with sweeping in the opposite direction as a comment in the
     * function already says.
     *
     * @image html livarot-images/find-point-same-opp-sweepsense.svg
     *
     * In this image, you can see the edges go bottom to top and then, you'd need cross(bNorm,
     * nNorm) to figure out the correct orientation.
     *
     * @param iPt The point whose location we need to find in the sweepline.
     * @param newOne The new edge that we wanna insert. To which point iPt belongs.
     * @param insertL The edge that should go on the left (looking from the position where the new
     * edge should go). This is set by the function.
     * @param insertR The edge that should go on the right (looking form the position where the new
     * edge should go). This is set by the function.
     * @param sweepSens TODO: Why is this set to true? When it should be false when coming from
     * ConvertToShape?
     */
    int Find(Geom::Point const &iPt, SweepTree *newOne, SweepTree *&insertL,
             SweepTree *&insertR, bool sweepSens = true);

    /**
     * Find the place for a point (not an edge) in the sweepline tree.
     *
     * @image html livarot-images/find-point.svg
     *
     * To learn the algorithm, check the comments in the function body while referring back to
     * the picture shown above. A brief summary follows.
     *
     * We start by taking our edge vector and if it goes bottom to top, or is horizontal and goes
     * right to left, we flip its direction. In the picture bNorm shows this edge vector after any
     * flipping. We rotate bNorm by 90 degrees counter-clockwise to get the normal vector.
     * Then we take the start point of the edge vector (the original start point not the
     * one after flipping) and draw a vector from the start point to the point whose position we
     * are trying to find (iPt), we call this the diff vector. In the picture I have drawn these
     * for three points red, blue and green. Now we take the dot product of this diff with the
     * normal vectors. As you would now, a dot product has the formula:
     * \f[ \vec{A}\cdot\vec{B} = |\vec{A}||\vec{B}|\cos\theta \f]
     * \f$ \theta \f$ here is the angle between the two. As you would know, \f$ \cos \f$ is
     * positive as long as the angle is between +90 and -90. At 90 degrees it becomes zero and
     * greater than 90 or smaller than -90 it becomes negative. Thus the sign of the dot product
     * can be used as an indicator of the angle between the normal vector and the diff vector.
     * If this angle is within 90 to -90, it means the point lies to the right of the original
     * edge. If it lies on 90 or -90, it means the point lies on the same line as the edge and
     * if it's greater than 90 or smaller than -90, the point lies on the left side of the
     * original edge.
     *
     * One thing to note, the blue point here is kinda wrong. You can't have another edge starting
     * above the already existing edge when sweeping done (that's just not possible). So sorry
     * about that. But the math checks out anyways. TODO: Maybe fix this to avoid confusion?
     *
     * One important point to see here is that the edge vector will be flipped however it's start
     * point remains the same as the original one, this is not a problem as you can see in the
     * image below. I chose the other point as the start point and everything works out the same.
     *
     * @image html livarot-images/find-point-2.svg
     *
     * I changed the starting point and redrew the diff vectors. Then I projected them such that
     * their origin is the same as that of the normal. Measuring the angles again, everything
     * remains the same.
     *
     * There is one more confusion part you'd find in this function. The part where left and right
     * child are checked and you'd see child as well as elem pointers being used.
     *
     * @image html livarot-images/sweep-tree.svg
     *
     * The picture above shows you a how the sweepline tree structure looks like. I've used numbers
     * instead of actual edges just for the purpose of illustration. The structure is an AVL tree
     * as well as double-linked list. The nodes are arranged in a tree structure that balances
     * itself but each node has two more points elem[LEFT] and elem[RIGHT] that can be used to
     * navigate the linked list. In reality, the linked list structure is what's needed, but having
     * an AVL tree makes searching really easy.
     *
     * See the comments in the if blocks in the function body. I've given examples from this
     * diagram to explain stuff.
     *
     * @param iPt The point whose position we are trying to find.
     * @param insertL The edge on the left (from the location where this point is supposed to go)
     * @param insertR The edge on the right (from the location where this point is supposed to go)
     *
     * @return The found_* codes from LivarotDefs.h. See the file to learn about them.
     */
    int Find(Geom::Point const &iPt, SweepTree *&insertL, SweepTree *&insertR);

    /// Remove sweepevents attached to this node.

    /**
     * Remove any events attached to this node.
     *
     * Since the event the other node referring to this event will also have it's
     * evt value cleared.
     *
     * @param queue Reference to the event queue.
     */
    void RemoveEvents(SweepEventQueue &queue);

    /**
     * Remove event on the side s if it exists from event queue.
     *
     * Since the event the other node referring to this event will also have it's
     * evt value cleared.
     *
     * @param queue Reference to the event queue.
     * @param s The side to remove the event from.
     */
    void RemoveEvent(SweepEventQueue &queue, Side s);

    // overrides of the AVLTree functions, to account for the sorting in the tree
    // and some other stuff
    int Remove(SweepTreeList &list, SweepEventQueue &queue, bool rebalance = true);

    /**
     * Insert this node at it's appropriate position in the sweepline tree.
     *
     * The function works by calling the Find function to let it find the appropriate
     * position where this node should go, which it does by traversing the whole search
     * tree.
     *
     * @param list A reference to the sweepline tree.
     * @param queue A reference to the event queue.
     * @param iDst Pointer to the shape to which this edge belongs to.
     * @param iAtPoint The point at which we are adding this edge.
     * @param rebalance TODO: Confirm this but most likely has to do with whether AVL should
     * rebalance or not.
     * @param sweepSens TODO: The same variable as in Find, has to do with sweepline direction.
     */
    int Insert(SweepTreeList &list, SweepEventQueue &queue, Shape *iDst,
               int iAtPoint, bool rebalance = true, bool sweepSens = true);

    /**
     * Insert this node near an existing node.
     *
     * This is a simplification to the other Insert function. The normal Insert function would
     * traverse the whole tree to find an appropriate place to add the current node. There are
     * situations where we just added an edge and we have more edges connected to the same point
     * that we wanna add. This function can be used to directly traverse left and right around
     * the existing node to see where this edge would fit. Saves us full Find call. This searching
     * is exactly how you'd insert something in a doubly-linked list. The function uses cross
     * products to see where this edge should go relative to the existing one and then has loops
     * to find the right spot for it.
     *
     * @image html livarot-images/find-point.svg
     *
     * I'll use this image to explain some stuff in the code body.
     *
     * @param list A reference to the sweepline tree.
     * @param queue A reference to the event queue.
     * @param iDst Pointer to the shape to which this edge belongs to.
     * @param insNode Pointer to the node near which this is to be added.
     * @param fromPt The point at which we are adding this edge.
     * @param rebalance TODO: Confirm this but most likely has to do with whether AVL should
     * rebalance or not.
     * @param sweepSens TODO: The same variable as in Find, has to do with sweepline direction.
     */
    int InsertAt(SweepTreeList &list, SweepEventQueue &queue, Shape *iDst,
                 SweepTree *insNode, int fromPt, bool rebalance = true, bool sweepSens = true);

    /// Swap nodes, or more exactly, swap the edges in them.

    /**
     * Used to swap two nodes with each other. Basically the data in the nodes are swapped
     * not their addresses or locations in memory.
     *
     * Hence anyone referencing these nodes will get invalid (or unexpected) references since
     * the data got swapped out. Therefore you must clear any events that might have references to
     * these nodes.
     *
     * @param list Reference to the sweepline tree. Useless parameter.
     * @param queue Reference to the event queue. Useless parameter.
     */
    void SwapWithRight(SweepTreeList &list, SweepEventQueue &queue);

    /**
     * TODO: Probably has to do with some AVL relocation. Only called once from Node removal code.
     */
    void Relocate(SweepTree *to);
};


#endif /* !INKSCAPE_LIVAROT_SWEEP_TREE_H */

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

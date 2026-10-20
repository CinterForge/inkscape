// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "livarot/sweep-event-queue.h"
#include "livarot/sweep-tree-list.h"
#include "livarot/sweep-tree.h"
#include "livarot/sweep-event.h"
#include "livarot/Shape.h"


/*
 * the AVL tree holding the edges intersecting the sweepline
 * that structure is very sensitive to anything
 * you have edges stored in nodes, the nodes are sorted in increasing x-order of intersection
 * with the sweepline, you have the 2 potential intersections of the edge in the node with its
 * neighbours, plus the fact that it's stored in an array that's realloc'd
 */

SweepTree::SweepTree()
{
    src = nullptr;
    bord = -1;
    evt[LEFT] = evt[RIGHT] = nullptr;
}

SweepTree::~SweepTree()
{
    MakeDelete();
}

void
SweepTree::MakeNew(Shape *iSrc, int iBord, int iWeight, int iStartPoint)
{
    AVLTree::MakeNew();
    ConvertTo(iSrc, iBord, iWeight, iStartPoint);
}

void
SweepTree::ConvertTo(Shape *iSrc, int iBord, int iWeight, int iStartPoint)
{
    src = iSrc;
    bord = iBord;
    evt[LEFT] = evt[RIGHT] = nullptr;
}


void SweepTree::MakeDelete()
{
    for (int i = 0; i < 2; i++) {
        if (evt[i]) {
            evt[i]->sweep[1 - i] = nullptr;
        }
        evt[i] = nullptr;
    }

    AVLTree::MakeDelete();
}


// find the position at which node "newOne" should be inserted in the subtree rooted here
// we want to order with respect to the order of intersections with the sweepline, currently 
// lying at y=px[1].
// px is the upper endpoint of newOne
int
SweepTree::Find(Geom::Point const &px, SweepTree *newOne, SweepTree *&insertL,
                SweepTree *&insertR, bool sweepSens)
{
    // get the edge associated with this node: one point+one direction
    // since we're dealing with line, the direction (bNorm) is taken downwards
    Geom::Point bOrig, bNorm;
    bOrig = src->pData[src->getEdge(bord).st].rx;
    bNorm = src->eData[bord].rdx;
    if (src->getEdge(bord).st > src->getEdge(bord).en) {
        bNorm = -bNorm;
    }
    // rotate to get the normal to the edge
    bNorm=bNorm.ccw();

    Geom::Point diff;
    diff = px - bOrig;

    // compute (px-orig)^dir to know on which side of this edge the point px lies
    double y = 0;
    //if ( startPoint == newOne->startPoint ) {
    //   y=0;
    //} else {
    y = dot(bNorm, diff);
    //}
    //y*=invDirLength;
    if (fabs(y) < 0.000001) {
        // that damn point px lies on me, so i need to consider to direction of the edge in
        // newOne to know if it goes toward my left side or my right side
        // sweepSens is needed (actually only used by the Scan() functions) because if the sweepline goes upward,
        // signs change
        // prendre en compte les directions
        Geom::Point nNorm;
        nNorm = newOne->src->eData[newOne->bord].rdx;
        if (newOne->src->getEdge(newOne->bord).st >
            newOne->src->getEdge(newOne->bord).en)
	{
            nNorm = -nNorm;
	}
        nNorm=nNorm.ccw();

        if (sweepSens) {
            y = cross(bNorm, nNorm);
        } else {
            y = cross(nNorm, bNorm);
        }
        if (y == 0) {
            y = dot(bNorm, nNorm);
            if (y == 0) {
                insertL = this;
                insertR = static_cast<SweepTree *>(elem[RIGHT]);
                return found_exact;
            }
        }
    }
    if (y < 0) {
        if (child[LEFT]) {
            return (static_cast<SweepTree *>(child[LEFT]))->Find(px, newOne,
                                                               insertL, insertR,
                                                               sweepSens);
	} else {
            insertR = this;
            insertL = static_cast<SweepTree *>(elem[LEFT]);
            if (insertL) {
                return found_between;
            } else {
                return found_on_left;
	    }
	}
    } else {
        if (child[RIGHT]) {
            return (static_cast<SweepTree *>(child[RIGHT]))->Find(px, newOne,
                                                                insertL, insertR,
                                                                sweepSens);
	} else {
            insertL = this;
            insertR = static_cast<SweepTree *>(elem[RIGHT]);
            if (insertR) {
                return found_between;
	    } else {
                return found_on_right;
	    }
	}
    }
    return not_found;
}

// only find a point's position
int
SweepTree::Find(Geom::Point const &px, SweepTree * &insertL,
		 SweepTree * &insertR)
{
    Geom::Point bOrig, bNorm;
    // The start point of the original edge vector
    bOrig = src->pData[src->getEdge(bord).st].rx;
    // The edge vector
    bNorm = src->eData[bord].rdx;
    // Flip the edge vector if it's bottom to top or horizontal and right to left
    if (src->getEdge(bord).st > src->getEdge(bord).en)
    {
        bNorm = -bNorm;
    }
    // rotate the edge vector counter clockwise by 90 degrees
    bNorm=bNorm.ccw();

    // draw a vector from the start point to the actual point
    Geom::Point diff;
    diff = px - bOrig;

    double y = 0;
    y = dot(bNorm, diff); // take the dot product
    // ANALOGY FROM DIAGRAM (See doc in header file):
    // this case is the same as if we are at node (15) and want to add (15). Usually, you can't
    // add same stuff in a binary search tree but here it's fine to do so (I guess)
    // In that case, we have found an exact match and the point belongs between 15 and the one
    // on it's right (16).
    if (y == 0) // point lies on the edge (or at least the line of the edge)
    {
        insertL = this;
        insertR = static_cast<SweepTree *>(elem[RIGHT]);
        return found_exact;
    }

    // ANALOGY FROM DIAGRAM (See doc in header file):
    // This is the same as inserting 3 while standing at 10, or inserting 3 while at 5 or inserting
    // 13 while at 15 or inserting 1 while at 2 or inserting 11 while at 12.
    if (y < 0) // lies to the left of the edge
    {
        if (child[LEFT]) // is there child on left? This is true at 10, 5, 15 but not at the nodes such as 2, 6, 12, 16
        {
            // if there is a child on left, let the child do the searching
            return (static_cast<SweepTree *>(child[LEFT]))->Find(px, insertL, // if yes, let that child do the finding now
                    insertR);
        }
        else // no child on the left? Means either a leaf node or node has no child on left.
        {
            // well we are sure that there is no child on the left, which means this new node goes
            // to the left of this node, but that doesn't really mean there no left element in the
            // linked list, there sure can be. For example, if you're inserting 11 while standing
            // at 12, there is no left child, but in the linked list, there is 10 to the left.
            insertR = this;
            insertL = static_cast<SweepTree *>(elem[LEFT]);
            if (insertL)
            {
                return found_between;
            }
            else // however, if you're at 2 and inserting 1, there is no left child, but there is also nothing on the left in the linked list either
            {
                return found_on_left;
            }
        }
    } // lies to the right of the edge
    // ANALOGY FROM DIAGRAM (See doc in header file):
    // This is the same as inserting 14 while standing at 10, 7 while standing at 5, 18 while
    // standing at 15, 7 while standing at 6, you get the point
    else
    {
        if (child[RIGHT]) // is there a child to the right? If you're at 10 or 5 or 15 there is child on right so you let the child decide where
        {                 // new node goes but not if you're at leaf nodes such as 2, 6, 12, 16 or any other node that doesn't have a right child
            return (static_cast<SweepTree *>(child[RIGHT]))->Find(px, insertL, // let that child do the finding now
                    insertR);
        }
        else
        {
            // okay so no right child, but stil you can have an element to the right in the linked
            // list. For example you are at 6 and want to insert 7, no child on the right so we are
            // sure the new node goes to the right of 6 but there is still 10 to the right in the
            // double-linked list.
            insertL = this;
            insertR = static_cast<SweepTree *>(elem[RIGHT]);
            if (insertR)
            {
                return found_between;
            }
            else
            {
                return found_on_right;
            }
        }
    }
    return not_found;
}

void
SweepTree::RemoveEvents(SweepEventQueue & queue)
{
    RemoveEvent(queue, LEFT);
    RemoveEvent(queue, RIGHT);
}

void SweepTree::RemoveEvent(SweepEventQueue &queue, Side s)
{
    if (evt[s]) {
        queue.remove(evt[s]);
        evt[s] = nullptr;
    }
}

int
SweepTree::Remove(SweepTreeList &list, SweepEventQueue &queue,
                  bool rebalance)
{
  RemoveEvents(queue);
  AVLTree *tempR = static_cast<AVLTree *>(list.racine);
  int err = AVLTree::Remove(tempR, rebalance);
  list.racine = static_cast<SweepTree *>(tempR);
  MakeDelete();
  if (list.nbTree <= 1)
    {
      list.nbTree = 0;
      list.racine = nullptr;
    }
  else
    {
      if (list.racine == list.trees + (list.nbTree - 1))
	list.racine = this;
      list.trees[--list.nbTree].Relocate(this);
    }
  return err;
}

int
SweepTree::Insert(SweepTreeList &list, SweepEventQueue &queue,
                  Shape *iDst, int iAtPoint, bool rebalance, bool sweepSens)
{
  // if the root node doesn't exist, make this one the root node
  if (list.racine == nullptr)
    {
      list.racine = this;
      return avl_no_err;
    }
  SweepTree *insertL = nullptr;
  SweepTree *insertR = nullptr;
  // use the Find call to figure out the exact position where this needs to go
  int insertion =
    list.racine->Find(iDst->getPoint(iAtPoint).x, this,
		       insertL, insertR, sweepSens);
  
  // if the insertion type is found_exact or found_between this new node is getting in between
  // two existing nodes, which demands that any intersection event that was recorded between
  // the two must be destroyed now *cuz they are no longer together* :-(
    if (insertion == found_exact) { // not sure if these if statements are really needed.
	if (insertR) {
	    insertR->RemoveEvent(queue, LEFT);
	}
	if (insertL) {
	    insertL->RemoveEvent(queue, RIGHT);
	}

    } else if (insertion == found_between) {
      insertR->RemoveEvent(queue, LEFT);
      insertL->RemoveEvent(queue, RIGHT);
    }
  // let the parent class do the adding now
  AVLTree *tempR = static_cast<AVLTree *>(list.racine);
  int err =
    AVLTree::Insert(tempR, insertion, static_cast<AVLTree *>(insertL),
		     static_cast<AVLTree *>(insertR), rebalance);
  list.racine = static_cast<SweepTree *>(tempR);
  return err;
}

// insertAt() is a speedup on the regular sweepline: if the polygon contains a point of high degree, you
// get a set of edge that are to be added in the same position. thus you insert one edge with a regular insert(),
// and then insert all the other in a doubly-linked list fashion. this avoids the Find() call, but is O(d^2) worst-case
// where d is the number of edge to add in this fashion. hopefully d remains small

int
SweepTree::InsertAt(SweepTreeList &list, SweepEventQueue &queue,
                    Shape */*iDst*/, SweepTree *insNode, int fromPt,
                    bool rebalance, bool sweepSens)
{
  // if root node not set, set it
  if (list.racine == nullptr)
    {
      list.racine = this;
      return avl_no_err;
    }

  // the common point between edges
  Geom::Point fromP;
  fromP = src->pData[fromPt].rx;
  // get the edge vector
  Geom::Point nNorm;
  nNorm = src->getEdge(bord).dx;
  // make sure the edge vector is top to bottom or if horizontal
  if (src->getEdge(bord).st > src->getEdge(bord).en)
    {
      nNorm = -nNorm;
    }
  if (sweepSens == false) // why tho?
    {
      nNorm = -nNorm;
    }

  Geom::Point bNorm; // the existing edge (kinda the reference node u can say) that we wanna add this one near to
  bNorm = insNode->src->getEdge(insNode->bord).dx;
  if (insNode->src->getEdge(insNode->bord).st >
      insNode->src->getEdge(insNode->bord).en)
    {
      bNorm = -bNorm;
    }

  SweepTree *insertL = nullptr;
  SweepTree *insertR = nullptr;
  double ang = cross(bNorm, nNorm); // you can use the diagram in the header documentation to make sense of cross product's direction.
  if (ang == 0) // node on top of this one, so we just add right here
    {
      insertL = insNode;
      insertR = static_cast<SweepTree *>(insNode->elem[RIGHT]);
    }
  else if (ang > 0) // edge is to the left
    {
      // initialize such that we are adding this edge between insNode (reference) and whatever is
      // to it's right, this position will change as we go left now
      insertL = insNode;
      insertR = static_cast<SweepTree *>(insNode->elem[RIGHT]);

      // start moving to the left
      while (insertL)
	{
	  if (insertL->src == src)
	    {
	      if (insertL->src->getEdge(insertL->bord).st != fromPt
		  && insertL->src->getEdge(insertL->bord).en != fromPt)
		{
		  break; // if the edge on the left has no endpoint that's fromPt, means we have gone too far, so break
                         // we only case about inserting this at the right position relative to the
                         // existing edges that are connected to fromPt
		}
	    }
	  else
	    { // TODO: has to do with case when an edge can come from another shape
	      int ils = insertL->src->getEdge(insertL->bord).st;
	      int ile = insertL->src->getEdge(insertL->bord).en;
	      if ((insertL->src->pData[ils].rx[0] != fromP[0]
		   || insertL->src->pData[ils].rx[1] != fromP[1])
		  && (insertL->src->pData[ile].rx[0] != fromP[0]
		      || insertL->src->pData[ile].rx[1] != fromP[1]))
		{
		  break;
		}
	    }
          // bNorm is the new edge (the new reference to which we will compare the new edge (to
          // add))
	  bNorm = insertL->src->getEdge(insertL->bord).dx;
	  if (insertL->src->getEdge(insertL->bord).st >
	      insertL->src->getEdge(insertL->bord).en)
	    {
	      bNorm = -bNorm;
	    }
	  ang = cross(bNorm, nNorm);
	  if (ang <= 0) // the new edge should go to the right of this one, so break as insertL and insertR are perfect as they are
	    {
	      break;
	    }
	  insertR = insertL; // otherwise go left
	  insertL = static_cast<SweepTree *>(insertR->elem[LEFT]);
	}
    }
  else if (ang < 0) // the new edge goes to the right
    {
      // initialize such that we are adding this edge between insNode (reference) and whatever is
      // to it's right, this position will change as we go left now
      insertL = insNode;
      insertR = static_cast<SweepTree *>(insNode->elem[RIGHT]);

      // start moving to the right now
      while (insertR)
	{
	  if (insertR->src == src)
	    {
	      if (insertR->src->getEdge(insertR->bord).st != fromPt
		  && insertR->src->getEdge(insertR->bord).en != fromPt)
		{
		  break; // is the right edge not really attached to fromPt at all? so break
		}
	    }
	  else
	    {
	      int ils = insertR->src->getEdge(insertR->bord).st;
	      int ile = insertR->src->getEdge(insertR->bord).en;
	      if ((insertR->src->pData[ils].rx[0] != fromP[0]
		   || insertR->src->pData[ils].rx[1] != fromP[1])
		  && (insertR->src->pData[ile].rx[0] != fromP[0]
		      || insertR->src->pData[ile].rx[1] != fromP[1]))
		{
		  break;
		}
	    }
          // the new reference vector that we wanna compare to
	  bNorm = insertR->src->getEdge(insertR->bord).dx;
	  if (insertR->src->getEdge(insertR->bord).st >
	      insertR->src->getEdge(insertR->bord).en)
	    {
	      bNorm = -bNorm;
	    }
	  ang = cross(bNorm, nNorm);
	  if (ang > 0) // oh the edge goes to the left? so we just break since insertL and insertR are perfect then
	    {
	      break;
	    }
	  insertL = insertR; // go further to the right
	  insertR = static_cast<SweepTree *>(insertL->elem[RIGHT]);
	}
    }

  int insertion = found_between; // by default set to found_between

  if (insertL == nullptr) { // if nothing to left, it's found_on_left
    insertion = found_on_left;
  }
  if (insertR == nullptr) { // if nothing on right, it's found_on_right
    insertion = found_on_right;
  }
  
  if (insertion == found_exact) {
      /* FIXME: surely this can never be called? */ // yea never called it looks like :P
      if (insertR) {
	  insertR->RemoveEvent(queue, LEFT);
      }
      if (insertL) {
	  insertL->RemoveEvent(queue, RIGHT);
      }
  } else if (insertion == found_between) { // if found_between we do clear any events associated to the two nodes who are now no longer gonna be adjacent
      insertR->RemoveEvent(queue, LEFT);
      insertL->RemoveEvent(queue, RIGHT);
  }

  // let the parent do the actual insertion stuff in the tree now
  AVLTree *tempR = static_cast<AVLTree *>(list.racine);
  int err =
    AVLTree::Insert(tempR, insertion, static_cast<AVLTree *>(insertL),
		     static_cast<AVLTree *>(insertR), rebalance);
  list.racine = static_cast<SweepTree *>(tempR);
  return err;
}

void
SweepTree::Relocate(SweepTree * to)
{
  if (this == to)
    return;
  AVLTree::Relocate(to);
  to->src = src;
  to->bord = bord;
  to->evt[LEFT] = evt[LEFT];
  to->evt[RIGHT] = evt[RIGHT];
  if (unsigned(bord) < src->swsData.size())
    src->swsData[bord].misc = to;
  if (unsigned(bord) < src->swrData.size())
    src->swrData[bord].misc = to;
  if (evt[LEFT])
    evt[LEFT]->sweep[RIGHT] = to;
  if (evt[RIGHT])
    evt[RIGHT]->sweep[LEFT] = to;
}

// TODO check if ignoring these parameters is bad
void
SweepTree::SwapWithRight(SweepTreeList &/*list*/, SweepEventQueue &/*queue*/)
{
    SweepTree *tL = this;
    SweepTree *tR = static_cast<SweepTree *>(elem[RIGHT]);

    tL->src->swsData[tL->bord].misc = tR;
    tR->src->swsData[tR->bord].misc = tL;

    std::swap(tL->src, tR->src);
    std::swap(tL->bord, tR->bord);
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

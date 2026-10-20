// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors:
 * see git history
 * Fred
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <glib.h>
#include <2geom/affine.h>
#include "Shape.h"
#include "livarot/sweep-event-queue.h"
#include "livarot/sweep-tree-list.h"
#include "livarot/sweep-tree.h"

//int   doDebug=0;

/*
 * El Intersector.
 * algorithm: 1) benley ottman to get intersections of all the polygon's edges
 *            2) rounding of the points of the polygon, Hooby's algorithm
 *            3) DFS with clockwise choice of the edge to compute the windings
 *            4) choose edges according to winding numbers and fill rule
 * some additional nastyness: step 2 needs a seed winding number for the upper-left point of each 
 * connex subgraph of the graph. computing these brutally is O(n^3): baaaad. so during the sweeping in 1)
 * we keep for each point the edge of the resulting graph (not the original) that lies just on its left; 
 * when the time comes for the point to get its winding number computed, that edge must have been treated,
 * because its upper end lies above the aforementioned point, meaning we know the winding number of the point.
 * only, there is a catch: since we're sweeping the polygon, the edge we want to link the point to has not yet been
 * added (that would be too easy...). so the points are put on a linked list on the original shape's edge, and the list
 * is flushed when the edge is added.
 * rounding: to do the rounding, we need to find which edges cross the surrounding of the rounded points (at 
 * each sweepline position). grunt method tries all combination of "rounded points in the sweepline"x"edges crossing 
 * the sweepline". That's bad (and that's what polyboolean does, if i am not mistaken). so for each point 
 * rounded in a given sweepline, keep immediate left and right edges at the time the point is treated.
 * when edges/points crossing are searched, walk the edge list (in the  sweepline at the end of the batch) starting 
 * from the rounded points' left and right from that time. may sound strange, but it works because edges that
 * end or start in the batch have at least one end in the batch.
 * all these are the cause of the numerous linked lists of points and edges maintained in the sweeping
 */

void
Shape::ResetSweep ()
{
  MakePointData (true);
  MakeEdgeData (true);
  MakeSweepSrcData (true);
}

void
Shape::CleanupSweep ()
{
  MakePointData (false);
  MakeEdgeData (false);
  MakeSweepSrcData (false);
}

void
Shape::ForceToPolygon ()
{
  type = shape_polygon;
}

int
Shape::Reoriente (Shape * a)
{
  Reset (0, 0);
  if (a->numberOfPoints() <= 1 || a->numberOfEdges() <= 1)
    return 0;
  if (directedEulerian(a) == false)
    return shape_input_err;

  _pts = a->_pts;
  if (numberOfPoints() > maxPt)
    {
      maxPt = numberOfPoints();
      if (_has_points_data) {
        pData.resize(maxPt);
        _point_data_initialised = false;
        _bbox_up_to_date = false;
        }
    }

  _aretes = a->_aretes;
  if (numberOfEdges() > maxAr)
    {
      maxAr = numberOfEdges();
      if (_has_edges_data)
	eData.resize(maxAr);
      if (_has_sweep_src_data)
	swsData.resize(maxAr);
      if (_has_sweep_dest_data)
	swdData.resize(maxAr);
      if (_has_raster_data)
	swrData.resize(maxAr);
    }

  MakePointData (true);
  MakeEdgeData (true);
  MakeSweepDestData (true);

  initialisePointData();

  for (int i = 0; i < numberOfPoints(); i++) {
      _pts[i].x = pData[i].rx;
      _pts[i].oldDegree = getPoint(i).totalDegree();
  }
  
  for (int i = 0; i < a->numberOfEdges(); i++)
    {
      eData[i].rdx = pData[getEdge(i).en].rx - pData[getEdge(i).st].rx;
      eData[i].weight = 1;
      _aretes[i].dx = eData[i].rdx;
    }

  SortPointsRounded ();

  _need_edges_sorting = true;
  GetWindings (true);

//      Plot(341,56,8,400,400,true,true,false,true);
  for (int i = 0; i < numberOfEdges(); i++)
    {
      swdData[i].leW %= 2;
      swdData[i].riW %= 2;
      if (swdData[i].leW < 0)
	swdData[i].leW = -swdData[i].leW;
      if (swdData[i].riW < 0)
	swdData[i].riW = -swdData[i].riW;
      if (swdData[i].leW > 0 && swdData[i].riW <= 0)
	{
	  eData[i].weight = 1;
	}
      else if (swdData[i].leW <= 0 && swdData[i].riW > 0)
	{
	  Inverse (i);
	  eData[i].weight = 1;
	}
      else
	{
	  eData[i].weight = 0;
	  SubEdge (i);
	  i--;
	}
    }

  MakePointData (false);
  MakeEdgeData (false);
  MakeSweepDestData (false);

  if (directedEulerian(this) == false)
    {
//              printf( "pas euclidian2");
      _pts.clear();
      _aretes.clear();
      return shape_euler_err;
    }

  type = shape_polygon;
  return 0;
}

int
Shape::ConvertToShape (Shape * a, FillRule directed, bool invert)
{
  // reset any existing stuff in this shape
  Reset (0, 0);

  // nothing to do with 0/1 points/edges
  if (a->numberOfPoints() <= 1 || a->numberOfEdges() <= 1) {
    return 0;
  }

  // if shape is not eulerian, we can't proceedA, why though you might ask?
  // I think because if it's not eulerian, winding numbers won't be consistent and thus
  // nothing else will work
  if ( directed != fill_justDont && directedEulerian(a) == false ) {
    g_warning ("Shape error in ConvertToShape: directedEulerian(a) == false\n");
    return shape_input_err;
  }

  a->ResetSweep();

  // allocating the sweepline data structures
  if (sTree == nullptr) {
    sTree = new SweepTreeList(a->numberOfEdges());
  }
  if (sEvts == nullptr) {
    sEvts = new SweepEventQueue(a->numberOfEdges());
  }

  // make room for stuff and set flags
  MakePointData(true);
  MakeEdgeData(true);
  MakeSweepSrcData(true);
  MakeSweepDestData(true);
  MakeBackData(a->_has_back_data);

  // initialize pData and eData arrays, stores rounded points, rounded edge vectors and their lengths
  a->initialisePointData();
  a->initialiseEdgeData();

  // sort points of a, top to bottom so we can sweepline
  a->SortPointsRounded();

  // clear the chgts array
  chgts.clear();

  double lastChange = a->pData[0].rx[1] - 1.0;
  int lastChgtPt = 0;
  int edgeHead = -1;
  Shape *shapeHead = nullptr;

  clearIncidenceData();

  // index of the current point in shape a
  int curAPt = 0;

  // as long as there is a point we haven't seen yet or events that we haven't popped yet
  while (curAPt < a->numberOfPoints() || sEvts->size() > 0) {
    Geom::Point ptX;
    double ptL, ptR;
    SweepTree *intersL = nullptr;
    SweepTree *intersR = nullptr;
    int nPt = -1;
    Shape *ptSh = nullptr;
    bool isIntersection = false;

    // this block gives us the earliest most point (sweeping top to bottom and left to right)
    // whether it comes from the intersection event priority queue sEvts or sweepline list sTree.
    // isIntersection tell us if it's coming from an intersection or from the endpoints list of shape a
    // ptX contains the actual point itself
    // ptSh contains the shape from which the point comes (if it does) (shape a always)
    // nPt is the index of the point if it's coming from a shape

    // is there an intersection event to pop?
    if (sEvts->peek(intersL, intersR, ptX, ptL, ptR))
    {
      // is that intersection event before the current point in shape a? If yes, we pop and process the intersection event otherwise
      // we process the point in shape a, whichever comes first (sweeping top to bottom) the one with smaller y or smaller x (if same y)
      if (a->pData[curAPt].pending > 0
          || (a->pData[curAPt].rx[1] > ptX[1]
            || (a->pData[curAPt].rx[1] == ptX[1]
              && a->pData[curAPt].rx[0] > ptX[0])))
      {
        // if yes, let's process the intersection point
        /* FIXME: could just be pop? */
        sEvts->extract(intersL, intersR, ptX, ptL, ptR);
        isIntersection = true;
      }
      else // otherwise, we process the current point in shape a
      {
        nPt = curAPt++;            // nPt stores the index of this point in shape a that we are going to process
        ptSh = a;
        ptX = ptSh->pData[nPt].rx; // get the rounded version of the current point in ptX
        isIntersection = false;    // not an intersection
      }
    }
    else
    {
      nPt = curAPt++;
      ptSh = a;
      ptX = ptSh->pData[nPt].rx;
      isIntersection = false;
    }

    if (isIntersection == false) // if this is not intersection event and the point has total degree of 0, we have nothing to do
    {
      if (ptSh->getPoint(nPt).dI == 0 && ptSh->getPoint(nPt).dO == 0)
        continue;
    }

    // wherever the point comes from, we add the rounded version to "this" shape's list of points
    Geom::Point rPtX;
    rPtX[0]= Round (ptX[0]);
    rPtX[1]= Round (ptX[1]);
    int lastPointNo = AddPoint (rPtX); // lastPointNo is the index of this last rounded point we added
    pData[lastPointNo].rx = rPtX;

    // this whole block deals with the reconstruction procedure only do this if the y level
    // changed, we don't need to do this as long as the y level has not changed, note that
    // sweepline in this algorithm moves top to bottom but also left to right when there are
    // multiple points at same y better to think of it as the sweepline being slightly slanted such
    // that it'll hit the left points earlier than the right ones In fact, it's better to visualize
    // as if we are iterating through set of points top to bottom and left ot right instead of a
    // sweepline.
    if (rPtX[1] > lastChange)
    {
      // the important thing this function does is that it sorts points and merges any duplicate points
      // so edges with different points (which were at the same coordinates) would be forced to point
      // to the same exact point. (useful for cases when multiple edges intersect at the same point)
      // Sort all points before lastPointNo
      int lastI = AssemblePoints (lastChgtPt, lastPointNo); // lastI is one more than the index of the last point that
      // was added by AssemblePoints after sorting and merging

      // after AssemblePoints is done sorting, the newInd variable holds the new index of that point

      // update the leftRnd and rightRnd indexes to newInd while traversing the linked list of
      // edges and shapes. leftRnd and rightRnd are the left most and right most points of an edge
      // in the resultant polygon that intersect the sweepline. In all non-horizontal edges, both
      // would be identical, only when the edge is horizontal, the two will be different since the
      // sweepline will intersect it at multiple endpoints.
      // To define it more strictly, you can think of leftRnd and rightRnd as being the left most
      // and right most point in the final resulted shape ("this" shape) at the y level lastChange.
      Shape *curSh = shapeHead;
      int curBo = edgeHead;
      while (curSh)
      {
        curSh->swsData[curBo].leftRnd =
          pData[curSh->swsData[curBo].leftRnd].newInd;
        curSh->swsData[curBo].rightRnd =
          pData[curSh->swsData[curBo].rightRnd].newInd;

        Shape *neSh = curSh->swsData[curBo].nextSh;
        curBo = curSh->swsData[curBo].nextBo;
        curSh = neSh;
      }

      for (auto & chgt : chgts)
      {
        chgt.ptNo = pData[chgt.ptNo].newInd; // this updates the ptNo index to the new index, so identical points really merge
        if (chgt.type == 0)
        {
          if (chgt.src->getEdge(chgt.bord).st <
              chgt.src->getEdge(chgt.bord).en)
          {
            chgt.src->swsData[chgt.bord].stPt = // <-- No idea where stPt and enPt are really used
              chgt.ptNo;
          }
          else
          {
            chgt.src->swsData[chgt.bord].enPt =
              chgt.ptNo;
          }
        }
        else if (chgt.type == 1)
        {
          if (chgt.src->getEdge(chgt.bord).st >
              chgt.src->getEdge(chgt.bord).en)
          {
            chgt.src->swsData[chgt.bord].stPt =
              chgt.ptNo;
          }
          else
          {
            chgt.src->swsData[chgt.bord].enPt =
              chgt.ptNo;
          }
        }
      }

      // finds if points at the y level "lastChange" lie on top of the edge and if yes, modifies
      // leftRnd and rightRnd of those edges accordingly
      CheckAdjacencies (lastI, lastChgtPt, shapeHead, edgeHead);

      // reconstruct the edges
      CheckEdges (lastI, lastChgtPt, a, nullptr, bool_op_union);

      // for each one of the points in the range lastChgtPt..(lastI-1) and if there are already
      // points associated to the edge pData[i].askForWindingB, push the current one (i) in the linked
      // list
      for (int i = lastChgtPt; i < lastI; i++) {
        if (pData[i].askForWindingS) {
          Shape *windS = pData[i].askForWindingS;
          int windB = pData[i].askForWindingB;
          pData[i].nextLinkedPoint = windS->swsData[windB].firstLinkedPoint;
          windS->swsData[windB].firstLinkedPoint = i;
        }
      }

      // note that AssemblePoints doesn't even touch lastPointNo, it iterates on all the points before
      // lastPointNo, so while that range might have shrinked because of duplicate points, lastPointNo
      // remains where it was, so we check if some points got merged, if yes, lastPointNo is kinda far away
      // from those points (there are garbage points in between due to merging), so we drag it back
      if (lastI < lastPointNo) {
        _pts[lastI] = getPoint(lastPointNo);
        pData[lastI] = pData[lastPointNo];
      }
      // set lastPointNo to lastI (the new index for lastPointNo)
      lastPointNo = lastI;
      _pts.resize(lastI + 1); // resize and delete everything beyond lastPointNo, we add 1 cuz lastI is an index so u
      // add one to convert it to size

      lastChgtPt = lastPointNo; // set lastChgtPt
      lastChange = rPtX[1]; // set lastChange
      chgts.clear(); // this chgts array gets cleared up whenever the y of the sweepline changes
      edgeHead = -1;
      shapeHead = nullptr;
    }


    // are we processing an intersection point?
    if (isIntersection)
    {
      //                      printf("(%i %i [%i %i]) ",intersL->bord,intersR->bord,intersL->startPoint,intersR->startPoint);
      // remove any intersections that have interesL as a RIGHT edge
      intersL->RemoveEvent (*sEvts, LEFT);
      // remove any intersections that have intersR as a LEFT edge
      intersR->RemoveEvent (*sEvts, RIGHT);

      // add the intersection point to the chgts array
      AddChgt (lastPointNo, lastChgtPt, shapeHead, edgeHead, INTERSECTION,
          intersL->src, intersL->bord, intersR->src, intersR->bord);

      // swap the edges intersL and intersR with each other (because they swap their intersection point with the sweepline as
      // you pass the intersection point)
      intersL->SwapWithRight (*sTree, *sEvts);

      // test intersection of the now left edge with the one on its left
      TesteIntersection (intersL, LEFT, false);
      // test intersection of the now right edge with the one on its right
      TesteIntersection (intersR, RIGHT, false);
    }
    else
    { // we are doing with a point in shape a (not an intersection point)
      int cb;

      // this whole block:
      // - Counts how many edges start at the current point (nbDn)
      // - Counts how many edges end at the current point (nbUp)
      // - Notes the last edge that starts here (dnNo)
      // - Notes the last edge that ends here (upNo)
      int nbUp = 0, nbDn = 0;
      int upNo = -1, dnNo = -1;
      cb = ptSh->getPoint(nPt).incidentEdge[FIRST];
      while (cb >= 0 && cb < ptSh->numberOfEdges())
      {
        if ((ptSh->getEdge(cb).st < ptSh->getEdge(cb).en
              && nPt == ptSh->getEdge(cb).en)
            || (ptSh->getEdge(cb).st > ptSh->getEdge(cb).en
              && nPt == ptSh->getEdge(cb).st))
        {
          upNo = cb;
          nbUp++;
        }
        if ((ptSh->getEdge(cb).st > ptSh->getEdge(cb).en
              && nPt == ptSh->getEdge(cb).en)
            || (ptSh->getEdge(cb).st < ptSh->getEdge(cb).en
              && nPt == ptSh->getEdge(cb).st))
        {
          dnNo = cb;
          nbDn++;
        }
        cb = ptSh->NextAt (nPt, cb);
      }

      if (nbDn <= 0)
      {
        upNo = -1;
      }
      if (upNo >= 0 && ptSh->swsData[upNo].misc == nullptr)
      {
        upNo = -1;
      }

      bool doWinding = true;

      // these blocks of code do the job of adding/removing edges
      // however, there are some optimizations which leads to their weird if blocks

      // Is there any edge that ends here?
      if (nbUp > 0)
      {
        cb = ptSh->getPoint(nPt).incidentEdge[FIRST];
        // for all edges that connect to this point
        while (cb >= 0 && cb < ptSh->numberOfEdges())
        { // if the edge ends here
          if ((ptSh->getEdge(cb).st < ptSh->getEdge(cb).en
                && nPt == ptSh->getEdge(cb).en)
              || (ptSh->getEdge(cb).st > ptSh->getEdge(cb).en
                && nPt == ptSh->getEdge(cb).st))
          {
            if (cb != upNo) // if this is not the last edge that ended at this point
            {
              SweepTree *node = ptSh->swsData[cb].misc; // get the sweepline node for this edge
              if (node == nullptr)
              {
              }
              else
              {
                AddChgt (lastPointNo, lastChgtPt, shapeHead, // add the corresponding remove event in chgts
                    edgeHead, EDGE_REMOVED, node->src, node->bord,
                    nullptr, -1);
                ptSh->swsData[cb].misc = nullptr;

                int onLeftB = -1, onRightB = -1;
                Shape *onLeftS = nullptr;
                Shape *onRightS = nullptr;
                if (node->elem[LEFT])
                {
                  onLeftB =
                    (static_cast <
                     SweepTree * >(node->elem[LEFT]))->bord;
                  onLeftS =
                    (static_cast <
                     SweepTree * >(node->elem[LEFT]))->src;
                }
                if (node->elem[RIGHT])
                {
                  onRightB =
                    (static_cast <
                     SweepTree * >(node->elem[RIGHT]))->bord;
                  onRightS =
                    (static_cast <
                     SweepTree * >(node->elem[RIGHT]))->src;
                }

                node->Remove (*sTree, *sEvts, true); // remove this edge
                // if there are edges on the left and right and they do not start or end at the current point
                // test if they interesect with each other (since now this edge just go removed and they are side to side)
                if (onLeftS && onRightS)
                {
                  SweepTree *onLeft = onLeftS->swsData[onLeftB].misc;
                  if (onLeftS == ptSh
                      && (onLeftS->getEdge(onLeftB).en == nPt
                        || onLeftS->getEdge(onLeftB).st ==
                        nPt))
                  {
                  }
                  else
                  {
                    if (onRightS == ptSh
                        && (onRightS->getEdge(onRightB).en ==
                          nPt
                          || onRightS->getEdge(onRightB).
                          st == nPt))
                    {
                    }
                    else
                    {
                      TesteIntersection (onLeft, RIGHT, false);
                    }
                  }
                }
              }
            }
          }
          cb = ptSh->NextAt (nPt, cb);
        }
      }

      // traitement du "upNo devient dnNo"
      SweepTree *insertionNode = nullptr;
      if (dnNo >= 0) // if there is a last edge that started here
      {
        if (upNo >= 0) // and there is a last edge that ended here
        {
          SweepTree *node = (SweepTree *) ptSh->swsData[upNo].misc;

          AddChgt (lastPointNo, lastChgtPt, shapeHead, edgeHead, EDGE_REMOVED, // add edge removal event to the list
              node->src, node->bord, nullptr, -1);

          ptSh->swsData[upNo].misc = nullptr;

          node->RemoveEvents (*sEvts); // remove any events associated with this node
          node->ConvertTo (ptSh, dnNo, 1, lastPointNo); // convert this node the last edge that got added at this point
          ptSh->swsData[dnNo].misc = node;     // store the sweepline edge tree node at misc for later use
          TesteIntersection (node, RIGHT, false); // test the interesction of this node with the one on its right
          TesteIntersection (node, LEFT, false); // test the interesection of this node with the one on its left
          insertionNode = node; // a variable to keep the pointer to this node for later use

          ptSh->swsData[dnNo].curPoint = lastPointNo; // mark the curPoint in swsData for later use in reconstruction
          AddChgt (lastPointNo, lastChgtPt, shapeHead, edgeHead, EDGE_INSERTED,
              node->src, node->bord, nullptr, -1); // add the edge insertion event to chgts
        }
        else // if there is a last edge that started at this point but not any that ended
        {
          SweepTree *node = sTree->add(ptSh, dnNo, 1, lastPointNo, this); // add this edge
          ptSh->swsData[dnNo].misc = node; // store in misc too
          node->Insert (*sTree, *sEvts, this, lastPointNo, true); // insert the node at its right location in the sweepline tree
          if (doWinding)
          {
            SweepTree *myLeft =
              static_cast < SweepTree * >(node->elem[LEFT]);
            if (myLeft)
            {
              pData[lastPointNo].askForWindingS = myLeft->src;
              pData[lastPointNo].askForWindingB = myLeft->bord;
            }
            else
            {
              pData[lastPointNo].askForWindingB = -1;
            }
            doWinding = false;
          }
          TesteIntersection (node, RIGHT, false); // test intersection of this newly inserted node with the one on its right
          TesteIntersection (node, LEFT, false); // test intersection of this newly inserted node with the one on its left
          insertionNode = node; // store insertionNode for later use

          ptSh->swsData[dnNo].curPoint = lastPointNo; // mark the curPoint appropriately
          AddChgt (lastPointNo, lastChgtPt, shapeHead, edgeHead, EDGE_INSERTED, // add the edge inserted event
              node->src, node->bord, nullptr, -1);
        }
      }

      if (nbDn > 1) // if there are more than 1 edges that start at this point
      {			// si nbDn == 1 , alors dnNo a deja ete traite
        cb = ptSh->getPoint(nPt).incidentEdge[FIRST];
        while (cb >= 0 && cb < ptSh->numberOfEdges()) // for all edges that connect to this point
        {
          if ((ptSh->getEdge(cb).st > ptSh->getEdge(cb).en
                && nPt == ptSh->getEdge(cb).en)
              || (ptSh->getEdge(cb).st < ptSh->getEdge(cb).en
                && nPt == ptSh->getEdge(cb).st))
          { // if the edge starts here
            if (cb != dnNo)
            {
              SweepTree *node = sTree->add(ptSh, cb, 1, lastPointNo, this); // add the node to the tree
              ptSh->swsData[cb].misc = node;
              node->InsertAt (*sTree, *sEvts, this, insertionNode, // insert it at appropriate position
                  nPt, true);
              if (doWinding)
              {
                SweepTree *myLeft =
                  static_cast < SweepTree * >(node->elem[LEFT]);
                if (myLeft)
                {
                  pData[lastPointNo].askForWindingS =
                    myLeft->src;
                  pData[lastPointNo].askForWindingB =
                    myLeft->bord;
                }
                else
                {
                  pData[lastPointNo].askForWindingB = -1;
                }
                doWinding = false;
              }
              TesteIntersection (node, RIGHT, false); // test intersection of this edge with one on its left
              TesteIntersection (node, LEFT, false); // test intersection of this edge with one on its right

              ptSh->swsData[cb].curPoint = lastPointNo; // store curPoint in sws
              AddChgt (lastPointNo, lastChgtPt, shapeHead, // add appropriate edge insertion event in chgts
                  edgeHead, EDGE_INSERTED, node->src, node->bord, nullptr,
                  -1);
            }
          }
          cb = ptSh->NextAt (nPt, cb);
        }
      }
    }
  }
  // a code block totally identical to the one found above in loop. Has to be run one last time
  // so written outside..
  {
    int lastI = AssemblePoints (lastChgtPt, numberOfPoints());


    Shape *curSh = shapeHead;
    int curBo = edgeHead;
    while (curSh)
    {
      curSh->swsData[curBo].leftRnd =
        pData[curSh->swsData[curBo].leftRnd].newInd;
      curSh->swsData[curBo].rightRnd =
        pData[curSh->swsData[curBo].rightRnd].newInd;

      Shape *neSh = curSh->swsData[curBo].nextSh;
      curBo = curSh->swsData[curBo].nextBo;
      curSh = neSh;
    }

    for (auto & chgt : chgts)
    {
      chgt.ptNo = pData[chgt.ptNo].newInd;
      if (chgt.type == 0)
      {
        if (chgt.src->getEdge(chgt.bord).st <
            chgt.src->getEdge(chgt.bord).en)
        {
          chgt.src->swsData[chgt.bord].stPt = chgt.ptNo;
        }
        else
        {
          chgt.src->swsData[chgt.bord].enPt = chgt.ptNo;
        }
      }
      else if (chgt.type == 1)
      {
        if (chgt.src->getEdge(chgt.bord).st >
            chgt.src->getEdge(chgt.bord).en)
        {
          chgt.src->swsData[chgt.bord].stPt = chgt.ptNo;
        }
        else
        {
          chgt.src->swsData[chgt.bord].enPt = chgt.ptNo;
        }
      }
    }

    CheckAdjacencies (lastI, lastChgtPt, shapeHead, edgeHead);

    CheckEdges (lastI, lastChgtPt, a, nullptr, bool_op_union);

    for (int i = lastChgtPt; i < lastI; i++)
    {
      if (pData[i].askForWindingS)
      {
        Shape *windS = pData[i].askForWindingS;
        int windB = pData[i].askForWindingB;
        pData[i].nextLinkedPoint = windS->swsData[windB].firstLinkedPoint;
        windS->swsData[windB].firstLinkedPoint = i;
      }
    }

    _pts.resize(lastI);

    edgeHead = -1;
    shapeHead = nullptr;
  }

  chgts.clear();

  //  Plot (98.0, 112.0, 8.0, 400.0, 400.0, true, true, true, true);
  //      Plot(200.0,200.0,2.0,400.0,400.0,true,true,true,true);

  //      AssemblePoints(a);

  //      GetAdjacencies(a);

  //      MakeAretes(a);
  clearIncidenceData();

  // deal with doublon edges (edges on top of each other)
  // we only keep one edge and set its weight equivalent to the net difference of the edges.
  // If two edges are exactly identical and in same direction their weights add up, if they
  // are in the opposite direction, weights are subtracted. The other edges are removed.
  AssembleAretes (directed);

  //  Plot (98.0, 112.0, 8.0, 400.0, 400.0, true, true, true, true);

  // store the degrees at this point in time
  for (int i = 0; i < numberOfPoints(); i++)
  {
    _pts[i].oldDegree = getPoint(i).totalDegree();
  }
  //      Validate();

  _need_edges_sorting = true;
  if ( directed == fill_justDont ) {
    SortEdges(); // sorting edges
  } else {
    GetWindings (); // getting winding numbers of the edges
  }
  //  Plot (98.0, 112.0, 8.0, 400.0, 400.0, true, true, true, true);
  //   if ( doDebug ) {
  //   a->CalcBBox();
  //     a->Plot(a->leftX,a->topY,32.0,0.0,0.0,true,true,true,true,"orig.svg");
  //     Plot(a->leftX,a->topY,32.0,0.0,0.0,true,true,true,true,"winded.svg");
  //   }

  // change edges depending on the fill rule. Decide whether to keep it, invert it or get rid of it
  if (directed == fill_positive)
  {
    if (invert)
    {
      for (int i = 0; i < numberOfEdges(); i++)
      {
        if (swdData[i].leW < 0 && swdData[i].riW >= 0)
        {
          eData[i].weight = 1;
        }
        else if (swdData[i].leW >= 0 && swdData[i].riW < 0)
        {
          Inverse (i);
          eData[i].weight = 1;
        }
        else
        {
          eData[i].weight = 0;
          SubEdge (i);
          i--;
        }
      }
    }
    else
    {
      for (int i = 0; i < numberOfEdges(); i++)
      {
        if (swdData[i].leW > 0 && swdData[i].riW <= 0)
        {
          eData[i].weight = 1;
        }
        else if (swdData[i].leW <= 0 && swdData[i].riW > 0)
        {
          Inverse (i);
          eData[i].weight = 1;
        }
        else
        {
          eData[i].weight = 0;
          SubEdge (i);
          i--;
        }
      }
    }
  }
  else if (directed == fill_nonZero)
  {
    if (invert)
    {
      for (int i = 0; i < numberOfEdges(); i++)
      {
        if (swdData[i].leW < 0 && swdData[i].riW == 0)
        {
          eData[i].weight = 1;
        }
        else if (swdData[i].leW > 0 && swdData[i].riW == 0)
        {
          eData[i].weight = 1;
        }
        else if (swdData[i].leW == 0 && swdData[i].riW < 0)
        {
          Inverse (i);
          eData[i].weight = 1;
        }
        else if (swdData[i].leW == 0 && swdData[i].riW > 0)
        {
          Inverse (i);
          eData[i].weight = 1;
        }
        else
        {
          eData[i].weight = 0;
          SubEdge (i);
          i--;
        }
      }
    }
    else
    {
      for (int i = 0; i < numberOfEdges(); i++)
      {
        if (swdData[i].leW > 0 && swdData[i].riW == 0)
        {
          eData[i].weight = 1;
        }
        else if (swdData[i].leW < 0 && swdData[i].riW == 0)
        {
          eData[i].weight = 1;
        }
        else if (swdData[i].leW == 0 && swdData[i].riW > 0)
        {
          Inverse (i);
          eData[i].weight = 1;
        }
        else if (swdData[i].leW == 0 && swdData[i].riW < 0)
        {
          Inverse (i);
          eData[i].weight = 1;
        }
        else
        {
          eData[i].weight = 0;
          SubEdge (i);
          i--;
        }
      }
    }
  }
  else if (directed == fill_oddEven)
  {
    for (int i = 0; i < numberOfEdges(); i++)
    {
      swdData[i].leW %= 2;
      swdData[i].riW %= 2;
      if (swdData[i].leW < 0)
        swdData[i].leW = -swdData[i].leW;
      if (swdData[i].riW < 0)
        swdData[i].riW = -swdData[i].riW;
      if (swdData[i].leW > 0 && swdData[i].riW <= 0)
      {
        eData[i].weight = 1;
      }
      else if (swdData[i].leW <= 0 && swdData[i].riW > 0)
      {
        Inverse (i);
        eData[i].weight = 1;
      }
      else
      {
        eData[i].weight = 0;
        SubEdge (i);
        i--;
      }
    }
  } else if ( directed == fill_justDont ) {
    for (int i=0;i<numberOfEdges();i++) {
      if ( getEdge(i).st < 0 || getEdge(i).en < 0 ) {
        SubEdge(i);
        i--;
      } else {
        eData[i].weight = 0;
      }
    }
  }

  //      Plot(200.0,200.0,2.0,400.0,400.0,true,true,true,true);

  delete sTree;
  sTree = nullptr;
  delete sEvts;
  sEvts = nullptr;

  MakePointData (false);
  MakeEdgeData (false);
  MakeSweepSrcData (false);
  MakeSweepDestData (false);
  a->CleanupSweep ();
  type = shape_polygon;
  return 0;
}

// technically it's just a ConvertToShape() on 2 polygons at the same time, and different rules
// for choosing the edges according to their winding numbers.
// probably one of the biggest function i ever wrote.
int
Shape::Booleen (Shape * a, Shape * b, BooleanOp mod,int cutPathID)
{
  if (a == b || a == nullptr || b == nullptr)
    return shape_input_err;
  Reset (0, 0);
  if (a->numberOfPoints() <= 1 || a->numberOfEdges() <= 1)
    return 0;
  if (b->numberOfPoints() <= 1 || b->numberOfEdges() <= 1)
    return 0;
  if ( mod == bool_op_cut ) {
  } else if ( mod == bool_op_slice ) {
  } else {
    if (a->type != shape_polygon)
      return shape_input_err;
    if (b->type != shape_polygon)
      return shape_input_err;
  }

  a->ResetSweep ();
  b->ResetSweep ();

  if (sTree == nullptr) {
      sTree = new SweepTreeList(a->numberOfEdges() + b->numberOfEdges());
  }
  if (sEvts == nullptr) {
      sEvts = new SweepEventQueue(a->numberOfEdges() + b->numberOfEdges());
  }
  
  MakePointData (true);
  MakeEdgeData (true);
  MakeSweepSrcData (true);
  MakeSweepDestData (true);
  if (a->hasBackData () && b->hasBackData ())
    {
      MakeBackData (true);
    }
  else
    {
      MakeBackData (false);
    }

  a->initialisePointData();
  b->initialisePointData();

  a->initialiseEdgeData();
  b->initialiseEdgeData();

  a->SortPointsRounded ();
  b->SortPointsRounded ();

  chgts.clear();

  double lastChange =
    (a->pData[0].rx[1] <
     b->pData[0].rx[1]) ? a->pData[0].rx[1] - 1.0 : b->pData[0].rx[1] - 1.0;
  int lastChgtPt = 0;
  int edgeHead = -1;
  Shape *shapeHead = nullptr;

  clearIncidenceData();

  int curAPt = 0;
  int curBPt = 0;

  while (curAPt < a->numberOfPoints() || curBPt < b->numberOfPoints() || sEvts->size() > 0)
    {
/*		for (int i=0;i<sEvts.nbEvt;i++) {
			printf("%f %f %i %i\n",sEvts.events[i].posx,sEvts.events[i].posy,sEvts.events[i].leftSweep->bord,sEvts.events[i].rightSweep->bord); // localizing ok
		}
		//		cout << endl;
		if ( sTree.racine ) {
			SweepTree*  ct=static_cast <SweepTree*> (sTree.racine->Leftmost());
			while ( ct ) {
				printf("%i %i [%i\n",ct->bord,ct->startPoint,(ct->src==a)?1:0);
				ct=static_cast <SweepTree*> (ct->elem[RIGHT]);
			}
		}
		printf("\n");*/

    Geom::Point ptX;
      double ptL, ptR;
      SweepTree *intersL = nullptr;
      SweepTree *intersR = nullptr;
      int nPt = -1;
      Shape *ptSh = nullptr;
      bool isIntersection = false;

      if (sEvts->peek(intersL, intersR, ptX, ptL, ptR))
	{
	  if (curAPt < a->numberOfPoints())
	    {
	      if (curBPt < b->numberOfPoints())
		{
		  if (a->pData[curAPt].rx[1] < b->pData[curBPt].rx[1]
		      || (a->pData[curAPt].rx[1] == b->pData[curBPt].rx[1]
			  && a->pData[curAPt].rx[0] < b->pData[curBPt].rx[0]))
		    {
		      if (a->pData[curAPt].pending > 0
			  || (a->pData[curAPt].rx[1] > ptX[1]
			      || (a->pData[curAPt].rx[1] == ptX[1]
				  && a->pData[curAPt].rx[0] > ptX[0])))
			{
			  /* FIXME: could be pop? */
			  sEvts->extract(intersL, intersR, ptX, ptL, ptR);
			  isIntersection = true;
			}
		      else
			{
			  nPt = curAPt++;
			  ptSh = a;
			  ptX = ptSh->pData[nPt].rx;
			  isIntersection = false;
			}
		    }
		  else
		    {
		      if (b->pData[curBPt].pending > 0
			  || (b->pData[curBPt].rx[1] > ptX[1]
			      || (b->pData[curBPt].rx[1] == ptX[1]
				  && b->pData[curBPt].rx[0] > ptX[0])))
			{
			  /* FIXME: could be pop? */
			  sEvts->extract(intersL, intersR, ptX, ptL, ptR);
			  isIntersection = true;
			}
		      else
			{
			  nPt = curBPt++;
			  ptSh = b;
			  ptX = ptSh->pData[nPt].rx;
			  isIntersection = false;
			}
		    }
		}
	      else
		{
		  if (a->pData[curAPt].pending > 0
		      || (a->pData[curAPt].rx[1] > ptX[1]
			  || (a->pData[curAPt].rx[1] == ptX[1]
			      && a->pData[curAPt].rx[0] > ptX[0])))
		    {
		      /* FIXME: could be pop? */
		      sEvts->extract(intersL, intersR, ptX, ptL, ptR);
		      isIntersection = true;
		    }
		  else
		    {
		      nPt = curAPt++;
		      ptSh = a;
		      ptX = ptSh->pData[nPt].rx;
		      isIntersection = false;
		    }
		}
	    }
	  else
	    {
	      if (b->pData[curBPt].pending > 0
		  || (b->pData[curBPt].rx[1] > ptX[1]
		      || (b->pData[curBPt].rx[1] == ptX[1]
			  && b->pData[curBPt].rx[0] > ptX[0])))
		{
		  /* FIXME: could be pop? */
		  sEvts->extract(intersL, intersR, ptX,  ptL, ptR);
		  isIntersection = true;
		}
	      else
		{
		  nPt = curBPt++;
		  ptSh = b;
		  ptX = ptSh->pData[nPt].rx;
		  isIntersection = false;
		}
	    }
	}
      else
	{
	  if (curAPt < a->numberOfPoints())
	    {
	      if (curBPt < b->numberOfPoints())
		{
		  if (a->pData[curAPt].rx[1] < b->pData[curBPt].rx[1]
		      || (a->pData[curAPt].rx[1] == b->pData[curBPt].rx[1]
			  && a->pData[curAPt].rx[0] < b->pData[curBPt].rx[0]))
		    {
		      nPt = curAPt++;
		      ptSh = a;
		    }
		  else
		    {
		      nPt = curBPt++;
		      ptSh = b;
		    }
		}
	      else
		{
		  nPt = curAPt++;
		  ptSh = a;
		}
	    }
	  else
	    {
	      nPt = curBPt++;
	      ptSh = b;
	    }
	  ptX = ptSh->pData[nPt].rx;
	  isIntersection = false;
	}

      if (isIntersection == false)
	{
	  if (ptSh->getPoint(nPt).dI == 0 && ptSh->getPoint(nPt).dO == 0)
	    continue;
	}

      Geom::Point rPtX;
      rPtX[0]= Round (ptX[0]);
      rPtX[1]= Round (ptX[1]);
      int lastPointNo = AddPoint (rPtX);
      pData[lastPointNo].rx = rPtX;

      if (rPtX[1] > lastChange)
	{
	  int lastI = AssemblePoints (lastChgtPt, lastPointNo);


	  Shape *curSh = shapeHead;
	  int curBo = edgeHead;
	  while (curSh)
	    {
	      curSh->swsData[curBo].leftRnd =
		pData[curSh->swsData[curBo].leftRnd].newInd;
	      curSh->swsData[curBo].rightRnd =
		pData[curSh->swsData[curBo].rightRnd].newInd;

	      Shape *neSh = curSh->swsData[curBo].nextSh;
	      curBo = curSh->swsData[curBo].nextBo;
	      curSh = neSh;
	    }

	  for (auto & chgt : chgts)
	    {
	      chgt.ptNo = pData[chgt.ptNo].newInd;
	      if (chgt.type == 0)
		{
		  if (chgt.src->getEdge(chgt.bord).st <
		      chgt.src->getEdge(chgt.bord).en)
		    {
		      chgt.src->swsData[chgt.bord].stPt =
			chgt.ptNo;
		    }
		  else
		    {
		      chgt.src->swsData[chgt.bord].enPt =
			chgt.ptNo;
		    }
		}
	      else if (chgt.type == 1)
		{
		  if (chgt.src->getEdge(chgt.bord).st >
		      chgt.src->getEdge(chgt.bord).en)
		    {
		      chgt.src->swsData[chgt.bord].stPt =
			chgt.ptNo;
		    }
		  else
		    {
		      chgt.src->swsData[chgt.bord].enPt =
			chgt.ptNo;
		    }
		}
	    }

	  CheckAdjacencies (lastI, lastChgtPt, shapeHead, edgeHead);

	  CheckEdges (lastI, lastChgtPt, a, b, mod);

	  for (int i = lastChgtPt; i < lastI; i++)
	    {
	      if (pData[i].askForWindingS)
		{
		  Shape *windS = pData[i].askForWindingS;
		  int windB = pData[i].askForWindingB;
		  pData[i].nextLinkedPoint =
		    windS->swsData[windB].firstLinkedPoint;
		  windS->swsData[windB].firstLinkedPoint = i;
		}
	    }

    if (lastI < lastPointNo)
	    {
	      _pts[lastI] = getPoint(lastPointNo);
	      pData[lastI] = pData[lastPointNo];
	    }
	  lastPointNo = lastI;
	  _pts.resize(lastI + 1);

	  lastChgtPt = lastPointNo;
	  lastChange = rPtX[1];
	  chgts.clear();
	  edgeHead = -1;
	  shapeHead = nullptr;
	}


      if (isIntersection)
	{
	  // les 2 events de part et d'autre de l'intersection
	  // (celui de l'intersection a deja ete depile)
	  intersL->RemoveEvent (*sEvts, LEFT);
	  intersR->RemoveEvent (*sEvts, RIGHT);

	  AddChgt (lastPointNo, lastChgtPt, shapeHead, edgeHead, INTERSECTION,
		   intersL->src, intersL->bord, intersR->src, intersR->bord);

	  intersL->SwapWithRight (*sTree, *sEvts);

	  TesteIntersection (intersL, LEFT, true);
	  TesteIntersection (intersR, RIGHT, true);
	}
      else
	{
	  int cb;

	  int nbUp = 0, nbDn = 0;
	  int upNo = -1, dnNo = -1;
	  cb = ptSh->getPoint(nPt).incidentEdge[FIRST];
	  while (cb >= 0 && cb < ptSh->numberOfEdges())
	    {
	      if ((ptSh->getEdge(cb).st < ptSh->getEdge(cb).en
		   && nPt == ptSh->getEdge(cb).en)
		  || (ptSh->getEdge(cb).st > ptSh->getEdge(cb).en
		      && nPt == ptSh->getEdge(cb).st))
		{
		  upNo = cb;
		  nbUp++;
		}
	      if ((ptSh->getEdge(cb).st > ptSh->getEdge(cb).en
		   && nPt == ptSh->getEdge(cb).en)
		  || (ptSh->getEdge(cb).st < ptSh->getEdge(cb).en
		      && nPt == ptSh->getEdge(cb).st))
		{
		  dnNo = cb;
		  nbDn++;
		}
	      cb = ptSh->NextAt (nPt, cb);
	    }

	  if (nbDn <= 0)
	    {
	      upNo = -1;
	    }
      if (upNo >= 0 && !ptSh->swsData[upNo].misc)
	    {
	      upNo = -1;
	    }

//                      upNo=-1;

	  bool doWinding = true;

	  if (nbUp > 0)
	    {
	      cb = ptSh->getPoint(nPt).incidentEdge[FIRST];
	      while (cb >= 0 && cb < ptSh->numberOfEdges())
		{
		  if ((ptSh->getEdge(cb).st < ptSh->getEdge(cb).en
		       && nPt == ptSh->getEdge(cb).en)
		      || (ptSh->getEdge(cb).st > ptSh->getEdge(cb).en
			  && nPt == ptSh->getEdge(cb).st))
		    {
		      if (cb != upNo)
			{
              SweepTree *node = ptSh->swsData[cb].misc;
			  if (node == nullptr)
			    {
			    }
			  else
			    {
			      AddChgt (lastPointNo, lastChgtPt, shapeHead,
				       edgeHead, EDGE_REMOVED, node->src, node->bord,
				       nullptr, -1);
			      ptSh->swsData[cb].misc = nullptr;

			      int onLeftB = -1, onRightB = -1;
			      Shape *onLeftS = nullptr;
			      Shape *onRightS = nullptr;
			      if (node->elem[LEFT])
				{
				  onLeftB =
				    (static_cast <
				     SweepTree * >(node->elem[LEFT]))->bord;
				  onLeftS =
				    (static_cast <
				     SweepTree * >(node->elem[LEFT]))->src;
				}
			      if (node->elem[RIGHT])
				{
				  onRightB =
				    (static_cast <
				     SweepTree * >(node->elem[RIGHT]))->bord;
				  onRightS =
				    (static_cast <
				     SweepTree * >(node->elem[RIGHT]))->src;
				}

			      node->Remove (*sTree, *sEvts, true);
			      if (onLeftS && onRightS)
				{
                  SweepTree *onLeft = onLeftS->swsData[onLeftB].misc;
//                                                                      SweepTree* onRight=(SweepTree*)onRightS->swsData[onRightB].misc;
				  if (onLeftS == ptSh
				      && (onLeftS->getEdge(onLeftB).en == nPt
					  || onLeftS->getEdge(onLeftB).st ==
					  nPt))
				    {
				    }
				  else
				    {
				      if (onRightS == ptSh
					  && (onRightS->getEdge(onRightB).en ==
					      nPt
					      || onRightS->getEdge(onRightB).
					      st == nPt))
					{
					}
				      else
					{
					  TesteIntersection (onLeft, RIGHT, true);
					}
				    }
				}
			    }
			}
		    }
		  cb = ptSh->NextAt (nPt, cb);
		}
	    }

	  // traitement du "upNo devient dnNo"
	  SweepTree *insertionNode = nullptr;
	  if (dnNo >= 0)
	    {
	      if (upNo >= 0)
		{
          SweepTree *node = ptSh->swsData[upNo].misc;

		  AddChgt (lastPointNo, lastChgtPt, shapeHead, edgeHead, EDGE_REMOVED,
			   node->src, node->bord, nullptr, -1);

		  ptSh->swsData[upNo].misc = nullptr;

		  node->RemoveEvents (*sEvts);
		  node->ConvertTo (ptSh, dnNo, 1, lastPointNo);
		  ptSh->swsData[dnNo].misc = node;
		  TesteIntersection (node, RIGHT, true);
		  TesteIntersection (node, LEFT, true);
		  insertionNode = node;

		  ptSh->swsData[dnNo].curPoint = lastPointNo;

		  AddChgt (lastPointNo, lastChgtPt, shapeHead, edgeHead, EDGE_INSERTED,
			   node->src, node->bord, nullptr, -1);
		}
	      else
		{
		  SweepTree *node = sTree->add(ptSh, dnNo, 1, lastPointNo, this);
		  ptSh->swsData[dnNo].misc = node;
		  node->Insert (*sTree, *sEvts, this, lastPointNo, true);

		  if (doWinding)
		    {
		      SweepTree *myLeft =
			static_cast < SweepTree * >(node->elem[LEFT]);
		      if (myLeft)
			{
			  pData[lastPointNo].askForWindingS = myLeft->src;
			  pData[lastPointNo].askForWindingB = myLeft->bord;
			}
		      else
			{
			  pData[lastPointNo].askForWindingB = -1;
			}
		      doWinding = false;
		    }

		  TesteIntersection (node, RIGHT, true);
		  TesteIntersection (node, LEFT, true);
		  insertionNode = node;

		  ptSh->swsData[dnNo].curPoint = lastPointNo;

		  AddChgt (lastPointNo, lastChgtPt, shapeHead, edgeHead, EDGE_INSERTED,
			   node->src, node->bord, nullptr, -1);
		}
	    }

	  if (nbDn > 1)
	    {			// si nbDn == 1 , alors dnNo a deja ete traite
	      cb = ptSh->getPoint(nPt).incidentEdge[FIRST];
	      while (cb >= 0 && cb < ptSh->numberOfEdges())
		{
		  if ((ptSh->getEdge(cb).st > ptSh->getEdge(cb).en
		       && nPt == ptSh->getEdge(cb).en)
		      || (ptSh->getEdge(cb).st < ptSh->getEdge(cb).en
			  && nPt == ptSh->getEdge(cb).st))
		    {
		      if (cb != dnNo)
			{
			  SweepTree *node = sTree->add(ptSh, cb, 1, lastPointNo, this);
			  ptSh->swsData[cb].misc = node;
//                                                      node->Insert(sTree,*sEvts,this,lastPointNo,true);
			  node->InsertAt (*sTree, *sEvts, this, insertionNode,
					  nPt, true);

			  if (doWinding)
			    {
			      SweepTree *myLeft =
				static_cast < SweepTree * >(node->elem[LEFT]);
			      if (myLeft)
				{
				  pData[lastPointNo].askForWindingS =
				    myLeft->src;
				  pData[lastPointNo].askForWindingB =
				    myLeft->bord;
				}
			      else
				{
				  pData[lastPointNo].askForWindingB = -1;
				}
			      doWinding = false;
			    }

			  TesteIntersection (node, RIGHT, true);
			  TesteIntersection (node, LEFT, true);

			  ptSh->swsData[cb].curPoint = lastPointNo;

			  AddChgt (lastPointNo, lastChgtPt, shapeHead,
				   edgeHead, EDGE_INSERTED, node->src, node->bord, nullptr,
				   -1);
			}
		    }
		  cb = ptSh->NextAt (nPt, cb);
		}
	    }
	}
    }
  {
    int lastI = AssemblePoints (lastChgtPt, numberOfPoints());


    Shape *curSh = shapeHead;
    int curBo = edgeHead;
    while (curSh)
      {
	curSh->swsData[curBo].leftRnd =
	  pData[curSh->swsData[curBo].leftRnd].newInd;
	curSh->swsData[curBo].rightRnd =
	  pData[curSh->swsData[curBo].rightRnd].newInd;

	Shape *neSh = curSh->swsData[curBo].nextSh;
	curBo = curSh->swsData[curBo].nextBo;
	curSh = neSh;
      }

    /* FIXME: this kind of code seems to appear frequently */
    for (auto & chgt : chgts)
      {
	chgt.ptNo = pData[chgt.ptNo].newInd;
	if (chgt.type == 0)
	  {
	    if (chgt.src->getEdge(chgt.bord).st <
		chgt.src->getEdge(chgt.bord).en)
	      {
		chgt.src->swsData[chgt.bord].stPt = chgt.ptNo;
	      }
	    else
	      {
		chgt.src->swsData[chgt.bord].enPt = chgt.ptNo;
	      }
	  }
	else if (chgt.type == 1)
	  {
	    if (chgt.src->getEdge(chgt.bord).st >
		chgt.src->getEdge(chgt.bord).en)
	      {
		chgt.src->swsData[chgt.bord].stPt = chgt.ptNo;
	      }
	    else
	      {
		chgt.src->swsData[chgt.bord].enPt = chgt.ptNo;
	      }
	  }
      }

    CheckAdjacencies (lastI, lastChgtPt, shapeHead, edgeHead);

    CheckEdges (lastI, lastChgtPt, a, b, mod);

    for (int i = lastChgtPt; i < lastI; i++)
      {
	if (pData[i].askForWindingS)
	  {
	    Shape *windS = pData[i].askForWindingS;
	    int windB = pData[i].askForWindingB;
	    pData[i].nextLinkedPoint = windS->swsData[windB].firstLinkedPoint;
	    windS->swsData[windB].firstLinkedPoint = i;
	  }
      }

    _pts.resize(lastI);

    edgeHead = -1;
    shapeHead = nullptr;
  }

  chgts.clear();
  clearIncidenceData();

//      Plot(190,70,6,400,400,true,false,true,true);

  if ( mod == bool_op_cut ) {
    AssembleAretes (fill_justDont);
    // duplicate the edges of the cut
    int i=numberOfEdges()-1;
    for (;i>=0;i--) {
      if ( ebData[i].pathID == cutPathID ) {
        // we duplicate
        int nEd=AddEdge(getEdge(i).en,getEdge(i).st);
        ebData[nEd].pathID=cutPathID;
        ebData[nEd].pieceID=ebData[i].pieceID;
        ebData[nEd].tSt=ebData[i].tEn;
        ebData[nEd].tEn=ebData[i].tSt;
        eData[nEd].weight=eData[i].weight;
        // give it the firstlinkedpoints if needed
        if ( getEdge(i).en >= getEdge(i).st ) {
          int cp = swsData[i].firstLinkedPoint;
          while (cp >= 0) {
            pData[cp].askForWindingB = nEd;
            cp = pData[cp].nextLinkedPoint;
          }
          swsData[nEd].firstLinkedPoint = swsData[i].firstLinkedPoint;
          swsData[i].firstLinkedPoint=-1;
        }
      }
    }
  } else if ( mod == bool_op_slice ) {
  } else {
    AssembleAretes ();
  }
  
  for (int i = 0; i < numberOfPoints(); i++)
    {
      _pts[i].oldDegree = getPoint(i).totalDegree();
    }

  _need_edges_sorting = true;
  if ( mod == bool_op_slice ) {
  } else {
    GetWindings (false);
  }
//      Plot(190,70,6,400,400,true,true,true,true);

  if (mod == bool_op_symdiff)
  {
    for (int i = 0; i < numberOfEdges(); i++)
    {
      if (swdData[i].leW < 0)
        swdData[i].leW = -swdData[i].leW;
      if (swdData[i].riW < 0)
        swdData[i].riW = -swdData[i].riW;
      
      if (swdData[i].leW > 0 && swdData[i].riW <= 0)
	    {
	      eData[i].weight = 1;
	    }
      else if (swdData[i].leW <= 0 && swdData[i].riW > 0)
	    {
	      Inverse (i);
	      eData[i].weight = 1;
	    }
      else
	    {
	      eData[i].weight = 0;
	      SubEdge (i);
	      i--;
	    }
    }
  }
  else if (mod == bool_op_union || mod == bool_op_diff)
  {
    for (int i = 0; i < numberOfEdges(); i++)
    {
      if (swdData[i].leW > 0 && swdData[i].riW <= 0)
	    {
	      eData[i].weight = 1;
	    }
      else if (swdData[i].leW <= 0 && swdData[i].riW > 0)
	    {
	      Inverse (i);
	      eData[i].weight = 1;
	    }
      else
	    {
	      eData[i].weight = 0;
	      SubEdge (i);
	      i--;
	    }
    }
  }
  else if (mod == bool_op_inters)
  {
    for (int i = 0; i < numberOfEdges(); i++)
    {
      if (swdData[i].leW > 1 && swdData[i].riW <= 1)
	    {
	      eData[i].weight = 1;
	    }
      else if (swdData[i].leW <= 1 && swdData[i].riW > 1)
	    {
	      Inverse (i);
	      eData[i].weight = 1;
	    }
      else
	    {
	      eData[i].weight = 0;
	      SubEdge (i);
	      i--;
	    }
    }
  } else if ( mod == bool_op_cut ) {
    // reverse the cutting edges if necessary
    for (int i=0;i<numberOfEdges();i++) {
      if ( getEdge(i).st < 0 || getEdge(i).en < 0 ) {
        if ( i < numberOfEdges()-1 ) {
          // shift the askForWinding
          int cp = swsData[numberOfEdges()-1].firstLinkedPoint;
          while (cp >= 0) {
            pData[cp].askForWindingB = i;
            cp = pData[cp].nextLinkedPoint;
          }
        }
        SwapEdges(i,numberOfEdges()-1);
        SubEdge(numberOfEdges()-1);
        i--;
      } else if ( ebData[i].pathID == cutPathID ) {
        swdData[i].leW=swdData[i].leW%2;
        swdData[i].riW=swdData[i].riW%2;
        if ( swdData[i].leW < swdData[i].riW ) {
          Inverse(i);
        }
      }
    }
  } else if ( mod == bool_op_slice ) {
    // supprimer les aretes de la coupe
    int i=numberOfEdges()-1;
    for (;i>=0;i--) {
      if ( ebData[i].pathID == cutPathID || getEdge(i).st < 0 || getEdge(i).en < 0 ) {
        SubEdge(i);
      }
    }
  }
  else
  {
    for (int i = 0; i < numberOfEdges(); i++)
    {
      if (swdData[i].leW > 0 && swdData[i].riW <= 0)
	    {
	      eData[i].weight = 1;
	    }
      else if (swdData[i].leW <= 0 && swdData[i].riW > 0)
	    {
	      Inverse (i);
	      eData[i].weight = 1;
	    }
      else
	    {
	      eData[i].weight = 0;
	      SubEdge (i);
	      i--;
	    }
    }
  }
  
  delete sTree;
  sTree = nullptr;
  delete sEvts;
  sEvts = nullptr;
  
  if ( mod == bool_op_cut ) {
    // on garde le askForWinding
  } else {
    MakePointData (false);
  }
  MakeEdgeData (false);
  MakeSweepSrcData (false);
  MakeSweepDestData (false);
  a->CleanupSweep ();
  b->CleanupSweep ();

  if (directedEulerian(this) == false)
    {
//              printf( "pas euclidian2");
      _pts.clear();
      _aretes.clear();
      return shape_euler_err;
    }
  type = shape_polygon;
  return 0;
}

// frontend to the TesteIntersection() below
void Shape::TesteIntersection(SweepTree *t, Side s, bool onlyDiff)
{
  // get the element that is to the side s of node t
  SweepTree *tt = static_cast<SweepTree*>(t->elem[s]);
  if (tt == nullptr) {
    return;
  }

  // set left right properly, a is left, b is right
  SweepTree *a = (s == LEFT) ? tt : t;
  SweepTree *b = (s == LEFT) ? t : tt;

  // call the actual intersection checking function and if an intersection
  // is detected, add it as an event
  Geom::Point atx;
  double atl;
  double atr;
  if (TesteIntersection(a, b, atx, atl, atr, onlyDiff)) {
    sEvts->add(a, b, atx, atl, atr);
  }
}

// a crucial piece of code: computing intersections between segments
bool
Shape::TesteIntersection (SweepTree * iL, SweepTree * iR, Geom::Point &atx, double &atL, double &atR, bool onlyDiff)
{
  // get the left edge's start and end point
  int lSt = iL->src->getEdge(iL->bord).st, lEn = iL->src->getEdge(iL->bord).en;
  // get the right edge's start and end point
  int rSt = iR->src->getEdge(iR->bord).st, rEn = iR->src->getEdge(iR->bord).en;
  // get both edge vectors
  Geom::Point ldir, rdir;
  ldir = iL->src->eData[iL->bord].rdx;
  rdir = iR->src->eData[iR->bord].rdx;
  // first, a round of checks to quickly dismiss edge which obviously dont intersect,
  // such as having disjoint bounding boxes

  // invert the edge vector and swap the endpoints if an edge is bottom to top
  // or horizontal and right to left
  if (lSt < lEn)
  {
  }
  else
  {
    std::swap(lSt, lEn);
    ldir = -ldir;
  }
  if (rSt < rEn)
  {
  }
  else
  {
    std::swap(rSt, rEn);
    rdir = -rdir;
  }

  // these blocks check if the bounding boxes of the two don't overlap, if they
  // don't overlap, we can just return false since non-overlapping bounding boxes
  // indicate they won't intersect
  if (iL->src->pData[lSt].rx[0] < iL->src->pData[lEn].rx[0])
  {
    if (iR->src->pData[rSt].rx[0] < iR->src->pData[rEn].rx[0])
    {
      if (iL->src->pData[lSt].rx[0] > iR->src->pData[rEn].rx[0])
        return false;
      if (iL->src->pData[lEn].rx[0] < iR->src->pData[rSt].rx[0])
        return false;
    }
    else
    {
      if (iL->src->pData[lSt].rx[0] > iR->src->pData[rSt].rx[0])
        return false;
      if (iL->src->pData[lEn].rx[0] < iR->src->pData[rEn].rx[0])
        return false;
    }
  }
  else
  {
    if (iR->src->pData[rSt].rx[0] < iR->src->pData[rEn].rx[0])
    {
      if (iL->src->pData[lEn].rx[0] > iR->src->pData[rEn].rx[0])
        return false;
      if (iL->src->pData[lSt].rx[0] < iR->src->pData[rSt].rx[0])
        return false;
    }
    else
    {
      if (iL->src->pData[lEn].rx[0] > iR->src->pData[rSt].rx[0])
        return false;
      if (iL->src->pData[lSt].rx[0] < iR->src->pData[rEn].rx[0])
        return false;
    }
  }

  // see the second image in the header docs of this function to visualize
  // this cross product
  double ang = cross (ldir, rdir);
  //      ang*=iL->src->eData[iL->bord].isqlength;
  //      ang*=iR->src->eData[iR->bord].isqlength;
  if (ang <= 0) return false;		// edges in opposite directions:  <-left  ... right ->
  // they can't intersect

  // d'abord tester les bords qui partent d'un meme point
  // if they come from same shape and they share the same start point
  if (iL->src == iR->src && lSt == rSt)
  {
    // if they share the end point too, it's a doublon doesn't count as intersection
    if (iL->src == iR->src && lEn == rEn)
      return false;		// c'est juste un doublon
    // if we are here, it means they share the start point only and that counts as an interesection
    // intersection point is the starting point and times are all set to -1
    atx = iL->src->pData[lSt].rx;
    atR = atL = -1;
    return true;		// l'ordre est mauvais
  }
  // if they only share the end points, doesn't count as intersection (no idea why)
  // in my opinion, both endpoints shouldn't count for intersection
  if (iL->src == iR->src && lEn == rEn)
    return false;		// rien a faire=ils vont terminer au meme endroit

  // tester si on est dans une intersection multiple

  // I'm not sure what onlyDiff does but it seems it stands for "only different", which means, the intersections
  // will only be detected if the two edges are coming from different shapes, if they come from the same shape,
  // don't do any checks, we are not interested. but this if statement should be somewhere above in the code
  // not here, shouldn't it? Why bother doing the bounding box checks?
  if (onlyDiff && iL->src == iR->src)
    return false;

  // on reprend les vrais points
  // get the start end points again, since we might have swapped them above
  lSt = iL->src->getEdge(iL->bord).st;
  lEn = iL->src->getEdge(iL->bord).en;
  rSt = iR->src->getEdge(iR->bord).st;
  rEn = iR->src->getEdge(iR->bord).en;

  // compute intersection (if there is one)
  // Boissonat anr Preparata said in one paper that double precision floats were sufficient for get single precision
  // coordinates for the intersection, if the endpoints are single precision. i hope they're right...
  // so till here, lSt, lEn, rSt, rEn have been reset, but note that ldir and rdir are the same
  {
    Geom::Point sDiff, eDiff;
    double slDot, elDot;
    double srDot, erDot;
    // a vector from the start point of right edge to the start point of left edge
    sDiff = iL->src->pData[lSt].rx - iR->src->pData[rSt].rx;
    // a vector from the start point of right edge to the end point of left edge
    eDiff = iL->src->pData[lEn].rx - iR->src->pData[rSt].rx;
    srDot = cross(rdir, sDiff);
    erDot = cross(rdir, eDiff);
    // a vector from the start point of left edge to the start point of right edge
    sDiff = iR->src->pData[rSt].rx - iL->src->pData[lSt].rx;
    // a vector from the start point of left edge to the end point of right edge
    eDiff = iR->src->pData[rEn].rx - iL->src->pData[lSt].rx;
    slDot = cross(ldir, sDiff);
    elDot = cross(ldir, eDiff);
    // these cross products above are shown in the third picture in the header docs of this function.
    // The only thing that matters to us at the moment about these cross products is their sign
    // not their value, the value comes in later. I've labelled the angle arcs with the name of the
    // cross product so that you can immediately visualize the sign that the cross product will take
    // take your right hand, index finger to first vector, middle finger to second vector, if thumb
    // is into the page, cross is positive, else it's negative.

    // basically these cross products give us a sense of where the endpoints of other vector is with
    // respect to a particular vector. If both are on the opposite sides, it indicates that an intersection
    // will happen. Of course you can have the edge far away and still have endpoints on opposite side,
    // they won't intersect but those cases have already been ruled out above

    // if both endpoints of left edge are on one side (the same side doesn't matter which one) of the right edge
    if ((srDot >= 0 && erDot >= 0) || (srDot <= 0 && erDot <= 0))
    {
      // you might think okay if both endpoints of left edge are on same side of right edge, then they can't intersect
      // right? no, they still can. An endpoint of the left edge can fall on the right edge and in that case
      // there is an intersection
      // the start point of the left edge is on the right edge
      if (srDot == 0)
      {
        // this condition here is quite weird, due to it, some intersections won't get detected while others might, I
        // don't really see why it has been done this way. See the fourth figure in the header docs of this function and
        // you'll see both cases. One where the condition lSt < lEn is valid and an intersection is detected and another
        // one where it isn't. In fact as I was testing this out, I realized the purpose of CheckAdjacencies. Apparently
        // when a point of intersection is missed by this function, the edge still gets split up at the intersection point
        // thanks to CheckAdjacencies. You can see this for yourself by taking a 1000 px by 1000 px canvas and the following
        // SVG path: M 500,200 L 500,800 L 200,800 L 500,500 L 200,200 Z
        // Try Path > Union with and without the CheckAdjacencies call and you'll see the difference in the resultant path.
        // So I was trying to find out a case where this if statement lSt < lEn would be true and an intersection would be
        // returned, I couldn't succeed at this. You can get lSt < lEn to be true, but something else above in the code will
        // cause an early return, most likely the ang <= 0 condition. So in order words, I couldn't get the code below to
        // run, ever.
        if (lSt < lEn)
        {
          atx = iL->src->pData[lSt].rx;
          atL = 0;
          atR = slDot / (slDot - elDot);
          return true;
        }
        else
        {
          return false;
        }
      }
      else if (erDot == 0)
      {
        if (lSt > lEn)
        {
          atx = iL->src->pData[lEn].rx;
          atL = 1;
          atR = slDot / (slDot - elDot);
          return true;
        }
        else
        {
          return false;
        }
      }
      // This code doesn't make sense either, I couldn't get it to trigger
      // TODO: Try again or just prove that it's impossible to reach here
      if (srDot > 0 && erDot > 0)
      {
        if (rEn < rSt)
        {
          if (srDot < erDot)
          {
            if (lSt < lEn)
            {
              atx = iL->src->pData[lSt].rx;
              atL = 0;
              atR = slDot / (slDot - elDot);
              return true;
            }
          }
          else
          {
            if (lEn < lSt)
            {
              atx = iL->src->pData[lEn].rx;
              atL = 1;
              atR = slDot / (slDot - elDot);
              return true;
            }
          }
        }
      }
      if (srDot < 0 && erDot < 0)
      {
        if (rEn > rSt)
        {
          if (srDot > erDot)
          {
            if (lSt < lEn)
            {
              atx = iL->src->pData[lSt].rx;
              atL = 0;
              atR = slDot / (slDot - elDot);
              return true;
            }
          }
          else
          {
            if (lEn < lSt)
            {
              atx = iL->src->pData[lEn].rx;
              atL = 1;
              atR = slDot / (slDot - elDot);
              return true;
            }
          }
        }
      }
      return false;
    }
    // if both endpoints of the right edge are on the same side of left edge
    if ((slDot >= 0 && elDot >= 0) || (slDot <= 0 && elDot <= 0))
    {
      if (slDot == 0)
      {
        if (rSt < rEn)
        {
          atx = iR->src->pData[rSt].rx;
          atR = 0;
          atL = srDot / (srDot - erDot);
          return true;
        }
        else
        {
          return false;
        }
      }
      else if (elDot == 0)
      {
        if (rSt > rEn)
        {
          atx = iR->src->pData[rEn].rx;
          atR = 1;
          atL = srDot / (srDot - erDot);
          return true;
        }
        else
        {
          return false;
        }
      }
      if (slDot > 0 && elDot > 0)
      {
        if (lEn > lSt)
        {
          if (slDot < elDot)
          {
            if (rSt < rEn)
            {
              atx = iR->src->pData[rSt].rx;
              atR = 0;
              atL = srDot / (srDot - erDot);
              return true;
            }
          }
          else
          {
            if (rEn < rSt)
            {
              atx = iR->src->pData[rEn].rx;
              atR = 1;
              atL = srDot / (srDot - erDot);
              return true;
            }
          }
        }
      }
      if (slDot < 0 && elDot < 0)
      {
        if (lEn < lSt)
        {
          if (slDot > elDot)
          {
            if (rSt < rEn)
            {
              atx = iR->src->pData[rSt].rx;
              atR = 0;
              atL = srDot / (srDot - erDot);
              return true;
            }
          }
          else
          {
            if (rEn < rSt)
            {
              atx = iR->src->pData[rEn].rx;
              atR = 1;
              atL = srDot / (srDot - erDot);
              return true;
            }
          }
        }
      }
      return false;
    }

    /*		double  slb=slDot-elDot,srb=srDot-erDot;
          if ( slb < 0 ) slb=-slb;
          if ( srb < 0 ) srb=-srb;*/
    // We use different formulas depending on whose sin is greater

    // These formulas would look really weird to you, but let's pick the first one
    // and I'll do some maths that you can see in the header docs to elaborate what's
    // happening
    if (iL->src->eData[iL->bord].siEd > iR->src->eData[iR->bord].siEd)
    {
      atx =
        (slDot * iR->src->pData[rEn].rx -
         elDot * iR->src->pData[rSt].rx) / (slDot - elDot);
    }
    else
    {
      atx =
        (srDot * iL->src->pData[lEn].rx -
         erDot * iL->src->pData[lSt].rx) / (srDot - erDot);
    }
    atL = srDot / (srDot - erDot);
    atR = slDot / (slDot - elDot);
    return true;
  }

  return true;
}

int
Shape::PushIncidence (Shape * a, int cb, int pt, double theta)
{
  if (theta < 0 || theta > 1)
    return -1;

  if (nbInc >= maxInc)
    {
      maxInc = 2 * nbInc + 1;
      iData =
	(incidenceData *) g_realloc(iData, maxInc * sizeof (incidenceData));
    }
  int n = nbInc++;
  iData[n].nextInc = a->swsData[cb].firstLinkedPoint;
  iData[n].pt = pt;
  iData[n].theta = theta;
  a->swsData[cb].firstLinkedPoint = n;
  return n;
}

int
Shape::CreateIncidence (Shape * a, int no, int nPt)
{
  Geom::Point adir, diff;
  adir = a->eData[no].rdx;
  diff = getPoint(nPt).x - a->pData[a->getEdge(no).st].rx;
  double t = dot (diff, adir);
  t *= a->eData[no].ilength;
  return PushIncidence (a, no, nPt, t);
}

int
Shape::Winding (int nPt) const
{
  // the array pData has a variable named askForWindingB
  // that tells us which edge to go to to find the winding number
  // of the pint nPt.
  int askTo = pData[nPt].askForWindingB;
  if (askTo < 0 || askTo >= numberOfEdges()) // if there is no info there, just return 0
    return 0;
  // if the edge is top to bottom, return left winding number otherwise the right winding number
  // actually, while sweeping, ConvertToShape stores the edge on the immediate left of each point,
  // hence, we are seeing the winding to the right of this edge and depending on orientation,
  // right is "left" if edge is top to bottom, right is "right" if edge is bottom to top
  if (getEdge(askTo).st < getEdge(askTo).en)
    {
      return swdData[askTo].leW;
    }
  else
    {
      return swdData[askTo].riW;
    }
  return 0;
}

int
Shape::Winding (const Geom::Point px) const
{
  int lr = 0, ll = 0, rr = 0;

  // for each edge
  for (int i = 0; i < numberOfEdges(); i++)
  {
    Geom::Point adir, diff, ast, aen;
    adir = eData[i].rdx;

    ast = pData[getEdge(i).st].rx; // start point of this edge
    aen = pData[getEdge(i).en].rx; // end point of this edge

    int nWeight = eData[i].weight; // weight of this edge

    // this block checks if the vertical lines crossing the start and end points of the edge
    // covers the point px or not. See the first figure in the header documentation to see
    // what I mean. The figure shows two contours one inside another. It shows the point px
    // and the current edge that the loop is processing is drawn in black color. Two dashed
    // vertical lines create a region. This block of code checks if the point px lies within
    // that region or not. Because if it doesn't, we really don't care about this edge at all
    // then.
    if (ast[0] < aen[0])
    {
      if (ast[0] > px[0])
        continue;
      if (aen[0] < px[0])
        continue;
    }
    else
    {
      if (ast[0] < px[0])
        continue;
      if (aen[0] > px[0])
        continue;
    }

    // the situations in these blocks are explained by the second and third figure in the header documentation
    // the fourth figure along with the documentation there describe what ll and rr really do
    if (ast[0] == px[0])
    {
      if (ast[1] >= px[1])
        continue;
      if (aen[0] == px[0])
        continue;
      if (aen[0] < px[0])
        ll += nWeight;
      else
        rr -= nWeight;
      continue;
    }
    if (aen[0] == px[0])
    {
      if (aen[1] >= px[1])
        continue;
      if (ast[0] == px[0])
        continue;
      if (ast[0] < px[0])
        ll -= nWeight;
      else
        rr += nWeight;
      continue;
    }

    // if the edge is below the point, it doesn't cut the ray at all
    // so we don't care about it
    if (ast[1] < aen[1])
    {
      if (ast[1] >= px[1])
        continue;
    }
    else
    {
      if (aen[1] >= px[1])
        continue;
    }

    // a vector from the edge start point to our point px whose winding we wanna calculate
    diff = px - ast;
    double cote = cross(adir, diff); // cross from edge vector to diff vector to figure out the orientation
    if (cote == 0)
      continue;
    if (cote < 0)
    {
      if (ast[0] > px[0])
        lr += nWeight;
    }
    else
    {
      if (ast[0] < px[0])
        lr -= nWeight;
    }
  }
  return lr + (ll + rr) / 2; // lr comes as it is, ll and rr get divided by two due to the reason I mention in the header file docs
}

// merging duplicate points and edges
int
Shape::AssemblePoints (int st, int en)
{
  if (en > st) {
   for (int i = st; i < en; i++) pData[i].oldInd = i;
//              SortPoints(st,en-1);
    SortPointsByOldInd (st, en - 1); // SortPointsByOldInd() is required here, because of the edges we have
                                       // associated with the point for later computation of winding numbers.
                                       // specifically, we need the first point we treated, it's the only one with a valid
                                       // associated edge (man, that was a nice bug).
     for (int i = st; i < en; i++) pData[pData[i].oldInd].newInd = i;

     int lastI = st;
     for (int i = st; i < en; i++) {
	      pData[i].pending = lastI++;
	      if (i > st && getPoint(i - 1).x[0] == getPoint(i).x[0] && getPoint(i - 1).x[1] == getPoint(i).x[1]) {
	        pData[i].pending = pData[i - 1].pending;
	        if (pData[pData[i].pending].askForWindingS == nullptr) {
		        pData[pData[i].pending].askForWindingS = pData[i].askForWindingS;
		        pData[pData[i].pending].askForWindingB = pData[i].askForWindingB;
		      } else {
		        if (pData[pData[i].pending].askForWindingS == pData[i].askForWindingS
		      && pData[pData[i].pending].askForWindingB == pData[i].askForWindingB) {
		      // meme bord, c bon
		        } else {
		      // meme point, mais pas le meme bord: ouille!
		      // il faut prendre le bord le plus a gauche
		      // en pratique, n'arrive que si 2 maxima sont dans la meme case -> le mauvais choix prend une arete incidente
		      // au bon choix
//                                              printf("doh");
		        }
		      }
	        lastI--;
	      } else {
	        if (i > pData[i].pending) {
		        _pts[pData[i].pending].x = getPoint(i).x;
		        pData[pData[i].pending].rx = getPoint(i).x;
		        pData[pData[i].pending].askForWindingS = pData[i].askForWindingS;
		        pData[pData[i].pending].askForWindingB = pData[i].askForWindingB;
		      }
	      }
	    }
      for (int i = st; i < en; i++) pData[i].newInd = pData[pData[i].newInd].pending;
      return lastI;
  }
  return en;
}

void
Shape::AssemblePoints (Shape * a)
{
  if (hasPoints())
    {
      int lastI = AssemblePoints (0, numberOfPoints());

      for (int i = 0; i < a->numberOfEdges(); i++)
	{
	  a->swsData[i].stPt = pData[a->swsData[i].stPt].newInd;
	  a->swsData[i].enPt = pData[a->swsData[i].enPt].newInd;
	}
      for (int i = 0; i < nbInc; i++)
	iData[i].pt = pData[iData[i].pt].newInd;

      _pts.resize(lastI);
    }
}
void
Shape::AssembleAretes (FillRule directed)
{
  if ( directed == fill_justDont && _has_back_data == false ) {
    directed=fill_nonZero;
  }
  
  // for each point in points
  for (int i = 0; i < numberOfPoints(); i++) {
    if (getPoint(i).totalDegree() == 2) { // if simple point with one incoming edge another outgoing edge
      int cb, cc;
      cb = getPoint(i).incidentEdge[FIRST]; // the first edge connected to the point
      cc = getPoint(i).incidentEdge[LAST]; // the last edge connected to the point
      bool  doublon=false;
      if ((getEdge(cb).st == getEdge(cc).st && getEdge(cb).en == getEdge(cc).en)
          || (getEdge(cb).st == getEdge(cc).en && getEdge(cb).en == getEdge(cc).en)) doublon=true; // if the start and end edges have same endpoints, it's a doublon edge
      if ( directed == fill_justDont ) {
        if ( doublon ) {
          // depending on pathID, pieceID and tSt reorient cb and cc if needed
          if ( ebData[cb].pathID > ebData[cc].pathID ) {
            cc = getPoint(i).incidentEdge[FIRST]; // on swappe pour enlever cc
            cb = getPoint(i).incidentEdge[LAST];
          } else if ( ebData[cb].pathID == ebData[cc].pathID ) {
            if ( ebData[cb].pieceID > ebData[cc].pieceID ) {
              cc = getPoint(i).incidentEdge[FIRST]; // on swappe pour enlever cc
              cb = getPoint(i).incidentEdge[LAST];
            } else if ( ebData[cb].pieceID == ebData[cc].pieceID ) { 
              if ( ebData[cb].tSt > ebData[cc].tSt ) {
                cc = getPoint(i).incidentEdge[FIRST]; // on swappe pour enlever cc
                cb = getPoint(i).incidentEdge[LAST];
              }
            }
          }
        }
        if ( doublon ) eData[cc].weight = 0; // make cc's weight zero
      } else {
      }
      if ( doublon ) {
        if (getEdge(cb).st == getEdge(cc).st) { // if both edges share same start point
          eData[cb].weight += eData[cc].weight; // you get double weight
        } else {
          eData[cb].weight -= eData[cc].weight; // if one's start is other's end, you subtract weight
        }
        eData[cc].weight = 0; // remove cc (set weight to zero)

        // winding number seed stuff
        if (swsData[cc].firstLinkedPoint >= 0) {
          int cp = swsData[cc].firstLinkedPoint;
          while (cp >= 0) {
            pData[cp].askForWindingB = cb;
            cp = pData[cp].nextLinkedPoint;
          }
          if (swsData[cb].firstLinkedPoint < 0) {
            swsData[cb].firstLinkedPoint = swsData[cc].firstLinkedPoint;
          } else {
            int ncp = swsData[cb].firstLinkedPoint;
            while (pData[ncp].nextLinkedPoint >= 0) {
              ncp = pData[ncp].nextLinkedPoint;
            }
            pData[ncp].nextLinkedPoint = swsData[cc].firstLinkedPoint;
          }
        }

        // disconnect start and end of cc
        DisconnectStart (cc);
        DisconnectEnd (cc);

        if (numberOfEdges() > 1) {
          int cp = swsData[numberOfEdges() - 1].firstLinkedPoint;
          while (cp >= 0) {
            pData[cp].askForWindingB = cc;
            cp = pData[cp].nextLinkedPoint;
          }
        }
        // swap cc with last edge
        SwapEdges (cc, numberOfEdges() - 1);
        if (cb == numberOfEdges() - 1) {
          cb = cc;
        }
        // pop back the last one (to completely remove it from the array)
        _aretes.pop_back();
      }
    } else {
      int cb;
      cb = getPoint(i).incidentEdge[FIRST];
      while (cb >= 0 && cb < numberOfEdges()) {
        int other = Other (i, cb);
        int cc;
        cc = getPoint(i).incidentEdge[FIRST];
        while (cc >= 0 && cc < numberOfEdges()) {
          int ncc = NextAt (i, cc);
          bool  doublon=false;
          if (cc != cb && Other (i, cc) == other ) doublon=true;
          if ( directed == fill_justDont ) {
            if ( doublon ) {
              if ( ebData[cb].pathID > ebData[cc].pathID ) {
                doublon=false;
              } else if ( ebData[cb].pathID == ebData[cc].pathID ) {
                if ( ebData[cb].pieceID > ebData[cc].pieceID ) {
                  doublon=false;
                } else if ( ebData[cb].pieceID == ebData[cc].pieceID ) { 
                  if ( ebData[cb].tSt > ebData[cc].tSt ) {
                    doublon=false;
                  }
                }
              }
            }
            if ( doublon ) eData[cc].weight = 0;
          } else {
          }
          if ( doublon ) {
//            if (cc != cb && Other (i, cc) == other) {
            // doublon
            if (getEdge(cb).st == getEdge(cc).st) {
              eData[cb].weight += eData[cc].weight;
            } else {
              eData[cb].weight -= eData[cc].weight;
            }
            eData[cc].weight = 0;
            
            if (swsData[cc].firstLinkedPoint >= 0) {
              int cp = swsData[cc].firstLinkedPoint;
              while (cp >= 0) {
                pData[cp].askForWindingB = cb;
                cp = pData[cp].nextLinkedPoint;
              }
              if (swsData[cb].firstLinkedPoint < 0) {
                swsData[cb].firstLinkedPoint = swsData[cc].firstLinkedPoint;
              } else {
                int ncp = swsData[cb].firstLinkedPoint;
                while (pData[ncp].nextLinkedPoint >= 0) {
                  ncp = pData[ncp].nextLinkedPoint;
                }
                pData[ncp].nextLinkedPoint = swsData[cc].firstLinkedPoint;
              }
            }
            
            DisconnectStart (cc);
            DisconnectEnd (cc);
            if (numberOfEdges() > 1) {
              int cp = swsData[numberOfEdges() - 1].firstLinkedPoint;
              while (cp >= 0) {
                pData[cp].askForWindingB = cc;
                cp = pData[cp].nextLinkedPoint;
              }
            }
            SwapEdges (cc, numberOfEdges() - 1);
            if (cb == numberOfEdges() - 1) {
              cb = cc;
            }
            if (ncc == numberOfEdges() - 1) {
              ncc = cc;
            }
            _aretes.pop_back();
          }
          cc = ncc;
          }
          cb = NextAt (i, cb);
        }
      }
    }

    if ( directed == fill_justDont ) {
      for (int i = 0; i < numberOfEdges(); i++)  {
        if (eData[i].weight == 0) {
          //        SubEdge(i);
          //       i--;
        } else {
          if (eData[i].weight < 0) Inverse (i);
        }
      }
    } else {
      for (int i = 0; i < numberOfEdges(); i++)  {
        if (eData[i].weight == 0) {
          //                      SubEdge(i);
          //                      i--;
        } else {
          if (eData[i].weight < 0) Inverse (i);
        }
      }
    }
  }
void
Shape::GetWindings (bool brutal)
{
  // preparation du parcours
  for (int i = 0; i < numberOfEdges(); i++)
  {
    swdData[i].misc = 0;
    swdData[i].precParc = swdData[i].suivParc = -1;
  }

  // we make sure that the edges are sorted. What this means is that for each point, all
  // the edges that are attached to it are arranged in the linked list according to
  // clockwise order (of their spatial orientation)
  // chainage
  SortEdges ();

  int searchInd = 0;

  int lastPtUsed = 0;
  // okay now let's see what this outer most loop is supposed to do. Look, you can have a directed graph
  // with multiple paths that don't touch each other. For example a rectangle inside another rectangle.
  // If you just start at the first point and follow the edges moving around, you'd have explored one
  // sub-graph but you wouldn't even touch the others. This outer loop ensures that all the points have
  // been walked over. We start at the first point and start exploring. When we reach the end of that
  // sub-graph, we update lastPtUsed and this outerloop will check if there are still points remaining
  // to be explored, if yes, we start with the first point (that we haven't touched yet)
  do
  {
    int startBord = -1;
    int outsideW = 0; // the winding number outside (to the top left) of the first point in a sub-graph
    {
      int fi = 0;
			// ignore all points that don't have any edges attached
      for (fi = lastPtUsed; fi < numberOfPoints(); fi++)
      {
        if (getPoint(fi).incidentEdge[FIRST] >= 0 && swdData[getPoint(fi).incidentEdge[FIRST]].misc == 0)
          break;
      }
      lastPtUsed = fi + 1;
      if (fi < numberOfPoints())
      {
				// get the first edge attached to the first point
        int bestB = getPoint(fi).incidentEdge[FIRST];
        if (bestB >= 0)
        {
					// let's start with this edge
          startBord = bestB;
					// is the first point the first in the array? if yes, this ensure it's at the top most and left most position.
          // Hence the winding number must be zero (since that region is literally outside everything)
          if (fi == 0)
          {
            outsideW = 0;
          }
          else
          {
            // you can either compute the winding number by iterating through all the edges
            // basically that would work by seeing how many edges a ray from (0, +infty) would cross
            // and in which order
            if (brutal)
            {
              outsideW = Winding (getPoint(fi).x);
            }
						// or we can get the winding number for that point computed by the sweepline.. this is pretty
            // interesting.
            else
            {
              outsideW = Winding (fi);
            }
          }
          if ( getPoint(fi).totalDegree() == 1 ) {
            if ( fi == getEdge(startBord).en ) {
              if ( eData[startBord].weight == 0 ) {
                // we simply reverse
                Inverse(startBord);
              } else {
                // we pass the askForWinding (otherwise it will remain startBord)
                pData[getEdge(startBord).st].askForWindingB=pData[getEdge(startBord).en].askForWindingB;
              }
            }
          }
          if (getEdge(startBord).en == fi)
            outsideW += eData[startBord].weight;
        }
      }
    }
    if (startBord >= 0)
    {
			// now start from this edge
      // in-depth exploration to put the leF and riF at their values
      swdData[startBord].misc = 1;
			// setting the winding numbers for this edge
			// one question I had was, would these values for the first edge will always be valid?
			// The answer is yes. Due to the fact that edges are sorted clockwise, and that
      // we start with the top most (and leftmost if mutliple top most points exist), and that
      // there is a piece of code above that adds weight to start edge if the edge ends at the current
      // point, I think these will always be correct values.
      swdData[startBord].leW = outsideW;
      swdData[startBord].riW = outsideW - eData[startBord].weight;
      // curBord is the current edge that we are at
      int curBord = startBord;
      // curDir is the direction, true means we are going in the direction of the edge vector, false means
      // we are going in the direction opposite to the edge vector
      bool curDir = true;
      swdData[curBord].precParc = -1;
      swdData[curBord].suivParc = -1;
      // the depth first search
      do
      {
        int cPt;
        // if curDir is true, we are going along the edge, so get the end point
        if (curDir)
          cPt = getEdge(curBord).en;
        else // if curDir is false, we are going opposite to the edge, so get the start point
          cPt = getEdge(curBord).st;

        // start finding the next edge to move to
        int nb = curBord;
        do
        {
          int nnb = -1;
          // see the diagram attached in the header file documentation of this function to see the
          // four situations that can come up.
          // outsideW here does not mean outside winding, in fact it means inside winding.
          // if we are going along the edge, we save the right winding number for later use
          if (getEdge(nb).en == cPt)
          {
            outsideW = swdData[nb].riW;
            nnb = CyclePrevAt (cPt, nb); // get the prev edge, since sorting was clockwise, this means get the first counter-clockwise edge
          }
          // if we are going against the edge, we save it's left winding number for later use
          else
          {
            outsideW = swdData[nb].leW;
            nnb = CyclePrevAt (cPt, nb);
          }
          if (nnb == nb) // if we didn't get any "new" edge
          {
            // cul-de-sac
            nb = -1;
            break;
          }
          nb = nnb;
        } // you can break for three reasons from this loop: having no other edge, we got the same one we started on, edge hasn't been visited yet
        while (nb >= 0 && nb != curBord && swdData[nb].misc != 0);
        // in the beginning, you'd break from the upper loop due to the misc condition and later on, you'll break due to nb != curBord which
        // means we need to start backtracking
        if (nb < 0 || nb == curBord) // backtracking block
        {
          // retour en arriere
          // so if we are here, we couldn't get any new edge that we haven't seen yet
          int oPt;
          // we wanna find the previous point (since we are going back)
          // if curDir is True, we were going along the edge, so get the start point (going backwards u see)
          if (curDir)
            oPt = getEdge(curBord).st;
          else // if curDir is false, we were going against edge, so get the end point (going backwards)
            oPt = getEdge(curBord).en;
          curBord = swdData[curBord].precParc; // make current edge the previous one in traversal (back tracking)
          if (curBord < 0) // if no edge to go back to, break
            break;
          if (oPt == getEdge(curBord).en) // if this new "current edge" ends at that point, curDir should be true, since we ideally have to go forward
            curDir = true;
          else // otherwise set it to false, so ideal direction would be against edge
            curDir = false;
          // I say ideal because this this is how backtracking is, if you have nothing new to go forward to, you go back once and see
          // if there is another new edge to go forward to, if not you go back again, and you keep doing this until the point comes where
          // you have nothing to go back to and then you break from the loop
        }
        else // okay we have new edge to compute windings
        {
          swdData[nb].misc = 1; // we visited this edge, so mark that
          swdData[nb].ind = searchInd++; // probably for use later on?
          if (cPt == getEdge(nb).st) // this outsideW is the winding stored before, see the diagram in header file
          {
            swdData[nb].riW = outsideW;
            swdData[nb].leW = outsideW + eData[nb].weight;
          }
          else
          {
            swdData[nb].leW = outsideW;
            swdData[nb].riW = outsideW - eData[nb].weight;
          }
          // maintaining the stack of traversal
          swdData[nb].precParc = curBord;
          swdData[curBord].suivParc = nb;
          // this edge becomes current edge now
          curBord = nb;

          // set direction depending on how this edge is oriented
          if (cPt == getEdge(nb).en)
            curDir = false;
          else
            curDir = true;
        }
      }
      while (true /*swdData[curBord].precParc >= 0 */ );
      // end of the unoriented case
    }
  }
  while (lastPtUsed < numberOfPoints());
}

bool
Shape::TesteIntersection (Shape * ils, Shape * irs, int ilb, int irb,
                          Geom::Point &atx, double &atL, double &atR,
			  bool /*onlyDiff*/)
{
  int lSt = ils->getEdge(ilb).st, lEn = ils->getEdge(ilb).en;
  int rSt = irs->getEdge(irb).st, rEn = irs->getEdge(irb).en;
  if (lSt == rSt || lSt == rEn)
    {
      return false;
    }
  if (lEn == rSt || lEn == rEn)
    {
      return false;
    }

  Geom::Point ldir, rdir;
  ldir = ils->eData[ilb].rdx;
  rdir = irs->eData[irb].rdx;

  double il = ils->pData[lSt].rx[0], it = ils->pData[lSt].rx[1], ir =
    ils->pData[lEn].rx[0], ib = ils->pData[lEn].rx[1];
  if (il > ir)
    {
      std::swap(il, ir);
    }
  if (it > ib)
    {
      std::swap(it, ib);
    }
  double jl = irs->pData[rSt].rx[0], jt = irs->pData[rSt].rx[1], jr =
    irs->pData[rEn].rx[0], jb = irs->pData[rEn].rx[1];
  if (jl > jr)
    {
      std::swap(jl, jr);
    }
  if (jt > jb)
    {
      std::swap(jt, jb);
    }

  if (il > jr || it > jb || ir < jl || ib < jt)
    return false;

  // pre-test
  {
    Geom::Point sDiff, eDiff;
    double slDot, elDot;
    double srDot, erDot;
    sDiff = ils->pData[lSt].rx - irs->pData[rSt].rx;
    eDiff = ils->pData[lEn].rx - irs->pData[rSt].rx;
    srDot = cross(rdir, sDiff);
    erDot = cross(rdir, eDiff);
    if ((srDot >= 0 && erDot >= 0) || (srDot <= 0 && erDot <= 0))
      return false;

    sDiff = irs->pData[rSt].rx - ils->pData[lSt].rx;
    eDiff = irs->pData[rEn].rx - ils->pData[lSt].rx;
    slDot = cross(ldir, sDiff);
    elDot = cross(ldir, eDiff);
    if ((slDot >= 0 && elDot >= 0) || (slDot <= 0 && elDot <= 0))
      return false;

    double slb = slDot - elDot, srb = srDot - erDot;
    if (slb < 0)
      slb = -slb;
    if (srb < 0)
      srb = -srb;
    if (slb > srb)
      {
	atx =
	  (slDot * irs->pData[rEn].rx - elDot * irs->pData[rSt].rx) / (slDot -
								       elDot);
      }
    else
      {
	atx =
	  (srDot * ils->pData[lEn].rx - erDot * ils->pData[lSt].rx) / (srDot -
								       erDot);
      }
    atL = srDot / (srDot - erDot);
    atR = slDot / (slDot - elDot);
    return true;
  }

  // a mettre en double precision pour des resultats exacts
  Geom::Point usvs;
  usvs = irs->pData[rSt].rx - ils->pData[lSt].rx;

  // pas sur de l'ordre des coefs de m
  Geom::Affine m(ldir[0], ldir[1],
	       rdir[0], rdir[1],
	       0, 0);
  double det = m.det();

  double tdet = det * ils->eData[ilb].isqlength * irs->eData[irb].isqlength;

  if (tdet > -0.0001 && tdet < 0.0001)
    {				// ces couillons de vecteurs sont colineaires
      Geom::Point sDiff, eDiff;
      double sDot, eDot;
      sDiff = ils->pData[lSt].rx - irs->pData[rSt].rx;
      eDiff = ils->pData[lEn].rx - irs->pData[rSt].rx;
      sDot = cross(rdir, sDiff);
      eDot = cross(rdir, eDiff);

      atx =
	(sDot * irs->pData[lEn].rx - eDot * irs->pData[lSt].rx) / (sDot -
								   eDot);
      atL = sDot / (sDot - eDot);

      sDiff = irs->pData[rSt].rx - ils->pData[lSt].rx;
       eDiff = irs->pData[rEn].rx - ils->pData[lSt].rx;
      sDot = cross(ldir, sDiff);
      eDot = cross(ldir, eDiff);

      atR = sDot / (sDot - eDot);

      return true;
    }

  // plus de colinearite ni d'extremites en commun
  m[1] = -m[1];
  m[2] = -m[2];
  {
    double swap = m[0];
    m[0] = m[3];
    m[3] = swap;
  }

  atL = (m[0]* usvs[0] + m[1] * usvs[1]) / det;
  atR = -(m[2] * usvs[0] + m[3] * usvs[1]) / det;
  atx = ils->pData[lSt].rx + atL * ldir;


  return true;
}

bool
Shape::TesteAdjacency (Shape * a, int no, const Geom::Point atx, int nPt,
		       bool push)
{
  if (nPt == a->swsData[no].stPt || nPt == a->swsData[no].enPt)
    return false;

  Geom::Point adir, diff, ast, aen, diff1, diff2, diff3, diff4;

  ast = a->pData[a->getEdge(no).st].rx;
  aen = a->pData[a->getEdge(no).en].rx;

  adir = a->eData[no].rdx;

  double sle = a->eData[no].length;
  double ile = a->eData[no].ilength;

  diff = atx - ast;
 
  double e = IHalfRound(cross(adir, diff) * a->eData[no].isqlength);
  if (-3 < e && e < 3)
    {
      double rad = HalfRound (0.501); // when using single precision, 0.505 is better (0.5 would be the correct value, 
                                      // but it produces lots of bugs)
      diff1[0] = diff[0] - rad;
      diff1[1] = diff[1] - rad;
      diff2[0] = diff[0] + rad;
      diff2[1] = diff[1] - rad;
      diff3[0] = diff[0] + rad;
      diff3[1] = diff[1] + rad;
      diff4[0] = diff[0] - rad;
      diff4[1] = diff[1] + rad;
      double di1, di2;
      bool adjacent = false;
      di1 = cross(adir, diff1);
      di2 = cross(adir, diff3);
      if ((di1 < 0 && di2 > 0) || (di1 > 0 && di2 < 0))
	{
	  adjacent = true;
	}
      else
	{
	  di1 = cross(adir, diff2);
	  di2 = cross(adir, diff4);
	  if ((di1 < 0 && di2 > 0) || (di1 > 0 && di2 < 0))
	    {
	      adjacent = true;
	    }
	}
      if (adjacent)
	{
	  double t = dot (diff, adir);
	  if (t > 0 && t < sle)
	    {
	      if (push)
		{
		  t *= ile;
		  PushIncidence (a, no, nPt, t);
		}
	      return true;
	    }
	}
    }
  return false;
}

void
Shape::CheckAdjacencies (int lastPointNo, int lastChgtPt, Shape * /*shapeHead*/,
			 int /*edgeHead*/)
{
  // for each event in chgts
  for (auto & chgt : chgts)
  {
    // get the chgt.ptoNo, which is the point at which the event happened
    int chLeN = chgt.ptNo;
    int chRiN = chgt.ptNo;
    if (chgt.src)
    {
      Shape *lS = chgt.src;
      int lB = chgt.bord;
      // get the leftRnd and rightRnd of this edge
      int lftN = lS->swsData[lB].leftRnd;
      int rgtN = lS->swsData[lB].rightRnd;
      // expand the range chLeN..chRiN
      if (lftN < chLeN)
        chLeN = lftN;
      if (rgtN > chRiN)
        chRiN = rgtN;
      // check each point from lastChgtPt to leftN-1 for a possible adjacency with
      // the edge, if detected mark it by modifying leftRnd
      // Note we do this in reverse order by starting at the point closer to the
      // edge first. If an adjacency is not detected, we immediately break since
      // if a point is not adjacent, another on its left won't be adjacent either
//                      for (int n=lftN;n<=rgtN;n++) CreateIncidence(lS,lB,n);
      for (int n = lftN - 1; n >= lastChgtPt; n--)
      {
        if (TesteAdjacency (lS, lB, getPoint(n).x, n, false) ==
            false)
          break;
        lS->swsData[lB].leftRnd = n;
      }
      // check each point from rgtN+1 to lastPointNo (not included) for a possible adjacency with
      // the edge, if detected mark it by modifying rightRnd
      // If an adjacency is not detected, we immediately break since if a point
      // is not adjacent, another one to its right won't be either.
      for (int n = rgtN + 1; n < lastPointNo; n++)
      {
        if (TesteAdjacency (lS, lB, getPoint(n).x, n, false) ==
            false)
          break;
        lS->swsData[lB].rightRnd = n;
      }
    }
    // totally identical to the block above
    if (chgt.osrc)
    {
      Shape *rS = chgt.osrc;
      int rB = chgt.obord;
      int lftN = rS->swsData[rB].leftRnd;
      int rgtN = rS->swsData[rB].rightRnd;
      if (lftN < chLeN)
        chLeN = lftN;
      if (rgtN > chRiN)
        chRiN = rgtN;
//                      for (int n=lftN;n<=rgtN;n++) CreateIncidence(rS,rB,n);
      for (int n = lftN - 1; n >= lastChgtPt; n--)
      {
        if (TesteAdjacency (rS, rB, getPoint(n).x, n, false) ==
            false)
          break;
        rS->swsData[rB].leftRnd = n;
      }
      for (int n = rgtN + 1; n < lastPointNo; n++)
      {
        if (TesteAdjacency (rS, rB, getPoint(n).x, n, false) ==
            false)
          break;
        rS->swsData[rB].rightRnd = n;
      }
    }
    // very interesting part, deals with edges to the left in the sweepline at the time
    // the event took place
    if (chgt.lSrc)
    {
      // is the left edge's leftRnd smaller than lastChgtPt, basically this is a way to check
      // if leftRnd got updated in the previous iteration of the main loop of Shape::ConvertToShape
      // or not.
      if (chgt.lSrc->swsData[chgt.lBrd].leftRnd < lastChgtPt)
      {
        // get the left edge and its shape
        Shape *nSrc = chgt.lSrc;
        int nBrd = chgt.lBrd /*,nNo=chgts[cCh].ptNo */ ;
        bool hit;

        // iterate through the linked list of edges to the left
        do
        {
          hit = false; // adjacency got detected?
          // check all points from chRiN to chLeN
          // we go right to left
          for (int n = chRiN; n >= chLeN; n--)
          {
            // test if the point is adjacent to the edge
            if (TesteAdjacency
                (nSrc, nBrd, getPoint(n).x, n, false))
            {
              // has the leftRnd been updated in previous iteration? if no? set it directly
              if (nSrc->swsData[nBrd].leftRnd < lastChgtPt)
              {
                nSrc->swsData[nBrd].leftRnd = n;
                nSrc->swsData[nBrd].rightRnd = n;
              }
              else // if yes, we do some checking and only update if it expands the span
              {
                if (n < nSrc->swsData[nBrd].leftRnd)
                  nSrc->swsData[nBrd].leftRnd = n;
                if (n > nSrc->swsData[nBrd].rightRnd)
                  nSrc->swsData[nBrd].rightRnd = n;
              }
              hit = true;
            }
          }
          // test all points between lastChgtPt and chLeN - 1
          for (int n = chLeN - 1; n >= lastChgtPt; n--)
          {
            if (TesteAdjacency
                (nSrc, nBrd, getPoint(n).x, n, false) == false)
              break;
            if (nSrc->swsData[nBrd].leftRnd < lastChgtPt)
            {
              nSrc->swsData[nBrd].leftRnd = n;
              nSrc->swsData[nBrd].rightRnd = n;
            }
            else
            {
              if (n < nSrc->swsData[nBrd].leftRnd)
                nSrc->swsData[nBrd].leftRnd = n;
              if (n > nSrc->swsData[nBrd].rightRnd)
                nSrc->swsData[nBrd].rightRnd = n;
            }
            hit = true;
          }
          // if no adjacency got detected, no point in going further left so break, if yes
          // we continue and see if we can repeat the process on the edge to the left
          // and so on (basically going left detecting adjacencies)
          if (hit)
          {
            // get the edge on the left, if non exist, break
            SweepTree *node = nSrc->swsData[nBrd].misc;
            if (node == nullptr)
              break;
            node = static_cast < SweepTree * >(node->elem[LEFT]);
            if (node == nullptr)
              break;
            nSrc = node->src;
            nBrd = node->bord;
            // the edge on the left, did its leftRnd update in the previous iteration of the main loop if ConvertToShape?
            if (nSrc->swsData[nBrd].leftRnd >= lastChgtPt)
              break;
          }
        }
        while (hit);

      }
    }
    // same thing but to the right side?
    if (chgt.rSrc)
    {
      if (chgt.rSrc->swsData[chgt.rBrd].leftRnd < lastChgtPt)
      {
        Shape *nSrc = chgt.rSrc;
        int nBrd = chgt.rBrd /*,nNo=chgts[cCh].ptNo */ ;
        bool hit;
        do
        {
          hit = false;
          // test points between chLeN and chRiN for adjacency
          // but go left to right
          for (int n = chLeN; n <= chRiN; n++)
          {
            if (TesteAdjacency
                (nSrc, nBrd, getPoint(n).x, n, false))
            {
              if (nSrc->swsData[nBrd].leftRnd < lastChgtPt)
              {
                nSrc->swsData[nBrd].leftRnd = n;
                nSrc->swsData[nBrd].rightRnd = n;
              }
              else
              {
                if (n < nSrc->swsData[nBrd].leftRnd)
                  nSrc->swsData[nBrd].leftRnd = n;
                if (n > nSrc->swsData[nBrd].rightRnd)
                  nSrc->swsData[nBrd].rightRnd = n;
              }
              hit = true;
            }
          }
          // testing points between chRiN and lastPointNo for adjacency
          for (int n = chRiN + 1; n < lastPointNo; n++)
          {
            if (TesteAdjacency
                (nSrc, nBrd, getPoint(n).x, n, false) == false)
              break;
            if (nSrc->swsData[nBrd].leftRnd < lastChgtPt)
            {
              nSrc->swsData[nBrd].leftRnd = n;
              nSrc->swsData[nBrd].rightRnd = n;
            }
            else
            {
              if (n < nSrc->swsData[nBrd].leftRnd)
                nSrc->swsData[nBrd].leftRnd = n;
              if (n > nSrc->swsData[nBrd].rightRnd)
                nSrc->swsData[nBrd].rightRnd = n;
            }
            hit = true;
          }
          if (hit)
          {
            SweepTree *node = nSrc->swsData[nBrd].misc;
            if (node == nullptr)
              break;
            node = static_cast < SweepTree * >(node->elem[RIGHT]);
            if (node == nullptr)
              break;
            nSrc = node->src;
            nBrd = node->bord;
            if (nSrc->swsData[nBrd].leftRnd >= lastChgtPt)
              break;
          }
        }
        while (hit);
      }
    }
  }
}


void Shape::AddChgt(int lastPointNo, int lastChgtPt, Shape * &shapeHead,
		    int &edgeHead, sTreeChangeType type, Shape * lS, int lB, Shape * rS,
		    int rB)
{
  // fill in the event details and push it
  sTreeChange c;
  c.ptNo = lastPointNo;
  c.type = type;
  c.src = lS;
  c.bord = lB;
  c.osrc = rS;
  c.obord = rB;
  chgts.push_back(c);
  // index of the newly added event
  const int nCh = chgts.size() - 1;

  /* FIXME: this looks like a cut and paste job */

  if (lS) {
    // if there is an edge to the left, mark it in lSrc
    SweepTree *lE = lS->swsData[lB].misc;
    if (lE && lE->elem[LEFT]) {
      SweepTree *llE = static_cast < SweepTree * >(lE->elem[LEFT]);
      chgts[nCh].lSrc = llE->src;
      chgts[nCh].lBrd = llE->bord;
    } else {
      chgts[nCh].lSrc = nullptr;
      chgts[nCh].lBrd = -1;
    }

    // lastChgtPt is same as lastPointNo if lastPointNo is the leftmost point in that y level and it's the
    // left most point on the y level of lastPointNo otherwise
    // leftRnd will be smaller than lastChgtPt if the last time the edge participated in any event (edge addition/intersection)
    // was before lastChgtPt.
    if (lS->swsData[lB].leftRnd < lastChgtPt) {
      lS->swsData[lB].leftRnd = lastPointNo; // set leftRnd to the current point
      lS->swsData[lB].nextSh = shapeHead; // add this edge in the linked list
      lS->swsData[lB].nextBo = edgeHead;
      edgeHead = lB;
      shapeHead = lS;
    } else { // If an event occurred to the edge after lastChgtPt (which is possible, for example, a horizontal edge
             // that intersects with other edges.
      // get the leftRnd already
      int old = lS->swsData[lB].leftRnd;
      // This seems really weird. I suspect this if statement will never be true in a top to bottom sweepline
      // see if we reached this point, it means old >= lastChgtPt
      // and lastChgtPt is the leftmost point at the current y level of lastPointNo
      // so how can old have an x greater than lastPoint? The sweepline hasn't reached that position yet!
      if (getPoint(old).x[0] > getPoint(lastPointNo).x[0]) {
        lS->swsData[lB].leftRnd = lastPointNo;
      }
    }
    // same logic as in the upper block
    if (lS->swsData[lB].rightRnd < lastChgtPt) {
      lS->swsData[lB].rightRnd = lastPointNo;
    } else {
      int old = lS->swsData[lB].rightRnd;
      if (getPoint(old).x[0] < getPoint(lastPointNo).x[0])
        lS->swsData[lB].rightRnd = lastPointNo;
    }
  }

  // it's an intersection event and rS is the right edge's shape and rB is the
  // right edge
  if (rS) {
    // get the edge on the right and set it in rBrd and rSrc
    SweepTree *rE = rS->swsData[rB].misc;
    if (rE->elem[RIGHT]) {
      SweepTree *rrE = static_cast < SweepTree * >(rE->elem[RIGHT]);
      chgts[nCh].rSrc = rrE->src;
      chgts[nCh].rBrd = rrE->bord;
    } else {
      chgts[nCh].rSrc = nullptr;
      chgts[nCh].rBrd = -1;
    }

    // same code as above, except that it's on rS
    if (rS->swsData[rB].leftRnd < lastChgtPt) {
      rS->swsData[rB].leftRnd = lastPointNo;
      rS->swsData[rB].nextSh = shapeHead;
      rS->swsData[rB].nextBo = edgeHead;
      edgeHead = rB;
      shapeHead = rS;
    } else {
      int old = rS->swsData[rB].leftRnd;
      if (getPoint(old).x[0] > getPoint(lastPointNo).x[0]) {
        rS->swsData[rB].leftRnd = lastPointNo;
      }
    }
    if (rS->swsData[rB].rightRnd < lastChgtPt) {
      rS->swsData[rB].rightRnd = lastPointNo;
    } else {
      int old = rS->swsData[rB].rightRnd;
      if (getPoint(old).x[0] < getPoint(lastPointNo).x[0])
        rS->swsData[rB].rightRnd = lastPointNo;
    }
  } else { // if rS wasn't set, this is not an intersection event, so
    // check if there is an edge to the right and set rBrd
    SweepTree *lE = lS->swsData[lB].misc;
    if (lE && lE->elem[RIGHT]) {
      SweepTree *rlE = static_cast < SweepTree * >(lE->elem[RIGHT]);
      chgts[nCh].rSrc = rlE->src;
      chgts[nCh].rBrd = rlE->bord;
    } else {
      chgts[nCh].rSrc = nullptr;
      chgts[nCh].rBrd = -1;
    }
  }
}

// is this a debug function?  It's calling localized "printf" ...
void
Shape::Validate ()
{
  for (int i = 0; i < numberOfPoints(); i++)
    {
      pData[i].rx = getPoint(i).x;
    }
  for (int i = 0; i < numberOfEdges(); i++)
    {
      eData[i].rdx = getEdge(i).dx;
    }
  for (int i = 0; i < numberOfEdges(); i++)
    {
      for (int j = i + 1; j < numberOfEdges(); j++)
	{
        Geom::Point atx;
        double   atL, atR;
	  if (TesteIntersection (this, this, i, j, atx, atL, atR, false))
	    {
	      printf ("%i %i  %f %f di=%f %f  dj=%f %f\n", i, j, atx[0],atx[1],getEdge(i).dx[0],getEdge(i).dx[1],getEdge(j).dx[0],getEdge(j).dx[1]);
	    }
	}
    }
  fflush (stdout);
}

void
Shape::CheckEdges (int lastPointNo, int lastChgtPt, Shape * a, Shape * b,
		   BooleanOp mod)
{

  for (auto & chgt : chgts)
  {
    // if an edge addition event, we want to set curPoint to the point at which
    // the edge was added. chgt.ptNo is automatically updated after any possible sorting and merging
    // so thats good too :-)
    if (chgt.type == 0)
    {
      Shape *lS = chgt.src;
      int lB = chgt.bord;
      lS->swsData[lB].curPoint = chgt.ptNo;
    }
  }
  // for each event in events
  for (auto & chgt : chgts)
  {
//              int   chLeN=chgts[cCh].ptNo;
//              int   chRiN=chgts[cCh].ptNo;
    // do the main edge (by do I mean process it, see if anything needs to be drawn, if yes, draw it)
    if (chgt.src)
    {
      Shape *lS = chgt.src;
      int lB = chgt.bord;
      Avance (lastPointNo, lastChgtPt, lS, lB, a, b, mod);
    }
    // do the other edge (the right in intersection, wont exist if chgt wasn't an intersection event)
    if (chgt.osrc)
    {
      Shape *rS = chgt.osrc;
      int rB = chgt.obord;
      Avance (lastPointNo, lastChgtPt, rS, rB, a, b, mod);
    }

    // See there are few cases due to which an edge will have a leftRnd >= lastChgtPt. Either the
    // edge had some event associated with it (addition/removal/intersection) at that y level. Or
    // there was some point in the previous y level that was on top of the edge, and thus an
    // adjacency was detected and the leftRnd/rightRnd were set accordingly. If you have neither of
    // these, leftRnd/rightRnd won't be set at all. If the case is former, that the edge had an
    // event at the previous y level, the blocks above will automatically call Avance on the edge.
    // However, for the latter, we have no chgt event associated with the edge.  Thus, the blocks
    // below calls Shape::Avance on edges to the left and to the right of the unique (or the left)
    // edge. However, it's called only if leftRnd >= lastChgtPt.
    if (chgt.lSrc)
    {
      Shape *nSrc = chgt.lSrc;
      int nBrd = chgt.lBrd;
      while (nSrc->swsData[nBrd].leftRnd >= // <-- if yes, means some event occurred to this event after lastChgtPt or an adjacency was detected
          lastChgtPt /*&& nSrc->swsData[nBrd].doneTo < lastChgtPt */ )
      {
        Avance (lastPointNo, lastChgtPt, nSrc, nBrd, a, b, mod);

        SweepTree *node = nSrc->swsData[nBrd].misc;
        if (node == nullptr)
          break;
        node = static_cast < SweepTree * >(node->elem[LEFT]);
        if (node == nullptr)
          break;
        nSrc = node->src;
        nBrd = node->bord;
      }
    }
    if (chgt.rSrc)
    {
      Shape *nSrc = chgt.rSrc;
      int nBrd = chgt.rBrd;
      while (nSrc->swsData[nBrd].rightRnd >=
          lastChgtPt /*&& nSrc->swsData[nBrd].doneTo < lastChgtPt */ )
      {
        Avance (lastPointNo, lastChgtPt, nSrc, nBrd, a, b, mod);

        SweepTree *node = nSrc->swsData[nBrd].misc;
        if (node == nullptr)
          break;
        node = static_cast < SweepTree * >(node->elem[RIGHT]);
        if (node == nullptr)
          break;
        nSrc = node->src;
        nBrd = node->bord;
      }
    }
  }
}

void
Shape::Avance (int lastPointNo, int lastChgtPt, Shape * lS, int lB, Shape * /*a*/,
	       Shape * b, BooleanOp mod)
{
  double dd = HalfRound (1); // get the smallest step you can take in the rounding grid.
  bool avoidDiag = false; // triggers some special diagonal avoiding code below
//      if ( lastChgtPt > 0 && pts[lastChgtPt-1].y+dd == pts[lastChgtPt].y ) avoidDiag=true;

  // my best guess is that direct acts kinda as an inverter. If set to true, edges are drawn
  // in the direction they should be drawn in, if set to false, they are inverted. This is needed
  // if the edge is coming from shape b and the mod is bool_op_diff or bool_op_symdiff. This is how
  // Livarot does these boolean operations.
  bool direct = true;
  if (lS == b && (mod == bool_op_diff || mod == bool_op_symdiff))
    direct = false;
  // get the leftRnd and rightRnd points of the edge lB. For most edges, leftRnd and rightRnd are identical
  // however when you have a horizontal edge, they can be different.
  int lftN = lS->swsData[lB].leftRnd;
  int rgtN = lS->swsData[lB].rightRnd;
  // doneTo acts as a marker. Once Avance processes an edge at a certain y level, it sets doneTo to the
  // right most point at that y level. See the end of this function to see how it does this.
  if (lS->swsData[lB].doneTo < lastChgtPt)
  {
    // the last point till which this edge has been drawn
    int lp = lS->swsData[lB].curPoint;
    // if the last point exists and lastChgtPt.y is just dd (1 rounded step) bigger than lastpoint.y
    // in simpler words, the last point drawn is just 1 rounded step above lastChgtPt
    // Look, if there is a potential edge to draw, that edge is going to have its upper endpoint at lp
    // and could have the lower endpoint lftN...rgtN whatever, but we are sure that it can't go any lower (downwards)
    // than lastChgtPt. If this "if" block evalues to true, there is a possibility we might an edge that would
    // be diagonal for example 1 rounded unit dd down and to the right. We would like to avoid this if there is
    // a point right below, so we can draw two edges, first down and then right. See the figure in the header
    // docs to see what I mean
    if (lp >= 0 && getPoint(lp).x[1] + dd == getPoint(lastChgtPt).x[1])
      avoidDiag = true;
    // if the edge is horizontal
    if (lS->eData[lB].rdx[1] == 0)
    {
      // tjs de gauche a droite et pas de diagonale -- > left to right horizonal edge won't be diagonal (since it's horizontal :P)
      if (lS->eData[lB].rdx[0] >= 0) // edge is left to right
      {
        for (int p = lftN; p <= rgtN; p++)
        {
          DoEdgeTo (lS, lB, p, direct, true);
          lp = p;
        }
      }
      else // edge is right to left
      {
        for (int p = lftN; p <= rgtN; p++)
        {
          DoEdgeTo (lS, lB, p, direct, false);
          lp = p;
        }
      }
    }
    else if (lS->eData[lB].rdx[1] > 0) // the edge is top to bottom
    {
      if (lS->eData[lB].rdx[0] >= 0) // edge is top to bottom and left to right
      {

        for (int p = lftN; p <= rgtN; p++) // for the range lftN..rgtN
        {
          // if avoidDiag is true, point p is lftN and the point the edge is to be drawn to (lftN) has an x that's
          // 1 rounded unit (dd) greater than the last point drawn. So basically lftN is one unit down and one unit
          // to the right of lp
          if (avoidDiag && p == lftN && getPoint(lftN).x[0] == getPoint(lp).x[0] + dd)
          {
            // we would want to avoid the diagonal but only if there is a point (in our directed graph)
            // just to the left of the original lower endpoint and instead of doing a diagonal, we can create an edge to that point
            // and then from that point to the original endpoint. see the figure in the header docs to see what I mean
            if (lftN > 0 && lftN - 1 >= lastChgtPt // if there is a point in the figure just below lp and to the left of lftN
                && getPoint(lftN - 1).x[0] == getPoint(lp).x[0]) // that point has x equal to that of lp (right below it)
            {
              DoEdgeTo (lS, lB, lftN - 1, direct, true); // draw an edge to lftn - 1
              DoEdgeTo (lS, lB, lftN, direct, true); // then draw an edge to lftN
            }
            else
            {
              DoEdgeTo (lS, lB, lftN, direct, true);
            }
          }
          else
          {
            DoEdgeTo (lS, lB, p, direct, true);
          }
          lp = p;
        }
      }
      else
      {

        for (int p = rgtN; p >= lftN; p--) // top to bottom and right to left
        {
          // exactly identical to the code above except that it's a diagonal down and to the left
          if (avoidDiag && p == rgtN && getPoint(rgtN).x[0] == getPoint(lp).x[0] - dd)
          {
            if (rgtN < numberOfPoints() && rgtN + 1 < lastPointNo // refer to first diagram in header docs to see why this condition
                && getPoint(rgtN + 1).x[0] == getPoint(lp).x[0])
            {
              DoEdgeTo (lS, lB, rgtN + 1, direct, true);
              DoEdgeTo (lS, lB, rgtN, direct, true);
            }
            else
            {
              DoEdgeTo (lS, lB, rgtN, direct, true);
            }
          }
          else
          {
            DoEdgeTo (lS, lB, p, direct, true);
          }
          lp = p;
        }
      }
    }
    else // edge is bottom to top
    {
      if (lS->eData[lB].rdx[0] >= 0) // edge is bottom to top and left to right
      {

        for (int p = rgtN; p >= lftN; p--)
        {
          if (avoidDiag && p == rgtN && getPoint(rgtN).x[0] == getPoint(lp).x[0] - dd)
          {
            if (rgtN < numberOfPoints() && rgtN + 1 < lastPointNo
                && getPoint(rgtN + 1).x[0] == getPoint(lp).x[0])
            {
              DoEdgeTo (lS, lB, rgtN + 1, direct, false); // draw an edge that goes down
              DoEdgeTo (lS, lB, rgtN, direct, false); // draw an edge that goes left
            }
            else
            {
              DoEdgeTo (lS, lB, rgtN, direct, false);
            }
          }
          else
          {
            DoEdgeTo (lS, lB, p, direct, false);
          }
          lp = p;
        }
      }
      else
      {
        // bottom to top and right to left edge
        for (int p = lftN; p <= rgtN; p++)
        {
          // totally identical as the first block that I explain with a figure in the header docs
          if (avoidDiag && p == lftN && getPoint(lftN).x[0] == getPoint(lp).x[0] + dd)
          {
            if (lftN > 0 && lftN - 1 >= lastChgtPt
                && getPoint(lftN - 1).x[0] == getPoint(lp).x[0])
            {
              DoEdgeTo (lS, lB, lftN - 1, direct, false);
              DoEdgeTo (lS, lB, lftN, direct, false);
            }
            else
            {
              DoEdgeTo (lS, lB, lftN, direct, false);
            }
          }
          else
          {
            DoEdgeTo (lS, lB, p, direct, false);
          }
          lp = p;
        }
      }
    }
    lS->swsData[lB].curPoint = lp;
  }
  // see how doneTo is being set to lastPointNo - 1? See the figure in the header docs of this function and you'll see
  // what lastPointNo is, subtract one and you get the right most point that's just above lastPointNo. This marks that
  // this edge has been processed til that y level.
  lS->swsData[lB].doneTo = lastPointNo - 1;
}

void
Shape::DoEdgeTo (Shape * iS, int iB, int iTo, bool direct, bool sens)
{
  int lp = iS->swsData[iB].curPoint;
  int ne = -1;
  if (sens)
  {
    if (direct)
      ne = AddEdge (lp, iTo);
    else
      ne = AddEdge (iTo, lp);
  }
  else
  {
    if (direct)
      ne = AddEdge (iTo, lp);
    else
      ne = AddEdge (lp, iTo);
  }
  if (ne >= 0 && _has_back_data)
  {
    ebData[ne].pathID = iS->ebData[iB].pathID;
    ebData[ne].pieceID = iS->ebData[iB].pieceID;
    if (iS->eData[iB].length < 0.00001)
    {
      ebData[ne].tSt = ebData[ne].tEn = iS->ebData[iB].tSt;
    }
    else
    {
      double bdl = iS->eData[iB].ilength;
      Geom::Point bpx = iS->pData[iS->getEdge(iB).st].rx;
      Geom::Point bdx = iS->eData[iB].rdx;
      Geom::Point psx = getPoint(getEdge(ne).st).x;
      Geom::Point pex = getPoint(getEdge(ne).en).x;
      Geom::Point psbx=psx-bpx;
      Geom::Point pebx=pex-bpx;
      double pst = dot(psbx,bdx) * bdl;
      double pet = dot(pebx,bdx) * bdl;
      pst = iS->ebData[iB].tSt * (1 - pst) + iS->ebData[iB].tEn * pst;
      pet = iS->ebData[iB].tSt * (1 - pet) + iS->ebData[iB].tEn * pet;
      ebData[ne].tEn = pet;
      ebData[ne].tSt = pst;
    }
  }
  iS->swsData[iB].curPoint = iTo;
  if (ne >= 0)
  {
    int cp = iS->swsData[iB].firstLinkedPoint;
    swsData[ne].firstLinkedPoint = iS->swsData[iB].firstLinkedPoint;
    while (cp >= 0)
    {
      pData[cp].askForWindingB = ne;
      cp = pData[cp].nextLinkedPoint;
    }
    iS->swsData[iB].firstLinkedPoint = -1;
  }
}

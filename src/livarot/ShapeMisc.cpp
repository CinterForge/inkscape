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

#include "livarot/Shape.h"
#include "livarot/Path.h"
#include "livarot/path-description.h"
#include <glib.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <2geom/point.h>
#include <2geom/affine.h>

/*
 * polygon offset and polyline to path reassembling (when using back data)
 */

// until i find something better
#define MiscNormalize(v) {\
  double _l=sqrt(dot(v,v)); \
    if ( _l < 0.0000001 ) { \
      v[0]=v[1]=0; \
    } else { \
      v/=_l; \
    }\
}

// extracting the contour of an uncrossed polygon: a mere depth first search
// more precisely that's extracting an eulerian path from a graph, but here we want to split
// the polygon into contours and avoid holes. so we take a "next counter-clockwise edge first" approach
// (make a checkboard and extract its contours to see the difference)
void
Shape::ConvertToForme (Path * dest)
{
  // this function is quite similar to Shape::GetWindings so please check it out
  // first to learn the overall technique and I'll make sure to comment the parts
  // that are different

  if (numberOfPoints() <= 1 || numberOfEdges() <= 1)
    return;
  
  // prepare
  dest->Reset ();
  
  MakePointData (true);
  MakeEdgeData (true);
  MakeSweepDestData (true);
  
  for (int i = 0; i < numberOfPoints(); i++)
  {
    pData[i].rx[0] = Round (getPoint(i).x[0]);
    pData[i].rx[1] = Round (getPoint(i).x[1]);
  }
  for (int i = 0; i < numberOfEdges(); i++)
  {
    eData[i].rdx = pData[getEdge(i).en].rx - pData[getEdge(i).st].rx;
  }
  
  // sort edge clockwise, with the closest after midnight being first in the doubly-linked list
  // that's vital to the algorithm...
  SortEdges ();
  
  // depth-first search implies: we make a stack of edges traversed.
  // precParc: previous in the stack
  // suivParc: next in the stack
  for (int i = 0; i < numberOfEdges(); i++)
  {
    swdData[i].misc = 0;
    swdData[i].precParc = swdData[i].suivParc = -1;
  }
  
  int searchInd = 0;
  
  int lastPtUsed = 0;
  do
  {
    // first get a starting point, and a starting edge
    // -> take the upper left point, and take its first edge
    // points traversed have swdData[].misc != 0, so it's easy
    int startBord = -1;
    {
      int fi = 0;
      for (fi = lastPtUsed; fi < numberOfPoints(); fi++)
      {
        if (getPoint(fi).incidentEdge[FIRST] >= 0 && swdData[getPoint(fi).incidentEdge[FIRST]].misc == 0)
          break;
      }
      lastPtUsed = fi + 1;
      if (fi < numberOfPoints())
      {
        int bestB = getPoint(fi).incidentEdge[FIRST];
        // we get the edge that starts at this point since we wanna follow the direction of the edges
        while (bestB >= 0 && getEdge(bestB).st != fi)
          bestB = NextAt (fi, bestB);
        if (bestB >= 0)
	      {
          startBord = bestB;
          dest->MoveTo (getPoint(getEdge(startBord).en).x);
	      }
      }
    }
    // and walk the graph, doing contours when needed
    if (startBord >= 0)
    {
      // parcours en profondeur pour mettre les leF et riF a leurs valeurs
      swdData[startBord].misc = 1;
      //                      printf("part de %d\n",startBord);
      int curBord = startBord;
      bool back = false; // a variable that if true, means we are back tracking
      swdData[curBord].precParc = -1;
      swdData[curBord].suivParc = -1;
      do
	    {
	      int cPt = getEdge(curBord).en; // get the end point, we want to follow the direction of the edge
	      int nb = curBord;
        //                              printf("de curBord= %d au point %i  -> ",curBord,cPt);
        // get next edge
	      do
        {
          int nnb = CycleNextAt (cPt, nb); // get the next (clockwise) edge (I don't see why the comment at the top says anti clockwise edge first)
          if (nnb == nb)
          {
            // cul-de-sac
            nb = -1;
            break;
          }
          nb = nnb;
          if (nb < 0 || nb == curBord) // if we got to the same edge, we break, cuz now we need to back track
            break;
        }
          while (swdData[nb].misc != 0 || getEdge(nb).st != cPt); // keep finding a new edge until we find an edge that we haven't seen or an edge
        // that starts at the point
        
        if (nb < 0 || nb == curBord)
        {
          // if we are here, means there was no new edge, so we start back tracking
          // no next edge: end of this contour, we get back
          if (back == false)
            dest->Close ();
          back = true;
          // retour en arriere
          curBord = swdData[curBord].precParc; // set previous edge to current edge and back is true to indicate we are backtracking
          //                                      printf("retour vers %d\n",curBord);
          if (curBord < 0) // break if no edge exists before this one
            break;
        }
	      else
        {
          // did we get this new edge after we started backtracking? if yes,  we need a moveTo
          // new edge, maybe for a new contour
          if (back)
          {
            // we were backtracking, so if we have a new edge, that means we're creating a new contour
            dest->MoveTo (getPoint(cPt).x);
            back = false; // we are no longer backtracking, we will follow this new edge now
          }
          swdData[nb].misc = 1;
          swdData[nb].ind = searchInd++;
          swdData[nb].precParc = curBord;
          swdData[curBord].suivParc = nb;
          curBord = nb;
          //                                      printf("suite %d\n",curBord);
          {
            // add that edge
            dest->LineTo (getPoint(getEdge(nb).en).x); // add a line segment
          }
        }
	    }
      while (true /*swdData[curBord].precParc >= 0 */ );
      // fin du cas non-oriente
    }
  }
  while (lastPtUsed < numberOfPoints());
  
  MakePointData (false);
  MakeEdgeData (false);
  MakeSweepDestData (false);
}

// same as before, but each time we have a contour, try to reassemble the segments on it to make chunks of
// the original(s) path(s)
// originals are in the orig array, whose size is nbP
void Shape::ConvertToForme(Path *dest, int nbP, Path *const *orig, bool never_split)
{
  // the function is similar to the other version of ConvertToForme, I'm adding comments
  // where there are significant differences to explain
  if (numberOfPoints() <= 1 || numberOfEdges() <= 1)
    return;
//  if (Eulerian (true) == false)
//    return;
  
  if (_has_back_data == false)
  {
    ConvertToForme (dest);
    return;
  }
  
  dest->Reset ();
  
  MakePointData (true);
  MakeEdgeData (true);
  MakeSweepDestData (true);
  
  for (int i = 0; i < numberOfPoints(); i++)
  {
    pData[i].rx[0] = Round (getPoint(i).x[0]);
    pData[i].rx[1] = Round (getPoint(i).x[1]);
  }
  for (int i = 0; i < numberOfEdges(); i++)
  {
    eData[i].rdx = pData[getEdge(i).en].rx - pData[getEdge(i).st].rx;
  }
  
  SortEdges ();
  
  for (int i = 0; i < numberOfEdges(); i++)
  {
    swdData[i].misc = 0;
    swdData[i].precParc = swdData[i].suivParc = -1;
  }
  
  int searchInd = 0;
  
  int lastPtUsed = 0;
  do
  {
    int startBord = -1;
    {
      int fi = 0;
      for (fi = lastPtUsed; fi < numberOfPoints(); fi++)
      {
        if (getPoint(fi).incidentEdge[FIRST] >= 0 && swdData[getPoint(fi).incidentEdge[FIRST]].misc == 0)
          break;
      }
      lastPtUsed = fi + 1;
      if (fi < numberOfPoints())
      {
        int bestB = getPoint(fi).incidentEdge[FIRST];
        while (bestB >= 0 && getEdge(bestB).st != fi)
          bestB = NextAt (fi, bestB);
        if (bestB >= 0)
	      {
          startBord = bestB; // no moveTo here unlike the other ConvertToForme because we want AddContour to do all the contour extraction stuff
	      }
      }
    }
    if (startBord >= 0)
    {
      // parcours en profondeur pour mettre les leF et riF a leurs valeurs
      swdData[startBord].misc = 1;
      //printf("part de %d\n",startBord);
      int curBord = startBord;
      bool back = false;
      swdData[curBord].precParc = -1;
      swdData[curBord].suivParc = -1;
      int curStartPt=getEdge(curBord).st; // we record the start point of the current edge (actually the start edge at the moment)
      do
	    {
	      int cPt = getEdge(curBord).en;
	      int nb = curBord;
        //printf("de curBord= %d au point %i  -> ",curBord,cPt);
	      do
        {
          int nnb = CycleNextAt (cPt, nb);
          if (nnb == nb)
          {
            // cul-de-sac
            nb = -1;
            break;
          }
          nb = nnb;
          if (nb < 0 || nb == curBord) // if we get the same edge, we break
            break;
        }
          while (swdData[nb].misc != 0 || getEdge(nb).st != cPt);
        
	      if (nb < 0 || nb == curBord)
        { // we are backtracking
          if (back == false) // if we weren't backtracking
          {
            if (curBord == startBord || curBord < 0) // is the current edge the one we started on?
            {
              // probleme -> on vire le moveto
              //                                                      dest->descr_nb--;
            }
            else // if not, we now have a contour to add
            {
              swdData[curBord].suivParc = -1;
              AddContour(dest, nbP, orig, startBord, never_split);
            }
            //                                              dest->Close();
          }
          back = true; // we are backtracking now
          // retour en arriere
          curBord = swdData[curBord].precParc; // get the previous edge
          //printf("retour vers %d\n",curBord);
          if (curBord < 0) // if no previous edge, we break
            break;
        }
	      else
        {
          if (back)
          { // if we are backtracking, now we go forward (since we have found a new edge to explore)
            back = false;
            startBord = nb;
            curStartPt=getEdge(nb).st; // reset curStartPt to this as a new contour is starting here
          } else {
            if ( getEdge(curBord).en == curStartPt ) { // if we are going forward and the endpoint of this edge is the actual start point
              //printf("contour %i ",curStartPt);
              swdData[curBord].suivParc = -1; //  why tho? seems useless since we set it right after this block ends
              AddContour(dest, nbP, orig, startBord, never_split); // add the contour
              startBord=nb; // set startBord to this edge
            }
          }
          swdData[nb].misc = 1;
          swdData[nb].ind = searchInd++;
          swdData[nb].precParc = curBord;
          swdData[curBord].suivParc = nb;
          curBord = nb;
          //printf("suite %d\n",curBord);
        }
	    }
      while (true /*swdData[curBord].precParc >= 0 */ );
      // fin du cas non-oriente
    }
  }
  while (lastPtUsed < numberOfPoints());
  
  MakePointData (false);
  MakeEdgeData (false);
  MakeSweepDestData (false);
}

void Shape::ConvertToFormeNested(Path *dest, int nbP, Path *const *orig, int &nbNest, int *&nesting, int *&contStart, bool never_split)
{
  nesting=nullptr;
  contStart=nullptr;
  nbNest=0;

  if (numberOfPoints() <= 1 || numberOfEdges() <= 1)
    return;
  
  if (_has_back_data == false)
  {
    ConvertToForme (dest);
    return;
  }
  
  dest->Reset ();
  
  MakeEdgeData (true);
  MakeSweepDestData (true);
  
  for (int i = 0; i < numberOfPoints(); i++)
  {
    pData[i].rx[0] = Round (getPoint(i).x[0]);
    pData[i].rx[1] = Round (getPoint(i).x[1]);
  }
  for (int i = 0; i < numberOfEdges(); i++)
  {
    eData[i].rdx = pData[getEdge(i).en].rx - pData[getEdge(i).st].rx;
  }
  
  SortEdges ();
  
  for (int i = 0; i < numberOfEdges(); i++)
  {
    swdData[i].misc = 0;
    swdData[i].precParc = swdData[i].suivParc = -1;
  }
  
  int searchInd = 0;
  
  int lastPtUsed = 0;
  int parentContour=-1;
  do
  {
    int childEdge = -1;
    bool foundChild = false;
    int startBord = -1;
    {
      int fi = 0;
      for (fi = lastPtUsed; fi < numberOfPoints(); fi++)
      {
        if (getPoint(fi).incidentEdge[FIRST] >= 0 && swdData[getPoint(fi).incidentEdge[FIRST]].misc == 0)
          break;
      }
      {
        if (pData.size()<= fi || fi == numberOfPoints()) {
            parentContour=-1;
        } else {
          int askTo = pData[fi].askForWindingB;
          if (askTo < 0 || askTo >= numberOfEdges() ) {
            parentContour=-1;
          } else {
            parentContour = swdData[askTo].misc;
            parentContour-=1; // to compensate for the discrepancy
            if (parentContour >= 0 && parentContour < nbNest && nesting[parentContour] != -1) {
              parentContour = nesting[parentContour];
            }
            childEdge = getPoint(fi).incidentEdge[FIRST];
          }
        }
      }
      lastPtUsed = fi + 1;
      if (fi < numberOfPoints())
      {
        int bestB = getPoint(fi).incidentEdge[FIRST];
        while (bestB >= 0 && getEdge(bestB).st != fi)
          bestB = NextAt (fi, bestB);
        if (bestB >= 0)
	      {
          startBord = bestB;
	      }
      }
    }
    if (startBord >= 0)
    {
      // parcours en profondeur pour mettre les leF et riF a leurs valeurs
      swdData[startBord].misc = 1 + nbNest;
      if (startBord == childEdge) {
          foundChild = true;
      }

      int curBord = startBord;
      bool back = false;
      swdData[curBord].precParc = -1;
      swdData[curBord].suivParc = -1;
      int curStartPt=getEdge(curBord).st;
      do
	    {
	      int cPt = getEdge(curBord).en;
	      int nb = curBord;

	      do
        {
          int nnb = CycleNextAt (cPt, nb);
          if (nnb == nb)
          {
            // cul-de-sac
            nb = -1;
            break;
          }
          nb = nnb;
          if (nb < 0 || nb == curBord)
            break;
        }
          while (swdData[nb].misc != 0 || getEdge(nb).st != cPt);
        
	      if (nb < 0 || nb == curBord)
        {
          if (back == false)
          {
            if (curBord == startBord || curBord < 0)
            {
              // probleme -> on vire le moveto
            }
            else
            {
              nesting=(int*)g_realloc(nesting,(nbNest+1)*sizeof(int));
              contStart=(int*)g_realloc(contStart,(nbNest+1)*sizeof(int));
              contStart[nbNest]=dest->descr_cmd.size();
              if (foundChild) {
                nesting[nbNest++]=parentContour;
                foundChild = false;
              } else {
                nesting[nbNest++]=-1; // contient des bouts de coupure -> a part
              }
              swdData[curBord].suivParc = -1;
              AddContour(dest, nbP, orig, startBord, never_split);
            }
          }
          back = true;
          // retour en arriere
          curBord = swdData[curBord].precParc;

          if (curBord < 0)
            break;
        }
	      else
        {
          if (back)
          {
            back = false;
            startBord = nb;
            curStartPt=getEdge(nb).st;
          } else {
            if ( getEdge(curBord).en == curStartPt ) {
              nesting=(int*)g_realloc(nesting,(nbNest+1)*sizeof(int));
              contStart=(int*)g_realloc(contStart,(nbNest+1)*sizeof(int));
              contStart[nbNest]=dest->descr_cmd.size();
              if (foundChild) {
                nesting[nbNest++]=parentContour;
                foundChild = false;
              } else {
                nesting[nbNest++]=-1; // contient des bouts de coupure -> a part
              }
              swdData[curBord].suivParc = -1;
              AddContour(dest, nbP, orig, startBord, never_split);
              startBord=nb;
            }
          }
          swdData[nb].misc = 1 + nbNest;
          swdData[nb].ind = searchInd++;
          swdData[nb].precParc = curBord;
          swdData[curBord].suivParc = nb;
          curBord = nb;
          if (nb == childEdge) {
              foundChild = true;
          }
        }
	    }
      while (true);
      // fin du cas non-oriente
    }
  }
  while (lastPtUsed < numberOfPoints());
  
  MakePointData (false);
  MakeEdgeData (false);
  MakeSweepDestData (false);
}


int
Shape::MakeTweak (int mode, Shape *a, double power, JoinType join, double miter, bool do_profile, Geom::Point c, Geom::Point vector, double radius, Geom::Affine *i2doc)
{
  Reset (0, 0);
  MakeBackData(a->_has_back_data);

	bool done_something = false;

  if (power == 0)
  {
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
      if (_has_back_data)
        ebData.resize(maxAr);
    }
    return 0;
  }
  if (a->numberOfPoints() <= 1 || a->numberOfEdges() <= 1 || a->type != shape_polygon)
    return shape_input_err;
  
  a->SortEdges ();
  
  a->MakeSweepDestData (true);
  a->MakeSweepSrcData (true);
  
  for (int i = 0; i < a->numberOfEdges(); i++)
  {
    int stB = -1, enB = -1;
    if (power <= 0 || mode == tweak_mode_push || mode == tweak_mode_repel || mode == tweak_mode_roughen)  {
      stB = a->CyclePrevAt (a->getEdge(i).st, i);
      enB = a->CycleNextAt (a->getEdge(i).en, i);
    } else {
      stB = a->CycleNextAt (a->getEdge(i).st, i);
      enB = a->CyclePrevAt (a->getEdge(i).en, i);
    }
    
    Geom::Point stD = a->getEdge(stB).dx;
    Geom::Point seD = a->getEdge(i).dx;
    Geom::Point enD = a->getEdge(enB).dx;

    double stL = sqrt (dot(stD,stD));
    double seL = sqrt (dot(seD,seD));
    //double enL = sqrt (dot(enD,enD));
    MiscNormalize (stD);
    MiscNormalize (enD);
    MiscNormalize (seD);
    
    Geom::Point ptP;
    int stNo, enNo;
    ptP = a->getPoint(a->getEdge(i).st).x;

  	Geom::Point to_center = ptP * (*i2doc) - c;
  	Geom::Point to_center_normalized = (1/Geom::L2(to_center)) * to_center;

		double this_power;
		if (do_profile && i2doc) {
			double alpha = 1;
			double x;
  		if (mode == tweak_mode_repel) {
				x = (Geom::L2(to_center)/radius);
			} else {
				x = (Geom::L2(ptP * (*i2doc) - c)/radius);
			}
			if (x > 1) {
				this_power = 0;
			} else if (x <= 0) {
    		if (mode == tweak_mode_repel) {
					this_power = 0;
				} else {
					this_power = power;
				}
			} else {
				this_power = power * (0.5 * cos (M_PI * (pow(x, alpha))) + 0.5);
			}
		} else {
  		if (mode == tweak_mode_repel) {
				this_power = 0;
			} else {
				this_power = power;
			}
		}

		if (this_power != 0)
			done_something = true;

		double scaler = 1 / (*i2doc).descrim();

		Geom::Point this_vec(0,0);
    if (mode == tweak_mode_push) {
			Geom::Affine tovec (*i2doc);
			tovec[4] = tovec[5] = 0;
			tovec = tovec.inverse();
			this_vec = this_power * (vector * tovec) ;
		} else if (mode == tweak_mode_repel) {
			this_vec = this_power * scaler * to_center_normalized;
		} else if (mode == tweak_mode_roughen) {
  		double angle = g_random_double_range(0, 2*M_PI);
	  	this_vec = g_random_double_range(0, 1) * this_power * scaler * Geom::Point(sin(angle), cos(angle));
		}

    int   usePathID=-1;
    int   usePieceID=0;
    double useT=0.0;
    if ( a->_has_back_data ) {
      if ( a->ebData[i].pathID >= 0 && a->ebData[stB].pathID == a->ebData[i].pathID && a->ebData[stB].pieceID == a->ebData[i].pieceID
           && a->ebData[stB].tEn == a->ebData[i].tSt ) {
        usePathID=a->ebData[i].pathID;
        usePieceID=a->ebData[i].pieceID;
        useT=a->ebData[i].tSt;
      } else {
        usePathID=a->ebData[i].pathID;
        usePieceID=0;
        useT=0;
      }
    }

		if (mode == tweak_mode_push || mode == tweak_mode_repel || mode == tweak_mode_roughen) {
			Path::DoLeftJoin (this, 0, join, ptP+this_vec, stD+this_vec, seD+this_vec, miter, stL, seL,
												stNo, enNo,usePathID,usePieceID,useT);
			a->swsData[i].stPt = enNo;
			a->swsData[stB].enPt = stNo;
		} else {
			if (power > 0) {
				Path::DoRightJoin (this, this_power * scaler, join, ptP, stD, seD, miter, stL, seL,
													 stNo, enNo,usePathID,usePieceID,useT);
				a->swsData[i].stPt = enNo;
				a->swsData[stB].enPt = stNo;
			} else {
				Path::DoLeftJoin (this, -this_power * scaler, join, ptP, stD, seD, miter, stL, seL,
													stNo, enNo,usePathID,usePieceID,useT);
				a->swsData[i].stPt = enNo;
				a->swsData[stB].enPt = stNo;
			}
		}
  }

  if (power < 0 || mode == tweak_mode_push || mode == tweak_mode_repel || mode == tweak_mode_roughen)
  {
    for (int i = 0; i < numberOfEdges(); i++)
      Inverse (i);
  }

  if ( _has_back_data ) {
    for (int i = 0; i < a->numberOfEdges(); i++)
    {
      int nEd=AddEdge (a->swsData[i].stPt, a->swsData[i].enPt);
      ebData[nEd]=a->ebData[i];
    }
  } else {
    for (int i = 0; i < a->numberOfEdges(); i++)
    {
      AddEdge (a->swsData[i].stPt, a->swsData[i].enPt);
    }
  }

  a->MakeSweepSrcData (false);
  a->MakeSweepDestData (false);
  
  return (done_something? 0 : shape_nothing_to_do);
}


// offsets
// take each edge, offset it, and make joins with previous at edge start and next at edge end (previous and
// next being with respect to the clockwise order)
// you gotta be very careful with the join, as anything but the right one will fuck everything up
// see PathStroke.cpp for the "right" joins
int
Shape::MakeOffset (Shape * a, double dec, JoinType join, double miter, bool do_profile, double cx, double cy, double radius, Geom::Affine *i2doc)
{
  Reset (0, 0);
  MakeBackData(a->_has_back_data);

	bool done_something = false;
  
  if (dec == 0)
  {
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
      if (_has_back_data)
        ebData.resize(maxAr);
    }
    return 0;
  }
  if (a->numberOfPoints() <= 1 || a->numberOfEdges() <= 1 || a->type != shape_polygon)
    return shape_input_err;
  
  a->SortEdges ();
  
  a->MakeSweepDestData (true);
  a->MakeSweepSrcData (true);
  
  for (int i = 0; i < a->numberOfEdges(); i++)
  {
    //              int    stP=a->swsData[i].stPt/*,enP=a->swsData[i].enPt*/;
    int stB = -1, enB = -1;
    if (dec > 0)
    {
      stB = a->CycleNextAt (a->getEdge(i).st, i);
      enB = a->CyclePrevAt (a->getEdge(i).en, i);
    }
    else
    {
      stB = a->CyclePrevAt (a->getEdge(i).st, i);
      enB = a->CycleNextAt (a->getEdge(i).en, i);
    }
    
    Geom::Point stD = a->getEdge(stB).dx;
    Geom::Point seD = a->getEdge(i).dx;
    Geom::Point enD = a->getEdge(enB).dx;

    double stL = sqrt (dot(stD,stD));
    double seL = sqrt (dot(seD,seD));
    //double enL = sqrt (dot(enD,enD));
    MiscNormalize (stD);
    MiscNormalize (enD);
    MiscNormalize (seD);
    
    Geom::Point ptP;
    int stNo, enNo;
    ptP = a->getPoint(a->getEdge(i).st).x;

		double this_dec;
		if (do_profile && i2doc) {
			double alpha = 1;
			double x = (Geom::L2(ptP * (*i2doc) - Geom::Point(cx,cy))/radius);
			if (x > 1) {
				this_dec = 0;
			} else if (x <= 0) {
				this_dec = dec;
			} else {
				this_dec = dec * (0.5 * cos (M_PI * (pow(x, alpha))) + 0.5);
			}
		} else {
			this_dec = dec;
		}

		if (this_dec != 0)
			done_something = true;

    int   usePathID=-1;
    int   usePieceID=0;
    double useT=0.0;
    if ( a->_has_back_data ) {
      if ( a->ebData[i].pathID >= 0 && a->ebData[stB].pathID == a->ebData[i].pathID && a->ebData[stB].pieceID == a->ebData[i].pieceID
           && a->ebData[stB].tEn == a->ebData[i].tSt ) {
        usePathID=a->ebData[i].pathID;
        usePieceID=a->ebData[i].pieceID;
        useT=a->ebData[i].tSt;
      } else {
        usePathID=a->ebData[i].pathID;
        usePieceID=0;
        useT=0;
      }
    }
    if (dec > 0)
    {
      Path::DoRightJoin (this, this_dec, join, ptP, stD, seD, miter, stL, seL,
                         stNo, enNo,usePathID,usePieceID,useT);
      a->swsData[i].stPt = enNo;
      a->swsData[stB].enPt = stNo;
    }
    else
    {
      Path::DoLeftJoin (this, -this_dec, join, ptP, stD, seD, miter, stL, seL,
                        stNo, enNo,usePathID,usePieceID,useT);
      a->swsData[i].stPt = enNo;
      a->swsData[stB].enPt = stNo;
    }
  }

  if (dec < 0)
  {
    for (int i = 0; i < numberOfEdges(); i++)
      Inverse (i);
  }

  if ( _has_back_data ) {
    for (int i = 0; i < a->numberOfEdges(); i++)
    {
      int nEd=AddEdge (a->swsData[i].stPt, a->swsData[i].enPt);
      ebData[nEd]=a->ebData[i];
    }
  } else {
    for (int i = 0; i < a->numberOfEdges(); i++)
    {
      AddEdge (a->swsData[i].stPt, a->swsData[i].enPt);
    }
  }

  a->MakeSweepSrcData (false);
  a->MakeSweepDestData (false);
  
  return (done_something? 0 : shape_nothing_to_do);
}

// we found a contour, now reassemble the edges on it, instead of dumping them in the Path "dest" as a
// polyline. since it was a DFS, the precParc and suivParc make a nice doubly-linked list of the edges in
// the contour. the first edge of the contour is start_edge.
void Shape::AddContour(Path *dest, int num_orig, Path *const *orig, int start_edge, bool never_split)
{
    int edge = start_edge;

    // Move to the starting point.
    dest->MoveTo(getPoint(getEdge(edge).st).x);

    while (true) {
        // Get the piece and path id.
        int nPiece = ebData[edge].pieceID;
        int nPath = ebData[edge].pathID;

        // If the path id is invalid in any way, just add the edge as a line segment.
        if (nPath < 0 || nPath >= num_orig || !orig[nPath]) {
            dest->LineTo(getPoint(getEdge(edge).en).x);
            edge = swdData[edge].suivParc;
            continue;
        }

        // Get pointer to the path where this edge came from.
        auto from = orig[nPath];

        // If the piece id is invalid in any way, again just add the edge as a line segment.
        if (nPiece < 0 || nPiece >= from->descr_cmd.size()) {
            dest->LineTo(getPoint(getEdge(edge).en).x);
            edge = swdData[edge].suivParc;
            continue;
        }

        // Handle the path command. This consumes multiple edges, and sets edge to the next edge to process.
        switch (from->descr_cmd[nPiece]->getType()) {
            case descr_lineto:
                edge = ReFormeLineTo(edge, dest, never_split);
                break;
            case descr_arcto:
                edge = ReFormeArcTo(edge, dest, from, never_split);
                break;
            case descr_cubicto:
                edge = ReFormeCubicTo(edge, dest, from, never_split);
                break;
            default:
                // Shouldn't happen, but if it does, just add a line segment.
                dest->LineTo(getPoint(getEdge(edge).en).x);
                edge = swdData[edge].suivParc;
                break;
        }

        // No more edges - exit.
        if (edge < 0) {
            break;
        }

        // Insert forced points.
        // Although forced points make no difference to the dumped SVG path, they do have some internal use.
        // For example, some functions like ConvertForcedToMoveTo() pay attention to them.
        // It's not clear exactly when or why these points are inserted, or when oldDegree and totalDegree() differ
        if (!never_split) {
            if (getPoint(getEdge(edge).st).totalDegree() > 2) {
                dest->ForcePoint();
            } else if (getPoint(getEdge(edge).st).oldDegree > 2 && getPoint(getEdge(edge).st).totalDegree() == 2)  {
                if (_has_back_data) {
                    int prevEdge = getPoint(getEdge(edge).st).incidentEdge[FIRST];
                    int nextEdge = getPoint(getEdge(edge).st).incidentEdge[LAST];
                    if (getEdge(prevEdge).en != getEdge(edge).st) {
                        std::swap(prevEdge, nextEdge);
                    }
                    if (ebData[prevEdge].pieceID == ebData[nextEdge].pieceID && ebData[prevEdge].pathID == ebData[nextEdge].pathID) {
                        if (std::abs(ebData[prevEdge].tEn - ebData[nextEdge].tSt) < 0.05) {
                        } else {
                            dest->ForcePoint();
                        }
                    } else {
                        dest->ForcePoint();
                    }
                } else {
                    dest->ForcePoint();
                }
            }
        }
    }

    dest->Close();
}

int Shape::ReFormeLineTo(int bord, Path *dest, bool never_split)
{
  int nPiece = ebData[bord].pieceID;
  int nPath = ebData[bord].pathID;
  double te = ebData[bord].tEn;
  Geom::Point nx = getPoint(getEdge(bord).en).x;
  bord = swdData[bord].suivParc;
  while (bord >= 0)
  {
    if (!never_split && (getPoint(getEdge(bord).st).totalDegree() > 2 || getPoint(getEdge(bord).st).oldDegree > 2))
    {
      break;
    }
    if (ebData[bord].pieceID == nPiece && ebData[bord].pathID == nPath)
    {
      if (fabs (te - ebData[bord].tSt) > 0.0001)
        break;
      nx = getPoint(getEdge(bord).en).x;
      te = ebData[bord].tEn;
    }
    else
    {
      break;
    }
    bord = swdData[bord].suivParc;
  }
  {
    dest->LineTo (nx);
  }
  return bord;
}

int Shape::ReFormeArcTo(int bord, Path *dest, Path *from, bool never_split)
{
  int nPiece = ebData[bord].pieceID;
  int nPath = ebData[bord].pathID;
  double ts = ebData[bord].tSt, te = ebData[bord].tEn;
  Geom::Point nx = getPoint(getEdge(bord).en).x;
  bord = swdData[bord].suivParc;
  while (bord >= 0)
  {
    if (!never_split && (getPoint(getEdge(bord).st).totalDegree() > 2 || getPoint(getEdge(bord).st).oldDegree > 2))
    {
      break;
    }
    if (ebData[bord].pieceID == nPiece && ebData[bord].pathID == nPath)
    {
      if (fabs (te - ebData[bord].tSt) > 0.0001)
	    {
	      break;
	    }
      nx = getPoint(getEdge(bord).en).x;
      te = ebData[bord].tEn;
    }
    else
    {
      break;
    }
    bord = swdData[bord].suivParc;
  }

  double sang, eang;
  PathDescrArcTo* nData = dynamic_cast<PathDescrArcTo *>(from->descr_cmd[nPiece]);
  bool nLarge = nData->large;
  bool nClockwise = nData->clockwise;
  Path::ArcAngles (from->PrevPoint (nPiece - 1), nData->p,nData->rx,nData->ry,nData->angle*M_PI/180.0, nLarge, nClockwise,  sang, eang);
  if (nClockwise)
  {
    if (sang < eang)
      sang += 2 * M_PI;
  }
  else
  {
    if (sang > eang)
      sang -= 2 * M_PI;
  }
  double delta = eang - sang;
  double ndelta = delta * (te - ts);
  if (ts > te)
    nClockwise = !nClockwise;
  if (ndelta < 0)
    ndelta = -ndelta;
  if (ndelta > M_PI)
    nLarge = true;
  else
    nLarge = false;

  dest->ArcTo (nx, nData->rx,nData->ry,nData->angle, nLarge, nClockwise);

  return bord;
}

int Shape::ReFormeCubicTo(int bord, Path *dest, Path *from, bool never_split)
{
  int nPiece = ebData[bord].pieceID;
  int nPath = ebData[bord].pathID;
  double ts = ebData[bord].tSt, te = ebData[bord].tEn;
  Geom::Point nx = getPoint(getEdge(bord).en).x;
  bord = swdData[bord].suivParc;
  while (bord >= 0)
  {
    if (!never_split && (getPoint(getEdge(bord).st).totalDegree() > 2 || getPoint(getEdge(bord).st).oldDegree > 2))
    {
      break;
    }
    if (ebData[bord].pieceID == nPiece && ebData[bord].pathID == nPath)
    {
      if (fabs (te - ebData[bord].tSt) > 0.0001)
	    {
	      break;
	    }
      nx = getPoint(getEdge(bord).en).x;
      te = ebData[bord].tEn;
    }
    else
    {
      break;
    }
    bord = swdData[bord].suivParc;
  }
  Geom::Point prevx = from->PrevPoint (nPiece - 1);
  
  Geom::Point sDx, eDx;
  {
    PathDescrCubicTo *nData = dynamic_cast<PathDescrCubicTo *>(from->descr_cmd[nPiece]);
    Path::CubicTangent (ts, sDx, prevx,nData->start,nData->p,nData->end);
    Path::CubicTangent (te, eDx, prevx,nData->start,nData->p,nData->end);
  }
  sDx *= (te - ts);
  eDx *= (te - ts);
  {
    dest->CubicTo (nx,sDx,eDx);
  }
  return bord;
}

#undef MiscNormalize

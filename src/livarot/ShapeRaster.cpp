// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
/*
 *  ShapeRaster.cpp
 *  nlivarot
 *
 *  Created by fred on Sat Jul 19 2003.
 *
 */

#include "Shape.h"

#include "livarot/float-line.h"
#include "livarot/sweep-event-queue.h"
#include "livarot/sweep-tree-list.h"
#include "livarot/sweep-tree.h"

/*
 * polygon rasterization: the sweepline algorithm in all its glory
 * nothing unusual in this implementation, so nothing special to say
 */

void Shape::BeginRaster(float &pos, int &curPt)
{
    if ( numberOfPoints() <= 1 || numberOfEdges() <= 1 ) {
        curPt = 0;
        pos = 0;
        return;
    }
    
    MakeRasterData(true);
    MakePointData(true);
    MakeEdgeData(true);

    if (sTree == nullptr) {
        sTree = new SweepTreeList(numberOfEdges());
    }
    if (sEvts == nullptr) {
        sEvts = new SweepEventQueue(numberOfEdges());
    }

    SortPoints();

    curPt = 0;
    pos = getPoint(0).x[1] - 1.0;

    for (int i = 0; i < numberOfPoints(); i++) {
        pData[i].pending = 0;
        pData[i].nextLinkedPoint = -1;
        pData[i].rx[0] = /*Round(*/getPoint(i).x[0]/*)*/;
        pData[i].rx[1] = /*Round(*/getPoint(i).x[1]/*)*/;
    }

    for (int i = 0;i < numberOfEdges(); i++) {
        swrData[i].misc = nullptr;
        eData[i].rdx=pData[getEdge(i).en].rx - pData[getEdge(i).st].rx;
    }
}


void Shape::EndRaster()
{
    delete sTree;
    sTree = nullptr;
    delete sEvts;
    sEvts = nullptr;
    
    MakePointData(false);
    MakeEdgeData(false);
    MakeRasterData(false);
}


// 2 versions of the Scan() series to move the scanline to a given position withou actually computing coverages
void Shape::Scan(float &pos, int &curP, float to, float step)
{
    if ( numberOfEdges() <= 1 ) {
        return;
    }

    if ( pos == to ) {
        return;
    }

    enum Direction {
        DOWNWARDS,
        UPWARDS
    };

    Direction const d = (pos < to) ? DOWNWARDS : UPWARDS;

    // points of the polygon are sorted top-down, so we take them in order, starting with the one at index curP,
    // until we reach the wanted position to.
    // don't forget to update curP and pos when we're done
    int curPt = curP;
    while ( ( d == DOWNWARDS && curPt < numberOfPoints() && getPoint(curPt).x[1]     <= to) ||
            ( d == UPWARDS   && curPt > 0                && getPoint(curPt - 1).x[1] >= to) )
    {
        int nPt = (d == DOWNWARDS) ? curPt++ : --curPt;
      
        // treat a new point: remove and add edges incident to it
        int nbUp;
        int nbDn;
        int upNo;
        int dnNo;
        _countUpDown(nPt, &nbUp, &nbDn, &upNo, &dnNo);

				if ( d == DOWNWARDS ) {
					if ( nbDn <= 0 ) {
            upNo = -1;
					}
					if ( upNo >= 0 && swrData[upNo].misc == nullptr ) {
            upNo = -1;
					}
				} else {
					if ( nbUp <= 0 ) {
            dnNo = -1;
					}
					if ( dnNo >= 0 && swrData[dnNo].misc == nullptr ) {
            dnNo = -1;
					}
				}
        
        if ( ( d == DOWNWARDS && nbUp > 0 ) || ( d == UPWARDS && nbDn > 0 ) ) {
            // first remove edges coming from above or below, as appropriate
            int cb = getPoint(nPt).incidentEdge[FIRST];
            while ( cb >= 0 && cb < numberOfEdges() ) {

                Shape::dg_arete const &e = getEdge(cb);
                if ( (d == DOWNWARDS && nPt == std::max(e.st, e.en)) ||
                     (d == UPWARDS   && nPt == std::min(e.st, e.en)) )
                {
                    if ( ( d == DOWNWARDS && cb != upNo ) || ( d == UPWARDS && cb != dnNo ) ) {
                        // we salvage the edge upNo to plug the edges we'll be addingat its place
                        // but the other edge don't have this chance
                        SweepTree *node = swrData[cb].misc;
                        if ( node ) {
                            swrData[cb].misc = nullptr;
                            node->Remove(*sTree, *sEvts, true);
                        }
                    }
                }
                cb = NextAt(nPt, cb);
            }
 				}
      
        // if there is one edge going down and one edge coming from above, we don't Insert() the new edge,
        // but replace the upNo edge by the new one (faster)
        SweepTree* insertionNode = nullptr;
        if ( dnNo >= 0 ) {
            if ( upNo >= 0 ) {
							int    rmNo=(d == DOWNWARDS) ? upNo:dnNo;
							int    neNo=(d == DOWNWARDS) ? dnNo:upNo;
							  SweepTree* node = swrData[rmNo].misc;
                swrData[rmNo].misc = nullptr;

                int const P = (d == DOWNWARDS) ? nPt : Other(nPt, neNo);
                node->ConvertTo(this, neNo, 1, P);
                
                swrData[neNo].misc = node;
                insertionNode = node;
                CreateEdge(neNo, to, step);
            } else {
							// always DOWNWARDS
                SweepTree* node = sTree->add(this, dnNo, 1, nPt, this);
                swrData[dnNo].misc = node;
                node->Insert(*sTree, *sEvts, this, nPt, true);
                insertionNode = node;
                CreateEdge(dnNo,to,step);
            }
        } else {
					if ( upNo >= 0 ) {
						// always UPWARDS
						SweepTree* node = sTree->add(this, upNo, 1, nPt, this);
						swrData[upNo].misc = node;
						node->Insert(*sTree, *sEvts, this, nPt, true);
						insertionNode = node;
						CreateEdge(upNo,to,step);
					}
				}
      
        // add the remaining edges
        if ( ( d == DOWNWARDS && nbDn > 1 ) || ( d == UPWARDS && nbUp > 1 ) ) {
            // si nbDn == 1 , alors dnNo a deja ete traite
            int cb = getPoint(nPt).incidentEdge[FIRST];
            while ( cb >= 0 && cb < numberOfEdges() ) {
                Shape::dg_arete const &e = getEdge(cb);
                if ( nPt == std::min(e.st, e.en) ) {
                    if ( cb != dnNo && cb != upNo ) {
                        SweepTree *node = sTree->add(this, cb, 1, nPt, this);
                        swrData[cb].misc = node;
                        node->InsertAt(*sTree, *sEvts, this, insertionNode, nPt, true);
                        CreateEdge(cb, to, step);
                    }
                }
                cb = NextAt(nPt,cb);
            }
        }
    }
        
    curP = curPt;
    if ( curPt > 0 ) {
        pos = getPoint(curPt - 1).x[1];
    } else {
        pos = to;
    }
    
    // the final touch: edges intersecting the sweepline must be update so that their intersection with
    // said sweepline is correct.
    pos = to;
    if ( sTree->racine ) {
        SweepTree* curS = static_cast<SweepTree*>(sTree->racine->Leftmost());
        while ( curS ) {
            int cb = curS->bord;
            AvanceEdge(cb, to, true, step);
            curS = static_cast<SweepTree*>(curS->elem[RIGHT]);
        }
    }
}

    





















    
    
    
    
    
    
    
    
    
    
    
    
    





    



// scan and compute coverage, FloatLigne version coverage of the line is bult in 2 parts: first a
// set of rectangles of height the height of the line (here: "step") one rectangle for each portion
// of the sweepline that is in the polygon at the beginning of the scan.  then a set ot trapezoids
// are added or removed to these rectangles, one trapezoid for each edge destroyed or edge crossing
// the entire line. think of it as a refinement of the coverage by rectangles

void Shape::Scan(float &pos, int &curP, float to, FloatLigne *line, bool exact, float step)
{
    if ( numberOfEdges() <= 1 ) {
        return;
    }
    
    if ( pos >= to ) {
        return;
    }
    
    // first step: the rectangles since we read the sweepline left to right, we know the
    // boundaries of the rectangles are appended in a list, hence the AppendBord(). we salvage
    // the guess value for the trapezoids the edges will induce
        
    if ( sTree->racine ) {
        SweepTree *curS = static_cast<SweepTree*>(sTree->racine->Leftmost());
        while ( curS ) {
            
            int lastGuess = -1;
            int cb = curS->bord;
            
            if ( swrData[cb].sens == false && curS->elem[LEFT] ) {
                
                int lb = (static_cast<SweepTree*>(curS->elem[LEFT]))->bord;
                
                lastGuess = line->AppendBord(swrData[lb].curX,
                                             to - swrData[lb].curY,
                                             swrData[cb].curX,
                                             to - swrData[cb].curY,0.0);
                
                swrData[lb].guess = lastGuess - 1;
                swrData[cb].guess = lastGuess;
            } else {
                int lb = curS->bord;
                swrData[lb].guess = -1;
            }
            
            curS=static_cast <SweepTree*> (curS->elem[RIGHT]);
        }
    }
    
    int curPt = curP;
    while ( curPt < numberOfPoints() && getPoint(curPt).x[1] <= to ) {
        
        int nPt = curPt++;

        // same thing as the usual Scan(), just with a hardcoded "indegree+outdegree=2" case, since
        // it's the most common one
        
        int nbUp;
        int nbDn;
        int upNo;
        int dnNo;
        if ( getPoint(nPt).totalDegree() == 2 ) {
            _countUpDownTotalDegree2(nPt, &nbUp, &nbDn, &upNo, &dnNo);
        } else {
            _countUpDown(nPt, &nbUp, &nbDn, &upNo, &dnNo);
        }
        
        if ( nbDn <= 0 ) {
            upNo = -1;
        }
        if ( upNo >= 0 && swrData[upNo].misc == nullptr ) {
            upNo = -1;
        }

        if ( nbUp > 1 || ( nbUp == 1 && upNo < 0 ) ) {
            int cb = getPoint(nPt).incidentEdge[FIRST];
            while ( cb >= 0 && cb < numberOfEdges() ) {
                Shape::dg_arete const &e = getEdge(cb);
                if ( nPt == std::max(e.st, e.en) ) {
                    if ( cb != upNo ) {
                        SweepTree* node = swrData[cb].misc;
                        if ( node ) {
                            _updateIntersection(cb, nPt);
                            // create trapezoid for the chunk of edge intersecting with the line
                            DestroyEdge(cb, to, line);
                            node->Remove(*sTree, *sEvts, true);
                        }
                    }
                }
                
                cb = NextAt(nPt,cb);
            }
        }

        // traitement du "upNo devient dnNo"
        SweepTree *insertionNode = nullptr;
        if ( dnNo >= 0 ) {
            if ( upNo >= 0 ) {
                SweepTree* node = swrData[upNo].misc;
                _updateIntersection(upNo, nPt);
                DestroyEdge(upNo, to, line);
                
                node->ConvertTo(this, dnNo, 1, nPt);
                
                swrData[dnNo].misc = node;
                insertionNode = node;
                CreateEdge(dnNo, to, step);
                swrData[dnNo].guess = swrData[upNo].guess;
            } else {
                SweepTree *node = sTree->add(this, dnNo, 1, nPt, this);
                swrData[dnNo].misc = node;
                node->Insert(*sTree, *sEvts, this, nPt, true);
                insertionNode = node;
                CreateEdge(dnNo, to, step);
            }
        }
        
        if ( nbDn > 1 ) { // si nbDn == 1 , alors dnNo a deja ete traite
            int cb = getPoint(nPt).incidentEdge[FIRST];
            while ( cb >= 0 && cb < numberOfEdges() ) {
                Shape::dg_arete const &e = getEdge(cb);
                if ( nPt == std::min(e.st, e.en) ) {
                    if ( cb != dnNo ) {
                        SweepTree *node = sTree->add(this, cb, 1, nPt, this);
                        swrData[cb].misc = node;
                        node->InsertAt(*sTree, *sEvts, this, insertionNode, nPt, true);
                        CreateEdge(cb, to, step);
                    }
                }
                cb = NextAt(nPt,cb);
            }
        }
    }
    
    curP = curPt;
    if ( curPt > 0 ) {
        pos = getPoint(curPt - 1).x[1];
    } else {
        pos = to;
    } 
    
    // update intersections with the sweepline, and add trapezoids for edges crossing the line
    pos = to;
    if ( sTree->racine ) {
        SweepTree* curS = static_cast<SweepTree*>(sTree->racine->Leftmost());
        while ( curS ) {
            int cb = curS->bord;
            AvanceEdge(cb, to, line, exact, step);
            curS = static_cast<SweepTree*>(curS->elem[RIGHT]);
        }
    }
}




















/*
 * operations de bases pour la rasterization
 *
 */
void Shape::CreateEdge(int no, float to, float step)
{
    int cPt;
    Geom::Point dir;
    if ( getEdge(no).st < getEdge(no).en ) {
        cPt = getEdge(no).st;
        swrData[no].sens = true;
        dir = getEdge(no).dx;
    } else {
        cPt = getEdge(no).en;
        swrData[no].sens = false;
        dir = -getEdge(no).dx;
    }

    swrData[no].lastX = swrData[no].curX = getPoint(cPt).x[0];
    swrData[no].lastY = swrData[no].curY = getPoint(cPt).x[1];
    
    if ( fabs(dir[1]) < 0.000001 ) {
        swrData[no].dxdy = 0;
    } else {
        swrData[no].dxdy = dir[0]/dir[1];
    }
    
    if ( fabs(dir[0]) < 0.000001 ) {
        swrData[no].dydx = 0;
    } else {
        swrData[no].dydx = dir[1]/dir[0];
    }
    
    swrData[no].calcX = swrData[no].curX + (to - step - swrData[no].curY) * swrData[no].dxdy;
    swrData[no].guess = -1;
}


void Shape::AvanceEdge(int no, float to, bool exact, float step)
{
    if ( exact ) {
        Geom::Point dir;
        Geom::Point stp;
        if ( swrData[no].sens ) {
            stp = getPoint(getEdge(no).st).x;
            dir = getEdge(no).dx;
        } else {
            stp = getPoint(getEdge(no).en).x;
            dir = -getEdge(no).dx;
        }
        
        if ( fabs(dir[1]) < 0.000001 ) {
            swrData[no].calcX = stp[0] + dir[0];
        } else {
            swrData[no].calcX = stp[0] + ((to - stp[1]) * dir[0]) / dir[1];
        }
    } else {
        swrData[no].calcX += step * swrData[no].dxdy;
    }
    
    swrData[no].lastX = swrData[no].curX;
    swrData[no].lastY = swrData[no].curY;
    swrData[no].curX = swrData[no].calcX;
    swrData[no].curY = to;
}

/*
 * specialisation par type de structure utilise
 */

void Shape::DestroyEdge(int no, float to, FloatLigne* line)
{
    if ( swrData[no].sens ) {

        if ( swrData[no].curX < swrData[no].lastX ) {

            swrData[no].guess = line->AddBordR(swrData[no].curX,
                                               to - swrData[no].curY,
                                               swrData[no].lastX,
                                               to - swrData[no].lastY,
                                               -swrData[no].dydx,
                                               swrData[no].guess);
            
        } else if ( swrData[no].curX > swrData[no].lastX ) {
            
            swrData[no].guess = line->AddBord(swrData[no].lastX,
                                              -(to - swrData[no].lastY),
                                              swrData[no].curX,
                                              -(to - swrData[no].curY),
                                              swrData[no].dydx,
                                              swrData[no].guess);
        }
        
    } else {
        
        if ( swrData[no].curX < swrData[no].lastX ) {

            swrData[no].guess = line->AddBordR(swrData[no].curX,
                                               -(to - swrData[no].curY),
                                               swrData[no].lastX,
                                               -(to - swrData[no].lastY),
                                               swrData[no].dydx,
                                               swrData[no].guess);
            
        } else if ( swrData[no].curX > swrData[no].lastX ) {
            
            swrData[no].guess = line->AddBord(swrData[no].lastX,
                                              to - swrData[no].lastY,
                                              swrData[no].curX,
                                              to - swrData[no].curY,
                                              -swrData[no].dydx,
                                              swrData[no].guess);
        }
    }
}



void Shape::AvanceEdge(int no, float to, FloatLigne *line, bool exact, float step)
{
    AvanceEdge(no,to,exact,step);

    if ( swrData[no].sens ) {
        
        if ( swrData[no].curX < swrData[no].lastX ) {
            
            swrData[no].guess = line->AddBordR(swrData[no].curX,
                                               to - swrData[no].curY,
                                               swrData[no].lastX,
                                               to - swrData[no].lastY,
                                               -swrData[no].dydx,
                                               swrData[no].guess);
            
        } else if ( swrData[no].curX > swrData[no].lastX ) {
            
            swrData[no].guess = line->AddBord(swrData[no].lastX,
                                              -(to - swrData[no].lastY),
                                              swrData[no].curX,
                                              -(to - swrData[no].curY),
                                              swrData[no].dydx,
                                              swrData[no].guess);
        }
        
    } else {

        if ( swrData[no].curX < swrData[no].lastX ) {

            swrData[no].guess = line->AddBordR(swrData[no].curX,
                                               -(to - swrData[no].curY),
                                               swrData[no].lastX,
                                               -(to - swrData[no].lastY),
                                               swrData[no].dydx,
                                               swrData[no].guess);
            
        } else if ( swrData[no].curX > swrData[no].lastX ) {
            
            swrData[no].guess = line->AddBord(swrData[no].lastX,
                                              to - swrData[no].lastY,
                                              swrData[no].curX,
                                              to - swrData[no].curY,
                                              -swrData[no].dydx,
                                              swrData[no].guess);
        }
    }
}




/**
 *    \param P point index.
 *    \param numberUp Filled in with the number of edges coming into P from above.
 *    \param numberDown Filled in with the number of edges coming exiting P to go below.
 *    \param upEdge One of the numberUp edges, or -1.
 *    \param downEdge One of the numberDown edges, or -1.
 */

void Shape::_countUpDown(int P, int *numberUp, int *numberDown, int *upEdge, int *downEdge) const
{
    *numberUp = 0;
    *numberDown = 0;
    *upEdge = -1;
    *downEdge = -1;
    
    int i = getPoint(P).incidentEdge[FIRST];
    
    while ( i >= 0 && i < numberOfEdges() ) {
        Shape::dg_arete const &e = getEdge(i);
        if ( P == std::max(e.st, e.en) ) {
            *upEdge = i;
            (*numberUp)++;
        }
        if ( P == std::min(e.st, e.en) ) {
            *downEdge = i;
            (*numberDown)++;
        }
        i = NextAt(P, i);
    }
    
}



/**
 *     Version of Shape::_countUpDown optimised for the case when getPoint(P).totalDegree() == 2.
 */

void Shape::_countUpDownTotalDegree2(int P,
                                     int *numberUp, int *numberDown, int *upEdge, int *downEdge) const
{
    *numberUp = 0;
    *numberDown = 0;
    *upEdge = -1;
    *downEdge = -1;
    
    for (int j : getPoint(P).incidentEdge) {
        Shape::dg_arete const &e = getEdge(j);
        if ( P == std::max(e.st, e.en) ) {
            *upEdge = j;
            (*numberUp)++;
        }
        if ( P == std::min(e.st, e.en) ) {
            *downEdge = j;
            (*numberDown)++;
        }
    }
}


void Shape::_updateIntersection(int e, int p)
{
    swrData[e].lastX = swrData[e].curX;
    swrData[e].lastY = swrData[e].curY;
    swrData[e].curX = getPoint(p).x[0];
    swrData[e].curY = getPoint(p).x[1];
    swrData[e].misc = nullptr;
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

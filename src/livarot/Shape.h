// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef my_shape
#define my_shape

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <2geom/point.h>

#include "livarot/LivarotDefs.h"

class Path;
class FloatLigne;

class SweepTree;
class SweepTreeList;
class SweepEventQueue;

enum {
  tweak_mode_grow,
  tweak_mode_push,
  tweak_mode_repel,
  tweak_mode_roughen
};

/*
 * the Shape class (was the Digraph class, as the header says) stores digraphs (no kidding!) of which 
 * a very interesting kind are polygons.
 * the main use of this class is the ConvertToShape() (or Booleen(), quite the same) function, which
 * removes all problems a polygon can present: duplicate points or edges, self-intersection. you end up with a
 * full-fledged polygon
 */

// possible values for the "type" field in the Shape class:
enum
{
  shape_graph = 0,                // it's just a graph; a bunch of edges, maybe intersections
  shape_polygon = 1               // a polygon: intersection-free, edges oriented so that the inside is on their left
};


/**
 * A class to store/manipulate directed graphs.
 *
 * This class is at the heart of everything we do in Livarot. When you first populate a Shape by calling
 * Path::Fill, it makes a directed graph of the type shape_graph. This one is exactly identical to the original
 * polyline except that it's a graph. Later, you call Shape::ConvertToShape to create another directed graph
 * from this one that is totally intersection free. All the intersections would have been calculated, edges broken
 * up at those points, all edges flipped such that inside is to their left. You ofcourse need a fill rule to do this.
 *
 * Once that's done, you can do all interesting operations such as Tweaking, Offsetting and Boolean Operations.
 */
class Shape
{
public:
    /**
     * A structure to store back data for an edge.
     */
    struct back_data
    {
        int pathID;  /*!< This is a unique number unique to a Path object given by the user to the Path::Fill function. */
        int pieceID; /*!< The path command to which this edge belongs to in the original Path object. */
        double tSt;  /*!< Time value in that path command for this edge's start point. */
        double tEn;  /*!< Time value in that path command for this edge's end point. */
    };

    /**
     * Enum describing all the events that can happen to a sweepline.
     */
    enum sTreeChangeType
    {
        EDGE_INSERTED = 0,  /*!< A new edge got added. */
        EDGE_REMOVED = 1,   /*!< An edge got removed. */
        INTERSECTION = 2    /*!< An intersection got detected. */
    };
  
    /**
     * A structure that represents a change that took place in the sweepline.
     */
    struct sTreeChange
    {
        sTreeChangeType type;       // type of modification to the sweepline:
        int ptNo;                   // point at which the modification takes place

        Shape *src;                 // left edge (or unique edge if not an intersection) involved in the event
        int bord;
        Shape *osrc;                // right edge (if intersection)
        int obord;
        Shape *lSrc;                // edge directly on the left in the sweepline at the moment of the event
        int lBrd;
        Shape *rSrc;                // edge directly on the right
        int rBrd;
    };
    
    struct incidenceData
    {
        int nextInc;                // next incidence in the linked list
        int pt;                        // point incident to the edge (there is one list per edge)
        double theta;                // coordinate of the incidence on the edge
    };
    
    Shape();
    ~Shape();

    void MakeBackData(bool nVal);

    void Affiche();

    // insertion/deletion/movement of elements in the graph
    void Copy(Shape *a);
    // -reset the graph, and ensure there's room for n points and m edges

    /**
     * Clear all data.
     *
     * Set shape type to shape_polygon. Clear all points and edges. Make room for
     * enough points and edges.
     *
     * @param n Number of points to make space for.
     * @param m Number of edges to make space for.
     */
    void Reset(int n = 0, int m = 0);
    //  -points:
    int AddPoint(const Geom::Point x);        // as the function name says
    // returns the index at which the point has been added in the array
    void SubPoint(int p);        // removes the point at index p
    // nota: this function relocates the last point to the index p
    // so don't trust point indices if you use SubPoint
    void SwapPoints(int a, int b);        // swaps 2 points at indices a and b
    void SwapPoints(int a, int b, int c);        // swaps 3 points: c <- a <- b <- c


    /**
     * Sort the points (all points) only if needed. (checked by flag)
     *
     * See the index version of SortPoints since this is exactly the same.
     */
    void SortPoints();        // sorts the points if needed (checks the need_points_sorting flag)

    //  -edges:
    // add an edge between points of indices st and en    
    int AddEdge(int st, int en);
    // return the edge index in the array
    
    // add an edge between points of indices st and en    
    int AddEdge(int st, int en, int leF, int riF);
    // return the edge index in the array
    
    // version for the voronoi (with faces IDs)
    void SubEdge(int e);                // removes the edge at index e (same remarks as for SubPoint)
    void SwapEdges(int a, int b);        // swaps 2 edges
    void SwapEdges(int a, int b, int c);        // swaps 3 edges

    /**
     * Sort all edges (clockwise) around each point.
     *
     * The function operates on each point and ensures that the linked list of the edges
     * connected to a point is in the clockwise direction spatially. The clockwise
     * angle that an edge line segment makes with the -y axis should increase (or remain same) as we move
     * forward in the linked list of edges.
     *
     * This sorting is done using edge vectors however note that if an edge ends at a point instead of starting
     * from there, we invert the edge to make the edge vector look like it started from there.
     */
    void SortEdges();        // sort the edges if needed (checks the need_edges_sorting falg)

    // primitives for topological manipulations
  
    // endpoint of edge at index b that is different from the point p      
    inline int Other(int p, int b) const
    {
        if (getEdge(b).st == p) {
            return getEdge(b).en;
        }
        return getEdge(b).st;
    }

    // next edge (after edge b) in the double-linked list at point p  
    inline int NextAt(int p, int b) const
    {
        if (p == getEdge(b).st) {
            return getEdge(b).nextS;
        }
        else if (p == getEdge(b).en) {
            return getEdge(b).nextE;
        }

        return -1;
    }

    // previous edge
    inline int PrevAt(int p, int b) const
    {
        if (p == getEdge(b).st) {
            return getEdge(b).prevS;
        }
        else if (p == getEdge(b).en) {
            return getEdge(b).prevE;
        }

        return -1;
    }

    // same as NextAt, but the list is considered circular  
    inline int CycleNextAt(int p, int b) const
    {
        if (p == getEdge(b).st) {
            if (getEdge(b).nextS < 0) {
                return getPoint(p).incidentEdge[FIRST];
            }
            return getEdge(b).nextS;
        } else if (p == getEdge(b).en) {
            if (getEdge(b).nextE < 0) {
                return getPoint(p).incidentEdge[FIRST];
            }

            return getEdge(b).nextE;
        }

        return -1;
    }

    // same as PrevAt, but the list is considered circular  
    inline int CyclePrevAt(int p, int b) const
    {
        if (p == getEdge(b).st) {
            if (getEdge(b).prevS < 0) {
                return getPoint(p).incidentEdge[LAST];
            }
            return getEdge(b).prevS;
        } else if (p == getEdge(b).en) {
            if (getEdge(b).prevE < 0) {
                return getPoint(p).incidentEdge[LAST];
            }
            return getEdge(b).prevE;
        }

        return -1;
    }
    
    void ConnectStart(int p, int b);        // set the point p as the start of edge b
    void ConnectEnd(int p, int b);        // set the point p as the end of edge b
    void DisconnectStart(int b);        // disconnect edge b from its start point
    void DisconnectEnd(int b);        // disconnect edge b from its end point

    // reverses edge b (start <-> end)    
    void Inverse(int b);
    // calc bounding box and sets leftX,rightX,topY and bottomY to their values
    void CalcBBox(bool strict_degree = false);
    
    // debug function: plots the graph
    void Plot(double ix, double iy, double ir, double mx, double my, bool doPoint,
              bool edgesNo, bool pointNo, bool doDir, char const *fileName);

    // transforms a polygon in a "forme" structure, ie a set of contours, which can be holes (see ShapeUtils.h)
    // return NULL in case it's not possible

    /**
     * Extract contours from a directed graph.
     *
     * The function doesn't care about any back data and thus all contours will be made up
     * of line segments. Any original curves would be lost.
     *
     * The traversal algorithm is totally identical to GetWindings with minor differences.
     *
     * @param[out] dest Pointer to the path where the extracted contours will be stored.
     */
    void ConvertToForme(Path *dest);
    
    // version to use when conversion was done with ConvertWithBackData(): will attempt to merge segment belonging to 
    // the same curve
    // nota: apparently the function doesn't like very small segments of arc

    /**
     * Extract contours from a directed graph while using back data.
     *
     * Since back data is used, the original curves are preserved.
     *
     * @param[out] dest Pointer to the path where the extracted contours will be stored.
     * @param nbP Number of paths that were originally fed to the directed graph with Path::Fill.
     * @param orig An array of pointers to Path, one Path object for each path id in the graph.
     */
    void ConvertToForme(Path *dest, int nbP, Path *const *orig, bool never_split = false);
    // version trying to recover the nesting of subpaths (ie: holes)
    void ConvertToFormeNested(Path *dest, int nbP, Path *const *orig, int &nbNest, int *&nesting, int *&contStart, bool never_split = false);
  
    // sweeping a digraph to produce a intersection-free polygon
    // return 0 if everything is ok and a return code otherwise (see LivarotDefs.h)
    // the input is the Shape "a"
    // directed=true <=> non-zero fill rule    

    /**
     * Using a given fill rule, find all intersections in the shape given, create a new
     * intersection free shape in the instance.
     *
     * The word "instance" or "this" is used throughout the documentation to refer to the instance on which
     * the function was called.
     *
     * Details of the algorithm:
     * The function does four things more or less:
     * 1. Find all self-intersections in the shape.
     * 2. Reconstruct the directed graph with the intersections now converted to vertices.
     * 3. Compute winding number seeds for later use by GetWindings.
     * 4. Do some processing on edges by calling AssembleAretes.
     * 5. Compute winding numbers and accordingly manipulate edges. (Deciding whether to keep, invert or destroy them)
     *
     * Finding self-intersections and reconstruction happens simultaneously. The function has a huge loop that moves
     * a sweepline top to bottom, finding intersections and reconstructing the new directed graph. Edges are added/removed
     * in the sweepline tree sTree and intersections detected go in sEvts. All events (Edge Addition/Removal/Intersection)
     * that take place at a constant `y` value are recorded in an array named `chgts` and the function call to CheckEdges
     * does the reconstruction.
     *
     * One important thing to note is that usually in a Bentley-Ottman sweepline algorithm, the heap also contains endpoints
     * (where edges start or end) but in this implementation that's not the case. The main loop takes care of the endpoints
     * and the heap takes care of intersections only.
     *
     * If you want a good theoretical overview of how all these things are done, please see the docs in livarot-doxygen.cpp.
     *
     * @param a The pointer to the shape that we want to process.
     * @param directed The fill rule.
     * @param invert TODO: Be sure about what this does
     *
     * @return 0 if everything went good, error code otherwise. (see LivarotDefs.h)
     */
    int ConvertToShape(Shape *a, FillRule directed = fill_nonZero, bool invert = false);


    // directed=false <=> even-odd fill rule
    // invert=true: make as if you inverted all edges in the source
    int Reoriente(Shape *a);        // subcase of ConvertToShape: the input a is already intersection-free
    // all that's missing are the correct directions of the edges
    // Reoriented is equivalent to ConvertToShape(a,false,false) , but faster sicne
    // it doesn't computes interections nor adjacencies
    void ForceToPolygon();        // force the Shape to believe it's a polygon (eulerian+intersection-free+no
    // duplicate edges+no duplicate points)
    // be careful when using this function

    // the coordinate rounding function
    inline static double Round(double x)
    {
        return ldexp(rint(ldexp(x, 9)), -9);
    }
    
    // 2 miscannellous variations on it, to scale to and back the rounding grid
    inline static double HalfRound(double x)
    {
        return ldexp(x, -9);
    }
    
    inline static double IHalfRound(double x)
    {
        return ldexp(x, 9);
    }

    // boolean operations on polygons (requests intersection-free poylygons)
    // boolean operation types are defined in LivarotDefs.h
    // same return code as ConvertToShape
    int Booleen(Shape *a, Shape *b, BooleanOp mod, int cutPathID = -1);

    // create a graph that is an offseted version of the graph "of"
    // the offset is dec, with joins between edges of type "join" (see LivarotDefs.h)
    // the result is NOT a polygon; you need a subsequent call to ConvertToShape to get a real polygon
    int MakeOffset(Shape *of, double dec, JoinType join, double miter, bool do_profile=false, double cx = 0, double cy = 0, double radius = 0, Geom::Affine *i2doc = nullptr);

    int MakeTweak (int mode, Shape *a, double dec, JoinType join, double miter, bool do_profile, Geom::Point c, Geom::Point vector, double radius, Geom::Affine *i2doc);
  
    int PtWinding(const Geom::Point px) const; // plus rapide
    /**
     * Compute the winding number of the point given. (brutually)
     *
     * The function works by bringing in a ray from (px.x, -infinity)
     * to (px.x, px.y) and seeing how many edges it cuts and the direction
     * of those edges. It uses this information to compute the winding number.
     *
     * The way this function works is that it iterates through all the edges
     * and for each edge it checks if the ray will intersect the edge and in
     * which orientation. See the function body to see exactly how this works.
     *
     * @image html livarot-images/winding-brutal-bounds.svg
     * @image html livarot-images/winding-brutal-endpoints-start.svg
     * @image html livarot-images/winding-brutal-endpoints-end.svg
     *
     * The algorithm is quite simple. For edges that simply cut the ray, we check
     * the direction of the edge and accordingly add/subtract from a variable to keep
     * track of the winding. However, a different case comes up when an edge has an
     * endpoint that cuts the ray. You can just see the direction and maybe change the same
     * variable, but then the problem is, another edge connected to the same point will also
     * do the same and you'd have two additions when you should only have one. Hence, the solution
     * is, we create two variables ll and rr and add/subtract to them, then, we sum them and divide
     * by 2 to get the contribution to the winding number.
     *
     * @image html livarot-images/winding-brutal-endpoints.svg
     *
     * @param px The point whose winding number to compute
     *
     * @return The winding number of the point px.
     */
    int Winding(const Geom::Point px) const;
  
    // rasterization
    void BeginRaster(float &pos, int &curPt);
    void EndRaster();
  
    void Scan(float &pos, int &curP, float to, float step);

    void Scan(float &pos, int &curP, float to, FloatLigne *line, bool exact, float step);

    void Transform(Geom::Affine const &tr)
        {for(auto & _pt : _pts) _pt.x*=tr;}

    std::vector<back_data> ebData;        /*!< Stores the back data for each edge. */

    std::vector<sTreeChange> chgts;    /*!< An array to store all the changes that happen to a sweepline within a y value */
    int nbInc;
    int maxInc;

    incidenceData *iData;
    // these ones are allocated at the beginning of each sweep and freed at the end of the sweep
    SweepTreeList *sTree;     /*!< Pointer to store the sweepline tree. To our use at least, it's a linear list of the edges that intersect with sweepline. */
    SweepEventQueue *sEvts;   /*!< Intersection events that we have detected that are to come. Sorted so closest one gets popped. */
    
    // bounding box stuff
    double leftX, topY, rightX, bottomY;

    /**
     * A point or vertex in the directed graph.
     *
     * Each point keeps track of the first edge that got connected to it and the last edge
     * that got connected to it. By connecting we mean both an edge starting at the point or
     * an edge ending at the point. This is needed for maintaining a linked list at each point.
     *
     * At each point, we maintain a linked list of edges that connect to that edge. incidentEdge
     * keeps the first and last edge of this double-linked list. The rest of the edge pointers
     * are stored in dg_arete.
     */
    struct dg_point
    {
        Geom::Point x;          /*!< The coordinates of the point. */
        int dI;                 /*!< Number of edges ending on this point. */
        int dO;                 /*!< Number of edges starting from this point. */
        int incidentEdge[2];    /*!< First (index 0) and last edge (index 1) that are attached to this point. */
        int oldDegree;          /*!< TODO: Not exactly sure why this is needed. Probably somewhere the degree changes and we retain the old degree for some reason. */

        int totalDegree() const { return dI + dO; }
    };
    
    /**
     * An edge in the directed graph.
     */
    struct dg_arete
    {
        Geom::Point dx;          /*!< edge vector (vector from start point to end point). */
        int st;                  /*!< start point of the edge. */
        int en;                  /*!< end   point of the edge. */
        int nextS;               /*!< next     edge in the double-linked list at the start point */
        int prevS;               /*!< previous edge in the double-linked list at the start point. */
        int nextE;               /*!< next     edge in the double-linked list at the end point. */
        int prevE;               /*!< previous edge in the double-linked list at the end point. */
    };

    // lists of the nodes and edges
    int maxPt; // [FIXME: remove this]
    int maxAr; // [FIXME: remove this]
    
    // flags
    int type;
    
    /**
     * Returns number of points.
     *
     * @return Number of points.
     */
    inline int numberOfPoints() const { return _pts.size(); }

    /**
     * Do we have points?
     *
     * @return True if we do, false otherwise.
     */
    inline bool hasPoints() const { return (_pts.empty() == false); }

    /**
     * Returns number of edges.
     *
     * @return Number of edges.
     */
    inline int numberOfEdges() const { return _aretes.size(); }

    /**
     * Do we have edges?
     *
     * @return True if we do, false otherwise.
     */
    inline bool hasEdges() const { return (_aretes.empty() == false); }

    /**
     * Do the points need sorting?
     *
     * @return True if we do, false otherwise.
     */
    inline void needPointsSorting() { _need_points_sorting = true; }

    /**
     * Do the edges need sorting?
     *
     * @return True if we do, false otherwise.
     */
    inline void needEdgesSorting()  { _need_edges_sorting = true; }
    
    /**
     * Do we have back data?
     *
     * @return True if we do, false otherwise.
     */
    inline bool hasBackData() const { return _has_back_data; }
    
    /**
     * Get a point.
     *
     * Be careful about the index.
     *
     * @param n Index of the point.
     *
     * @return Reference to the point.
     */
    inline dg_point const &getPoint(int n) const { return _pts[n]; }

    /**
     * Get an edge.
     *
     * Be careful about the index.
     *
     * @param n Index of the edge.
     *
     * @return Reference to the edge.
     */
    inline dg_arete const &getEdge(int n) const { return _aretes[n]; }

private:

    friend class SweepTree;
    friend class SweepEvent;
    friend class SweepEventQueue;
  
    /**
     * Extra data that some algorithms use.
     */
    struct edge_data
    {
        int weight;            /*!< Weight of the edge. If weight is 2, it means there are two identical edges on top of each other. */
        Geom::Point rdx;       /*!< Rounded edge vector */
        double length;         /*!< length of edge vector squared. */  // <-- epic naming here folks
        double sqlength;       /*!< length of edge vector */           // <-- epic naming here too
        double ilength;        /*!< Inverse of length squared */
        double isqlength;      /*!< Inverse of length */
        double siEd, coEd;     /*!< siEd=abs(rdy/length) and coEd=rdx/length */
        edge_data() : weight(0), length(0.0), sqlength(0.0), ilength(0.0), isqlength(0.0), siEd(0.0), coEd(0.0) {}
        // used to determine the "most horizontal" edge between 2 edges
    };
    
    struct sweep_src_data
    {
        SweepTree *misc;             // pointer to the SweepTree* in the sweepline
        int firstLinkedPoint;        // not used
        int stPt, enPt;                // start- end end- points for this edge in the resulting polygon
        int ind;                        // for the GetAdjacencies function: index in the sliceSegs array (for quick deletions)
        int leftRnd, rightRnd;        // leftmost and rightmost points (in the result polygon) that are incident to
        // the edge, for the current sweep position
        // not set if the edge doesn't start/end or intersect at the current sweep position
        Shape *nextSh;                // nextSh and nextBo identify the next edge in the list
        int nextBo;                        // they are used to maintain a linked list of edge that start/end or intersect at
        // the current sweep position
        int curPoint, doneTo;
        double curT;
    };
    
    struct sweep_dest_data
    {
        int misc;                        // used to check if an edge has already been seen during the depth-first search
        int suivParc, precParc;        // previous and current next edge in the depth-first search
        int leW, riW;                // left and right winding numbers for this edge
        int ind;                        // order of the edges during the depth-first search
    };
    
    struct raster_data
    {
        SweepTree *misc;                // pointer to the associated SweepTree* in the sweepline
        double lastX, lastY, curX, curY;        // curX;curY is the current intersection of the edge with the sweepline
        // lastX;lastY is the intersection with the previous sweepline
        bool sens;                        // true if the edge goes down, false otherwise
        double calcX;                // horizontal position of the intersection of the edge with the
        // previous sweepline
        double dxdy, dydx;                // horizontal change per unit vertical move of the intersection with the sweepline
        int guess;
    };
    
    /**
     * Extra data for points used at various occasions.
     */
    struct point_data
    {
        int oldInd, newInd;                // back and forth indices used when sorting the points, to know where they have
        // been relocated in the array
        int pending;                // number of intersection attached to this edge, and also used when sorting arrays
        int nextLinkedPoint;        // not used
        Shape *askForWindingS;
        int askForWindingB;
        Geom::Point  rx;          /*!< rounded coordinates of the point */
    };
    
    
    /**
     * A structure to help with sorting edges around a point.
     */
    struct edge_list
    {
        int no;
        bool starting;
        Geom::Point x;
    };

    void initialisePointData();
    void initialiseEdgeData();
    void clearIncidenceData();

    void _countUpDown(int P, int *numberUp, int *numberDown, int *upEdge, int *downEdge) const;
    void _countUpDownTotalDegree2(int P, int *numberUp, int *numberDown, int *upEdge, int *downEdge) const;
    void _updateIntersection(int e, int p);
  
    // activation/deactivation of the temporary data arrays

    /**
     * Initialize the point data cache.
     *
     * @param nVal If set to true, it sets some flags and then resizes pData to maxPt. Does nothing if false.
     */
    void MakePointData(bool nVal);

    /**
     * Initialize the edge data cache.
     *
     * @param nVal If set to true, it sets some flags and then resizes eData to maxAr. If set to false, it clears all edge data.
     */
    void MakeEdgeData(bool nVal);

    /**
     * Initialize the sweep source data cache.
     *
     * @param nVal If set to true, it sets some flags and then resizes swsData to maxAr. If set to false, it clears all swsData.
     */
    void MakeSweepSrcData(bool nVal);

    /**
     * Initialize the sweep destination data cache.
     *
     * @param nVal If set to true, it sets some flags and then resizes swdData to maxAr. If set to false, it clears all swdData.
     */
    void MakeSweepDestData(bool nVal);

    void MakeRasterData(bool nVal);

    /**
     * Sort the points
     *
     * Nothing fancy here. Please note, sorting really means just making sure the
     * points exist in the array in a sorted manner. All we do here is just change
     * the point's position in the array according to their location in physical space
     * and make sure all edges still point to the original point they were pointing to. (:-D)
     *
     * Sorting condition: Given two points LEFT and RIGHT (where LEFT's index < RIGHT's index)
     * we swap only if LEFT.y > RIGHT.y || (LEFT.y == RIGHT.y && LEFT.x > RIGHT.x)
     *
     * After sorting, not only are the points sorted in _pts, but also in pData. So both arrays
     * will have same index for the same point.
     *
     * Sorting algorithm looks like quick sort.
     *
     * @param s The start index.
     * @param e The end index.
     */
    void SortPoints(int s, int e);

    /**
     * Sort the points (take oldInd into account)
     *
     * Same as SortPoints except the sorting condition.
     *
     * Sorting condition: Given two points LEFT and RIGHT (where LEFT's index < RIGHT's index)
     * we swap only if LEFT.y > RIGHT.y || (LEFT.y == RIGHT.y && LEFT.x > RIGHT.x) ||
     * (LEFT.y == RIGHT.y && LEFT.x == RIGHT.x && LEFT.oldInd > RIGHT.oldInd)
     *
     * After sorting, not only are the points sorted in _pts, but also in pData. So both arrays
     * will have same index for the same point.
     *
     * @param s The start index.
     * @param e The end index.
     */
    void SortPointsByOldInd(int s, int e);

    // fonctions annexes pour ConvertToShape et Booleen

    /**
     * Prepare point data cache, edge data cache and sweep source cache.
     */
    void ResetSweep();        // allocates sweep structures

    /**
     * Clear point data cache, edge data cache and sweep source cache.
     */
    void CleanupSweep();        // deallocates them

    // edge sorting function    

    /**
     * Sort edges given in a list.
     *
     * Swapping is done in place so the original list will be modified to a sorted one.
     *
     * Edges between (inclusive) edges[s] and edges[e] are all sorted.
     *
     * @param edges The list of edges to sort.
     * @param s The index of the beginning of the list to sort.
     * @param s The index of the end of the list to sort.
     */
    void SortEdgesList(edge_list *edges, int s, int e);
  
    /**
     * Test if there is an intersection of an edge on a particular side.
     *
     * The actual intersection checking is performed by the other TesteIntersection and this function
     * calls it, creating an intersection event if an intersection is detected.
     *
     * @param t The pointer to the node of the edge whose intersection we wanna test.
     * @param s The side that we want to test for intersection. If RIGHT, the edge on the right is tested with this one. If LEFT, the edge on the
     * left is tested with this one.
     * @param onlyDiff My best guess about onlyDiff is it stands for "only different". Only detect intersections if
     * both edges come from different shapes, otherwise don't bother.
     */
    void TesteIntersection(SweepTree *t, Side s, bool onlyDiff);        // test if there is an intersection

    /**
     * Test intersection between the two edges.
     *
     * This is the function that does the actual checking.
     *
     * An important point to remember is that left and right aren't just two names for
     * the edges, that's how the edges should be in the sweepline at the moment, otherwise, the
     * intersection won't be detected.
     *
     * @image html livarot-images/teste-intersection.svg
     *
     * This is a very special point as it prevents detecting an intersection that has already passed. See
     * when an intersection has already passed, the order of nodes in the sweepline have switched, thus the
     * function won't detect the intersection.
     *
     * @image html livarot-images/intersection-cross-product.svg
     *
     * @image html livarot-images/intersection-cross-products.svg
     *
     * @image html livarot-images/problematic-intersection-case.svg
     *
     * This picture is related to the intersection point calculation formulas:
     *
     * @image html livarot-images/intersection-point-calculation.svg
     *
     * \f[ |\vec{slDot}| = |\vec{left}||\vec{sl}|\sin\theta_{sl}\f]
     * \f[ |\vec{elDot}| = |\vec{left}||\vec{el}|\sin\theta_{el}\f]
     *
     * These cross products (weirdly named) do have a direction too but you need to figure that out
     * with your fingers. These here only give us the magnitude, however the actual variables in code also have
     * a positive or negative sign depending on the direction. Index finger of right hand to the first vector,
     * middle finger to the second vector and if thumb points out of page, cross product is negative, if it
     * points in the page, cross product is positive. From figure 2 you can already guess that \f$ slDot < 0\f$
     * and \f$ elDot > 0 \f$. So let's rewrite the formula in the code while taking into account these signs.
     *
     * \f[ \vec{atx} = \frac{-|\vec{slDot}|*\vec{rEn} -|\vec{elDot}|*\vec{rSt}}{-|\vec{slDot}|-|\vec{elDot}|}\f]
     * You can cancel out all the minus signs:
     * \f[ \vec{atx} = \frac{|\vec{slDot}|*\vec{rEn} + |\vec{elDot}|*\vec{rSt}}{|\vec{slDot}|+|\vec{elDot}|}\f]
     * \f[ \vec{atx} = \frac{|\vec{left}||\vec{sl}||\sin\theta_{sl}|*\vec{rEn} + |\vec{left}||\vec{el}||\sin\theta_{el}|*\vec{rSt}}{|\vec{left}||\vec{sl}||\sin\theta_{sl}| + |\vec{left}||\vec{el}||\sin\theta_{el}|} \f]
     * We can cancel the left and we are left with (no word twisting intended):
     * \f[ \vec{atx} = \frac{|\vec{sl}||\sin\theta_{sl}|*\vec{rEn} + |\vec{el}||\sin\theta_{el}|*\vec{rSt}}{||\vec{sl}||\sin\theta_{sl}| + |\vec{el}||\sin\theta_{el}|} \f]
     *
     * \f[ \vec{atx} =  \frac{|\vec{sl}||\sin\theta_{sl}|}{|\vec{sl}||\sin\theta_{sl}|+|\vec{el}||\sin\theta_{el}|}*\vec{rEn} + \frac{|\vec{el}||\sin\theta_{el}|}{|\vec{sl}||\sin\theta_{sl}|+|\vec{el}||\sin\theta_{el}|}*\vec{rSt} \f]
     *
     * What you see here is a simple variant of the midpoint formula that can give us the intersection point. The sin terms when combined with sl or el
     * are simply the perpendiculars you see in figure 2 and 3. See how the perpendiculars' relative length change as the intersection point changes on
     * the right edge? This is exactly the mechanism used to find out the intersection point and its time on each edge. Look at figure 3, the point I'm
     * trying to make is that the red perpendicular's length divided by sum of length of both red and blue perpendiculars is the same factor as the
     * (length of the part of the right edge that's to the "right" of intersection) divided by total length of right edge. These ratios are exactly
     * what we use to find the intersection point as well as the time of these intersection points.
     *
     * @param iL Pointer to the left edge's node.
     * @param iR Pointer to the right edge's node.
     * @param atx The point of intersection. The function sets this if an intersection was detected.
     * @param atL The time on the left edge at the intersection point.
     * @param atR The time on the right edge at the intersection point.
     * @param onlyDiff My best guess about onlyDiff is it stands for "only different". Only detect intersections if
     * both edges come from different shapes, otherwise don't bother.
     *
     * @return true if intersection detected, otherwise false.
     */
    bool TesteIntersection(SweepTree *iL, SweepTree *iR, Geom::Point &atx, double &atL, double &atR, bool onlyDiff);
    bool TesteIntersection(Shape *iL, Shape *iR, int ilb, int irb,
                           Geom::Point &atx, double &atL, double &atR,
                           bool onlyDiff);
    bool TesteAdjacency(Shape *iL, int ilb, const Geom::Point atx, int nPt,
                        bool push);
    int PushIncidence(Shape *a, int cb, int pt, double theta);
    int CreateIncidence(Shape *a, int cb, int pt);
    void AssemblePoints(Shape *a);

    /**
     * Sort the points and merge duplicate ones.
     *
     * Sort the points from st to en - 1. No idea why the parameters were
     * set up in this weird way.
     *
     * The function also sets up newInd to the new indices so everything else
     * can update the indices instantly.
     *
     * @param st The start of the range of points to sort.
     * @param en One more than the end of the range of points to sort.
     *
     * @return If st and en are the same, nothing is done and en is returned. Otherwise, an index one more than the last point
     * in the sorted and merged list is returned. So say we gave the sequence of points indexed 2,4,5,6,7 and 4 and 5 were duplicates
     * so final sequence would have indices 2,4,5,6 and the function will return 7.
     */
    int AssemblePoints(int st, int en);
    void AssembleAretes(FillRule directed = fill_nonZero);

    /**
     * Add the event in chgts.
     *
     * The function adds stuff to the edgehead and shapeHead linked lists as well as set the
     * leftRnd and rightRnd in swsData of the edges.
     *
     * @image html livarot-images/situation-from-add-chgt.svg
     *
     * @param lastPointNo The point that was just added in "this" shape.
     * @param lastChgtPt Either lastPointNo if it's the left most point at that y level or whichever point is the left most at the
     * same y level as lastPointNo.
     * @param shapeHead The linked list from ConvertToShape and Booleen.
     * @param edgeHead The linked list from ConvertToShape and Booleen.
     * @param type The type of event this is.
     * @param lS Pointer to the unique edge's shape if this is edge addition/removal or the left edge's shape if an intersection event.
     * @param lB The unique edge (or the left edge if an intersection event).
     * @param rS Pointer to the right edge's shape if this is an intersection event.
     * @param rB The right edge if this is an intersection event.
     */
    void AddChgt(int lastPointNo, int lastChgtPt, Shape *&shapeHead,
                 int &edgeHead, sTreeChangeType type, Shape *lS, int lB, Shape *rS,
                 int rB);

    /**
     * If there are points that lie on edges, mark this by modifying leftRnd and rightRnd variables.
     *
     * Please note that an adjacency means a point lying on an edge somewhere. This is checked by
     * the function TesteAdjacency.
     *
     * Before I go into discussing how it works please review the following figure to have an idea
     * about how the points look like when this function is called from Shape::ConvertToShape.
     *
     * @image html livarot-images/lastChgtPt-from-avance.svg
     *
     * This function has a main loop that runs on all events in chgts. Each event can either be
     * an edge addition or edge addition or edge intersection. Each event (chgt) keeps four important
     * things. A unique edge associated with the event. For addition/removal it's the main edge that got
     * added/removed. For intersection it's the left one. It stores the right edge too in case of an
     * intersection. Then there are two pointers to edges to the left and right in the sweepline at the time
     * the event happened. This function does four things:
     *
     * 1. For the unique edge, get the leftRnd and rightRnd. Sets chLeN and chRiN depending on ptNo of the
     * event and the leftRnd and rightRnd. See the code to see how this is done. We test all points ranging
     * from lastChgtPt to leftRnd-1 for a possible adjacency with this unique edge. If found, we set leftRnd
     * of the unique edge accordingly. We also test points in the range rightRnd+1 to lastPointNo (not included)
     * for an adjacency and if found, we set rightRnd accordingly.
     * 2. Exactly identical thing is done with the right edge (if this is an intersection event).
     * 3. Then there is the possibility of having an edge on the left in the sweepline at the time the event happened.
     * If that's the case, we do something very special. We check if the left edge's leftRnd is smaller than lastChgtPt,
     * if not, it means the leftRnd got updated in the previous iteration of the main loop, thus, we don't need to take
     * care of it or do anything about it. If it is smaller than lastChgtPt, we run a loop testing all points in the range
     * chLeN..chRiN of having an adjacency with that edge. We update leftRnd and rightRnd of the left edge after doing some
     * checks. This process gets repeated for all the edges to the left.
     * 4. Same as 3 but for edges to the right.
     *
     * 3 and 4 are very useful. They deal with cases when you have some other edge's endpoint exactly on some edge and these
     * modify leftRnd and rightRnd of the edge so that CheckEdges will later split that edge at that endpoint. For example,
     * a shape like:
     * SVG path: M 500,200 L 500,800 L 200,800 L 500,500 L 200,200 Z
     *
     */
    void CheckAdjacencies(int lastPointNo, int lastChgtPt, Shape *shapeHead, int edgeHead);

    /**
     * Check if there are edges to draw and draw them.
     *
     * @image html livarot-images/lastChgtPt-from-avance.svg
     *
     * @param lastPointNo The point that was just added. See the figure above.
     * @param lastChgtPt See the figure above.
     * @param a The main shape a.
     * @param b The other shape if called from Shape::Booleen.
     * @param mod The boolean operation mode if called from Shape::Booleen.
     */
    void CheckEdges(int lastPointNo, int lastChgtPt, Shape *a, Shape *b, BooleanOp mod);

    /**
     * Do the edge.
     *
     * That's a very vague thing to say but basically this function checks the leftRnd and rightRnd
     * of the edge and if anything needs to be drawn, it draws them.
     *
     * Most of the code you see in the function body deals with a very rare diagonal case that I suspect would be
     * extremely rare. I'll add comments in the code body to further highlight this. But if you ignore all of that
     * whatever remains is quite simple.
     *
     * @image html livarot-images/lastChgtPt-from-avance.svg
     *
     * This picture shows you how the variables look like when Avance is called by CheckEdges which is called from
     * a block of code in Shape::ConvertToShape or Shape::Booleen. You can see that lastPointNo is the point that
     * just got added to the list and it has to be the left most, there can't be another point at the same y and
     * to the left of lastPointNo. The reason is, the block which calls CheckEdges is called at the leftmost point
     * at a particular y. You should also note how lastChgtPt is the left most point but just above lastPointNo
     * (smaller y), can't be the same y.
     *
     * @image html livarot-images/rounded-edge-diagonal-avoid.svg
     *
     * This image is referred from code comments to help explain a point.
     *
     * @param lastPointNo The new point that the sweepline just jumped on. No edge processing (adding/removal) has
     * been done on the point yet. The point has only been added in "this" shape and this is its index.
     * @param lastChgtPt This is hard to visualize but imagine the set of points having a y just smaller than lastPointNo's y
     * and now within those points (if there are multiple ones), get the left most one. That's what lastChgtPt will be.
     * @param iS The shape to which edge iB belongs.
     * @param iB The index of the edge to draw/do.
     * @param a Shape a. Not really used.
     * @param b Shape b if called ultimately from Shape::Booleen.
     * @param mod The mode of boolean operation.
     */
    void Avance(int lastPointNo, int lastChgtPt, Shape *iS, int iB, Shape *a, Shape *b, BooleanOp mod);

    /**
     * Draw edge to a passed point.
     *
     * You might ask, don't we need two points to draw an edge. Well iB is the original edge
     * passed. The edge stores the last point until which it was drawn. We will simply draw
     * an edge between that last point and the point iTo.
     *
     * Say that lp is the last point drawn and iTo is the new point. The edge will be drawn
     * in the following fashion depending on the values of direct and sens
     *
     *  direct &  sens : lp  -> iTo
     * !direct &  sens : iTo -> lp
     *  direct & !sens : iTo -> lp
     * !direct & !sens : lp  -> iTo
     *
     * If the edges had back data, the backdata for the new points is carefully calculated by doing
     * some maths so the correct t values are found. The function also updates "last point drawn" of
     * that edge to the point iTo. There is one more important thing that this function does. If the
     * edge iB has a linked list of points associated with it (due to computation of seed winding numbers)
     * then we make sure to transfer that linked list of points to the new edge that we just drew and
     * destroy the association with the original edge iB.
     *
     * @param iS The shape to which the original edge belongs.
     * @param iB The original edge in shape iS.
     * @param iTo The point to draw the edge to.
     * @param direct Used to control direction of the edge.
     * @param sens Used to control direction of the edge.
     */
    void DoEdgeTo(Shape *iS, int iB, int iTo, bool direct, bool sens);

    /**
     * Calculates the winding numbers to the left and right of all edges of this shape.
     *
     * The winding numbers that are calculated are stored in swdData.
     *
     * The winding number computation algorithm is a very interesting one and I'll get into
     * its details too. The algorithm essentially follows sub-graphs to calculate winding
     * numbers. A sub-graph is basically a set of edges and points that are connected to each
     * other in the sense that you can walk on the edges to move around them. The winding number
     * computation algorithm starts at the top most (and left most if there are multiple points at same y).
     * There, it is known that the winding number outside the edges is definitely 0 since it's the outer most
     * point. However, when you are starting on an edge that's inside the shape, a rectangle inside another one,
     * you need to know what outside winding number really is for that point. See the following image to see
     * what I mean.
     *
     * @image html livarot-images/winding-computation-seed.svg
     *
     * For the outer contour, we know it's definitely 0 but for the inside one it needs to be calculated and it's -1.
     * There are two ways to figure out this "seed" winding number. You can either iterate through all edges and calculate
     * it manually. This is known as the brutual method. The other method is to use the winding number info left from the
     * sweepline algorithm.
     *
     * I have explained the winding number computation algorithm in detail in the code comments. Once we have a seed, we start
     * walking on the edges. Once you have the left and right winding number for the first edge, you can move to its endpoint
     * and depending on the relative directions of the edge vectors, you can definitely calculate the winding number for the
     * next edge. See the code comments for details on this procedure.
     *
     * @image html livarot-images/winding-computation.svg
     *
     * Basically, given the winding numbers to the left and right of the current edge, and the orientation of the next edge
     * with this one, we can calculate the winding number of the next edge and then repeat this process.
     *
     * @param brutal Should the algorithm use winding number seeds left by the sweepline or brutually compute the seeds?
     */
    void GetWindings(bool brutal = false);

    void Validate();

    /**
     * Get the winding number for a point but from the data left by the sweepline algorithm.
     *
     * @param nPt Index of the point whose finding number we wanna calculate.
     */
    int Winding(int nPt) const;

    /**
     * Sort all points by their rounded coordinates.
     */
    void SortPointsRounded();

    /**
     * Sort points by their rounded coordinates.
     *
     * Exactly the same as SortPoints but instead of using the actual point coordinates
     * rounded coordinates are used.
     *
     * @param s The start index.
     * @param e The end index.
     */
    void SortPointsRounded(int s, int e);
    
    void CreateEdge(int no, float to, float step);
    void AvanceEdge(int no, float to, bool exact, float step);
    void DestroyEdge(int no, float to, FloatLigne *line);
    void AvanceEdge(int no, float to, FloatLigne *line, bool exact, float step);
  
    /**
     * Add a contour.
     *
     * @param dest The pointer to the Path object where we want to add contours.
     * @param num_orig The total number of path object points in the array orig.
     * @param orig A pointer of Path object pointers. These are the original Path objects which were used to fill the directed graph.
     * @param start_edge The first edge in the contour.
     * @param never_split Always coalesce pieces that come from the same original path, and don't insert forced points.
     */
    void AddContour(Path *dest, int num_orig, Path *const *orig, int start_edge, bool never_split = false);

    int ReFormeLineTo(int bord, Path *dest, bool never_split);
    int ReFormeArcTo(int bord, Path *dest, Path *orig, bool never_split);
    int ReFormeCubicTo(int bord, Path *dest, Path *orig, bool never_split);

    bool _need_points_sorting;  ///< points have been added or removed: we need to sort the points again
    bool _need_edges_sorting;   ///< edges have been added: maybe they are not ordered clockwise
    ///< nota: if you remove an edge, the clockwise order still holds
    bool _has_points_data;      ///< the pData array is allocated
    bool _point_data_initialised;///< the pData array is up to date
    bool _has_edges_data;       ///< the eData array is allocated
    bool _has_sweep_src_data;   ///< the swsData array is allocated
    bool _has_sweep_dest_data;  ///< the swdData array is allocated
    bool _has_raster_data;      ///< the swrData array is allocated
    bool _has_back_data;        //< the ebData array is allocated
    bool _bbox_up_to_date;      ///< the leftX/rightX/topY/bottomY are up to date

    std::vector<dg_point> _pts;    /*!< The array of points */
    std::vector<dg_arete> _aretes; /*!< The array of edges */
  
    // the arrays of temporary data
    // these ones are dynamically kept at a length of maxPt or maxAr
    std::vector<edge_data> eData;           /*!< Extra edge data */
    std::vector<sweep_src_data> swsData;
    std::vector<sweep_dest_data> swdData;
    std::vector<raster_data> swrData;
    std::vector<point_data> pData;          /*!< Extra point data */

    // edge direction comparison function    

    /**
     * Edge comparison function.
     *
     * The function returns +1 when a swap is needed. The arguments
     * are arranged in a weird way. Say you have two edges: w x y z and you wanna ask
     * if w and x should be swapped, you pass parameters such that ax = x & bx = w.
     *
     * The explaination of the function in the code body uses this picture to help explain.
     *
     * @image html livarot-images/edge-sorting.svg
     *
     * @param ax The right edge in the list before sorting.
     * @param bx The left edge in the list before sorting.
     * @param as True if the edge of vector ax started from the point. False if it ended there.
     * @param bs True if the edge of vector bx started from the point. False if it ended there.
     *
     * @return A positive number if the arrangement bx ax is wrong and should be swapped.
     */
    static int CmpToVert(const Geom::Point ax, const Geom::Point bx, bool as, bool bs);
};

/**
 * Is the graph Eulerian?
 *
 * A directed graph is Eulerian if every vertex has equal indegree and outdegree.
 * http://mathworld.wolfram.com/EulerianGraph.html
 *
 * @param s Pointer to the shape object.
 * @return True if shape is Eulerian.
 */
bool directedEulerian(Shape const *s);
double distance(Shape const *s, Geom::Point const &p);
bool distanceLessThanOrEqual(Shape const *s, Geom::Point const &p, double const max_l2);

#endif

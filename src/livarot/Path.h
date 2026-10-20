// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2014 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
/*
 *  Path.h
 *  nlivarot
 *
 *  Created by fred on Tue Jun 17 2003.
 *
 */

#ifndef my_path
#define my_path

#include <span>
#include <vector>
#include "LivarotDefs.h"
#include <2geom/point.h>

struct PathDescr;
struct PathDescrLineTo;
struct PathDescrArcTo;
struct PathDescrCubicTo;

class SPStyle;

/*
 * the Path class: a structure to hold path description and their polyline approximation (not kept in sync)
 * the path description is built with regular commands like MoveTo() LineTo(), etc
 * the polyline approximation is built by a call to Convert() or its variants
 * another possibility would be to call directly the AddPoint() functions, but that is not encouraged
 * the conversion to polyline can salvage data as to where on the path each polyline's point lies; use
 * ConvertWithBackData() for this. after this call, it's easy to rewind the polyline: sequences of points
 * of the same path command can be reassembled in a command
 */

// polyline description commands
enum
{
  polyline_lineto = 0,  // a lineto 
  polyline_moveto = 1,  // a moveto
  polyline_forced = 2   // a forced point, ie a point that was an angle or an intersection in a previous life
                        // or more realistically a control point in the path description that created the polyline
                        // forced points are used as "breakable" points for the polyline -> cubic bezier patch operations
                        // each time the bezier fitter encounters such a point in the polyline, it decreases its treshhold,
                        // so that it is more likely to cut the polyline at that position and produce a bezier patch
};

class Shape;

// path creation: 2 phases: first the path is given as a succession of commands (MoveTo, LineTo, CurveTo...); then it
// is converted in a polyline
// a polylone can be stroked or filled to make a polygon

/**
 * Path and its polyline approximation.
 *
 * A Path is exactly analogous to an SVG path element. Like the SVG path element, this class
 * stores path commands. A Path can be approximated by line segments and this approximation
 * is known as a "polyline approximation". Internally, the polyline approximation is stored
 * as a set of points.
 *
 * Each path command (except the MoveTo), creates a new segment. A path segment can be defined
 * as a function of time over the interval [0, 1]. Each point in the polyline approximation can
 * store the index of the path command that created the path segment that it came from and the time
 * value at which it existed. The midpoint of a line segment would be at \f[ t = 0.5 \f] for
 * example. This information is known as "back data" since it preserves the information about the
 * original segments that existed in the path and can help us recreate them or their portions back.
 * Note that the first point of a subpath stores the index of the moveTo command.
 *
 * To use this class create a new instance. Call the command functions such as Path::MoveTo,
 * Path::LineTo, Path::CubicTo, etc to append path commands. Then call one of Path::Convert,
 * Path::ConvertEvenLines or Path::ConvertWithBackData to generate the polyline approximation.
 * Then you can do simplification by calling Path::Simplify or fill a Shape by calling Path::Fill
 * on the shape to use features such as Offsetting, Boolean Operations and Tweaking.
 *
 *     Path *path = new Path;
 *     path->MoveTo(Geom::Point(10, 10));
 *     path->LineTo(Geom::Point(100, 10));
 *     path->LineTo(Geom::Point(100, 100));
 *     path->Close();
 *     path->ConvertEvenLines(0.001); // You can use the other variants too
 *     // insteresting stuff here
 *
 */
class Path
{
  friend class Shape;

  // flags for the path construction
  enum
  {
    descr_ready = 0,        
    descr_doing_subpath = 2, // we're doing a path, so there is a moveto somewhere
    descr_dirty = 16         // the path description was modified
  };

  // some data for the construction: what's pending, and some flags
  int         descr_flags = descr_ready;

public:
  std::vector<PathDescr*> descr_cmd; /*!< A vector of owned pointers to path commands. */

  /**
   * Points of the polyline approximation.
   *
   * Since the polyline approximation approximates a Path which can have multiple subpaths, the
   * approximation can also have a set of continuous polylines.
   */
  struct path_lineto
  {
    path_lineto(bool m, Geom::Point const &pp) : isMoveTo(m), p(pp) {}
    path_lineto(bool m, Geom::Point const &pp, int pie, double tt) : isMoveTo(m), p(pp), piece(pie), t(tt) {}
    
    int isMoveTo;        /*!< A flag that stores one of polyline_lineto, polyline_moveto, polyline_forced */
    Geom::Point p;       /*!< The point itself. */
    int piece = -1;      /*!< Index of the path command that created the path segment that this point comes from.*/
    double t = 0.0;      /*!< The time at which this point exists in the path segment. A value between 0 and 1. */
    bool closed = false; /*!< True indicates that subpath is closed (this point is the last point of a closed subpath) */
  };
  
  std::vector<path_lineto> pts; /*!< A vector storing the polyline approximation points. */

private:
  bool back = false; /*!< If true, indicates that the polyline approximation is going to have backdata.
                          No need to set this manually though. When Path::Convert or any of its variants is called, it's set automatically. */

  // list of points at which to ensure the polyline approximation will be subdivided, sorted ascending by path time
  struct ForcedSubdivision
  {
    int piece;
    double t;
  };
  std::vector<ForcedSubdivision> forced_subdivisions;

public:
  ~Path();

  // creation of the path description

  /**
   * Clears all stored path commands and resets flags that are used by command functions while adding path
   * commands.
   */
  void Reset();

  /**
   * Clear all stored path commands, resets flags and imports path commands from the passed Path
   * object.
   *
   * @param who Path object whose path commands to copy.
   */
  void Copy (Path * who);

  /**
   * Appends a forced point path command.
   *
   * Forced points are places in the path which are preferred to be kept in the simplification
   * algorithm. The simplification algorithm will try to retain those points. This can be beneficial
   * in situations such as self-intersections where we would want the intersection point to remain
   * unchanged after any simplification is done.
   *
   * TODO: Confirm this with some testing.
   *
   * A forced point command can't be appended if there is no active subpath that we are drawing on.
   * If you imagine calling these command functions as giving instructions to a pen, a forced point
   * command requires that the pen is already touching the canvas. The pen is not on the canvas
   * when you instantiate the Path object and it also leaves it when you call Path::Close. The term
   * "active subpath" simply means that the pen is already touching the canvas.
   *
   * @return Index of the path command in the path commands array if it got appended, -1 otherwise.
   */
  int ForcePoint();

  /**
   * Appends a close path command.
   *
   * Close path command can't be appended if there is no acive subpath.
   *
   * @return Index of the path command in the path commands array if it got appended, -1 otherwise.
   */
  int Close();

  /** Appends a MoveTo path command.
   *
   * @param ip The point to move to.
   *
   * @return The index of the path description added.
   */
  int MoveTo ( Geom::Point const &ip);

  /** Appends a LineTo path command.
   *
   * @param ip The point to draw a line to.
   *
   * @return The index of the path description added.
   */
  int LineTo ( Geom::Point const &ip);

  /**
   * Appends a CubicBezier path command.
   *
   * In order to understand the parameters let p0, p1, p2, p3 denote the four points of a
   * cubic Bezier curve. p0 is the start point. p3 is the end point. p1 and p2 are the
   * two control points.
   *
   * @param ip The final point of the bezier curve or p3.
   * @param iStD 3 * (p1 - p0). Weird way to store it but that's how it is.
   * @param iEnD 3 * (p3 - p2). Weird way to store it but that's how it is.
   *
   * @return The index of the path description added.
   */
  int CubicTo ( Geom::Point const &ip,  Geom::Point const &iStD,  Geom::Point const &iEnD);

  /**
   * Appends an ArcTo path command.
   *
   * The parameters are identical to the SVG elliptical arc command.
   *
   * @param ip The final point of the arc.
   * @param iRx The radius in the x direction.
   * @param iRy The radius in the y direction.
   * @param angle The angle w.r.t x axis in degrees. TODO: Confirm this
   * @param iLargeArc If true, it's the larger arc, if false, it's the smaller one.
   * @param iClockwise If true, it's the clockwise arc, if false, it's the anti-clockwise one.
   *
   * @return The index of the path description added.
   */
  int ArcTo ( Geom::Point const &ip, double iRx, double iRy, double angle, bool iLargeArc, bool iClockwise);

  // transforms a description in a polyline (for stroking and filling)
  // treshhold is the max length^2 (sort of)

  /**
   * Creates a polyline approximation of the path. Doesn't keep any back data. Line segments are
   * not split into smaller line segments.
   *
   * Threshold has no strict definition. It means different things for each path segment.
   *
   * @param threshhold The error threshold used to approximate curves by line segments. The smaller
   * this is, the more line segments there will be.
   */
  void Convert (double treshhold);

  /**
   * Creates a polyline approximation of the path. Line segments are split into further smaller line segments
   * such that each of those line segments is no bigger than threshold.
   *
   * Breaking up into further smaller line segments is useful for path simplification as you can
   * then fit cubic Bezier patches on those small line segments.
   *
   * Threshold has no strict definition. It means different things for each path segment.
   *
   * @param threshhold The error threshold used to approximate the path. The smaller this is, the
   * more line segments there will be and the better the polyline approximation would be.
   */
  void ConvertEvenLines (double treshhold);	// decomposes line segments too, for later recomposition

  /**
   * Creates a polyline approximation of the path. Line segments are
   * not split into smaller line segments. Stores back data for later recomposition.
   *
   * Threshold has no strict definition. It means different things for each path segment.
   *
   * @param threshhold The error threshold used to approximate the path. The smaller this is, the
   * more line segments there will be and the better the polyline approximation would be.
   *
   * @param relative Whether to interpret threshold relatively, rather than absolutely.
   */
  void ConvertWithBackData(double threshhold, bool relative = false);

  // creation of the polyline (you can tinker with these function if you want)

  /**
   * Sets the back variable to the value passed in and clears the polyline approximation.
   *
   * @param nVal True if we are going to have backdata and false otherwise.
   */
  void SetBackData (bool nVal);	// has back data?

  /**
   * Clears the polyline approximation.
   */
  void ResetPoints(); // resets to the empty polyline

private:
  /**
   * Adds a point to the polyline approximation's list of points.
   *
   * This is used internally by Path::Convert and its variants, so you'd not need to use it by
   * yourself.
   *
   * If back variable of the instance is set to true, dummy back data will be used with the point.
   * Piece being -1 and time being 0. Since this function doesn't take any back data you'll have to
   * fill in something.
   *
   * The point doesn't get added if it's a lineto and the point before it has the same coordinates.
   *
   * @param iPt The point itself.
   * @param mvto If true, it's a moveTo otherwise it's a lineto.
   *
   * @return Index of the point added if it was added, -1 otherwise.
   */
  int AddPoint ( Geom::Point const &iPt, bool mvto = false);	// add point

  /**
   * Adds a point to the polyline approximation's list of points. Let's you specify back data.
   *
   * This is used internally by Path::Convert and its variants, so you'd not need to use it by
   * yourself.
   *
   * @param iPt The point itself.
   * @param ip The index of the path command that created the segment that this point belongs to.
   * @param it The time in that path segment at which this point exists. 0 is beginning and 1
   * is end.
   * @param mvto If true, it's a moveTo otherwise it's a lineto.
   *
   * The point doesn't get added if it's a lineto and the point before it has the same coordinates.
   *
   * @return Index of the point added if it was added, -1 otherwise.
   */
  int AddPoint ( Geom::Point const &iPt, int ip, double it, bool mvto = false);

  /**
   * Adds a forced point to the polyline approximation's list of points.
   *
   * The point that gets added as a forced point has the same coordinates as the last point that
   * was added (and the same backdata too, if backdata is enabled). If no points exist or the last
   * one isn't a lineto, nothing gets added.
   *
   * @return Index of the point added if it was added, -1 otherwise.
   */
  int AddForcedPoint();

public:
  // transform in a polygon (in a graph, in fact; a subsequent call to ConvertToShape is needed)
  //  - fills the polyline; justAdd=true doesn't reset the Shape dest, but simply adds the polyline into it
  // closeIfNeeded=false prevent the function from closing the path (resulting in a non-eulerian graph
  // pathID is a identification number for the path, and is used for recomposing curves from polylines
  // give each different Path a different ID, and feed the appropriate orig[] to the ConvertToForme() function

  /**
   * Fills the shape with the polyline approximation stored in this object.
   *
   * For each line segment in the polyline approximation, an edge is created in the shape.
   *
   * One important point to highlight is the closeIfNeeded argument. For each subpath (where a
   * sub path is a moveTo followed by one or more lineTo points) you can either have the start and end
   * points being identical or very close (a closed contour) or have them apart (an open contour).
   * If you set closeIfNeeded to true, it'll automatically add a closing segment if needed and
   * close an open contour by itself. If your contour is already closed, it makes sure that the
   * first and last point are the same ones in the graph (instead of being two indentical points).
   * If closeIfNeeded is false, it just doesn't care at all. Even if your contour is closed, the
   * first and last point will be separate (even though they would be duplicates).
   *
   * @param dest The shape to fill.
   * @param pathID A unique number for this path. The shape will associate this number with each
   * edge that comes from this path. Later on, when you use Shape::ConvertToForme you'll pass an array
   * of Path objects (named orig) and the shape will use that pathID to do orig[pathID] and get the
   * original path information.
   * @param justAdd If set to true, this will function will just fill stuff in without resetting
   * any existing stuff in Shape. If set to false, it'll make sure to reset the shape and already
   * make room for the maximum number of possible points and edges.
   * @param closeIfNeeded If set to true, the graph will be closed always. Otherwise, it won't be
   * closed.
   * @param invert If set to true, the graph is drawn exactly in the manner opposite to the actual
   * polyline approximation that this object stores, if false, it's stored indentical to how it's
   * in the polyline approximation.
   *
   * @todo "the graph is drawn exactly in the manner opposite"? Does this mean the edges of the
   * directed graph are reversed?
   */
  void Fill(Shape *dest, int pathID = -1, bool justAdd = false,
            bool closeIfNeeded = true, bool invert = false);

  // - stroke the path; usual parameters: type of cap=butt, type of join=join and miter (see LivarotDefs.h)
  // doClose treat the path as closed (ie a loop)
  void Stroke(Shape *dest, bool doClose, double width, JoinType join,
              ButtType butt, double miter, bool justAdd = false);

  // build a Path that is the outline of the Path instance's description (the result is stored in dest)
  // it doesn't compute the exact offset (it's way too complicated, but an approximation made of cubic bezier patches
  //  and segments. the algorithm was found in a plugin for Impress (by Chris Cox), but i can't find it back...
  void Outline(Path *dest, double width, JoinType join, ButtType butt,
               double miter);

  // half outline with edges having the same direction as the original
  void OutsideOutline(Path *dest, double width, JoinType join, ButtType butt,
                      double miter);

  // half outline with edges having the opposite direction as the original
  void InsideOutline (Path * dest, double width, JoinType join, ButtType butt,
		      double miter);

  // polyline to cubic bezier patches

  /**
   * Simplify the path.
   *
   * Fit the least possible number of cubic Bezier patches on the polyline approximation while
   * respecting the threshold (keeping the error small). The function clears existing path commands
   * and the resulting cubic Bezier patches will be pushed as path commands in the instance.
   *
   * The algorithm to fit cubic Bezier curves on the polyline approximation's points.
   *
   * http://www.cs.mtu.edu/~shene/COURSES/cs3621/NOTES/INT-APP/CURVE-APP-global.html
   *
   * @param threshold The threshold for simplification. A measure of how much error is okay. The
   * smaller this number is, the more conservative the fitting algorithm will be.
   */
  void Simplify (double treshhold);

  /**
   * Simplify the path with a different approach.
   *
   * This function is also supposed to do simplification but by merging (coalescing) existing
   * path descriptions instead of doing any fitting. But I seriously doubt whether this is useful
   * at all or works at all. More experimentation needed. TODO
   *
   * @param threshold The threshold for simplification.
   */
  void Coalesce (double tresh);

  // utilities
  // piece is a command no in the command list
  // "at" is an abcissis on the path portion associated with this command
  // 0=beginning of portion, 1=end of portion.
  void PointAt (int piece, double at, Geom::Point & pos);
  void PointAndTangentAt (int piece, double at, Geom::Point & pos, Geom::Point & tgt);

  // last control point before the command i (i included)
  // used when dealing with quadratic bezier spline, cause these can contain arbitrarily many commands
  const Geom::Point PrevPoint (const int i) const;

  // dash the polyline
  // the result is stored in the polyline, so you lose the original. make a copy before if needed
  void  DashPolyline(float head,float tail,float body,int nbD, const float dashs[],bool stPlain,float stOffset);

  void  DashPolyline(std::vector<double> array_values, double offset, float scale, float min_len);

  //utilitaire pour inkscape

  /**
   * Load a lib2geom Geom::Path in this path object.
   *
   * The Geom::Path object is read and path commands making it up are appended in the Path object.
   *
   * @param path The Geom::Path object to load.
   * @param tr A transformation matrix.
   * @param doTransformation If set to true, the transformation matrix tr is applied on the path
   * before it's loaded in this path object.
   * @param append If set to true, any existing path commands in this object are retained. If
   * set to false, any existing path commands will be cleared.
   */
  void  LoadPath(Geom::Path const &path, Geom::Affine const &tr, bool doTransformation, bool append = false);

  /**
   * Load a lib2geom Geom::PathVector in this path object. (supports transformation)
   *
   * Any existing path commands in this object are cleared.
   *
   * @param pv The Geom::PathVector object to load.
   * @param tr A transformation to apply on each path.
   * @param doTransformation If set to true, the transformation in tr is applied.
   */
  void  LoadPathVector(Geom::PathVector const &pv, Geom::Affine const &tr, bool doTransformation);

  /**
   * Load a lib2geom Geom::PathVector in this path object.
   *
   * Any existing path commands in this object are cleared.
   *
   * @param pv A reference to the Geom::PathVector object to load.
   */
  void  LoadPathVector(Geom::PathVector const &pv);

  /**
   * Load a lib2geom Geom::PathVector in this path object, plus a list of forced subdivision points.
   *
   * Any existing path commands in this object are cleared.
   *
   * @param pv A reference to the Geom::PathVector object to load.
   * @param cuts A list of times where ConvertWithBackData() should insert forced subdivisions, sorted ascending.
   */
  void LoadPathVector(Geom::PathVector const &pv, std::vector<Geom::PathVectorTime> const &cuts);

  /**
   * Create a lib2geom Geom::PathVector from this Path object.
   *
   * Looks like the time this was written Geom::PathBuilder didn't exist or maybe
   * the author wasn't aware of it.
   *
   * @return The Geom::PathVector created.
   */
  Geom::PathVector MakePathVector() const;

  /**
   * Apply a transformation on all path commands.
   *
   * Done by calling the transform method on each path command.
   *
   * @param trans The transformation to apply.
   */
  void  Transform(const Geom::Affine &trans);

  // decompose le chemin en ses sous-chemin
  // killNoSurf=true -> oublie les chemins de surface nulle
  Path**      SubPaths(int &outNb,bool killNoSurf);
  // pour recuperer les trous
  // nbNest= nombre de contours
  // conts= debut de chaque contour
  // nesting= parent de chaque contour
  Path**      SubPathsWithNesting(int &outNb,bool killNoSurf,int nbNest,int* nesting,int* conts);
  // surface du chemin (considere comme ferme)
  double      Surface();
  void        PolylineBoundingBox(double &l,double &t,double &r,double &b);
  void        FastBBox(double &l,double &t,double &r,double &b);
  // longueur (totale des sous-chemins)
  double      Length();
  
  void             ConvertForcedToMoveTo();
  void             ConvertForcedToVoid();
  struct cut_position {
    int          piece;
    double        t;
  };
  cut_position*    CurvilignToPosition(int nbCv,double* cvAbs,int &nbCut);
  cut_position    PointToCurvilignPosition(Geom::Point const &pos, unsigned seg = 0) const;
  //Should this take a cut_position as a param?
  double           PositionToLength(int piece, double t);
  
  // caution: not tested on quadratic b-splines, most certainly buggy
  void             ConvertPositionsToMoveTo(int nbPos,cut_position* poss);
  void             ConvertPositionsToForced(int nbPos,cut_position* poss);

  void  Affiche();
  std::string svg_dump_path() const;
  
  bool IsLineSegment(int piece);

    private:
  // utilitary functions for the path construction
  void CloseSubpath();
  void InsertMoveTo (Geom::Point const &iPt,int at);
  void InsertForcePoint (int at);
  void InsertLineTo (Geom::Point const &iPt,int at);
  void InsertArcTo (Geom::Point const &ip, double iRx, double iRy, double angle, bool iLargeArc, bool iClockwise,int at);
  void InsertCubicTo (Geom::Point const &ip,  Geom::Point const &iStD,  Geom::Point const &iEnD,int at);
  
  // creation of dashes: take the polyline given by spP (length spL) and dash it according to head, body, etc. put the result in
  // the polyline of this instance
  void DashSubPath(int spL, int spP, std::vector<path_lineto> const &orig_pts, float head,float tail,float body,int nbD, const float dashs[],bool stPlain,float stOffset);

  // Functions used by the conversion.
  // they append points to the polyline
  /**
   * The function is quite similar to RecCubicTo. Some of the maths, specially that in
   * ArcAnglesAndCenter is too cryptic and I have not spent enough time deriving it yet either. The
   * important thing is how the Arc is split into line segments and that I can explain. Given the
   * threshold and the two radii, a maximum angle is calculated. This angle is a measure of how big
   * a sub-arc you can substitute with a line segment without breaking the threshold. Then you
   * divide the whole arc into sectors such that each one's angle is under or equal to maximum
   * angle.
   *
   * @image html livarot-images/arc-threshold.svg
   *
   * In this image, the red dashed arc is the actual arc that was to be approximated. The blue arcs
   * are sectors, each one having an angle equal to or smaller than maximum angle (which is 20
   * degrees) in this example. The final polyline approximation is shown by the pink dotted line
   * segments.
   *
   * TODO: Understand the maths in ArcAnglesAndCenter and how the maximum angle is calculated.
   *
   */
  void DoArc ( Geom::Point const &iS,  Geom::Point const &iE, double rx, double ry,
	      double angle, bool large, bool wise, double tresh);
  /**
   * Approximate the passed cubic bezier with line segments.
   *
   * Basically the function checks if the passed cubic Bezier is "small enough" and if
   * it is, it does nothing, if it however isn't "small enough", it splits the cubic Bezier
   * curve into two cubic Bezier curves (split at mid point), recursively calls itself on the
   * left cubic, add the midpoint to the polyline approximation, call itself on the right
   * cubic and done. lev is the maximum recursion depth possible, once it's reached, the function
   * returns doing nothing immediately. See the code to understand more about maxL.
   *
   * The way the algorithm checks if the curve is "small enough" is maths so I'll try to
   * explain it here so you can see the equations printed and probably refer it when reading code.
   *
   * Let \f$\vec{p_{0}}\f$, \f$\vec{p_{1}}\f$, \f$\vec{p_{2}}\f$ and \f$\vec{p_{3}}\f$ be the four
   * points that define a cubic Bezier. The first is the start point, last is the end point,
   * the two in between are the control points. Given this let me relate these points to the
   * arguments that were passed in.
   *
   * \f[ \vec{iS} = \vec{p_{0}}\f]
   * \f[ \vec{iE} = \vec{p_{3}}\f]
   * \f[ \vec{iSd} = 3 (\vec{p_{1}} - \vec{p_{0}})\f]
   * \f[ \vec{iEd} = 3 (\vec{p_{3}} - \vec{p_{2}})\f]
   *
   * This is just how livarot represents a Cubic Bezier, nothing I can do about that. The code
   * starts by calculating a vector from start point to end point.
   *
   * \f[ \vec{se} = \vec{iE} - \vec{iS} ]\f
   *
   * If the length of \f$\vec{se}\f$ is smaller than 0.01, then the cubic bezier's endpoints are
   * kinda close, but if the control points are too far away, it can still be a long tall curve,
   * so let's see the control points and see how far away they are from the \f$\vec{se}\f$ vector.
   * To do that, we measure the lengths of \f$\vec{iSd}\f$ and \f$\vec{iEd}\f$. If both are below
   * threshold, we return immediately since it indicates the cubic bezier is "small enough".
   *
   * if the length is greater than 0.01, we still check the y projections of the control handles
   * on the line between start and end points, if these projections are limited by the threshold
   * and we didn't mess up the maxL restriction, we are good.
   *
   * If we ran out of recursion levels, we return anyways. In case this cubic bezier isn't small
   * enough, we split it in two parts. There are math equations in the code that do this and I
   * spent hours deriving it and they are totally correct. Basically take the usual maths to split
   * a cubic Bezier into two parts and just account for the factor of 3 in the control handles
   * that livarot adds and you'll end up with correct equations.
   *
   * TODO: Add derivation here maybe?
   *
   * If relative is true, then instead of "small enough", the criterion is "linear enough".
   * That is, the threshold parameter is interpreted relatively rather than absolutely.
   */
  void RecCubicTo ( Geom::Point const &iS,  Geom::Point const &iSd,  Geom::Point const &iE,  Geom::Point const &iEd, double tresh, int lev,
		   double maxL = -1.0);

  void DoArc(Geom::Point const &iS,  Geom::Point const &iE, double rx, double ry,
             double angle, bool large, bool wise, double tresh, int piece,
             bool relative, std::vector<double> const &cuts = {});
  void RecCubicTo ( Geom::Point const &iS,  Geom::Point const &iSd,  Geom::Point const &iE,  Geom::Point const &iEd, double tresh, int lev,
                  double st, double et, int piece, bool relative);
  void RecCubicTo(Geom::Point const &iS, Geom::Point const &iSd, Geom::Point const &iE, Geom::Point const &iEd, double thresh, int lev,
                  double st, double et, int piece, bool relative, std::span<double> const &cuts);

  static void ArcAngles ( Geom::Point const &iS,  Geom::Point const &iE, double rx,
                         double ry, double angle, bool large, bool wise,
                         double &sang, double &eang);
  static void CubicTangent (double t,  Geom::Point &oPt,  Geom::Point const &iS,
			     Geom::Point const &iSd,  Geom::Point const &iE,
			     Geom::Point const &iEd);

  struct outline_callback_data
  {
    Path *orig;
    int piece;
    double tSt, tEn;
    Path *dest;
    double x1, y1, x2, y2;
    union
    {
      struct
      {
	double dx1, dy1, dx2, dy2;
      }
      c;
      struct
      {
	double mx, my;
      }
      b;
      struct
      {
	double rx, ry, angle;
	bool clock, large;
	double stA, enA;
      }
      a;
    }
    d;
  };

  typedef void (outlineCallback) (outline_callback_data * data, double tol,  double width);
  struct outline_callbacks
  {
    outlineCallback *cubicto;
    outlineCallback *arcto;
  };

  void SubContractOutline (int off, int num_pd,
			   Path * dest, outline_callbacks & calls,
			   double tolerance, double width, JoinType join,
			   ButtType butt, double miter, bool closeIfNeeded,
			   bool skipMoveto, Geom::Point & lastP, Geom::Point & lastT);
  void DoStroke(int off, int N, Shape *dest, bool doClose, double width, JoinType join,
		ButtType butt, double miter, bool justAdd = false);

  static void TangentOnSegAt(double at, Geom::Point const &iS, PathDescrLineTo const &fin,
			     Geom::Point &pos, Geom::Point &tgt, double &len);
  static void TangentOnArcAt(double at, Geom::Point const &iS, PathDescrArcTo const &fin,
			     Geom::Point &pos, Geom::Point &tgt, double &len, double &rad);
  static void TangentOnCubAt (double at, Geom::Point const &iS, PathDescrCubicTo const &fin, bool before,
			      Geom::Point &pos, Geom::Point &tgt, double &len, double &rad);
  static void OutlineJoin (Path * dest, Geom::Point pos, Geom::Point stNor, Geom::Point enNor,
			   double width, JoinType join, double miter, int nType);

  static bool IsNulCurve (std::vector<PathDescr*> const &cmd, int curD, Geom::Point const &curX);

  static void RecStdCubicTo (outline_callback_data * data, double tol,
			     double width, int lev);
  static void StdCubicTo (outline_callback_data * data, double tol,
			  double width);
  static void RecStdArcTo (outline_callback_data * data, double tol,
			   double width, int lev);
  static void StdArcTo (outline_callback_data * data, double tol, double width);

  // fonctions annexes pour le stroke
  static void DoButt (Shape * dest, double width, ButtType butt, Geom::Point pos,
		      Geom::Point dir, int &leftNo, int &rightNo);
  static void DoJoin (Shape * dest, double width, JoinType join, Geom::Point pos,
		      Geom::Point prev, Geom::Point next, double miter, double prevL,
		      double nextL, int *stNo, int *enNo);
  static void DoLeftJoin (Shape * dest, double width, JoinType join, Geom::Point pos,
			  Geom::Point prev, Geom::Point next, double miter, double prevL,
			  double nextL, int &leftStNo, int &leftEnNo,int pathID=-1,int pieceID=0,double tID=0.0);
  static void DoRightJoin (Shape * dest, double width, JoinType join, Geom::Point pos,
			   Geom::Point prev, Geom::Point next, double miter, double prevL,
			   double nextL, int &rightStNo, int &rightEnNo,int pathID=-1,int pieceID=0,double tID=0.0);
    static void RecRound (Shape * dest, int sNo, int eNo,
            Geom::Point const &iS, Geom::Point const &iE,
            Geom::Point const &nS, Geom::Point const &nE,
            Geom::Point &origine,float width);


  /**
   * Simpilfy a sequence of points.
   *
   * Fit cubic Bezier patches on the sequence of points defined by the passed parameters. This
   * sequence is just a subset of the polyline approximation points stored in the Path object.
   *
   * @param off The offset to the first point to process.
   * @param N The total number of points in the sequence.
   * @param threshhold The threshold to respect during simplification. The higher this number is,
   * the more relaxed you're making the simplifier. The smaller the number, the more strict you're
   * making the simplifier.
   */
  void DoSimplify(int off, int N, double treshhold);

  /**
   * Fit a cubic Bezier patch on the sequence of points.
   *
   * @param off The index of the first point of the sequence to fit on.
   * @param N The total number of points you want to fit on.
   * @param res Reference to the Cubic Bezier description where the resulting control points will
   * be stored.
   * @param worstP Reference to a point index. This will be changed to whichever point measures the
   * highest deviation from the fitted curve.
   *
   * @return True if the fit respected threshold, false otherwise.
   */
  bool AttemptSimplify(int off, int N, double treshhold, PathDescrCubicTo &res, int &worstP);
  /*
   * The actual fitting code that takes a sequence and fits stuff on it.
   *
   * Totally based on the algorithm from:
   * http://www.cs.mtu.edu/~shene/COURSES/cs3621/NOTES/INT-APP/CURVE-APP-global.html
   *
   * @param start The start point of the cubic bezier which is already known.
   * @param res The cubic Bezier path command that function will populate after doing the maths.
   * @param Xk The array of X coordinates of the point to fit.
   * @param Yk The array of Y coordinates of the point to fit.
   * @param Qk An array to store some intermediate values.
   * @param tk The time values for the points.
   * @param nbPt The total points to fit on.
   *
   * @return True if the fit was done correctly, false if something bad happened. (Non-invertible
   * matrix).
   */
  static bool FitCubic(Geom::Point const &start,
		       PathDescrCubicTo &res,
		       double *Xk, double *Yk, double *Qk, double *tk, int nbPt);
  /**
   * Structure to keep some data for fitting.
   *
   * Note that the pointers in the structure are going to store arrays. The comments explain what
   * each element of a particular array stores. Also note that the length mentioned in the comment
   * for tk and lk is not the straight line distance but the length as measured by walking on the
   * line segments connecting the points.
   */
  struct fitting_tables {
    int      nbPt;   /*!< The points to fit on in a particular iteration */
    int      maxPt;  /*!< Maximum number of points these arrays here can store */
    int      inPt;   /*!< Total points whose X, Y, lk are all populated here */
    double   *Xk;    /*!< X coordinate of the point */
    double   *Yk;    /*!< Y coordinate of the point */
    double   *Qk;    /*!< A special value needed by the fitting algorithm */
    double   *tk;    /*!< A number between 0 and 1 that is the fraction (length b/w first point to this point along the line segments)/(total length) */
    double   *lk;    /*!< Length of the line segment from the previous point to this point */
    char     *fk;    /*!< A flag if 0x01 indicates forced point and if 0x00 indicates a normal point */
    double   totLen; /*!< Total length of the polyline or you can say the sum of lengths of all line segments. */
  };

  /**
   * Fit Cubic Bezier patch using the fitting table data.
   *
   * @param data The fitting_tables data needed for fitting. ExtendFit sets that up for this
   * function.
   * @param threshhold The threshold to respect.
   * @param res The cubic Bezier command which this function will populate.
   * @param worstP The point with the worst error.
   */
  bool   AttemptSimplify (fitting_tables &data,double treshhold, PathDescrCubicTo & res,int &worstP);

  /**
   * Fit Cubic Bezier patch on the points.
   *
   * This uses data already calculated by probably the same function if it exists.
   * The data that's reused is apparently the X, Y and lk values. However, I think there is a
   * problem with this caching mechanism. See the inline comments of ExtendFit.
   *
   * This function prepares data in fitting tables and calls the AttemptSimplify version that takes
   * fitting_tables data.
   *
   * @param off The offset to the first point.
   * @param N The total number of points in that sequence.
   * @param data The data structure which keeps data saved for later use by the same function.
   * @param threshhold The threshold to respect.
   * @param res cubic Bezier path command where this function will store the control point handles.
   * @param worstP Function will set this to the point with the worst error.
   *
   * @return True if the threshold was respected, otherwise false.
   */
  bool   ExtendFit(int off, int N, fitting_tables &data,double treshhold, PathDescrCubicTo & res,int &worstP);
  /**
   * Peform an iteration of Newton-Raphson to improve t values.
   *
   * TODO: Place derivation here with embedded latex maybe.
   */
  double RaffineTk (Geom::Point pt, Geom::Point p0, Geom::Point p1, Geom::Point p2, Geom::Point p3, double it);
  void   FlushPendingAddition(Path* dest,PathDescr *lastAddition,PathDescrCubicTo &lastCubic,int lastAD);

private:
  /**
   * Add a Geom::Curve's equivalent path description.
   *
   * Any straight curve (line or otherwise that's straight) is added as line. CubicBezier
   * and EllipticalArcs are handled manually, while any other Geom::Curve type is handled by
   * converting to cubic beziers using Geom::cubicbezierpath_from_sbasis and recursively calling
   * the same function.
   *
   * There is one special reason for using is_straight_curve to figure out if a CubicBezier is
   * actually a line and making sure that it is added as a line not as a straight line CubicBezier
   * (a CubicBezier with control points being the same as end points). Sometimes when you're drawing
   * straight line segments with the Bezier (pen) tool, Inkscape would place a straight CubicBezier
   * instead of a line segment. The call to Path::Convert or Path::ConvertWithBackData would break
   * up this line segment into smaller line segments which is not what we want (we want it to break
   * only real curves) not curves that are actually just straight lines.
   *
   * @param c The Geom::Curve whose path description to create/add.
   */
    void AddCurve(Geom::Curve const &c);
};

#endif

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

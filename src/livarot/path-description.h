// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_INKSCAPE_LIVAROT_PATH_DESCRIPTION_H
#define SEEN_INKSCAPE_LIVAROT_PATH_DESCRIPTION_H

#include <2geom/point.h>
#include "svg/stringstream.h"

// path description commands
/* FIXME: these should be unnecessary once the refactoring of the path
** description stuff is finished.
*/

/**
 * An enum to store the path command type. A number is assigned to each type and the
 * PathDescr class stores a flag variable that stores the number to know which type it is.
 */
enum
{
  descr_moveto = 0,         /*!< A MoveTo path command */
  descr_lineto = 1,         /*!< A LineTo path command */
  descr_cubicto = 2,        /*!< A CubicTo path command. Basically a cubic Bezier */
  descr_arcto = 4,          /*!< An elliptical arc */
  descr_close = 5,          /*!< A close path command */
  descr_forced = 7,         /*!< Not exactly sure what a forced point means. As far as I have seen in the simplify code, a forced
                              point is preferred to be kept. The simplification algorithm would make sure the forced point makes
                              its way to the final result. However, as far as I can see, forced point stuff is not used in Inkscape.
                              TODO: Explore how forced points might be being used when Simplify is done after Boolean Ops */
  descr_type_mask = 15      /*!< As mentioned above, the flag variable of PathDescr stores the type of the command using this enum. The higher bits
                              (after the first four (bit 0 to bit 3) could be used for other flag stuff, so this descr_type_mask can be used to just
                              extract the type. 15 in HEX is 0xF and in BIN is 0000 1111 so ANDing with it will zero any higher bits just leaving you
                              with the type. */
};

/**
 * A base class for Livarot's path commands. Each curve type such as Line, CubicBezier
 * derives from this base class.
 */
struct PathDescr
{
  PathDescr() : flags(0), associated(-1), tSt(0), tEn(1) {}
  PathDescr(int f) : flags(f), associated(-1), tSt(0), tEn(1) {}
  virtual ~PathDescr() = default;

  int getType() const { return flags & descr_type_mask; }
  void setType(int t) {
    flags &= ~descr_type_mask;
    flags |= t;
  }

    /**
     * A virtual function that derived classes will implement. Dumps the SVG path d attribute
     * for this path description.
     *
     * @param s The stream to put the SVG description in.
     * @param last The last point before this path description. This is needed for the computation
     * of SVG descriptions of instructions such as Cubic and Arc.
     */
    virtual void dumpSVG(Inkscape::SVGOStringStream &/*s*/, Geom::Point const &/*last*/) const {}

    /**
     * A virtual function that derived classes will implement. Returns a newly allocated copy
     * of the path description.
     */
    virtual PathDescr *clone() const = 0;

    /**
     * A virtual function that derived classes will implement. Similar to dumpSVG however this
     * prints a simpler path description that's not SVG, only used for debugging purposes.
     *
     * @param s The stream to print to.
     */
    virtual void dump(std::ostream &/*s*/) const {}

    int    flags;         /*!< Lower 4 bits contain the type of the path description as decided by the enum
                            above, upper bits could contain other information but don't know if they really do at all */
    int    associated;    /*!< Index of the last polyline point associated with this path description. Interestingly, Path::ConvertWithBackData
                            doesn't set this field at all while Path::Convert and Path::ConvertEvenLines do. */
    double tSt;           /*!< By default set to 0. No idea if this is used at all. TODO */
    double tEn;           /*!< By default set to 1. No idea if this is used at all. TODO */
};

/**
 * A MoveTo path command.
 */
struct PathDescrMoveTo : public PathDescr
{
  PathDescrMoveTo(Geom::Point const &pp)
      : PathDescr(descr_moveto), p(pp) {}

  void dumpSVG(Inkscape::SVGOStringStream &s, Geom::Point const &last) const override;
  PathDescr *clone() const override;
  void dump(std::ostream &s) const override;

  Geom::Point p; /*!< The point to move to. */
};

/**
 * A LineTo path command.
 */
struct PathDescrLineTo : public PathDescr
{
  PathDescrLineTo(Geom::Point const &pp)
    : PathDescr(descr_lineto), p(pp) {}

  void dumpSVG(Inkscape::SVGOStringStream &s, Geom::Point const &last) const override;
  PathDescr *clone() const override;
  void dump(std::ostream &s) const override;

  Geom::Point p; /*!< The point to draw a line to. */
};

/**
 * Cubic Bezier path command.
 *
 * There is something funny about this one. A typical BezierCurve consists of points
 * p0, p1, p2, p3 where p1 and p2 are control points. This is a command so it's
 * quite expected that p0 is not needed. What's interesting is that instead of storing
 * p1 and p2, (p1 - p0) * 3 and (p3 - p2) * 3 are stored in start and end respectively.
 * I can't see a good reason for why this was done. Because of this, there is additional
 * mess required in the formulas for bezier curve splitting.
 */
struct PathDescrCubicTo : public PathDescr
{
  PathDescrCubicTo(Geom::Point const &pp, Geom::Point const &s, Geom::Point const& e)
    : PathDescr(descr_cubicto), p(pp), start(s), end(e) {}

  void dumpSVG(Inkscape::SVGOStringStream &s, Geom::Point const &last) const override;
  PathDescr *clone() const override;
  void dump(std::ostream &s) const override;

  Geom::Point p;     /*!< The final point of the bezier curve. */
  Geom::Point start; /*!< 3 * (p1 - p0) where p0 is the start point of the cubic bezier and
                       p1 is the first control point. */
  Geom::Point end;   /*!< 3 * (p3 - p2) where p3 is the final point of the cubic bezier and
                       p2 is the second control point. */
};

/**
 * Elliptical Arc path command.
 *
 * Exactly the equivalent of an SVG elliptical arc description.
 */
struct PathDescrArcTo : public PathDescr
{
  PathDescrArcTo(Geom::Point const &pp, double x, double y, double a, bool l, bool c)
    : PathDescr(descr_arcto), p(pp), rx(x), ry(y), angle(a), large(l), clockwise(c) {}

  void dumpSVG(Inkscape::SVGOStringStream &s, Geom::Point const &last) const override;
  PathDescr *clone() const override;
  void dump(std::ostream &s) const override;

  Geom::Point p;   /*!< The final point of the arc. */
  double rx;       /*!< The radius in the x direction. */
  double ry;       /*!< The radius in the y direction. */
  double angle;    /*!< The angle it makes with the x axis in degrees. TODO confirm that */
  bool large;      /*!< The large arc or the small one? */
  bool clockwise;  /*!< Clockwise arc or anti-clockwise one? */
};

/**
 * A forced point path command.
 *
 * TODO: This needs more research. Why is this useful and where?
 */
struct PathDescrForced : public PathDescr
{
  PathDescrForced() : PathDescr(descr_forced), p(0, 0) {}

  PathDescr *clone() const override;

  /* FIXME: not sure whether _forced should have a point associated with it;
  ** Path::ConvertForcedToMoveTo suggests that maybe it should.
  */
  Geom::Point p; /*!< The forced point itself? */
};


/**
 * Close Path instruction.
 */
struct PathDescrClose : public PathDescr
{
  PathDescrClose() : PathDescr(descr_close) {}

  void dumpSVG(Inkscape::SVGOStringStream &s, Geom::Point const &last) const override;
  PathDescr *clone() const override;

  /* FIXME: not sure whether _forced should have a point associated with it;
  ** Path::ConvertForcedToMoveTo suggests that maybe it should.
  */
  Geom::Point p; /*!< Useless since close instruction needs no point. */
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

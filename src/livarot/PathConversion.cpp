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

#include <glib.h>
#include <2geom/bezier.h>
#include <2geom/transforms.h>
#include "Path.h"
#include "Shape.h"
#include "livarot/path-description.h"

/*
 * path description -> polyline
 * and Path -> Shape (the Fill() function at the bottom)
 * nathing fancy here: take each command and append an approximation of it to the polyline
 */

void Path::ConvertWithBackData(double treshhold, bool relative)
{
    // are we doing a sub path? if yes, clear the flags. CloseSubPath just clears the flags
    // it doesn't close a sub path
    if (descr_flags & descr_doing_subpath) {
        CloseSubpath();
    }

    // set the backdata flag to true since this function will be calculating and storing backdata stuff
    SetBackData(true);
    // clears any pre-existing polyline approximation stuff
    ResetPoints();

    // nothing to approximate so return
    if (descr_cmd.empty()) {
        return;
    }

    Geom::Point curX; // the last point added
    // the description to process. We start with 1 usually since the first is always a MoveTo that
    // we handle before the loop (see below). In the case that the first is not a moveTo, We set
    // curP to 0.
    int curP = 1;
    // index of the last moveto point. Useful when a close path command is encountered.
    int lastMoveTo = -1;

    // The initial moveto.
    // if the first command is a moveTo, set that as the lastPoint (curX) otherwise add a point at
    // the origin as a moveTo.
    if (descr_cmd[0]->getType() == descr_moveto) {
        curX = static_cast<PathDescrMoveTo *>(descr_cmd[0])->p;
    } else {
        curP = 0;
    }
    // tiny detail to see here is that piece (the index of the path command this point comes from) is set to 0 which
    // may or may not be true. If there was not a MoveTo, index 0 can have other description types.
    lastMoveTo = AddPoint(curX, 0, 0.0, true);

    // Prepare the iterator which moves along forced_subdivisions as we iterate over path commands.
    auto cuts_it = forced_subdivisions.begin();
    std::vector<double> cuts;

    // And the rest, one by one.
    // within this loop, curP holds the current path command index, curX holds the last point added
    for (; curP < descr_cmd.size(); curP++) {
        Geom::Point nextX;

        // Collect the list of cut times for this path command, in ascending order.
        cuts.clear();
        while (cuts_it != forced_subdivisions.end() && cuts_it->piece == curP) {
            cuts.emplace_back(cuts_it->t);
            ++cuts_it;
        }

        switch (descr_cmd[curP]->getType()) {
            case descr_forced: {
                // just add a forced point (at the last point added).
                AddForcedPoint();
                break;
            }

            case descr_moveto: {
                auto cmd = static_cast<PathDescrMoveTo *>(descr_cmd[curP]);
                nextX = cmd->p;
                // add the moveTo point and also store this in lastMoveTo
                lastMoveTo = AddPoint(nextX, curP, 0.0, true);
                break;
            }

            case descr_close: {
                // add the lastMoveTo point again
                nextX = pts[lastMoveTo].p;
                int n = AddPoint(nextX, curP, 1.0, false);
                // we check if n > 0 because in some cases the last point has already been added so AddPoint would
                // return -1 .. but then that last point won't get marked with closed = true .. I wonder if that would cause
                // problems. Just to explain this say:
                // MoveTo(0, 0); LineTo(10, 0); LineTo(10, 10); LineTo(0, 10); LineTo(0, 0); Close();
                // LineTo(0, 0) would have already added a point at origin. So AddPoint won't add anything. But my point is that
                // then the last point (0,0) won't get marked as closed = true which it should be.
                // But then maybe this doesn't matter because closed variable is barely used.
                if (n > 0) pts[n].closed = true;
                break;
            }

            case descr_lineto: {
                auto cmd = static_cast<PathDescrLineTo *>(descr_cmd[curP]);
                nextX = cmd->p;
                AddPoint(nextX, curP, 1.0, false);
                break;
            }

            case descr_cubicto: {
                auto cmd = static_cast<PathDescrCubicTo *>(descr_cmd[curP]);
                nextX = cmd->p;
                // RecCubicTo will see if threshold is fine with approximating this cubic bezier with
                // a line segment through the start and end points. If no, it'd split the cubic at its
                // center point and recursively call itself on the left and right side. The center point
                // gets added in the points list too.
                RecCubicTo(curX, cmd->start, nextX, cmd->end, treshhold, 8, 0, 1, curP, relative, cuts);
                // RecCubicTo adds any points inside the cubic and last one is added here
                AddPoint(nextX, curP, 1.0, false);
                break;
            }

            case descr_arcto: {
                auto cmd = static_cast<PathDescrArcTo *>(descr_cmd[curP]);
                nextX = cmd->p;
                // Similar to RecCubicTo, just for Arcs
                DoArc(curX, nextX, cmd->rx, cmd->ry, cmd->angle, cmd->large, cmd->clockwise, treshhold, curP, relative, cuts);
                AddPoint(nextX, curP, 1.0, false);
                break;
            }
        }
        curX = nextX;
    }

    assert(cuts_it == forced_subdivisions.end());
}

void Path::Convert(double treshhold)
{
    if ( descr_flags & descr_doing_subpath ) {
        CloseSubpath();
    }

    SetBackData(false);
    ResetPoints();
    if ( descr_cmd.empty() ) {
        return;
    }

    Geom::Point curX;
    int curP = 1;
    int lastMoveTo = 0;

    // le moveto
    {
        int const firstTyp = descr_cmd[0]->getType();
        if ( firstTyp == descr_moveto ) {
            curX = dynamic_cast<PathDescrMoveTo *>(descr_cmd[0])->p;
        } else {
            curP = 0;
            curX[0] = curX[1] = 0;
        }
        lastMoveTo = AddPoint(curX, true);
    }
    descr_cmd[0]->associated = lastMoveTo;

    // et le reste, 1 par 1
    while ( curP < int(descr_cmd.size()) ) {

        int const nType = descr_cmd[curP]->getType();
        Geom::Point nextX;

        switch (nType) {
            case descr_forced: {
                descr_cmd[curP]->associated = AddForcedPoint();
                curP++;
                break;
            }

            case descr_moveto: {
                PathDescrMoveTo *nData = dynamic_cast<PathDescrMoveTo *>(descr_cmd[curP]);
                nextX = nData->p;
                lastMoveTo = AddPoint(nextX, true);
                descr_cmd[curP]->associated = lastMoveTo;

                // et on avance
                curP++;
                break;
            }

            case descr_close: {
                nextX = pts[lastMoveTo].p;
                descr_cmd[curP]->associated = AddPoint(nextX, false);
                if ( descr_cmd[curP]->associated < 0 ) {
                    if ( curP == 0 ) {
                        descr_cmd[curP]->associated = 0;
                    } else {
                        descr_cmd[curP]->associated = descr_cmd[curP - 1]->associated;
                    }
                }
                if ( descr_cmd[curP]->associated > 0 ) {
                    pts[descr_cmd[curP]->associated].closed = true;
                }
                curP++;
                break;
            }

            case descr_lineto: {
                PathDescrLineTo *nData = dynamic_cast<PathDescrLineTo *>(descr_cmd[curP]);
                nextX = nData->p;
                descr_cmd[curP]->associated = AddPoint(nextX, false);
                if ( descr_cmd[curP]->associated < 0 ) {
                    if ( curP == 0 ) {
                        descr_cmd[curP]->associated = 0;
                    } else {
                        descr_cmd[curP]->associated = descr_cmd[curP - 1]->associated;
                    }
                }
                // et on avance
                curP++;
                break;
            }

            case descr_cubicto: {
                PathDescrCubicTo *nData = dynamic_cast<PathDescrCubicTo *>(descr_cmd[curP]);
                nextX = nData->p;
                RecCubicTo(curX, nData->start, nextX, nData->end, treshhold, 8);
                descr_cmd[curP]->associated = AddPoint(nextX,false);
                if ( descr_cmd[curP]->associated < 0 ) {
                    if ( curP == 0 ) {
                        descr_cmd[curP]->associated = 0;
                    } else {
                        descr_cmd[curP]->associated = descr_cmd[curP - 1]->associated;
                    }
                }
                // et on avance
                curP++;
                break;
            }

            case descr_arcto: {
                PathDescrArcTo *nData = dynamic_cast<PathDescrArcTo *>(descr_cmd[curP]);
                nextX = nData->p;
                DoArc(curX, nextX, nData->rx, nData->ry, nData->angle, nData->large, nData->clockwise, treshhold);
                descr_cmd[curP]->associated = AddPoint(nextX, false);
                if ( descr_cmd[curP]->associated < 0 ) {
                    if ( curP == 0 ) {
                        descr_cmd[curP]->associated = 0;
                    } else {
                        descr_cmd[curP]->associated = descr_cmd[curP - 1]->associated;
                    }
                }
                // et on avance
                curP++;
                break;
            }
        }

        curX = nextX;
    }
}

void Path::ConvertEvenLines(double treshhold)
{
    if ( descr_flags & descr_doing_subpath ) {
        CloseSubpath();
    }

    SetBackData(false);
    ResetPoints();
    if ( descr_cmd.empty() ) {
        return;
    }

    Geom::Point curX;
    int curP = 1;
    int lastMoveTo = 0;

    // le moveto
    {
        int const firstTyp = descr_cmd[0]->getType();
        if ( firstTyp == descr_moveto ) {
            curX = dynamic_cast<PathDescrMoveTo *>(descr_cmd[0])->p;
        } else {
            curP = 0;
            curX[0] = curX[1] = 0;
        }
        lastMoveTo = AddPoint(curX, true);
    }
    descr_cmd[0]->associated = lastMoveTo;

    // et le reste, 1 par 1
    while ( curP < int(descr_cmd.size()) ) {

        int const nType = descr_cmd[curP]->getType();
        Geom::Point nextX;

        switch (nType) {
            case descr_forced: {
                descr_cmd[curP]->associated = AddForcedPoint();
                curP++;
                break;
            }

            case descr_moveto: {
                PathDescrMoveTo* nData = dynamic_cast<PathDescrMoveTo *>(descr_cmd[curP]);
                nextX = nData->p;
                lastMoveTo = AddPoint(nextX,true);
                descr_cmd[curP]->associated = lastMoveTo;

                // et on avance
                curP++;
                break;
            }

            case descr_close: {
                nextX = pts[lastMoveTo].p;
                {
                    Geom::Point nexcur;
                    nexcur = nextX - curX;
                    const double segL = Geom::L2(nexcur);
                    if ( (segL > treshhold) && (treshhold > 0) ) {
                        for (double i = treshhold; i < segL; i += treshhold) {
                            Geom::Point nX;
                            nX = (segL - i) * curX + i * nextX;
                            nX /= segL;
                            AddPoint(nX);
                        }
                    }
                }

                descr_cmd[curP]->associated = AddPoint(nextX,false);
                if ( descr_cmd[curP]->associated < 0 ) {
                    if ( curP == 0 ) {
                        descr_cmd[curP]->associated = 0;
                    } else {
                        descr_cmd[curP]->associated = descr_cmd[curP - 1]->associated;
                    }
                }
                if ( descr_cmd[curP]->associated > 0 ) {
                    pts[descr_cmd[curP]->associated].closed = true;
                }
                curP++;
                break;
            }

            case descr_lineto: {
                PathDescrLineTo* nData = dynamic_cast<PathDescrLineTo *>(descr_cmd[curP]);
                nextX = nData->p;
                Geom::Point nexcur = nextX - curX;
                const double segL = L2(nexcur);
                if ( (segL > treshhold) && (treshhold > 0)) {
                    for (double i = treshhold; i < segL; i += treshhold) {
                        Geom::Point nX = ((segL - i) * curX + i * nextX) / segL;
                        AddPoint(nX);
                    }
                }

                descr_cmd[curP]->associated = AddPoint(nextX,false);
                if ( descr_cmd[curP]->associated < 0 ) {
                    if ( curP == 0 ) {
                        descr_cmd[curP]->associated = 0;
                    } else {
                        descr_cmd[curP]->associated = descr_cmd[curP - 1]->associated;
                    }
                }
                // et on avance
                curP++;
                break;
            }

            case descr_cubicto: {
                PathDescrCubicTo *nData = dynamic_cast<PathDescrCubicTo *>(descr_cmd[curP]);
                nextX = nData->p;
                RecCubicTo(curX, nData->start, nextX, nData->end, treshhold, 8, 4 * treshhold);
                descr_cmd[curP]->associated = AddPoint(nextX, false);
                if ( descr_cmd[curP]->associated < 0 ) {
                    if ( curP == 0 ) {
                        descr_cmd[curP]->associated = 0;
                    } else {
                        descr_cmd[curP]->associated = descr_cmd[curP - 1]->associated;
                    }
                }
                // et on avance
                curP++;
                break;
            }

            case descr_arcto: {
                PathDescrArcTo *nData = dynamic_cast<PathDescrArcTo *>(descr_cmd[curP]);
                nextX = nData->p;
                DoArc(curX, nextX, nData->rx, nData->ry, nData->angle, nData->large, nData->clockwise, treshhold);
                descr_cmd[curP]->associated =AddPoint(nextX, false);
                if ( descr_cmd[curP]->associated < 0 ) {
                    if ( curP == 0 ) {
                        descr_cmd[curP]->associated = 0;
                    } else {
                        descr_cmd[curP]->associated = descr_cmd[curP - 1]->associated;
                    }
                }

                // et on avance
                curP++;
                break;
            }
        }
        if ( Geom::LInfty(curX - nextX) > 0.00001 ) {
            curX = nextX;
        }
    }
}

const Geom::Point Path::PrevPoint(int i) const
{
    /* TODO: I suspect this should assert `(unsigned) i < descr_nb'.  We can probably change
       the argument to unsigned.  descr_nb should probably be changed to unsigned too. */
    g_assert( i >= 0 );
    switch ( descr_cmd[i]->getType() ) {
        case descr_moveto: {
            PathDescrMoveTo *nData = dynamic_cast<PathDescrMoveTo *>(descr_cmd[i]);
            return nData->p;
        }
        case descr_lineto: {
            PathDescrLineTo *nData = dynamic_cast<PathDescrLineTo *>(descr_cmd[i]);
            return nData->p;
        }
        case descr_arcto: {
            PathDescrArcTo *nData = dynamic_cast<PathDescrArcTo *>(descr_cmd[i]);
            return nData->p;
        }
        case descr_cubicto: {
            PathDescrCubicTo *nData = dynamic_cast<PathDescrCubicTo *>(descr_cmd[i]);
            return nData->p;
        }
        case descr_close:
        case descr_forced:
            return PrevPoint(i - 1);
        default:
            g_assert_not_reached();
            return Geom::Point(0, 0);
    }
}

// idem for cubic bezier patch
void Path::CubicTangent(double t, Geom::Point &oPt, const Geom::Point &iS, const Geom::Point &isD,
                        const Geom::Point &iE, const Geom::Point &ieD)
{
    Geom::Point const ax = ieD - 2 * iE + 2 * iS + isD;
    Geom::Point const bx = 3 * iE - ieD - 2 * isD - 3 * iS;
    Geom::Point const cx = isD;

    oPt = 3 * t * t * ax + 2 * t * bx + cx;
}

// extract interesting info of a SVG arc description
static void ArcAnglesAndCenter(Geom::Point const &iS, Geom::Point const &iE,
			       double rx, double ry, double angle,
			       bool large, bool wise,
			       double &sang, double &eang, Geom::Point &dr);

void Path::ArcAngles(const Geom::Point &iS, const Geom::Point &iE,
                     double rx, double ry, double angle, bool large, bool wise, double &sang, double &eang)
{
    Geom::Point dr;
    ArcAnglesAndCenter(iS, iE, rx, ry, angle, large, wise, sang, eang, dr);
}

/* N.B. If iS == iE then sang,eang,dr each become NaN.  Probably a bug. */
static void ArcAnglesAndCenter(Geom::Point const &iS, Geom::Point const &iE,
                               double rx, double ry, double angle,
                               bool large, bool wise,
                               double &sang, double &eang, Geom::Point &dr)
{
    Geom::Point se = iE - iS;
    Geom::Point ca(cos(angle), sin(angle));
    Geom::Point cse(dot(ca, se), cross(ca, se));
    cse[0] /= rx;
    cse[1] /= ry;
    double const lensq = dot(cse,cse);
    Geom::Point csd = ( ( lensq < 4
                        ? sqrt( 1/lensq - .25 )
                        : 0.0 )
                      * cse.ccw() );

    Geom::Point ra = -csd - 0.5 * cse;
    if ( ra[0] <= -1 ) {
        sang = M_PI;
    } else if ( ra[0] >= 1 ) {
        sang = 0;
    } else {
        sang = acos(ra[0]);
        if ( ra[1] < 0 ) {
            sang = 2 * M_PI - sang;
        }
    }

    ra = -csd + 0.5 * cse;
    if ( ra[0] <= -1 ) {
        eang = M_PI;
    } else if ( ra[0] >= 1 ) {
        eang = 0;
    } else {
        eang = acos(ra[0]);
        if ( ra[1] < 0 ) {
            eang = 2 * M_PI - eang;
        }
    }

    csd[0] *= rx;
    csd[1] *= ry;
    ca[1] = -ca[1]; // because it's the inverse rotation

    dr[0] = dot(ca, csd);
    dr[1] = cross(ca, csd);

    ca[1] = -ca[1];

    if ( wise ) {

        if (large) {
            dr = -dr;
            double swap = eang;
            eang = sang;
            sang = swap;
            eang += M_PI;
            sang += M_PI;
            if ( eang >= 2*M_PI ) {
                eang -= 2*M_PI;
            }
            if ( sang >= 2*M_PI ) {
                sang -= 2*M_PI;
            }
        }

    } else {
        if (!large) {
            dr = -dr;
            double swap = eang;
            eang = sang;
            sang = swap;
            eang += M_PI;
            sang += M_PI;
            if ( eang >= 2*M_PI ) {
                eang -= 2 * M_PI;
            }
            if ( sang >= 2*M_PI ) {
                sang -= 2 * M_PI;
            }
        }
    }

    dr += 0.5 * (iS + iE);
}



void Path::DoArc(Geom::Point const &iS, Geom::Point const &iE,
                 double const rx, double const ry, double const angle,
                 bool const large, bool const wise, double const tresh)
{
    /* TODO: Check that our behaviour is standards-conformant if iS and iE are (much) further
       apart than the diameter.  Also check that we do the right thing for negative radius.
       (Same for the other DoArc functions in this file.) */
    if ( rx <= 0.0001 || ry <= 0.0001 || tresh <= 1e-8) {
        return;
        // We always add a lineto afterwards, so this is fine.
        // [on ajoute toujours un lineto apres, donc c bon]
    }

    double sang;
    double eang;
    Geom::Point dr_temp;
    ArcAnglesAndCenter(iS, iE, rx, ry, angle*M_PI/180.0, large, wise, sang, eang, dr_temp);
    Geom::Point dr = dr_temp;
    /* TODO: This isn't as good numerically as treating iS and iE as primary.  E.g. consider
       the case of low curvature (i.e. very large radius). */

    Geom::Scale const ar(rx, ry);
    Geom::Rotate cb(sang);
    Geom::Rotate cbangle(angle*M_PI/180.0);
    double max_ang = 2 * acos ( 1 - tresh / (fmax(rx, ry) ) );
    max_ang = fmin (max_ang, M_PI / 2 );
    int const num_sectors = abs(sang - eang) / max_ang + 1;

    if (wise) {


        if ( sang < eang ) {
            sang += 2*M_PI;
        }
        double const incr = (eang - sang) / num_sectors;
        Geom::Rotate const omega(incr);
        for (double b = sang + incr ; b > eang ; b += incr) {
            cb = omega * cb;
            AddPoint( cb.vector() * ar * cbangle + dr );
        }

    } else {

        if ( sang > eang ) {
            sang -= 2*M_PI;
        }
        double const incr = (eang - sang) / num_sectors;
        Geom::Rotate const omega(incr);
        for (double b = sang + incr ; b < eang ; b += incr) {
            cb = omega * cb;
            AddPoint( cb.vector() * ar * cbangle + dr);
        }
    }
}


void Path::RecCubicTo( Geom::Point const &iS, Geom::Point const &isD,
                       Geom::Point const &iE, Geom::Point const &ieD,
                       double tresh, int lev, double maxL)
{
    // vector from start to end point
    Geom::Point se = iE - iS;
    // length of that vector
    const double dC = Geom::L2(se);
    // if the vector from start to end point is smaller than 0.01
    if ( dC < 0.01 ) {
        // we still need to get an idea of how far away the curve goes from the start to end line segment se
        // for that, we measure lengths of isD and ieD
        const double sC = dot(isD,isD);
        const double eC = dot(ieD,ieD);
        // if they are limited by tresh, great
        if ( sC < tresh && eC < tresh ) {
            return;
        }
        // otherwise proceed

    } else {
        // okay so length is greater than or equal to 0.01, we can still check the perpendicular component
        // of the control handles and see if they are limited by tresh
        const double sC = fabs(cross(se, isD)) / dC;
        const double eC = fabs(cross(se, ieD)) / dC;
        if ( sC < tresh && eC < tresh ) {
            // presque tt droit -> attention si on nous demande de bien subdiviser les petits segments
            // if the perpendicular is limited and a maxL is set, check if maxL is being respected, if yes
            // return otherwise we split
            if ( maxL > 0 && dC > maxL ) {
                if ( lev <= 0 ) {
                    return;
                }
                // maths for splitting one cubic bezier into two
                Geom::Point m = 0.5 * (iS + iE) + 0.125 * (isD - ieD);
                Geom::Point md = 0.75 * (iE - iS) - 0.125 * (isD + ieD);

                Geom::Point hisD = 0.5 * isD;
                Geom::Point hieD = 0.5 * ieD;

                RecCubicTo(iS, hisD, m, md, tresh, lev - 1, maxL);
                AddPoint(m);
                RecCubicTo(m, md, iE, hieD, tresh, lev - 1,maxL);
            }
            return;
        }
    }

    if ( lev <= 0 ) {
        return;
    }

    {
        Geom::Point m = 0.5 * (iS + iE) + 0.125 * (isD - ieD);
        Geom::Point md = 0.75 * (iE - iS) - 0.125 * (isD + ieD);

        Geom::Point hisD = 0.5 * isD;
        Geom::Point hieD = 0.5 * ieD;

        RecCubicTo(iS, hisD, m, md, tresh, lev - 1, maxL);
        AddPoint(m);
        RecCubicTo(m, md, iE, hieD, tresh, lev - 1,maxL);
    }
}

void Path::DoArc(Geom::Point const &iS, Geom::Point const &iE,
                 double const rx, double const ry, double const angle,
                 bool const large, bool const wise, double const thresh, int const piece,
                 bool relative, std::vector<double> const &cuts)
{
    /* TODO: Check that our behaviour is standards-conformant if iS and iE are (much) further
       apart than the diameter.  Also check that we do the right thing for negative radius.
       (Same for the other DoArc functions in this file.) */
    if (rx <= 0.0001 || ry <= 0.0001 || thresh <= 1e-8) {
        return;
        // We always add a lineto afterwards, so this is fine.
        // [on ajoute toujours un lineto apres, donc c bon]
    }

    auto const angle_rad = Geom::rad_from_deg(angle);
    auto const angle_rot = Geom::Rotate(angle_rad);

    double sang;
    double eang;
    Geom::Point center;
    ArcAnglesAndCenter(iS, iE, rx, ry, angle_rad, large, wise, sang, eang, center);
    /* TODO: This isn't as good numerically as treating iS and iE as primary.  E.g. consider
       the case of low curvature (i.e. very large radius). */

    if (wise) {
        if (sang < eang) {
            sang += 2 * M_PI;
        }
    } else {
        if (sang > eang) {
            sang -= 2 * M_PI;
        }
    }

    // max angle is basically the maximum arc angle you can have that won't create
    // an arc that exceeds the threshold
    auto const max_ang = std::min(relative
                                    ? std::sqrt(thresh) / 2
                                    : 2 * std::acos(1 - thresh / std::max(rx, ry)),
                                  M_PI / 2);

    auto rot = Geom::Rotate(sang);

    auto arc = [&, this] (double st, double et, bool last) {
        // divide the arc range into sectors such that each sector
        // is no bigger than max_ang
        int const num_sectors = std::ceil(std::abs(eang - sang) * (et - st) / max_ang);

        auto const dt = (et - st) / num_sectors;
        auto const drot = Geom::Rotate((eang - sang) * dt);

        auto t = st;

        for (int i = 1; i < num_sectors + last; i++) {
            t += dt;
            rot *= drot;
            AddPoint(rot.vector() * Geom::Scale(rx, ry) * angle_rot + center, piece, t);
        }
    };

    double prev = 0.0;
    for (auto c : cuts) {
        arc(prev, c, true);
        prev = c;
    }
    arc(prev, 1.0, false);
}

void Path::RecCubicTo(Geom::Point const &iS, Geom::Point const &isD,
                      Geom::Point const &iE, Geom::Point const &ieD,
                      double thresh, int lev, double st, double et, int piece,
                      bool relative, std::span<double> const &cuts)
{
    if (cuts.empty()) {
        RecCubicTo(iS, isD, iE, ieD, thresh, lev, st, et, piece, relative);
        return;
    }

    int const mid = cuts.size() / 2;
    double const midt = cuts[mid];
    double const midt_scaled = (midt - st) / (et - st);

    auto split = [&] (std::array<double, 4> &left, std::array<double, 4> &right, Geom::Dim2 dim) {
        auto const in = std::array<double, 4>{iS[dim], iS[dim] + isD[dim] / 3, iE[dim] - ieD[dim] / 3, iE[dim]};
        Geom::casteljau_subdivision(midt_scaled, in.data(), left.data(), right.data(), 3);
    };
    
    std::array<double, 4> xsl, ysl, xsr, ysr;
    split(xsl, xsr, Geom::X);
    split(ysl, ysr, Geom::Y);
    
    auto l = [&] (int i) { return Geom::Point(xsl[i], ysl[i]); };
    auto r = [&] (int i) { return Geom::Point(xsr[i], ysr[i]); };
    
    auto cutsl = cuts.subspan(0, mid);
    auto cutsr = cuts.subspan(mid + 1);
    
    RecCubicTo(l(0), 3 * (l(1) - l(0)), l(3), 3 * (l(3) - l(2)), thresh, lev, st, midt, piece, relative, cutsl);
    AddPoint(r(0), piece, midt);
    RecCubicTo(r(0), 3 * (r(1) - r(0)), r(3), 3 * (r(3) - r(2)), thresh, lev, midt, et, piece, relative, cutsr);
}

void Path::RecCubicTo(Geom::Point const &iS, Geom::Point const &isD,
                      Geom::Point const &iE, Geom::Point const &ieD,
                      double tresh, int lev, double st, double et, int piece,
                      bool relative)
{
    if (lev <= 0) {
        return;
    }

    auto const se = iE - iS;
    auto const y1 = std::abs(Geom::cross(isD, se));
    auto const y2 = std::abs(Geom::cross(ieD, se));
    if (relative) {
        auto const bound = se.lengthSq() * tresh;
        if (y1 < bound && y2 < bound) {
            return;
        }
    } else if (se.lengthSq() < Geom::sqr(0.01)) {
        if (isD.lengthSq() < tresh && ieD.lengthSq() < tresh) {
            return;
        }
    } else {
        auto const bound = se.length() * tresh;
        if (y1 < bound && y2 < bound) {
            return;
        }
    }

    Geom::Point m = 0.5 * (iS + iE) + 0.125 * (isD - ieD);
    Geom::Point md = 0.75 * (iE - iS) - 0.125 * (isD + ieD);
    double mt = (st + et) / 2;

    Geom::Point hisD = 0.5 * isD;
    Geom::Point hieD = 0.5 * ieD;

    RecCubicTo(iS, hisD, m, md, tresh, lev - 1, st, mt, piece, relative);
    AddPoint(m, piece, mt);
    RecCubicTo(m, md, iE, hieD, tresh, lev - 1, mt, et, piece, relative);
}

/*
 * put a polyline in a Shape instance, for further fun
 * pathID is the ID you want this Path instance to be associated with, for when you're going to recompose the polyline
 * in a path description ( you need to have prepared the back data for that, of course)
 */

void Path::Fill(Shape* dest, int pathID, bool justAdd, bool closeIfNeeded, bool invert)
{
    if ( dest == nullptr ) {
        return;
    }

    if ( justAdd == false ) {
        dest->Reset(pts.size(), pts.size());
    }

    if ( pts.size() <= 1 ) {
        return;
    }

    int first = dest->numberOfPoints();

    if ( back ) {
        dest->MakeBackData(true);
    }

    if ( invert ) {
        if ( back ) {
            {
                // invert && back && !weighted
                for (auto & pt : pts) {
                    dest->AddPoint(pt.p);
                }
                int lastM = 0;
                int curP = 1;
                int pathEnd = 0;
                bool closed = false;
                int lEdge = -1;

                while ( curP < int(pts.size()) ) {
                    int sbp = curP;
                    int lm = lastM;
                    int prp = pathEnd;

                    if ( pts[sbp].isMoveTo == polyline_moveto ) {

                        if ( closeIfNeeded ) {
                            if ( closed && lEdge >= 0 ) {
                                dest->DisconnectStart(lEdge);
                                dest->ConnectStart(first + lastM, lEdge);
                            } else {
                                lEdge = dest->AddEdge(first + lastM, first+pathEnd);
                                if ( lEdge >= 0 ) {
                                    dest->ebData[lEdge].pathID = pathID;
                                    dest->ebData[lEdge].pieceID = pts[lm].piece;
                                    dest->ebData[lEdge].tSt = 1.0;
                                    dest->ebData[lEdge].tEn = 0.0;
                                }
                            }
                        }

                        lastM = curP;
                        pathEnd = curP;
                        closed = false;
                        lEdge = -1;

                    } else {

                        if ( Geom::LInfty(pts[sbp].p - pts[prp].p) >= 0.00001 ) {
                            lEdge = dest->AddEdge(first + curP, first + pathEnd);
                            if ( lEdge >= 0 ) {
                                dest->ebData[lEdge].pathID = pathID;
                                dest->ebData[lEdge].pieceID = pts[sbp].piece;
                                if ( pts[sbp].piece == pts[prp].piece ) {
                                    dest->ebData[lEdge].tSt = pts[sbp].t;
                                    dest->ebData[lEdge].tEn = pts[prp].t;
                                } else {
                                    dest->ebData[lEdge].tSt = pts[sbp].t;
                                    dest->ebData[lEdge].tEn = 0.0;
                                }
                            }
                            pathEnd = curP;
                            if ( Geom::LInfty(pts[sbp].p - pts[lm].p) < 0.00001 ) {
                                closed = true;
                            } else {
                                closed = false;
                            }
                        }
                    }

                    curP++;
                }

                if ( closeIfNeeded ) {
                    if ( closed && lEdge >= 0 ) {
                        dest->DisconnectStart(lEdge);
                        dest->ConnectStart(first + lastM, lEdge);
                    } else {
                        int lm = lastM;
                        lEdge = dest->AddEdge(first + lastM, first + pathEnd);
                        if ( lEdge >= 0 ) {
                            dest->ebData[lEdge].pathID = pathID;
                            dest->ebData[lEdge].pieceID = pts[lm].piece;
                            dest->ebData[lEdge].tSt = 1.0;
                            dest->ebData[lEdge].tEn = 0.0;
                        }
                    }
                }
            }

        } else {

            {
                // invert && !back && !weighted
                for (auto & pt : pts) {
                    dest->AddPoint(pt.p);
                }
                int lastM = 0;
                int curP = 1;
                int pathEnd = 0;
                bool closed = false;
                int lEdge = -1;
                while ( curP < int(pts.size()) ) {
                    int sbp = curP;
                    int lm = lastM;
                    int prp = pathEnd;
                    if ( pts[sbp].isMoveTo == polyline_moveto ) {
                        if ( closeIfNeeded ) {
                            if ( closed && lEdge >= 0 ) {
                                dest->DisconnectStart(lEdge);
                                dest->ConnectStart(first + lastM, lEdge);
                            } else {
                                dest->AddEdge(first + lastM, first + pathEnd);
                            }
                        }
                        lastM = curP;
                        pathEnd = curP;
                        closed = false;
                        lEdge = -1;
                    } else {
                        if ( Geom::LInfty(pts[sbp].p - pts[prp].p) >= 0.00001 ) {
                            lEdge = dest->AddEdge(first+curP, first+pathEnd);
                            pathEnd = curP;
                            if ( Geom::LInfty(pts[sbp].p - pts[lm].p) < 0.00001 ) {
                                closed = true;
                            } else {
                                closed = false;
                            }
                        }
                    }
                    curP++;
                }

                if ( closeIfNeeded ) {
                    if ( closed && lEdge >= 0 ) {
                        dest->DisconnectStart(lEdge);
                        dest->ConnectStart(first + lastM, lEdge);
                    } else {
                        dest->AddEdge(first + lastM, first + pathEnd);
                    }
                }

            }
        }

    } else {

        if ( back ) {
            {
                // !invert && back && !weighted

                // add all points to the shape
                for (auto & pt : pts) {
                    dest->AddPoint(pt.p);
                }

                int lastM = 0;
                int curP = 1;
                int pathEnd = 0;
                bool closed = false;
                int lEdge = -1;
                while ( curP < int(pts.size()) ) {
                    int sbp = curP;
                    int lm = lastM;
                    int prp = pathEnd;
                    if ( pts[sbp].isMoveTo == polyline_moveto ) {
                        if ( closeIfNeeded ) {
                            if ( closed && lEdge >= 0 ) {
                                dest->DisconnectEnd(lEdge);
                                dest->ConnectEnd(first + lastM, lEdge);
                            } else {
                                lEdge = dest->AddEdge(first + pathEnd, first+lastM);
                                if ( lEdge >= 0 ) {
                                    dest->ebData[lEdge].pathID = pathID;
                                    dest->ebData[lEdge].pieceID = pts[lm].piece;
                                    dest->ebData[lEdge].tSt = 0.0;
                                    dest->ebData[lEdge].tEn = 1.0;
                                }
                            }
                        }
                        lastM = curP;
                        pathEnd = curP;
                        closed = false;
                        lEdge = -1;
                    } else {
                        if ( Geom::LInfty(pts[sbp].p - pts[prp].p) >= 0.00001 ) {
                            lEdge = dest->AddEdge(first + pathEnd, first + curP);
                            dest->ebData[lEdge].pathID = pathID;
                            dest->ebData[lEdge].pieceID = pts[sbp].piece;
                            if ( pts[sbp].piece == pts[prp].piece ) {
                                dest->ebData[lEdge].tSt = pts[prp].t;
                                dest->ebData[lEdge].tEn = pts[sbp].t;
                            } else {
                                dest->ebData[lEdge].tSt = 0.0;
                                dest->ebData[lEdge].tEn = pts[sbp].t;
                            }
                            pathEnd = curP;
                            if ( Geom::LInfty(pts[sbp].p - pts[lm].p) < 0.00001 ) {
                                closed = true;
                            } else {
                                closed = false;
                            }
                        }
                    }
                    curP++;
                }

                if ( closeIfNeeded ) {
                    if ( closed && lEdge >= 0 ) {
                        dest->DisconnectEnd(lEdge);
                        dest->ConnectEnd(first + lastM, lEdge);
                    } else {
                        int lm = lastM;
                        lEdge = dest->AddEdge(first + pathEnd, first + lastM);
                        if ( lEdge >= 0 ) {
                            dest->ebData[lEdge].pathID = pathID;
                            dest->ebData[lEdge].pieceID = pts[lm].piece;
                            dest->ebData[lEdge].tSt = 0.0;
                            dest->ebData[lEdge].tEn = 1.0;
                        }
                    }
                }
            }

        } else {
            {
                // !invert && !back && !weighted
                for (auto & pt : pts) {
                    dest->AddPoint(pt.p);
                }

                int lastM = 0;
                int curP = 1;
                int pathEnd = 0;
                bool closed = false;
                int lEdge = -1;
                while ( curP < int(pts.size()) ) {
                    int sbp = curP;
                    int lm = lastM;
                    int prp = pathEnd;
                    if ( pts[sbp].isMoveTo == polyline_moveto ) {
                        if ( closeIfNeeded ) {
                            if ( closed && lEdge >= 0 ) {
                                dest->DisconnectEnd(lEdge);
                                dest->ConnectEnd(first + lastM, lEdge);
                            } else {
                                dest->AddEdge(first + pathEnd, first + lastM);
                            }
                        }
                        lastM = curP;
                        pathEnd = curP;
                        closed = false;
                        lEdge = -1;
                    } else {
                        if ( Geom::LInfty(pts[sbp].p - pts[prp].p) >= 0.00001 ) {
                            lEdge = dest->AddEdge(first+pathEnd, first+curP);
                            pathEnd = curP;
                            if ( Geom::LInfty(pts[sbp].p - pts[lm].p) < 0.00001 ) {
                                closed = true;
                            } else {
                                closed = false;
                            }
                        }
                    }
                    curP++;
                }

                if ( closeIfNeeded ) {
                    if ( closed && lEdge >= 0 ) {
                        dest->DisconnectEnd(lEdge);
                        dest->ConnectEnd(first + lastM, lEdge);
                    } else {
                        dest->AddEdge(first + pathEnd, first + lastM);
                    }
                }

            }
        }
    }
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

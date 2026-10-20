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

#include <memory>
#include <glib.h>
#include <2geom/affine.h>
#include "livarot/Path.h"
#include "livarot/path-description.h"

/*
 * Reassembling polyline segments into cubic bezier patches
 * thes functions do not need the back data. but they are slower than recomposing
 * path descriptions when you have said back data (it's always easier with a model)
 * there's a bezier fitter in bezier-utils.cpp too. the main difference is the way bezier patch are split
 * here: walk on the polyline, trying to extend the portion you can fit by respecting the treshhold, split when 
 * treshhold is exceeded. when encountering a "forced" point, lower the treshhold to favor splitting at that point
 * in bezier-utils: fit the whole polyline, get the position with the higher deviation to the fitted curve, split
 * there and recurse
 */


// algo d'origine: http://www.cs.mtu.edu/~shene/COURSES/cs3621/NOTES/INT-APP/CURVE-APP-global.html

// need the b-spline basis for cubic splines
// pas oublier que c'est une b-spline clampee
// et que ca correspond a une courbe de bezier normale
#define N03(t) ((1.0-t)*(1.0-t)*(1.0-t))
#define N13(t) (3*t*(1.0-t)*(1.0-t))
#define N23(t) (3*t*t*(1.0-t))
#define N33(t) (t*t*t)
// quadratic b-splines (jsut in case)
#define N02(t) ((1.0-t)*(1.0-t))
#define N12(t) (2*t*(1.0-t))
#define N22(t) (t*t)
// linear interpolation b-splines
#define N01(t) ((1.0-t))
#define N11(t) (t)



void Path::Simplify(double treshhold)
{
    // There is nothing to fit if you have 0 to 1 points
    if (pts.size() <= 1) {
        return;
    }
    
    // clear all existing path descriptions
    Reset();
  
    // each path (where a path is a MoveTo followed by one or more LineTo) is fitted on separately
    // Say you had M L L L L M L L L M L L L L
    // pattern     --------  ------- ---------
    //              path 1    path 2  path 3
    // Each would be simplified individually.

    int lastM = 0; // index of the lastMove
    while (lastM < int(pts.size())) {
        int lastP = lastM + 1; // last point
        while (lastP < int(pts.size())
               && (pts[lastP].isMoveTo == polyline_lineto
                   || pts[lastP].isMoveTo == polyline_forced)) // if it's a LineTo or a forcedPoint we move forward
        {
            lastP++;
        }
        // we would only come out of the above while loop when pts[lastP] becomes a MoveTo or we
        // run out of points
        // M L L L L L
        // 0 1 2 3 4 5 6 <-- we came out from loop here
        // lastM = 0; lastP = 6; lastP - lastM = 6;
        // We pass the first point the algorithm should start at (lastM) and the total number of
        // points (lastP - lastM)
        DoSimplify(lastM, lastP - lastM, treshhold);

        lastM = lastP;
    }
}


#if 0
// dichomtomic method to get distance to curve approximation
// a real polynomial solver would get the minimum more efficiently, but since the polynom
// would likely be of degree >= 5, that would imply using some generic solver, liek using the sturm method
static double RecDistanceToCubic(Geom::Point const &iS, Geom::Point const &isD,
                                 Geom::Point const &iE, Geom::Point const &ieD,
                                 Geom::Point &pt, double current, int lev, double st, double et)
{
    if ( lev <= 0 ) {
        return current;
    }
	
    Geom::Point const m = 0.5 * (iS + iE) + 0.125 * (isD - ieD);
    Geom::Point const md = 0.75 * (iE - iS) - 0.125 * (isD + ieD);
    double const mt = (st + et) / 2;
	
    Geom::Point const hisD = 0.5 * isD;
    Geom::Point const hieD = 0.5 * ieD;
	
    Geom::Point const mp = pt - m;
    double nle = Geom::dot(mp, mp);
    
    if ( nle < current ) {

        current = nle;
        nle = RecDistanceToCubic(iS, hisD, m, md, pt, current, lev - 1, st, mt);
        if ( nle < current ) {
            current = nle;
        }
        nle = RecDistanceToCubic(m, md, iE, hieD, pt, current, lev - 1, mt, et);
        if ( nle < current ) {
            current = nle;
        }
        
    } else if ( nle < 2 * current ) {

        nle = RecDistanceToCubic(iS, hisD, m, md, pt, current, lev - 1, st, mt);
        if ( nle < current ) {
            current = nle;
        }
        nle = RecDistanceToCubic(m, md, iE, hieD, pt, current, lev - 1, mt, et);
        if ( nle < current ) {
            current = nle;
        }
    }
    
    return current;
}
#endif

/**
 * Smallest distance from a point to a line.
 *
 * You might think this function calculates the distance from a CubicBezier to a point? Neh, it
 * doesn't do that. Firstly, this function doesn't care at all about a CubicBezier, as you can see
 * res.start and res.end are not even used in the function body. It calculates the shortest
 * possible distance to get from the point pt to the line from start to res.p.
 * There two possibilities so let me illustrate:
 * Case 1:
 *
 *              o (pt)
 *              |
 *              |   <------ returns this distance
 *              |
 * o-------------------------o
 * start                    res.p
 *
 * Case 2:
 *
 *        o (pt)
 *         -
 *          -
 *           - <--- returns this distance
 *            -
 *             -
 *              o-------------------------o
 *              start                    res.p
 * if the line is defined by l(t) over 0 <= t <= 1, this distance between pt and any point on line
 * l(t) is defined as |l(t) - pt|. This function returns the minimum of this function over t.
 *
 * @param start The start point of the cubic Bezier.
 * @param res The cubic Bezier command which we really don't care about. We only use res.p.
 * @param pt The point to measure the distance from.
 *
 * @return See the comments above.
 */
static double DistanceToCubic(Geom::Point const &start, PathDescrCubicTo res, Geom::Point &pt)
{
    Geom::Point const sp = pt - start;
    Geom::Point const ep = pt - res.p;
    double nle = Geom::dot(sp, sp);
    double nnle = Geom::dot(ep, ep);
    if ( nnle < nle ) {
        nle = nnle;
    }
    
    Geom::Point seg = res.p - start;
    nnle = Geom::cross(sp, seg);
    nnle *= nnle;
    nnle /= Geom::dot(seg, seg);
    if ( nnle < nle ) {
        if ( Geom::dot(sp,seg) >= 0 ) {
            seg = start - res.p;
            if ( Geom::dot(ep,seg) >= 0 ) {
                nle = nnle;
            }
        }
    }
    
    return nle;
}


/**
 *    Simplification on a subpath.
 */

void Path::DoSimplify(int off, int N, double treshhold)
{
  // non-dichotomic method: grow an interval of points approximated by a curve, until you reach the treshhold, and repeat
    // nothing to do with one/zero point(s).
    if (N <= 1) {
        return;
    }
    
    int curP = 0;
  
    fitting_tables data;
    data.Xk = data.Yk = data.Qk = nullptr;
    data.tk = data.lk = nullptr;
    data.fk = nullptr;
    data.totLen = 0;
    data.nbPt = data.maxPt = data.inPt = 0;
  
    // MoveTo to the first point
    Geom::Point const moveToPt = pts[off].p;
    MoveTo(moveToPt);
    // endToPt stores the last point of each cubic bezier patch (or line segment) that we add
    Geom::Point endToPt = moveToPt;
  
    // curP is a local index and we add the offset (off) in it to get the real
    // index. The loop has N - 1 instead of N because there is no point starting
    // the fitting process on the last point
    while (curP < N - 1) {
        // lastP becomes the lastPoint to fit on, basically, we wanna try fitting
        // on a sequence of points that start with curP and ends at lastP
        // We start with curP being 0 (the first point) and lastP being 1 (the second point)
        // M holds the total number of points we are trying to fix
        int lastP = curP + 1;
        int M = 2;

        // remettre a zero
        data.inPt = data.nbPt = 0;

        // a cubic bezier command that will be the patch fitted, which will be appended to the list
        // of path commands
        PathDescrCubicTo res(Geom::Point(0, 0), Geom::Point(0, 0), Geom::Point(0, 0));
        // a flag to indicate if there is a forced point in the current sequence that
        // we are trying to fit
        bool contains_forced = false;
        // the fitting code here is called in a binary search fashion, you can say
        // that we have fixed the start point for our fitting sequence (curP) and
        // need the highest possible endpoint (lastP) (highest in index) such
        // that the threshold is respected.
        // To do this search, we add "step" number of points to the current sequence
        // and fit, if successful, we add "step" more points, if not, we go back to
        // the last sequence of points, divide step by 2 and try extending again, this
        // process repeats until step becomes 0.
        // One more point to mention is how forced points are handled. Once lastP is
        // at a forced point, threshold will not be happy with any point after lastP, thus,
        // step will slowly reduce to 0, ultimately finishing the patch at the forced point.
        int step = 64;
        while ( step > 0 ) {   
            int forced_pt = -1;
            int worstP = -1;
            // this loop attempts to fit and if the threshold was fine with our fit, we
            // add "step" more points to the sequence that we are fitting on
            do {
                // if the point if forced, we set the flag, basically if there is a forced
                // point in the sequence (anywhere), this code will trigger at some point (I think)
                if (pts[off + lastP].isMoveTo == polyline_forced) {
                    contains_forced = true;
                }
                forced_pt = lastP; // store the forced point (any regular point also gets stored :/)
                lastP += step; // move the end marker by + step
                M += step; // add "step" to number of points we are trying to fit
                // the loop breaks if we either ran out of boundaries or the threshold didn't like
                // the fit
            } while (lastP < N && ExtendFit(off + curP, M, data,
                                            (contains_forced) ? 0.05 * treshhold : treshhold, // <-- if the last point here is a forced one we
                                            res, worstP) ); // make the threshold really strict so it'll definitely complain about the fit thus
                                                            // favoring us to stop and go back by "step" units
            // did we go out of boundaries?
            if (lastP >= N) {
                lastP -= step; // okay, come back by "step" units
                M -= step;
            } else { // the threshold complained
                // le dernier a echoue
                lastP -= step; // come back by "step" units
                M -= step;
                
                if ( contains_forced ) {
                    lastP = forced_pt; // we ensure that we are back at "forced" point.
                    M = lastP - curP + 1;
                }

                // fit stuff again (so we save the results in res); Threshold shouldn't complain
                // with this btw
                AttemptSimplify(off + curP, M, treshhold, res, worstP);       // ca passe forcement
            }
            step /= 2; // divide step by 2
        }
    
        // mark lastP as the end point of the sequence we are fitting on
        endToPt = pts[off + lastP].p;
        // add a patch, for two points a line else a cubic bezier, res has already been calculated
        // by AttemptSimplify
        if (M <= 2) {
            LineTo(endToPt);
        } else {
            CubicTo(endToPt, res.start, res.end);
        }
        // next patch starts where this one ended
        curP = lastP;
    }
  
    // if the last point that we added is very very close to the first one, it's a loop so close
    // it.
    if (Geom::LInfty(endToPt - moveToPt) < 0.00001) {
        Close();
    }
  
    g_free(data.Xk);
    g_free(data.Yk);
    g_free(data.Qk);
    g_free(data.tk);
    g_free(data.lk);
    g_free(data.fk);
}


// warning: slow
// idea behind this feature: splotches appear when trying to fit a small number of points: you can
// get a cubic bezier that fits the points very well but doesn't fit the polyline itself
// so we add a bit of the error at the middle of each segment of the polyline
// also we restrict this to <=20 points, to avoid unnecessary computations
#define with_splotch_killer

// primitive= calc the cubic bezier patche that fits Xk and Yk best
// Qk est deja alloue
// retourne false si probleme (matrice non-inversible)
bool Path::FitCubic(Geom::Point const &start, PathDescrCubicTo &res,
                    double *Xk, double *Yk, double *Qk, double *tk, int nbPt)
{
    Geom::Point const end = res.p;
    
    // la matrice tNN
    Geom::Affine M(0, 0, 0, 0, 0, 0);
    for (int i = 1; i < nbPt - 1; i++) {
        M[0] += N13(tk[i]) * N13(tk[i]);
        M[1] += N23(tk[i]) * N13(tk[i]);
        M[2] += N13(tk[i]) * N23(tk[i]);
        M[3] += N23(tk[i]) * N23(tk[i]);
    }
  
    double const det = M.det();
    if (fabs(det) < 0.000001) {
        res.start[0]=res.start[1]=0.0;
        res.end[0]=res.end[1]=0.0;
        return false;
    }
    
    Geom::Affine const iM = M.inverse();
    M = iM;
  
    // phase 1: abcisses
    // calcul des Qk
    Xk[0] = start[0];
    Yk[0] = start[1];
    Xk[nbPt - 1] = end[0];
    Yk[nbPt - 1] = end[1];
  
    for (int i = 1; i < nbPt - 1; i++) {
        Qk[i] = Xk[i] - N03 (tk[i]) * Xk[0] - N33 (tk[i]) * Xk[nbPt - 1];
    }
  
    // le vecteur Q
    Geom::Point Q(0, 0);
    for (int i = 1; i < nbPt - 1; i++) {
        Q[0] += N13 (tk[i]) * Qk[i];
        Q[1] += N23 (tk[i]) * Qk[i];
    }
  
    Geom::Point P = Q * M;
    Geom::Point cp1;
    Geom::Point cp2;
    cp1[Geom::X] = P[Geom::X];
    cp2[Geom::X] = P[Geom::Y];
  
    // phase 2: les ordonnees
    for (int i = 1; i < nbPt - 1; i++) {
        Qk[i] = Yk[i] - N03 (tk[i]) * Yk[0] - N33 (tk[i]) * Yk[nbPt - 1];
    }
  
    // le vecteur Q
    Q = Geom::Point(0, 0);
    for (int i = 1; i < nbPt - 1; i++) {
        Q[0] += N13 (tk[i]) * Qk[i];
        Q[1] += N23 (tk[i]) * Qk[i];
    }
  
    P = Q * M;
    cp1[Geom::Y] = P[Geom::X];
    cp2[Geom::Y] = P[Geom::Y];
  
    res.start = 3.0 * (cp1 - start);
    res.end = 3.0 * (end - cp2 );

    return true;
}


bool Path::ExtendFit(int off, int N, fitting_tables &data, double treshhold, PathDescrCubicTo &res, int &worstP)
{
    // if N is greater or equal to data.maxPt, reallocate total of 2*N+1 and copy existing data to
    // those arrays
    if ( N >= data.maxPt ) {
        data.maxPt = 2 * N + 1;
        data.Xk = (double *) g_realloc(data.Xk, data.maxPt * sizeof(double));
        data.Yk = (double *) g_realloc(data.Yk, data.maxPt * sizeof(double));
        data.Qk = (double *) g_realloc(data.Qk, data.maxPt * sizeof(double));
        data.tk = (double *) g_realloc(data.tk, data.maxPt * sizeof(double));
        data.lk = (double *) g_realloc(data.lk, data.maxPt * sizeof(double));
        data.fk = (char *) g_realloc(data.fk, data.maxPt * sizeof(char));
    }
    
    // is N greater than data.inPt? data.inPt seems to hold the total number of points stored in X,
    // Y, fk of data and thus, we add those that are not there but now should be.
    if ( N > data.inPt ) {
        for (int i = data.inPt; i < N; i++) {
            data.Xk[i] = pts[off + i].p[Geom::X];
            data.Yk[i] = pts[off + i].p[Geom::Y];
            data.fk[i] = ( pts[off + i].isMoveTo == polyline_forced ) ? 0x01 : 0x00;        
        }
        data.lk[0] = 0;
        data.tk[0] = 0;
        
        // calculate the total length of the points that already existed in data
        double prevLen = 0;
        for (int i = 0; i < data.inPt; i++) {
            prevLen += data.lk[i];
        }
        data.totLen = prevLen;
        
        // calculate the lengths data.lk for the new points that we just added
        for (int i = ( (data.inPt > 0) ? data.inPt : 1); i < N; i++) {
            Geom::Point diff;
            diff[Geom::X] = data.Xk[i] - data.Xk[i - 1];
            diff[Geom::Y] = data.Yk[i] - data.Yk[i - 1];
            data.lk[i] = Geom::L2(diff);
            data.totLen += data.lk[i];
            data.tk[i] = data.totLen; // set tk[i] to the length so far...
        }
        
        // for the points that existed already, multiply by their previous total length to convert
        // the time values into the actual "length so far".
        for (int i = 0; i < data.inPt; i++) {
            data.tk[i] *= prevLen;
            data.tk[i] /= data.totLen; // divide by the new total length to get the newer t value
        }
        
        for (int i = data.inPt; i < N; i++) { // now divide for the newer ones too
            data.tk[i] /= data.totLen;
        }
        data.inPt = N; // update inPt to include the new points too now
    }
    
    // this block is for the situation where you just did a fitting on say 0 to 20 points and now
    // you are doing one on 0 to 15 points. N was previously 20 and now it's 15. While your lk
    // values are still fine, your tk values are messed up since the tk is 1 at point 20 when
    // it should be 1 at point 15 now. So we recalculate tk values.
    if ( N < data.nbPt ) {
        // We've gone too far; we'll have to recalulate the .tk.
        data.totLen = 0;
        data.tk[0] = 0;
        data.lk[0] = 0;
        for (int i = 1; i < N; i++) {
            data.totLen += data.lk[i];
            data.tk[i] = data.totLen;
        }
        
        for (int i = 1; i < N; i++) {
            data.tk[i] /= data.totLen;
        }
    }
  
    data.nbPt = N;
    
    /*
     * There is something that I think is wrong with this implementation. Say we initially fit on
     * point 0 to 10, it works well, so we add 10 more points and fit on 0-20 points. Out tk values
     * are correct so far. That fit is problematic so maybe we fit on 0-15 but now, N < data.nbPt
     * block will run and recalculate tk values. So we will have correct tk values from 0-15 but
     * the older tk values in 15-20. Say after this we fit on 0-18 points. Well we ran into a
     * problem N > data.nbPt so no recalculation happens. data.inPt is already 20 so nothing
     * happens in that block either. We have invalid tk values and FitCubic will be called with
     * these invalid values. I've drawn paths and seen this situation where FitCubic was called
     * with wrong tk values. Values that would go 0, 0.25, 0.5, 0.75, 1, 0.80, 0.90. Maybe the
     * consequences are not that terrible so we didn't notice this in results?
     * TODO: Do we fix this or investigate more?
     */

    if ( data.nbPt <= 0 ) {
        return false;
    }
  
    res.p[0] = data.Xk[data.nbPt - 1];
    res.p[1] = data.Yk[data.nbPt - 1];
    res.start[0] = res.start[1] = 0;
    res.end[0] = res.end[1] = 0;
    worstP = 1;
    if ( N <= 2 ) {
        return true;
    }
    // this is the same popular block that's found in the other AttemptSimplify so please go there
    // to see what it does and how
    if ( data.totLen < 0.0001 ) {
        double worstD = 0;
        Geom::Point start;
        worstP = -1;
        start[0] = data.Xk[0];
        start[1] = data.Yk[0];
        for (int i = 1; i < N; i++) {
            Geom::Point nPt;
            bool isForced = data.fk[i];
            nPt[0] = data.Xk[i];
            nPt[1] = data.Yk[i];
      
            double nle = DistanceToCubic(start, res, nPt);
            if ( isForced ) {
                // forced points are favored for splitting the recursion; we do this by increasing their distance
                if ( worstP < 0 || 2*nle > worstD ) {
                    worstP = i;
                    worstD = 2*nle;
                }
            } else {
                if ( worstP < 0 || nle > worstD ) {
                    worstP = i;
                    worstD = nle;
                }
            }
        }
        return true; // it's weird that this is the only block of this kind that returns true instead of false. Livarot
                     // can you please be more consistent? -_-
    }
  
    return AttemptSimplify(data, treshhold, res, worstP);
}


// fit a polyline to a bezier patch, return true is treshhold not exceeded (ie: you can continue)
// version that uses tables from the previous iteration, to minimize amount of work done
bool Path::AttemptSimplify (fitting_tables &data,double treshhold, PathDescrCubicTo & res,int &worstP)
{
    Geom::Point start,end;
    // pour une coordonnee
    Geom::Point cp1, cp2;
  
    worstP = 1;
    if (pts.size() == 2) {
        return true;
    }
  
    start[0] = data.Xk[0];
    start[1] = data.Yk[0];
    cp1[0] = data.Xk[1];
    cp1[1] = data.Yk[1];
    end[0] = data.Xk[data.nbPt - 1];
    end[1] = data.Yk[data.nbPt - 1];
    cp2 = cp1;
  
    if (pts.size()  == 3) {
        // start -> cp1 -> end
        res.start = cp1 - start;
        res.end = end - cp1;
        worstP = 1;
        return true;
    }
  
    if ( FitCubic(start, res, data.Xk, data.Yk, data.Qk, data.tk, data.nbPt) ) {
        cp1 = start + res.start / 3;
        cp2 = end - res.end / 3;
    } else {
        // aie, non-inversible
        double worstD = 0;
        worstP = -1;
        for (int i = 1; i < data.nbPt; i++) {
            Geom::Point nPt;
            nPt[Geom::X] = data.Xk[i];
            nPt[Geom::Y] = data.Yk[i];
            double nle = DistanceToCubic(start, res, nPt);
            if ( data.fk[i] ) {
                // forced points are favored for splitting the recursion; we do this by increasing their distance
                if ( worstP < 0 || 2 * nle > worstD ) {
                    worstP = i;
                    worstD = 2 * nle;
                }
            } else {
                if ( worstP < 0 || nle > worstD ) {
                    worstP = i;
                    worstD = nle;
                }
            }
        }
        return false;
    }
   
    // calcul du delta= pondere par les longueurs des segments
    double delta = 0;
    {
        double worstD = 0;
        worstP = -1;
        Geom::Point prevAppP;
        Geom::Point prevP;
        double prevDist;
        prevP[Geom::X] = data.Xk[0];
        prevP[Geom::Y] = data.Yk[0];
        prevAppP = prevP; // le premier seulement
        prevDist = 0;
#ifdef with_splotch_killer
        if ( data.nbPt <= 20 ) {
            for (int i = 1; i < data.nbPt - 1; i++) {
                Geom::Point curAppP;
                Geom::Point curP;
                double curDist;
                Geom::Point midAppP;
                Geom::Point midP;
                double midDist;
                
                curAppP[Geom::X] = N13(data.tk[i]) * cp1[Geom::X] +
                    N23(data.tk[i]) * cp2[Geom::X] +
                    N03(data.tk[i]) * data.Xk[0] +
                    N33(data.tk[i]) * data.Xk[data.nbPt - 1];
                
                curAppP[Geom::Y] = N13(data.tk[i]) * cp1[Geom::Y] +
                    N23(data.tk[i]) * cp2[Geom::Y] +
                    N03(data.tk[i]) * data.Yk[0] +
                    N33(data.tk[i]) * data.Yk[data.nbPt - 1];
                
                curP[Geom::X] = data.Xk[i];
                curP[Geom::Y] = data.Yk[i];
                double mtk = 0.5 * (data.tk[i] + data.tk[i - 1]);
                
                midAppP[Geom::X] = N13(mtk) * cp1[Geom::X] +
                    N23(mtk) * cp2[Geom::X] +
                    N03(mtk) * data.Xk[0] +
                    N33(mtk) * data.Xk[data.nbPt - 1];
                
                midAppP[Geom::Y] = N13(mtk) * cp1[Geom::Y] +
                    N23(mtk) * cp2[Geom::Y] +
                    N03(mtk) * data.Yk[0] +
                    N33(mtk) * data.Yk[data.nbPt - 1];
                
                midP = 0.5 * (curP + prevP);
        
                Geom::Point diff = curAppP - curP;
                curDist = dot(diff, diff);
                diff = midAppP - midP;
                midDist = dot(diff, diff);
        
                delta += 0.3333 * (curDist + prevDist + midDist) * data.lk[i];
                if ( curDist > worstD ) {
                    worstD = curDist;
                    worstP = i;
                } else if ( data.fk[i] && 2 * curDist > worstD ) {
                    worstD = 2*curDist;
                    worstP = i;
                }
                prevP = curP;
                prevAppP = curAppP;
                prevDist = curDist;
            }
            delta /= data.totLen;
            
        } else {
#endif
            for (int i = 1; i < data.nbPt - 1; i++) {
                Geom::Point curAppP;
                Geom::Point curP;
                double    curDist;
        
                curAppP[Geom::X] = N13(data.tk[i]) * cp1[Geom::X] +
                    N23(data.tk[i]) * cp2[Geom::X] +
                    N03(data.tk[i]) * data.Xk[0] +
                    N33(data.tk[i]) * data.Xk[data.nbPt - 1];
                
                curAppP[Geom::Y] = N13(data.tk[i]) * cp1[Geom::Y] +
                    N23(data.tk[i]) * cp2[Geom::Y] +
                    N03(data.tk[i]) * data.Yk[0] +
                    N33(data.tk[i]) * data.Yk[data.nbPt - 1];
                
                curP[Geom::X] = data.Xk[i];
                curP[Geom::Y] = data.Yk[i];
      
                Geom::Point diff = curAppP-curP;
                curDist = dot(diff, diff);
                delta += curDist;
        
                if ( curDist > worstD ) {
                    worstD = curDist;
                    worstP = i;
                } else if ( data.fk[i] && 2 * curDist > worstD ) {
                    worstD = 2*curDist;
                    worstP = i;
                }
                prevP = curP;
                prevAppP = curAppP;
                prevDist = curDist;
            }
#ifdef with_splotch_killer
        }
#endif
    }
  
    if (delta < treshhold * treshhold) {
        // premier jet
    
        // Refine a little.
        for (int i = 1; i < data.nbPt - 1; i++) {
            Geom::Point pt(data.Xk[i], data.Yk[i]);
            data.tk[i] = RaffineTk(pt, start, cp1, cp2, end, data.tk[i]);
            if (data.tk[i] < data.tk[i - 1]) {
                // Force tk to be monotonic non-decreasing.
                data.tk[i] = data.tk[i - 1];
	    }
        }
    
        if ( FitCubic(start, res, data.Xk, data.Yk, data.Qk, data.tk, data.nbPt) == false) {
            // ca devrait jamais arriver, mais bon
            res.start = 3.0 * (cp1 - start);
            res.end = 3.0 * (end - cp2 );
            return true;
        }
        
        double ndelta = 0;
        {
            double worstD = 0;
            worstP = -1;
            Geom::Point prevAppP;
            Geom::Point prevP(data.Xk[0], data.Yk[0]);
            double prevDist = 0;
            prevAppP = prevP; // le premier seulement
#ifdef with_splotch_killer
            if ( data.nbPt <= 20 ) {
                for (int i = 1; i < data.nbPt - 1; i++) {
                    Geom::Point curAppP;
                    Geom::Point curP;
                    double  curDist;
                    Geom::Point midAppP;
                    Geom::Point midP;
                    double  midDist;
          
                    curAppP[Geom::X] = N13(data.tk[i]) * cp1[Geom::X] +
                        N23(data.tk[i]) * cp2[Geom::X] +
                        N03(data.tk[i]) * data.Xk[0] +
                        N33(data.tk[i]) * data.Xk[data.nbPt - 1];
                    
                    curAppP[Geom::Y] = N13(data.tk[i]) * cp1[Geom::Y] +
                        N23(data.tk[i]) * cp2[Geom::Y] +
                        N03(data.tk[i]) * data.Yk[0] +
                        N33(data.tk[i]) * data.Yk[data.nbPt - 1];
                    
                    curP[Geom::X] = data.Xk[i];
                    curP[Geom::Y] = data.Yk[i];
                    double mtk = 0.5 * (data.tk[i] + data.tk[i - 1]);
                    
                    midAppP[Geom::X] = N13(mtk) * cp1[Geom::X] +
                        N23(mtk) * cp2[Geom::X] +
                        N03(mtk) * data.Xk[0] +
                        N33(mtk) * data.Xk[data.nbPt - 1];
                    
                    midAppP[Geom::Y] = N13(mtk) * cp1[Geom::Y] +
                        N23(mtk) * cp2[Geom::Y] +
                        N03(mtk) * data.Yk[0] +
                        N33(mtk) * data.Yk[data.nbPt - 1];
                    
                    midP = 0.5 * (curP + prevP);
          
                    Geom::Point diff = curAppP - curP;
                    curDist = dot(diff, diff);
          
                    diff = midAppP - midP;
                    midDist = dot(diff, diff);
          
                    ndelta += 0.3333 * (curDist + prevDist + midDist) * data.lk[i];
          
                    if ( curDist > worstD ) {
                        worstD = curDist;
                        worstP = i;
                    } else if ( data.fk[i] && 2 * curDist > worstD ) {
                        worstD = 2*curDist;
                        worstP = i;
                    }
                    
                    prevP = curP;
                    prevAppP = curAppP;
                    prevDist = curDist;
                }
                ndelta /= data.totLen;
            } else {
#endif
                for (int i = 1; i < data.nbPt - 1; i++) {
                    Geom::Point curAppP;
                    Geom::Point curP;
                    double    curDist;
                    
                    curAppP[Geom::X] = N13(data.tk[i]) * cp1[Geom::X] +
                        N23(data.tk[i]) * cp2[Geom::X] +
                        N03(data.tk[i]) * data.Xk[0] +
                        N33(data.tk[i]) * data.Xk[data.nbPt - 1];
                    
                    curAppP[Geom::Y] = N13(data.tk[i]) * cp1[Geom::Y] +
                        N23(data.tk[i]) * cp2[1] +
                        N03(data.tk[i]) * data.Yk[0] +
                        N33(data.tk[i]) * data.Yk[data.nbPt - 1];
                    
                    curP[Geom::X] = data.Xk[i];
                    curP[Geom::Y] = data.Yk[i];
        
                    Geom::Point diff = curAppP - curP;
                    curDist = dot(diff, diff);

                    ndelta += curDist;

                    if ( curDist > worstD ) {
                        worstD = curDist;
                        worstP = i;
                    } else if ( data.fk[i] && 2 * curDist > worstD ) {
                        worstD = 2 * curDist;
                        worstP = i;
                    }
                    prevP = curP;
                    prevAppP = curAppP;
                    prevDist = curDist;
                }
#ifdef with_splotch_killer
            }
#endif
        }
    
        if (ndelta < delta + 0.00001) {
            return true;
        } else {
            // nothing better to do
            res.start = 3.0 * (cp1 - start);
            res.end = 3.0 * (end - cp2 );
        }
        
        return true;
    }
  
  return false;
}


bool Path::AttemptSimplify(int off, int N, double treshhold, PathDescrCubicTo &res,int &worstP)
{
    Geom::Point start;
    Geom::Point end;
    
    // pour une coordonnee
    double *Xk;				// la coordonnee traitee (x puis y)
    double *Yk;				// la coordonnee traitee (x puis y)
    double *lk;				// les longueurs de chaque segment
    double *tk;				// les tk
    double *Qk;				// les Qk
    char *fk;       // si point force
  
    Geom::Point cp1; // first control point of the cubic patch
    Geom::Point cp2; // second control point of the cubic patch
  
    // If only two points, return immediately, but why say that point 1 has worst error though?
    if (N == 2) {
        worstP = 1;
        return true;
    }
  
    start = pts[off].p; // point at index "off" is start
    cp1 = pts[off + 1].p;  // for now take the first control point to be the point after "off"
    end = pts[off + N - 1].p; // last point would be end of the patch of course
  
    // we will return the control handles in "res" right? so set the end point but set the handles
    // to zero for now
    res.p = end;
    res.start[0] = res.start[1] = 0;
    res.end[0] = res.end[1] = 0;

    // if there are 3 points only, we fit a cubic patch that starts at the first point, ends at the
    // third point and has both control points on the 2nd point
    if (N == 3) {
        // start -> cp1 -> end
        res.start = cp1 - start;
        res.end = end - cp1;
        worstP = 1;
        return true;
    }
  
    // Totally inefficient, allocates & deallocates all the time.
    // allocate arrows for fitting information
    tk = (double *) g_malloc(N * sizeof(double));
    Qk = (double *) g_malloc(N * sizeof(double));
    Xk = (double *) g_malloc(N * sizeof(double));
    Yk = (double *) g_malloc(N * sizeof(double));
    lk = (double *) g_malloc(N * sizeof(double));
    fk = (char *) g_malloc(N * sizeof(char));
  
    // chord length method
    tk[0] = 0.0;
    lk[0] = 0.0;
    {
        // Not setting Xk[0], Yk[0] might look like a bug, but FitCubic sets it later before
        // it performs the fit
        Geom::Point prevP = start; // store the first point
        for (int i = 1; i < N; i++) { // start the loop on the second point
            Xk[i] = pts[off + i].p[Geom::X]; // store the x coordinate
            Yk[i] = pts[off + i].p[Geom::Y]; // store the y coordinate

            // mark the point as forced if it's forced
            if ( pts[off + i].isMoveTo == polyline_forced ) {
                fk[i] = 0x01;
            } else {
                fk[i] = 0;
            }
            
            // calculate a vector from previous point to this point and store its length in lk
            Geom::Point diff(Xk[i] - prevP[Geom::X], Yk[i] - prevP[1]);
            prevP[0] = Xk[i]; // set prev to current point for next iteration
            prevP[1] = Yk[i]; // set prev to current point for next iteration
            lk[i] = Geom::L2(diff);
            tk[i] = tk[i - 1] + lk[i]; // tk should have the length till this point, later we divide whole thing by total length to calculate time
        }
    }
    
    // if total length is below 0.00001 (very unrealistic scenario)
    if (tk[N - 1] < 0.00001) {
        // longueur nulle 
        res.start[0] = res.start[1] = 0; // set start handle to zero
        res.end[0] = res.end[1] = 0;     // set end handle to zero
        double worstD = 0;               // worst distance b/w the curve and the polyline (to fit)
        worstP = -1;                     // point at which worst difference came up
        for (int i = 1; i < N; i++) { // start at second point till the last one
            Geom::Point nPt;
            bool isForced = fk[i];
            nPt[0] = Xk[i]; // get the point
            nPt[1] = Yk[i];
 
            double nle = DistanceToCubic(start, res, nPt); // distance from point. see the documentation on that function
            if ( isForced ) {
                // forced points are favored for splitting the recursion; we do this by increasing their distance
                if ( worstP < 0 || 2 * nle > worstD ) { // exaggerating distance for the forced point?
                    worstP = i;
                    worstD = 2 * nle;
                }
            } else {
                if ( worstP < 0 || nle > worstD ) { // if worstP not set or the distance for this point is greater than previous worse
                    worstP = i;                     // make this one the worse
                    worstD = nle;
                }
            }
        }
        
        g_free(tk);
        g_free(Qk);
        g_free(Xk);
        g_free(Yk);
        g_free(fk);
        g_free(lk);
        
        return false; // not sure why we return false? because the length of points to fit is zero?
        // anyways, rare case, so fine to return false and a cubic bezier that starts
        // at first and ends at last point with control points being same as
        // endpoins.
    }
    
    // divide by total length to make "time" values. See documentation of fitting_table structure
    double totLen = tk[N - 1];
    for (int i = 1; i < N - 1; i++) {
        tk[i] /= totLen;
    }
  
    res.p = end;
    if ( FitCubic(start, res, Xk, Yk, Qk, tk, N) ) { // fit a cubic, if goes well, set cp1 and cp2
        cp1 = start + res.start / 3; // this factor of three comes from the fact that this is how livarot stores them. See docs in path-description.h
        cp2 = end + res.end / 3;
    } else {
        // aie, non-inversible
        // okay we couldn't fit one, don't know when this would happen but probably due to matrix
        // being non-inversible (no idea when that would happen either)

        // same code from above, calculates error (kinda, see comments on DistanceToCubic) and returns false
        res.start[0] = res.start[1] = 0;
        res.end[0] = res.end[1] = 0;
        double worstD = 0;
        worstP = -1;
        for (int i = 1; i < N; i++) {
            Geom::Point nPt(Xk[i], Yk[i]);
            bool isForced = fk[i];
            double nle = DistanceToCubic(start, res, nPt);
            if ( isForced ) {
                // forced points are favored for splitting the recursion; we do this by increasing their distance
                if ( worstP < 0 || 2 * nle > worstD ) {
                    worstP = i;
                    worstD = 2 * nle;
                }
            } else {
                if ( worstP < 0 || nle > worstD ) {
                    worstP = i;
                    worstD = nle;
                }
            }
        }
        
        g_free(tk);
        g_free(Qk);
        g_free(Xk);
        g_free(Yk);
        g_free(fk);
        g_free(lk);
        return false;
    }
   
    // calcul du delta= pondere par les longueurs des segments
    // if we are here, fitting went well, let's calculate error between the cubic bezier that we
    // fit and the actual polyline
    double delta = 0;
    {
        double worstD = 0;
        worstP = -1;
        Geom::Point prevAppP;
        Geom::Point   prevP;
        double      prevDist;
        prevP[0] = Xk[0];
        prevP[1] = Yk[0];
        prevAppP = prevP; // le premier seulement
        prevDist = 0;
#ifdef with_splotch_killer  // <-- ignore the splotch killer if you're new and go to the part where this (N <= 20) if's else is
        // so the thing is, if the number of points you're fitting on are few, you can often have a
        // fit where the error on your polyline points are zero but it's a terrible fit, for
        // example imagine three point  o------o-------o and an S fitting on these. Error is zero,
        // but it's a terrible fit. So to fix this problem, we calculate error at mid points of the
        // line segments too, to be a better judge, and if it's terrible, we just reject this fit
        // and the parent algorithm will do something about it (fit a smaller one or just do a line
        // segment)
        if ( N <= 20 ) {
            for (int i = 1; i < N - 1; i++) // for every point that's not an endpoint
            {
                Geom::Point curAppP;
                Geom::Point curP;
                double    curDist;
                Geom::Point midAppP;
                Geom::Point midP;
                double    midDist;

                curAppP[0] = N13 (tk[i]) * cp1[0] + N23 (tk[i]) * cp2[0] + N03 (tk[i]) * Xk[0] + N33 (tk[i]) * Xk[N - 1]; // point on the curve
                curAppP[1] = N13 (tk[i]) * cp1[1] + N23 (tk[i]) * cp2[1] + N03 (tk[i]) * Yk[0] + N33 (tk[i]) * Yk[N - 1];
                curP[0] = Xk[i]; // polyline point
                curP[1] = Yk[i];
                midAppP[0] = N13 (0.5*(tk[i]+tk[i-1])) * cp1[0] + N23 (0.5*(tk[i]+tk[i-1])) * cp2[0] + N03 (0.5*(tk[i]+tk[i-1])) * Xk[0] + N33 (0.5*(tk[i]+tk[i-1])) * Xk[N - 1]; // midpoint on the curve
                midAppP[1] = N13 (0.5*(tk[i]+tk[i-1])) * cp1[1] + N23 (0.5*(tk[i]+tk[i-1])) * cp2[1] + N03 (0.5*(tk[i]+tk[i-1])) * Yk[0] + N33 (0.5*(tk[i]+tk[i-1])) * Yk[N - 1];
                midP=0.5*(curP+prevP); // midpoint on the polyline

                Geom::Point diff;
                diff = curAppP-curP;
                curDist = dot(diff,diff); // squared distance between point on cubic curve and the polyline point

                diff = midAppP-midP;
                midDist = dot(diff,diff); // squared distance between midpoint on polyline and the equivalent on cubic curve

                delta+=0.3333*(curDist+prevDist+midDist)/**lk[i]*/; // The multiplication by lk[i] here kind of creates a weightage mechanism
                // we divide delta by total length afterwards, so line segments with more share in
                // the length get weighted more. Why do we care about prevDist though? Anyways,
                // it's a way of measuring error, so probably fine

                if ( curDist > worstD ) { // worst error management
                    worstD = curDist;
                    worstP = i;
                } else if ( fk[i] && 2*curDist > worstD ) {
                    worstD = 2*curDist;
                    worstP = i;
                }
                prevP = curP;
                prevAppP = curAppP; // <-- useless
                prevDist = curDist;
            }
            delta/=totLen;
        } else {
#endif
            for (int i = 1; i < N - 1; i++) // for each point in the polyline that's not an end point
            {
                Geom::Point curAppP; // the current point on the bezier patch
                Geom::Point curP;    // the polyline point
                double    curDist;

                // https://en.wikipedia.org/wiki/B%C3%A9zier_curve
                curAppP[0] = N13 (tk[i]) * cp1[0] + N23 (tk[i]) * cp2[0] + N03 (tk[i]) * Xk[0] + N33 (tk[i]) * Xk[N - 1]; // <-- cubic bezier function
                curAppP[1] = N13 (tk[i]) * cp1[1] + N23 (tk[i]) * cp2[1] + N03 (tk[i]) * Yk[0] + N33 (tk[i]) * Yk[N - 1];
                curP[0] = Xk[i];
                curP[1] = Yk[i];

                Geom::Point diff;
                diff = curAppP-curP;      // difference between the two
                curDist = dot(diff,diff); // square of the difference
                delta += curDist;         // add to total error
                if ( curDist > worstD ) { // management of worst error (max finder basically)
                    worstD = curDist;
                    worstP = i;
                } else if ( fk[i] && 2*curDist > worstD ) { // it wasn't the worse, but if it's forced point, judge more harshly
                    worstD = 2*curDist;
                    worstP = i;
                }
                prevP = curP;
                prevAppP = curAppP; // <-- we are not using these
                prevDist = curDist; // <-- we are not using these
            }
#ifdef with_splotch_killer
        }
#endif
    }

    // are we fine with the error? Compare it to threshold squared
    if (delta < treshhold * treshhold)
    {
        // premier jet
        res.start = 3.0 * (cp1 - start); // calculate handles
        res.end = -3.0 * (cp2 - end);
        res.p = end;

        // Refine a little.
        for (int i = 1; i < N - 1; i++) // do Newton Raphson iterations
        {
            Geom::Point
                pt;
            pt[0] = Xk[i];
            pt[1] = Yk[i];
            tk[i] = RaffineTk (pt, start, cp1, cp2, end, tk[i]);
            if (tk[i] < tk[i - 1])
            {
                // Force tk to be monotonic non-decreasing.
                tk[i] = tk[i - 1];
            }
        }

        if ( FitCubic(start,res,Xk,Yk,Qk,tk,N) ) { // fit again and this should work
        } else { // just in case it doesn't
            // ca devrait jamais arriver, mais bon
            res.start = 3.0 * (cp1 - start);
            res.end = -3.0 * (cp2 - end);
            g_free(tk);
            g_free(Qk);
            g_free(Xk);
            g_free(Yk);
            g_free(fk);
            g_free(lk);
            return true;
        }
        double ndelta = 0; // the same error computing stuff with and without splotch killer as before
        {
            double worstD = 0;
            worstP = -1;
            Geom::Point   prevAppP;
            Geom::Point   prevP;
            double      prevDist;
            prevP[0] = Xk[0];
            prevP[1] = Yk[0];
            prevAppP = prevP; // le premier seulement
            prevDist = 0;
#ifdef with_splotch_killer
            if ( N <= 20 ) {
                for (int i = 1; i < N - 1; i++)
                {
                    Geom::Point curAppP;
                    Geom::Point curP;
                    double    curDist;
                    Geom::Point midAppP;
                    Geom::Point midP;
                    double    midDist;

                    curAppP[0] = N13 (tk[i]) * cp1[0] + N23 (tk[i]) * cp2[0] + N03 (tk[i]) * Xk[0] + N33 (tk[i]) * Xk[N - 1];
                    curAppP[1] = N13 (tk[i]) * cp1[1] + N23 (tk[i]) * cp2[1] + N03 (tk[i]) * Yk[0] + N33 (tk[i]) * Yk[N - 1];
                    curP[0] = Xk[i];
                    curP[1] = Yk[i];
                    midAppP[0] = N13 (0.5*(tk[i]+tk[i-1])) * cp1[0] + N23 (0.5*(tk[i]+tk[i-1])) * cp2[0] + N03 (0.5*(tk[i]+tk[i-1])) * Xk[0] + N33 (0.5*(tk[i]+tk[i-1])) * Xk[N - 1];
                    midAppP[1] = N13 (0.5*(tk[i]+tk[i-1])) * cp1[1] + N23 (0.5*(tk[i]+tk[i-1])) * cp2[1] + N03 (0.5*(tk[i]+tk[i-1])) * Yk[0] + N33 (0.5*(tk[i]+tk[i-1])) * Yk[N - 1];
                    midP = 0.5*(curP+prevP);

                    Geom::Point diff;
                    diff = curAppP-curP;
                    curDist = dot(diff,diff);
                    diff = midAppP-midP;
                    midDist = dot(diff,diff);

                    ndelta+=0.3333*(curDist+prevDist+midDist)/**lk[i]*/;

                    if ( curDist > worstD ) {
                        worstD = curDist;
                        worstP = i;
                    } else if ( fk[i] && 2*curDist > worstD ) {
                        worstD = 2*curDist;
                        worstP = i;
                    }
                    prevP = curP;
                    prevAppP = curAppP;
                    prevDist = curDist;
                }
                ndelta /= totLen;
            } else {
#endif
                for (int i = 1; i < N - 1; i++)
                {
                    Geom::Point curAppP;
                    Geom::Point curP;
                    double    curDist;

                    curAppP[0] = N13 (tk[i]) * cp1[0] + N23 (tk[i]) * cp2[0] + N03 (tk[i]) * Xk[0] + N33 (tk[i]) * Xk[N - 1];
                    curAppP[1] = N13 (tk[i]) * cp1[1] + N23 (tk[i]) * cp2[1] + N03 (tk[i]) * Yk[0] + N33 (tk[i]) * Yk[N - 1];
                    curP[0]=Xk[i];
                    curP[1]=Yk[i];

                    Geom::Point diff;
                    diff=curAppP-curP;
                    curDist=dot(diff,diff);
                    ndelta+=curDist;

                    if ( curDist > worstD ) {
                        worstD=curDist;
                        worstP=i;
                    } else if ( fk[i] && 2*curDist > worstD ) {
                        worstD=2*curDist;
                        worstP=i;
                    }
                    prevP=curP;
                    prevAppP=curAppP;
                    prevDist=curDist;
                }
#ifdef with_splotch_killer
            }
#endif
        }

        g_free(tk);
        g_free(Qk);
        g_free(Xk);
        g_free(Yk);
        g_free(fk);
        g_free(lk);

        if (ndelta < delta + 0.00001) // is the new one after newton raphson better? they are stored in "res"
        {
            return true; // okay great return those handles.
        } else {
            // nothing better to do
            res.start = 3.0 * (cp1 - start); // new ones aren't better? use the old ones stored in cp1 and cp2
            res.end = -3.0 * (cp2 - end);
        }
        return true;
    } else { // threshold got disrespected, return false
        // nothing better to do
    }

    g_free(tk);
    g_free(Qk);
    g_free(Xk);
    g_free(Yk);
    g_free(fk);
    g_free(lk);
    return false;
}

double Path::RaffineTk (Geom::Point pt, Geom::Point p0, Geom::Point p1, Geom::Point p2, Geom::Point p3, double it)
{
    // Refinement of the tk values. 
    // Just one iteration of Newtow Raphson, given that we're approaching the curve anyway.
    // [fr: vu que de toute facon la courbe est approchC)e]
    double const Ax = pt[Geom::X] -
        p0[Geom::X] * N03(it) -
        p1[Geom::X] * N13(it) -
        p2[Geom::X] * N23(it) -
        p3[Geom::X] * N33(it);
    
    double const Bx = (p1[Geom::X] - p0[Geom::X]) * N02(it) +
        (p2[Geom::X] - p1[Geom::X]) * N12(it) +
        (p3[Geom::X] - p2[Geom::X]) * N22(it);
  
    double const Cx = (p0[Geom::X] - 2 * p1[Geom::X] + p2[Geom::X]) * N01(it) +
        (p3[Geom::X] - 2 * p2[Geom::X] + p1[Geom::X]) * N11(it);
    
    double const Ay =  pt[Geom::Y] -
        p0[Geom::Y] * N03(it) -
        p1[Geom::Y] * N13(it) -
        p2[Geom::Y] * N23(it) -
        p3[Geom::Y] * N33(it);
    
    double const By = (p1[Geom::Y] - p0[Geom::Y]) * N02(it) +
        (p2[Geom::Y] - p1[Geom::Y]) * N12(it) +
        (p3[Geom::Y] - p2[Geom::Y]) * N22(it);
    
    double const Cy = (p0[Geom::Y] - 2 * p1[Geom::Y] + p2[Geom::Y]) * N01(it) +
        (p3[Geom::Y] - 2 * p2[Geom::Y] + p1[Geom::Y]) * N11(it);
    
    double const dF = -6 * (Ax * Bx + Ay * By);
    double const ddF = 18 * (Bx * Bx + By * By) - 12 * (Ax * Cx + Ay * Cy);
    if (fabs (ddF) > 0.0000001) {
        return it - dF / ddF;
    }
    
    return it;
}

// Variation on the fitting theme: try to merge path commands into cubic bezier patches.
// The goal is to reduce the number of path commands, especially when operations on path produce
// lots of small path elements; ideally you could get rid of very small segments at reduced visual cost.
void Path::Coalesce(double tresh)
{
    if ( descr_flags & descr_doing_subpath ) {
        CloseSubpath();
    }
    
    if (descr_cmd.size() <= 2) {
        return;
    }
  
    SetBackData(false);
    Path* tempDest = new Path();
    tempDest->SetBackData(false);
  
    ConvertEvenLines(0.25*tresh);
  
    int lastP = 0;
    int lastAP = -1;
    // As the elements are stored in a separate array, it's no longer worth optimizing
    // the rewriting in the same array.
    // [[comme les elements sont stockes dans un tableau a part, plus la peine d'optimiser
    // la réécriture dans la meme tableau]]
    
    int lastA = descr_cmd[0]->associated;
    int prevA = lastA;
    Geom::Point firstP;

    /* FIXME: the use of this variable probably causes a leak or two.
    ** It's a hack anyway, and probably only needs to be a type rather than
    ** a full PathDescr.
    */
    std::unique_ptr<PathDescr> lastAddition(new PathDescrMoveTo(Geom::Point(0, 0)));
    bool containsForced = false;
    PathDescrCubicTo pending_cubic(Geom::Point(0, 0), Geom::Point(0, 0), Geom::Point(0, 0));
  
    for (int curP = 0; curP < int(descr_cmd.size()); curP++) {
        int typ = descr_cmd[curP]->getType();
        int nextA = lastA;

        if (typ == descr_moveto) {

            if (lastAddition->flags != descr_moveto) {
                FlushPendingAddition(tempDest,lastAddition.get(),pending_cubic,lastAP);
            }
            lastAddition.reset(descr_cmd[curP]->clone());
            lastAP = curP;
            FlushPendingAddition(tempDest, lastAddition.get(), pending_cubic, lastAP);
            // Added automatically (too bad about multiple moveto's).
            // [fr: (tant pis pour les moveto multiples)]
            containsForced = false;
      
            PathDescrMoveTo *nData = dynamic_cast<PathDescrMoveTo *>(descr_cmd[curP]);
            firstP = nData->p;
            lastA = descr_cmd[curP]->associated;
            prevA = lastA;
            lastP = curP;
            
        } else if (typ == descr_close) {
            nextA = descr_cmd[curP]->associated;
            if (lastAddition->flags != descr_moveto) {
        
                PathDescrCubicTo res(Geom::Point(0, 0), Geom::Point(0, 0), Geom::Point(0, 0));
                int worstP = -1;
                if (AttemptSimplify(lastA, nextA - lastA + 1, (containsForced) ? 0.05 * tresh : tresh, res, worstP)) {
                    lastAddition.reset(new PathDescrCubicTo(Geom::Point(0, 0),
                                                          Geom::Point(0, 0),
                                                          Geom::Point(0, 0)));
                    pending_cubic = res;
                    lastAP = -1;
                }

                FlushPendingAddition(tempDest, lastAddition.get(), pending_cubic, lastAP);
                FlushPendingAddition(tempDest, descr_cmd[curP], pending_cubic, curP);
        
	    } else {
                FlushPendingAddition(tempDest,descr_cmd[curP],pending_cubic,curP);
	    }
            
            containsForced = false;
            lastAddition.reset(new PathDescrMoveTo(Geom::Point(0, 0)));
            prevA = lastA = nextA;
            lastP = curP;
            lastAP = curP;
            
        } else if (typ == descr_forced) {
            
            nextA = descr_cmd[curP]->associated;
            if (lastAddition->flags != descr_moveto) {
                
                PathDescrCubicTo res(Geom::Point(0, 0), Geom::Point(0, 0), Geom::Point(0, 0));
                int worstP = -1;
                if (AttemptSimplify(lastA, nextA - lastA + 1, 0.05 * tresh, res, worstP)) {
                    // plus sensible parce que point force
                    // ca passe
                    /* (Possible translation: More sensitive because contains a forced point.) */
                    containsForced = true;
                } else  {
                    // Force the addition.
                    FlushPendingAddition(tempDest, lastAddition.get(), pending_cubic, lastAP);
                    lastAddition.reset(new PathDescrMoveTo(Geom::Point(0, 0)));
                    prevA = lastA = nextA;
                    lastP = curP;
                    lastAP = curP;
                    containsForced = false;
                }
	    }
            
        } else if (typ == descr_lineto || typ == descr_cubicto || typ == descr_arcto) {
            
            nextA = descr_cmd[curP]->associated;
            if (lastAddition->flags != descr_moveto) {
                
                PathDescrCubicTo res(Geom::Point(0, 0), Geom::Point(0, 0), Geom::Point(0, 0));
                int worstP = -1;
                if (AttemptSimplify(lastA, nextA - lastA + 1, tresh, res, worstP)) {
                    lastAddition.reset(new PathDescrCubicTo(Geom::Point(0, 0),
                                                          Geom::Point(0, 0),
                                                          Geom::Point(0, 0)));
                    pending_cubic = res;
                    lastAddition->associated = lastA;
                    lastP = curP;
                    lastAP = -1;
                }  else {
                    lastA = descr_cmd[lastP]->associated;	// pourrait etre surecrit par la ligne suivante
                        /* (possible translation: Could be overwritten by the next line.) */
                    FlushPendingAddition(tempDest, lastAddition.get(), pending_cubic, lastAP);
                    lastAddition.reset(descr_cmd[curP]->clone());
                    if ( typ == descr_cubicto ) {
                        pending_cubic = *(dynamic_cast<PathDescrCubicTo*>(descr_cmd[curP]));
                    }
                    lastAP = curP;
                    containsForced = false;
                }
        
	    } else {
                lastA = prevA /*descr_cmd[curP-1]->associated */ ;
                lastAddition.reset(descr_cmd[curP]->clone());
                if ( typ == descr_cubicto ) {
                    pending_cubic = *(dynamic_cast<PathDescrCubicTo*>(descr_cmd[curP]));
                }
                lastAP = curP;
                containsForced = false;
	    }
            prevA = nextA;
        } else {
            continue;
        }
    }
    
    if (lastAddition->flags != descr_moveto) {
        FlushPendingAddition(tempDest, lastAddition.get(), pending_cubic, lastAP);
    }
  
    Copy(tempDest);
    delete tempDest;
}


void Path::FlushPendingAddition(Path *dest, PathDescr *lastAddition,
                                PathDescrCubicTo &lastCubic, int lastAP)
{
    switch (lastAddition->getType()) {

    case descr_moveto:
        if ( lastAP >= 0 ) {
            PathDescrMoveTo* nData = dynamic_cast<PathDescrMoveTo *>(descr_cmd[lastAP]);
            dest->MoveTo(nData->p);
        }
        break;
        
    case descr_close:
        dest->Close();
        break;

    case descr_cubicto:
        dest->CubicTo(lastCubic.p, lastCubic.start, lastCubic.end);
        break;

    case descr_lineto:
        if ( lastAP >= 0 ) {
            PathDescrLineTo *nData = dynamic_cast<PathDescrLineTo *>(descr_cmd[lastAP]);
            dest->LineTo(nData->p);
        }
        break;

    case descr_arcto:
        if ( lastAP >= 0 ) {
            PathDescrArcTo *nData = dynamic_cast<PathDescrArcTo *>(descr_cmd[lastAP]);
            dest->ArcTo(nData->p, nData->rx, nData->ry, nData->angle, nData->large, nData->clockwise);
        }
        break;
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

// SPDX-License-Identifier: GPL-2.0-or-later

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include "splinefont.h"
#include "splinefit.h"

#define FONTFORGE_CONFIG_USE_DOUBLE 1

bigreal BPDot(BasePoint v1, BasePoint v2) {
    return v1.x * v2.x + v1.y * v2.y;
}

bigreal BPCross(BasePoint v1, BasePoint v2) {
    return v1.x * v2.y - v1.y * v2.x;
}

BasePoint BPRev(BasePoint v) {
    return (BasePoint) { -v.x, -v.y };
}

int RealWithin(real a,real b,real fudge) {
    return( b>=a-fudge && b<=a+fudge );
}

BOOL RealNear(real a,real b) {
    real d = a-b;
#ifdef FONTFORGE_CONFIG_USE_DOUBLE
    // These tighter equals-zero tests are retained for code tuned when
    // passing zero as a constant
    if ( a==0 )
	return b>-1e-8 && b<1e-8;
    if ( b==0 )
	return a>-1e-8 && a<1e-8;

    return d>-1e-6 && d<1e-6;
#else		/* For floats */
    return d>-1e-5 && d<1e-5
#endif
}

int RealApprox(real a,real b) {

    if ( a==0 ) {
	if ( b<.0001 && b>-.0001 )
return( true );
    } else if ( b==0 ) {
	if ( a<.0001 && a>-.0001 )
return( true );
    } else {
	a /= b;
	if ( a>=.95 && a<=1.05 )
return( true );
    }
return( false );
}

void LineListFree(LineList *ll) {
    LineList *next;

    while ( ll!=NULL ) {
	next = ll->next;
	chunkfree(ll,sizeof(LineList));
	ll = next;
    }
}

void LinearApproxFree(LinearApprox *la) {
    LinearApprox *next;

    while ( la!=NULL ) {
	next = la->next;
	LineListFree(la->lines);
	chunkfree(la,sizeof(LinearApprox));
	la = next;
    }
}

void SplineFree(Spline *spline) {
    LinearApproxFree(spline->approx);
    chunkfree(spline,sizeof(Spline));
}

SplinePoint *SplinePointCreate(real x, real y) {
    SplinePoint *sp;
    if ( (sp=chunkalloc(sizeof(SplinePoint)))!=NULL ) {
	sp->me.x = x; sp->me.y = y;
	sp->nextcp = sp->prevcp = sp->me;
	sp->nonextcp = sp->noprevcp = true;
	sp->nextcpdef = sp->prevcpdef = false;
	sp->ttfindex = sp->nextcpindex = 0xfffe;
	sp->name = NULL;
    }
    return( sp );
}

void SplinePointsFree(SplinePointList *spl) {
    Spline *first, *spline, *next;
    int nonext;

    if ( spl==NULL )
      return;
    if ( spl->first!=NULL ) {
	nonext = spl->first->next==NULL; // If there is no spline, we set a flag.
	first = NULL;
        // We start on the first spline if it exists.
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline = next ) {
	    next = spline->to->next; // Cache the location of the next spline.
	    SplinePointFree(spline->to); // Free the destination point.
	    SplineFree(spline); // Free the spline.
	    if ( first==NULL ) first = spline; // We want to avoid repeating the circuit.
	}
        // If the path is open or has no splines, free the starting point.
	if ( spl->last!=spl->first || nonext )
	    SplinePointFree(spl->first);
    }
}

void SplinePointListFree(SplinePointList *spl) {

    if ( spl==NULL ) return;
    SplinePointsFree(spl);
    // free(spl->spiros);
    free(spl->contour_name);
    chunkfree(spl,sizeof(SplinePointList));
}

void SplineRefigure2(Spline *spline) {
    SplinePoint *from = spline->from, *to = spline->to;
    Spline1D *xsp = &spline->splines[0], *ysp = &spline->splines[1];
    Spline old;

#ifdef DEBUG
    if ( RealNear(from->me.x,to->me.x) && RealNear(from->me.y,to->me.y))
	IError("Zero length spline created");
#endif
    if ( spline->acceptableextrema )
	old = *spline;

    if (    ( from->nextcp.x==from->me.x && from->nextcp.y==from->me.y && from->nextcpindex>=0xfffe )
         || ( to->prevcp.x==to->me.x && to->prevcp.y==to->me.y && from->nextcpindex>=0xfffe ) ) {
	from->nonextcp = to->noprevcp = true;
	from->nextcp = from->me;
	to->prevcp = to->me;
    } else {
	from->nonextcp = to->noprevcp = false;
	if ( from->nextcp.x==from->me.x && from->nextcp.y==from->me.y )
	    to->prevcp = from->me;
	else if ( to->prevcp.x==to->me.x && to->prevcp.y==to->me.y )
	    from->nextcp = to->me;
    }

    if ( from->nonextcp && to->noprevcp )
	/* Ok */;
    else if ( from->nextcp.x!=to->prevcp.x || from->nextcp.y!=to->prevcp.y ) {
	if ( RealNear(from->nextcp.x,to->prevcp.x) &&
		RealNear(from->nextcp.y,to->prevcp.y)) {
	    from->nextcp.x = to->prevcp.x = (from->nextcp.x+to->prevcp.x)/2;
	    from->nextcp.y = to->prevcp.y = (from->nextcp.y+to->prevcp.y)/2;
	} else {
	    IError("Invalid 2nd order spline in SplineRefigure2" );
#ifndef GWW_TEST
	    /* I don't want these to go away when I'm debugging. I want to */
	    /*  know how I got them */
	    from->nextcp.x = to->prevcp.x = (from->nextcp.x+to->prevcp.x)/2;
	    from->nextcp.y = to->prevcp.y = (from->nextcp.y+to->prevcp.y)/2;
#endif
	}
    }

    xsp->d = from->me.x; ysp->d = from->me.y;
    if ( from->nonextcp && to->noprevcp ) {
	spline->islinear = true;
	xsp->c = to->me.x-from->me.x;
	ysp->c = to->me.y-from->me.y;
	xsp->a = xsp->b = 0;
	ysp->a = ysp->b = 0;
    } else {
	/* from p. 393 (Operator Details, curveto) PostScript Lang. Ref. Man. (Red book) */
	xsp->c = 2*(from->nextcp.x-from->me.x);
	ysp->c = 2*(from->nextcp.y-from->me.y);
	xsp->b = to->me.x-from->me.x-xsp->c;
	ysp->b = to->me.y-from->me.y-ysp->c;
	xsp->a = 0;
	ysp->a = 0;
	if ( RealNear(xsp->c,0)) xsp->c=0;
	if ( RealNear(ysp->c,0)) ysp->c=0;
	if ( RealNear(xsp->b,0)) xsp->b=0;
	if ( RealNear(ysp->b,0)) ysp->b=0;
	spline->islinear = false;
	if ( ysp->b==0 && xsp->b==0 )
	    spline->islinear = true;	/* This seems extremely unlikely... */
	if ( from->nextcpselected || to->prevcpselected ) {
            // The convention for tracking selection of quadratic control
	    // points is to use nextcpselected except at the tail of the
	    // list, where it's prevcpselected on the first point.
	    from->nextcpselected = true;
	    to->prevcpselected = false;
	}
    }
    if ( isnan(ysp->b) || isnan(xsp->b) )
	IError("NaN value in spline creation");
    LinearApproxFree(spline->approx);
    spline->approx = NULL;
    spline->knowncurved = false;
    spline->knownlinear = spline->islinear;
    SplineIsLinear(spline);
    spline->isquadratic = !spline->knownlinear;
    spline->order2 = true;

    if ( spline->acceptableextrema ) {
	/* I don't check "d", because changes to that reflect simple */
	/*  translations which will not affect the shape of the spline */
	/* (I don't check "a" because it is always 0 in a quadratic spline) */
	if ( !RealNear(old.splines[0].b,spline->splines[0].b) ||
		!RealNear(old.splines[0].c,spline->splines[0].c) ||
		!RealNear(old.splines[1].b,spline->splines[1].b) ||
		!RealNear(old.splines[1].c,spline->splines[1].c) )
	    spline->acceptableextrema = false;
    }
}

Spline *SplineMake(SplinePoint *from, SplinePoint *to, int order2) {
    if (order2 > 0)
return( SplineMake2(from,to));
    else
return( SplineMake3(from,to));
}

Spline *SplineMake2(SplinePoint *from, SplinePoint *to) {
    Spline *spline = chunkalloc(sizeof(Spline));

    spline->from = from; spline->to = to;
    from->next = to->prev = spline;
    spline->order2 = true;
    SplineRefigure2(spline);
return( spline );
}

Spline *SplineMake3(SplinePoint *from, SplinePoint *to) {
    Spline *spline = chunkalloc(sizeof(Spline));

    spline->from = from; spline->to = to;
    from->next = to->prev = spline;
    SplineRefigure3(spline);
return( spline );
}

void SplinePointFree(SplinePoint *sp) {
    // chunkfree(sp->hintmask,sizeof(HintMask));
	free(sp->name);
    chunkfree(sp,sizeof(SplinePoint));
}

void SplineRefigure(Spline *spline) {
    if ( spline==NULL )
return;
    if ( spline->order2 )
	SplineRefigure2(spline);
    else
	SplineRefigure3(spline);
}

# define RE_NearZero	.00000001
# define RE_Factor	(1024.0*1024.0*1024.0*1024.0*1024.0*2.0) /* 52 bits => divide by 2^51 */

int Within16RoundingErrors(bigreal v1, bigreal v2) {
    bigreal temp=v1*v2;
    bigreal re;

    if ( temp<0 ) /* Ok, if the two values are on different sides of 0 there */
return( false );	/* is no way they can be within a rounding error of each other */
    else if ( temp==0 ) {
	if ( v1==0 )
return( v2<RE_NearZero && v2>-RE_NearZero );
	else
return( v1<RE_NearZero && v1>-RE_NearZero );
    } else if ( v1>0 ) {
	if ( v1>v2 ) {		/* Rounding error from the biggest absolute value */
	    re = v1/ (RE_Factor/16);
return( v1-v2 < re );
	} else {
	    re = v2/ (RE_Factor/16);
return( v2-v1 < re );
	}
    } else {
	if ( v1<v2 ) {
	    re = v1/ (RE_Factor/16);	/* This will be a negative number */
return( v1-v2 > re );
	} else {
	    re = v2/ (RE_Factor/16);
return( v2-v1 > re );
	}
    }
}

/* An IEEE double has 52 bits of precision. So one unit of rounding error will be */
/*  the number divided by 2^51 */
# define D_RE_Factor	(1024.0*1024.0*1024.0*1024.0*1024.0*2.0)
/* But that's not going to work near 0, so, since the t values we care about */
/*  are [0,1], let's use 1.0/D_RE_Factor */

double CheckExtremaForSingleBitErrors(const Spline1D *sp, double t, double othert) {
    double u1, um1;
    double slope, slope1, slopem1;
    int err;
    double diff, factor;

    if ( t<0 || t>1 )
return( t );

    factor = t*0x40000/D_RE_Factor;
    if ( (diff = t-othert)<0 ) diff= -diff;
    if ( factor>diff/4 && diff!=0 )		/* This little check is to insure we don't skip beyond the well of this extremum into the next */
	factor = diff/4;

    slope = (3*(double) sp->a*t+2*sp->b)*t+sp->c;
    if ( slope<0 ) slope = -slope;

    for ( err = 0x40000; err!=0; err>>=1 ) {
	u1 = t+factor;
	slope1 = (3*(double) sp->a*u1+2*sp->b)*u1+sp->c;
	if ( slope1<0 ) slope1 = -slope1;

	um1 = t-factor;
	slopem1 = (3*(double) sp->a*um1+2*sp->b)*um1+sp->c;
	if ( slopem1<0 ) slopem1 = -slopem1;

	if ( slope1<slope && slope1<=slopem1 && u1<=1.0 ) {
	    t = u1;
	} else if ( slopem1<slope && slopem1<=slope1 && um1>=0.0 ) {
	    t = um1;
	}
	factor /= 2.0;
    }
    /* that seems as good as it gets */

return( t );
}

void SplineFindExtrema(const Spline1D *sp, extended *_t1, extended *_t2 ) {
    extended t1= -1, t2= -1;
    extended b2_fourac;

    /* Find the extreme points on the curve */
    /*  Set to -1 if there are none or if they are outside the range [0,1] */
    /*  Order them so that t1<t2 */
    /*  If only one valid extremum it will be t1 */
    /*  (Does not check the end points unless they have derivative==0) */
    /*  (Does not check to see if d/dt==0 points are inflection points (rather than extrema) */
    if ( sp->a!=0 ) {
	/* cubic, possibly 2 extrema (possibly none) */
	b2_fourac = 4*(extended) sp->b*sp->b - 12*(extended) sp->a*sp->c;
	if ( b2_fourac>=0 ) {
	    b2_fourac = sqrt(b2_fourac);
	    t1 = (-2*sp->b - b2_fourac) / (6*sp->a);
	    t2 = (-2*sp->b + b2_fourac) / (6*sp->a);
	    t1 = CheckExtremaForSingleBitErrors(sp,t1,t2);
	    t2 = CheckExtremaForSingleBitErrors(sp,t2,t1);
	    if ( t1>t2 ) { extended temp = t1; t1 = t2; t2 = temp; }
	    else if ( t1==t2 ) t2 = -1;
	    if ( RealNear(t1,0)) t1=0; else if ( RealNear(t1,1)) t1=1;
	    if ( RealNear(t2,0)) t2=0; else if ( RealNear(t2,1)) t2=1;
	    if ( t2<=0 || t2>=1 ) t2 = -1;
	    if ( t1<=0 || t1>=1 ) { t1 = t2; t2 = -1; }
	}
    } else if ( sp->b!=0 ) {
	/* Quadratic, at most one extremum */
	t1 = -sp->c/(2.0*(extended) sp->b);
	if ( t1<=0 || t1>=1 ) t1 = -1;
    } else /*if ( sp->c!=0 )*/ {
	/* linear, no extrema */
    }
    *_t1 = t1; *_t2 = t2;
}

int IntersectLines(BasePoint *inter,
	BasePoint *line1_1, BasePoint *line1_2,
	BasePoint *line2_1, BasePoint *line2_2) {
    // A lot of functions call this with the same address as an input and the output.
    // In order to avoid unexpected behavior, we delay writing to the output until the end.
    bigreal s1, s2;
    BasePoint _output;
    BasePoint * output = &_output;
    if ( line1_1->x == line1_2->x ) {
        // Line 1 is vertical.
	output->x = line1_1->x;
	if ( line2_1->x == line2_2->x ) {
            // Line 2 is vertical.
	    if ( line2_1->x!=line1_1->x )
              return( false );		/* Parallel vertical lines */
	    output->y = (line1_1->y+line2_1->y)/2;
	} else {
	    output->y = line2_1->y + (output->x-line2_1->x) * (line2_2->y - line2_1->y)/(line2_2->x - line2_1->x);
        }
        *inter = *output;
        return( true );
    } else if ( line2_1->x == line2_2->x ) {
        // Line 2 is vertical, but we know that line 1 is not.
	output->x = line2_1->x;
	output->y = line1_1->y + (output->x-line1_1->x) * (line1_2->y - line1_1->y)/(line1_2->x - line1_1->x);
        *inter = *output;
        return( true );
    } else {
        // Both lines are oblique.
	s1 = (line1_2->y - line1_1->y)/(line1_2->x - line1_1->x);
	s2 = (line2_2->y - line2_1->y)/(line2_2->x - line2_1->x);
	if ( RealNear(s1,s2)) {
	    if ( !RealNear(line1_1->y + (line2_1->x-line1_1->x) * s1,line2_1->y))
              return( false );
	    output->x = (line1_2->x+line2_2->x)/2;
	    output->y = (line1_2->y+line2_2->y)/2;
	} else {
	    output->x = (s1*line1_1->x - s2*line2_1->x - line1_1->y + line2_1->y)/(s1-s2);
	    output->y = line1_1->y + (output->x-line1_1->x) * s1;
	}
        *inter = *output;
        return( true );
    }
}

static int MinMaxWithin(Spline *spline) {
    extended dx, dy;
    int which;
    extended t1, t2;
    extended w;
    /* We know that this "spline" is basically one dimensional. As long as its*/
    /*  extrema are between the start and end points on that line then we can */
    /*  treat it as a line. If the extrema are way outside the line segment */
    /*  then it's a line that backtracks on itself */

    if ( (dx = spline->to->me.x - spline->from->me.x)<0 ) dx = -dx;
    if ( (dy = spline->to->me.y - spline->from->me.y)<0 ) dy = -dy;
    which = dx<dy;
    SplineFindExtrema(&spline->splines[which],&t1,&t2);
    if ( t1==-1 )
return( true );
    w = ((spline->splines[which].a*t1 + spline->splines[which].b)*t1
	     + spline->splines[which].c)*t1 + spline->splines[which].d;
    if ( RealNear(w, (&spline->to->me.x)[which]) || RealNear(w, (&spline->from->me.x)[which]) )
	/* Close enough */;
    else if ( (w<(&spline->to->me.x)[which] && w<(&spline->from->me.x)[which]) ||
	    (w>(&spline->to->me.x)[which] && w>(&spline->from->me.x)[which]) )
return( false );		/* Outside */

    w = ((spline->splines[which].a*t2 + spline->splines[which].b)*t2
	     + spline->splines[which].c)*t2 + spline->splines[which].d;
    if ( RealNear(w, (&spline->to->me.x)[which]) || RealNear(w, (&spline->from->me.x)[which]) )
	/* Close enough */;
    else if ( (w<(&spline->to->me.x)[which] && w<(&spline->from->me.x)[which]) ||
	    (w>(&spline->to->me.x)[which] && w>(&spline->from->me.x)[which]) )
return( false );		/* Outside */

return( true );
}

int SplineIsLinear(Spline *spline) {
    bigreal t1,t2, t3,t4;
    int ret;

    if ( spline->knownlinear )
return( true );
    if ( spline->knowncurved )
return( false );

    if ( spline->splines[0].a==0 && spline->splines[0].b==0 &&
	    spline->splines[1].a==0 && spline->splines[1].b==0 )
return( true );

    /* Something is linear if the control points lie on the line between the */
    /*  two base points */

    /* Vertical lines */
    if ( RealNear(spline->from->me.x,spline->to->me.x) ) {
	ret = RealNear(spline->from->me.x,spline->from->nextcp.x) &&
	    RealNear(spline->from->me.x,spline->to->prevcp.x);
	if ( ret && ! ((spline->from->nextcp.y >= spline->from->me.y &&
		        spline->from->nextcp.y <= spline->to->me.y &&
		        spline->to->prevcp.y >= spline->from->me.y &&
		        spline->to->prevcp.y <= spline->to->me.y ) ||
		       (spline->from->nextcp.y <= spline->from->me.y &&
		        spline->from->nextcp.y >= spline->to->me.y &&
		        spline->to->prevcp.y <= spline->from->me.y &&
		        spline->to->prevcp.y >= spline->to->me.y )) )
	    ret = MinMaxWithin(spline);
    /* Horizontal lines */
    } else if ( RealNear(spline->from->me.y,spline->to->me.y) ) {
	ret = RealNear(spline->from->me.y,spline->from->nextcp.y) &&
	    RealNear(spline->from->me.y,spline->to->prevcp.y);
	if ( ret && ! ((spline->from->nextcp.x >= spline->from->me.x &&
		       spline->from->nextcp.x <= spline->to->me.x &&
		       spline->to->prevcp.x >= spline->from->me.x &&
		       spline->to->prevcp.x <= spline->to->me.x) ||
		      (spline->from->nextcp.x <= spline->from->me.x &&
		       spline->from->nextcp.x >= spline->to->me.x &&
		       spline->to->prevcp.x <= spline->from->me.x &&
		       spline->to->prevcp.x >= spline->to->me.x)) )
	    ret = MinMaxWithin(spline);
    } else {
	ret = true;
	t1 = (spline->from->nextcp.y-spline->from->me.y)/(spline->to->me.y-spline->from->me.y);
	t2 = (spline->from->nextcp.x-spline->from->me.x)/(spline->to->me.x-spline->from->me.x);
	t3 = (spline->to->me.y-spline->to->prevcp.y)/(spline->to->me.y-spline->from->me.y);
	t4 = (spline->to->me.x-spline->to->prevcp.x)/(spline->to->me.x-spline->from->me.x);
	ret = (Within16RoundingErrors(t1,t2) || (RealApprox(t1,0) && RealApprox(t2,0))) &&
		(Within16RoundingErrors(t3,t4) || (RealApprox(t3,0) && RealApprox(t4,0)));
	if ( ret ) {
	    if ( t1<0 || t2<0 || t3<0 || t4<0 ||
		    t1>1 || t2>1 || t3>1 || t4>1 )
		ret = MinMaxWithin(spline);
	}
    }
    spline->knowncurved = !ret;
    spline->knownlinear = ret;
    if ( ret ) {
	/* A few places that if the spline is knownlinear then its splines[?] */
	/*  are linear. So give the linear version and not that suggested by */
	/*  the control points */
	spline->splines[0].a = spline->splines[0].b = 0;
	spline->splines[0].d = spline->from->me.x;
	spline->splines[0].c = spline->to->me.x-spline->from->me.x;
	spline->splines[1].a = spline->splines[1].b = 0;
	spline->splines[1].d = spline->from->me.y;
	spline->splines[1].c = spline->to->me.y-spline->from->me.y;
    }
return( ret );
}

static bigreal FindZero5(bigreal w[7],bigreal tlow, bigreal thigh) {
    /* Somewhere between tlow and thigh there is a value of t where w(t)==0 */
    /*  It is conceiveable that there might be 3 such ts if there are some high frequency effects */
    /*  but I ignore that for now */
    bigreal t, test;
    int bot_negative;

    t = tlow;
    test = ((((w[5]*t+w[4])*t+w[3])*t+w[2])*t+w[1])*t + w[0];
    bot_negative = test<0;

    for (;;) {
	t = (thigh+tlow)/2;
	if ( thigh==t || tlow==t )
return( t );		/* close as we can get */
	test = ((((w[5]*t+w[4])*t+w[3])*t+w[2])*t+w[1])*t + w[0];
	if ( test==0 )
return( t );
	if ( bot_negative ) {
	    if ( test<0 )
		tlow = t;
	    else
		thigh = t;
	} else {
	    if ( test<0 )
		thigh = t;
	    else
		tlow = t;
	}
    }
}

static bigreal FindZero3(bigreal w[7],bigreal tlow, bigreal thigh) {
    /* Somewhere between tlow and thigh there is a value of t where w(t)==0 */
    /*  It is conceiveable that there might be 3 such ts if there are some high frequency effects */
    /*  but I ignore that for now */
    bigreal t, test;
    int bot_negative;

    t = tlow;
    test = ((w[3]*t+w[2])*t+w[1])*t + w[0];
    bot_negative = test<0;

    for (;;) {
	t = (thigh+tlow)/2;
	if ( thigh==t || tlow==t )
return( t );		/* close as we can get */
	test = ((w[3]*t+w[2])*t+w[1])*t + w[0];
	if ( test==0 )
return( t );
	if ( bot_negative ) {
	    if ( test<0 )
		tlow = t;
	    else
		thigh = t;
	} else {
	    if ( test<0 )
		thigh = t;
	    else
		tlow = t;
	}
    }
}

bigreal SplineMinDistanceToPoint(Spline *s, BasePoint *p) {
    /* So to find the minimum distance we want the sqrt( (sx(t)-px)^2 + (sy(t)-py)^2 ) */
    /*  Same minima as (sx(t)-px)^2 + (sy(t)-py)^2, which is easier to deal with */
    bigreal w[7];
    Spline1D *x = &s->splines[0], *y = &s->splines[1];
    bigreal off[2], best;

    off[0] = (x->d-p->x); off[1] = (y->d-p->y);

    w[6] = (x->a*x->a) + (y->a*y->a);
    w[5] = 2*(x->a*x->b + y->a*y->b);
    w[4] = (x->b*x->b) + 2*(x->a*x->c) + (y->b*y->b) + 2*(y->a*y->c);
    w[3] = 2* (x->b*x->c + x->a*off[0] + y->b*y->c + y->a*off[1]);
    w[2] = (x->c*x->c) + 2*(x->b*off[0]) + (y->c*y->c) + 2*y->b*off[1];
    w[1] = 2*(x->c*off[0] + y->c*off[1]);
    w[0] = off[0]*off[0] + off[1]*off[1];

    /* Take derivative */
    w[0] = w[1];
    w[1] = 2*w[2];
    w[2] = 3*w[3];
    w[3] = 4*w[4];
    w[4] = 5*w[5];
    w[5] = 6*w[6];
    w[6] = 0;

    if ( w[5]!=0 ) {
	bigreal tzeros[8], t, incr, test, lasttest, zerot;
	int i, zcnt=0;
	/* Well, we've got a 5th degree poly and no way to play cute tricks. */
	/* brute force it */
	incr = 1.0/1024;
	lasttest = w[0];
	for ( t = incr; t<=1.0; t += incr ) {
	    test = ((((w[5]*t+w[4])*t+w[3])*t+w[2])*t+w[1])*t + w[0];
	    if ( test==0 )
		tzeros[zcnt++] = t;
	    else {
		if ( lasttest!=0 && (test>0) != (lasttest>0) ) {
		    zerot = FindZero5(w,t-incr,t);
		    if ( zerot>0 )
			tzeros[zcnt++] = zerot;
		}
	    }
	    lasttest = test;
	}
	best = off[0]*off[0] + off[1]*off[1];		/* t==0 */
	test = (x->a+x->b+x->c+off[0])*(x->a+x->b+x->c+off[0]) +
		(y->a+y->b+y->c+off[1])*(y->a+y->b+y->c+off[1]); 	/* t==1 */
	if ( best>test ) best = test;
	for ( i=0; i<zcnt; ++i ) {
	    bigreal tx, ty;
	    tx = ((x->a*tzeros[i]+x->b)*tzeros[i]+x->c)*tzeros[i] + off[0];
	    ty = ((y->a*tzeros[i]+y->b)*tzeros[i]+y->c)*tzeros[i] + off[1];
	    test = tx*tx + ty*ty;
	    if ( best>test ) best = test;
	}
return( sqrt(best));
    } else if ( w[4]==0 && w[3]!=0 ) {
	/* Started with a quadratic -- now, find 0s of a cubic */
	/* We could find the extrema, so we have a bunch of monotonics */
	/* Or we could brute force it as above */
	bigreal tzeros[8], test, zerot;
	bigreal quad[3], disc, e[5], t1, t2;
	int i, zcnt=0, ecnt;

	quad[2] = 3*w[3]; quad[1] = 2*w[2]; quad[0] = w[1];
	disc = (-quad[1]*quad[1] - 4*quad[2]*quad[0]);
	e[0] = 0;
	if ( disc<0 ) {
	    e[1] = 1.0;
	    ecnt = 2;
	} else
	    disc = sqrt(disc);
	t1 = (-w[1] - disc) / (2*w[2]);
	t2 = (-w[1] + disc) / (2*w[2]);
	if ( t1>t2 ) {
	    bigreal temp = t1;
	    t1 = t2;
	    t2 = temp;
	}
	ecnt=1;
	if ( t1>0 && t1<1 )
	    e[ecnt++] = t1;
	if ( t2>0 && t2<1 && t1!=t2 )
	    e[ecnt++] = t2;
	e[ecnt++] = 1.0;
	for ( i=1; i<ecnt; ++i ) {
	    zerot = FindZero3(w,e[i-1],e[i]);
	    if ( zerot>0 )
		tzeros[zcnt++] = zerot;
	}
	best = off[0]*off[0] + off[1]*off[1];		/* t==0 */
	test = (x->b+x->c+off[0])*(x->b+x->c+off[0]) +
		(y->b+y->c+off[1])*(y->b+y->c+off[1]); 	/* t==1 */
	if ( best>test ) best = test;
	for ( i=0; i<zcnt; ++i ) {
	    bigreal tx, ty;
	    tx = (x->b*tzeros[i]+x->c)*tzeros[i] + off[0];
	    ty = (y->b*tzeros[i]+y->c)*tzeros[i] + off[1];
	    test = tx*tx + ty*ty;
	    if ( best>test ) best = test;
	}
return( sqrt(best));
    } else if ( w[2]==0 && w[1]!=0 ) {
	/* Started with a line */
	bigreal t = -w[0]/w[1], test, best;
	best = off[0]*off[0] + off[1]*off[1];		/* t==0 */
	test = (x->c+off[0])*(x->c+off[0]) + (y->c+off[1])*(y->c+off[1]); 	/* t==1 */
	if ( best>test ) best = test;
	if ( t>0 && t<1 ) {
	    test = (x->c*t+off[0])*(x->c*t+off[0]) + (y->c*t+off[1])*(y->c*t+off[1]);
	    if ( best>test ) best = test;
	}
return(sqrt(best));
    } else if ( w[4]!=0 && w[3]!=0 && w[2]!=0 && w[1]!=0 ) {
	IError( "Impossible condition in SplineMinDistanceToPoint");
    } else {
	/* It's a point, minimum distance is the only distance */
return( sqrt(off[0]*off[0] + off[1]*off[1]) );
    }
return( -1 );
}

/* This returns all real solutions, even those out of bounds */
/* I use -999999 as an error flag, since we're really only interested in */
/*  solns near 0 and 1 that should be ok. -1 is perhaps a little too close */
/* Sigh. When solutions are near 0, the rounding errors are appalling. */
int _CubicSolve(const Spline1D *sp,bigreal sought, extended ts[3]) {
    extended d, xN, yN, delta2, temp, delta, h, t2, t3, theta;
    extended sa=sp->a, sb=sp->b, sc=sp->c, sd=sp->d-sought;
    int i=0;

    ts[0] = ts[1] = ts[2] = -999999;
    if ( sd==0 && sa!=0 ) {
	/* one of the roots is 0, the other two are the soln of a quadratic */
	ts[0] = 0;
	if ( sc==0 ) {
	    ts[1] = -sb/(extended) sa;	/* two zero roots */
	} else {
	    temp = sb*(extended) sb-4*(extended) sa*sc;
	    if ( RealNear(temp,0))
		ts[1] = -sb/(2*(extended) sa);
	    else if ( temp>=0 ) {
		temp = sqrt(temp);
		ts[1] = (-sb+temp)/(2*(extended) sa);
		ts[2] = (-sb-temp)/(2*(extended) sa);
	    }
	}
    } else if ( sa!=0 ) {
    /* http://www.m-a.org.uk/eb/mg/mg077ch.pdf */
    /* this nifty solution to the cubic neatly avoids complex arithmetic */
	xN = -sb/(3*(extended) sa);
	yN = ((sa*xN + sb)*xN+sc)*xN + sd;

	delta2 = (sb*(extended) sb-3*(extended) sa*sc)/(9*(extended) sa*sa);
	/*if ( RealWithin(delta2,0,.00000001) ) delta2 = 0;*/

	/* the descriminant is yN^2-h^2, but delta might be <0 so avoid using h */
	d = yN*yN - 4*sa*sa*delta2*delta2*delta2;
	if ( ((yN>.01 || yN<-.01) && RealNear(d/yN,0)) || ((yN<=.01 && yN>=-.01) && RealNear(d,0)) )
	    d = 0;
	if ( d>0 ) {
	    temp = sqrt(d);
	    t2 = (-yN-temp)/(2*sa);
	    t2 = (t2==0) ? 0 : (t2<0) ? -pow(-t2,1./3.) : pow(t2,1./3.);
	    t3 = (-yN+temp)/(2*sa);
	    t3 = t3==0 ? 0 : (t3<0) ? -pow(-t3,1./3.) : pow(t3,1./3.);
	    ts[0] = xN + t2 + t3;
	} else if ( d<0 ) {
	    if ( delta2>=0 ) {
		delta = sqrt(delta2);
		h = 2*sa*delta2*delta;
		temp = -yN/h;
		if ( temp>=-1.0001 && temp<=1.0001 ) {
		    if ( temp<-1 ) temp = -1; else if ( temp>1 ) temp = 1;
		    theta = acos(temp)/3;
		    ts[i++] = xN+2*delta*cos(theta);
		    ts[i++] = xN+2*delta*cos(2.0943951+theta);	/* 2*pi/3 */
		    ts[i++] = xN+2*delta*cos(4.1887902+theta);	/* 4*pi/3 */
		}
	    }
	} else if ( /* d==0 && */ delta2!=0 ) {
	    delta = yN/(2*sa);
	    delta = delta==0 ? 0 : delta>0 ? pow(delta,1./3.) : -pow(-delta,1./3.);
	    ts[i++] = xN + delta;	/* this root twice, but that's irrelevant to me */
	    ts[i++] = xN - 2*delta;
	} else if ( /* d==0 && */ delta2==0 ) {
	    if ( xN>=-0.0001 && xN<=1.0001 ) ts[0] = xN;
	}
    } else if ( sb!=0 ) {
	extended d = sc*(extended) sc-4*(extended) sb*sd;
	if ( d<0 && RealNear(d,0)) d=0;
	if ( d<0 )
return(false);		/* All roots imaginary */
	d = sqrt(d);
	ts[0] = (-sc-d)/(2*(extended) sb);
	ts[1] = (-sc+d)/(2*(extended) sb);
    } else if ( sc!=0 ) {
	ts[0] = -sd/(extended) sc;
    } else {
	/* If it's a point then either everything is a solution, or nothing */
    }
return( ts[0]!=-999999 );
}

int _QuarticSolve(Quartic *q,extended ts[4]) {
    extended extrema[5];
    Spline1D sp;
    int ecnt = 0, i, zcnt;

    /* Two special cases */
    if ( q->a==0 ) {	/* It's really a cubic */
	sp.a = q->b;
	sp.b = q->c;
	sp.c = q->d;
	sp.d = q->e;
	ts[3] = -999999;
return( _CubicSolve(&sp,0,ts));
    } else if ( q->e==0 ) {	/* we can factor out a zero root */
	sp.a = q->a;
	sp.b = q->b;
	sp.c = q->c;
	sp.d = q->d;
	ts[0] = 0;
return( _CubicSolve(&sp,0,ts+1)+1);
    }

    sp.a = 4*q->a;
    sp.b = 3*q->b;
    sp.c = 2*q->c;
    sp.d = q->d;
    if ( _CubicSolve(&sp,0,extrema)) {
	ecnt = 1;
	if ( extrema[1]!=-999999 ) {
	    ecnt = 2;
	    if ( extrema[1]<extrema[0] ) {
		extended temp = extrema[1]; extrema[1] = extrema[0]; extrema[0]=temp;
	    }
	    if ( extrema[2]!=-999999 ) {
		ecnt = 3;
		if ( extrema[2]<extrema[0] ) {
		    extended temp = extrema[2]; extrema[2] = extrema[0]; extrema[0]=temp;
		}
		if ( extrema[2]<extrema[1] ) {
		    extended temp = extrema[2]; extrema[2] = extrema[1]; extrema[1]=temp;
		}
	    }
	}
    }
    for ( i=ecnt-1; i>=0 ; --i )
	extrema[i+1] = extrema[i];
    /* Upper and lower bounds within which we'll search */
    extrema[0] = -999;
    extrema[ecnt+1] = 999;
    ecnt += 2;
    /* divide into monotonic sections & use binary search to find zeroes */
    for ( i=zcnt=0; i<ecnt-1; ++i ) {
	extended top, bottom, val;
	extended topt, bottomt, t;
	topt = extrema[i+1];
	bottomt = extrema[i];
	top = (((q->a*topt+q->b)*topt+q->c)*topt+q->d)*topt+q->e;
	bottom = (((q->a*bottomt+q->b)*bottomt+q->c)*bottomt+q->d)*bottomt+q->e;
	if ( top<bottom ) {
	    extended temp = top; top = bottom; bottom = temp;
	    temp = topt; topt = bottomt; bottomt = temp;
	}
	if ( bottom>.001 )	/* this monotonic is all above 0 */
    continue;
	if ( top<-.001 )	/* this monotonic is all below 0 */
    continue;
	if ( bottom>0 ) {
	    ts[zcnt++] = bottomt;
    continue;
	}
	if ( top<0 ) {
	    ts[zcnt++] = topt;
    continue;
	}
	for (;;) {
	    t = (topt+bottomt)/2;
	    if ( isnan(t) ) {
		break;
	    } else if ( t==topt || t==bottomt ) {
		ts[zcnt++] = t;
	break;
	    }

	    val = (((q->a*t+q->b)*t+q->c)*t+q->d)*t+q->e;
	    if ( val>-.0001 && val<.0001 ) {
		ts[zcnt++] = t;
	break;
	    } else if ( val>0 ) {
		top = val;
		topt = t;
	    } else {
		bottom = val;
		bottomt = t;
	    }
	}
    }
    for ( i=zcnt; i<4; ++i )
	ts[i] = -999999;
return( zcnt );
}

/* calculating the actual length of a spline is hard, this gives a very */
/*  rough (but quick) approximation */
static bigreal SplineLenApprox(Spline *spline) {
    bigreal len, slen, temp;

    if ( (temp = spline->to->me.x-spline->from->me.x)<0 ) temp = -temp;
    len = temp;
    if ( (temp = spline->to->me.y-spline->from->me.y)<0 ) temp = -temp;
    len += temp;
    if ( !spline->to->noprevcp || !spline->from->nonextcp ) {
	if ( (temp = spline->from->nextcp.x-spline->from->me.x)<0 ) temp = -temp;
	slen = temp;
	if ( (temp = spline->from->nextcp.y-spline->from->me.y)<0 ) temp = -temp;
	slen += temp;
	if ( (temp = spline->to->prevcp.x-spline->from->nextcp.x)<0 ) temp = -temp;
	slen += temp;
	if ( (temp = spline->to->prevcp.y-spline->from->nextcp.y)<0 ) temp = -temp;
	slen += temp;
	if ( (temp = spline->to->me.x-spline->to->prevcp.x)<0 ) temp = -temp;
	slen += temp;
	if ( (temp = spline->to->me.y-spline->to->prevcp.y)<0 ) temp = -temp;
	slen += temp;
	len = (len + slen)/2;
    }
return( len );
}

FitPoint *SplinesFigureFPsBetween(SplinePoint *from, SplinePoint *to,
	int *tot) {
    int cnt, i, j, pcnt;
    bigreal len, slen, lbase;
    SplinePoint *np;
    FitPoint *fp;
    bigreal _lens[10], *lens = _lens;
    int _cnts[10], *cnts = _cnts;
    /* I used just to give every spline 10 points. But that gave much more */
    /*  weight to small splines than to big ones */

    cnt = 0;
    for ( np = from->next->to; ; np = np->next->to ) {
	++cnt;
	if ( np==to )
    break;
    }
    if ( cnt>10 ) {
	lens = malloc(cnt*sizeof(bigreal));
	cnts = malloc(cnt*sizeof(int));
    }
    cnt = 0; len = 0;
    for ( np = from->next->to; ; np = np->next->to ) {
	lens[cnt] = SplineLenApprox(np->prev);
	len += lens[cnt];
	++cnt;
	if ( np==to )
    break;
    }
    if ( len!=0 ) {
	pcnt = 0;
	for ( i=0; i<cnt; ++i ) {
	    int pnts = rint( (10*cnt*lens[i])/len );
	    if ( pnts<2 ) pnts = 2;
	    cnts[i] = pnts;
	    pcnt += pnts;
	}
    } else
	pcnt = 2*cnt;

    fp = malloc((pcnt+1)*sizeof(FitPoint)); i = 0;
    if ( len==0 ) {
	for ( i=0; i<=pcnt; ++i ) {
	    fp[i].t = i/(pcnt);
	    fp[i].p.x = from->me.x;
	    fp[i].p.y = from->me.y;
	}
    } else {
	lbase = 0;
	for ( i=cnt=0, np = from->next->to; ; np = np->next->to, ++cnt ) {
	    slen = SplineLenApprox(np->prev);
	    for ( j=0; j<cnts[cnt]; ++j ) {
		bigreal t = j/(bigreal) cnts[cnt];
		fp[i].t = (lbase+ t*slen)/len;
		fp[i].p.x = ((np->prev->splines[0].a*t+np->prev->splines[0].b)*t+np->prev->splines[0].c)*t + np->prev->splines[0].d;
		fp[i++].p.y = ((np->prev->splines[1].a*t+np->prev->splines[1].b)*t+np->prev->splines[1].c)*t + np->prev->splines[1].d;
	    }
	    lbase += slen;
	    if ( np==to )
	break;
	}
    }
    if ( cnts!=_cnts ) free(cnts);
    if ( lens!=_lens ) free(lens);

    *tot = i;
	
return( fp );
}

static int SplinePointCategory(SplinePoint *sp) {
    enum pointtype pt;

    pt = pt_corner;
    if ( sp->next==NULL && sp->prev==NULL )
	;
    else if ( (sp->next!=NULL && sp->next->to->me.x==sp->me.x && sp->next->to->me.y==sp->me.y) ||
	    (sp->prev!=NULL && sp->prev->from->me.x==sp->me.x && sp->prev->from->me.y==sp->me.y ))
	;
    else if ( sp->next==NULL ) {
	pt = sp->noprevcp ? pt_corner : pt_curve;
    } else if ( sp->prev==NULL ) {
	pt = sp->nonextcp ? pt_corner : pt_curve;
    } else if ( sp->nonextcp && sp->noprevcp ) {
	;
    } else {
	BasePoint ndir, ncdir, ncunit, pdir, pcdir, pcunit;
	bigreal nlen, nclen, plen, pclen;
	bigreal cross, bounds;

	ncdir.x = sp->nextcp.x - sp->me.x; ncdir.y = sp->nextcp.y - sp->me.y;
	pcdir.x = sp->prevcp.x - sp->me.x; pcdir.y = sp->prevcp.y - sp->me.y;
	ndir.x = ndir.y = pdir.x = pdir.y = 0;
	if ( sp->next!=NULL ) {
	    ndir.x = sp->next->to->me.x - sp->me.x; ndir.y = sp->next->to->me.y - sp->me.y;
	}
	if ( sp->prev!=NULL ) {
	    pdir.x = sp->prev->from->me.x - sp->me.x; pdir.y = sp->prev->from->me.y - sp->me.y;
	}
	nclen = sqrt(ncdir.x*ncdir.x + ncdir.y*ncdir.y);
	pclen = sqrt(pcdir.x*pcdir.x + pcdir.y*pcdir.y);
	nlen = sqrt(ndir.x*ndir.x + ndir.y*ndir.y);
	plen = sqrt(pdir.x*pdir.x + pdir.y*pdir.y);
	ncunit = ncdir; pcunit = pcdir;
	if ( nclen!=0 ) { ncunit.x /= nclen; ncunit.y /= nclen; }
	if ( pclen!=0 ) { pcunit.x /= pclen; pcunit.y /= pclen; }
	if ( nlen!=0 ) { ndir.x /= nlen; ndir.y /= nlen; }
	if ( plen!=0 ) { pdir.x /= plen; pdir.y /= plen; }

	/* find out which side has the shorter control vector. Cross that vector */
	/*  with the normal of the unit vector on the other side. If the */
	/*  result is less than 1 em-unit then we've got colinear control points */
	/*  (within the resolution of the integer grid) */
	/* Not quite... they could point in the same direction */
        if ( sp->pointtype==pt_curve )
            bounds = 4.0;
        else
            bounds = 1.0;
	if ( nclen!=0 && pclen!=0 &&
		((nclen>=pclen && (cross = pcdir.x*ncunit.y - pcdir.y*ncunit.x)<bounds && cross>-bounds ) ||
		 (pclen>nclen && (cross = ncdir.x*pcunit.y - ncdir.y*pcunit.x)<bounds && cross>-bounds )) &&
		 ncdir.x*pcdir.x + ncdir.y*pcdir.y < 0 )
	    pt = pt_curve;
	/* Cross product of control point with unit vector normal to line in */
	/*  opposite direction should be less than an em-unit for a tangent */
	else if (    (   nclen==0 && pclen!=0
	              && (cross = pcdir.x*ndir.y-pcdir.y*ndir.x)<bounds
	              && cross>-bounds && (pcdir.x*ndir.x+pcdir.y*ndir.y)<0 )
	          ||
	             (   pclen==0 && nclen!=0
	              && (cross = ncdir.x*pdir.y-ncdir.y*pdir.x)<bounds
	              && cross>-bounds && (ncdir.x*pdir.x+ncdir.y*pdir.y)<0 ) )
	    pt = pt_tangent;

	if (pt == pt_curve &&
		((sp->nextcp.x==sp->me.x && sp->prevcp.x==sp->me.x && sp->nextcp.y!=sp->me.y) ||
		 (sp->nextcp.y==sp->me.y && sp->prevcp.y==sp->me.y && sp->nextcp.x!=sp->me.x)))
	    pt = pt_hvcurve;
    }
    return pt;
}

static enum pointtype SplinePointDowngrade(int current, int geom) {
	enum pointtype np = current;

	if ( current==pt_curve && geom!=pt_curve ) {
		if ( geom==pt_hvcurve )
			np = pt_curve;
		else
			np = pt_corner;
	} else if ( current==pt_hvcurve && geom!=pt_hvcurve ) {
		if ( geom==pt_curve )
			np = pt_curve;
		else
			np = pt_corner;
	} else if ( current==pt_tangent && geom!=pt_tangent ) {
		np = pt_corner;
	}

	return np;
}

// Assumes flag combinations are already verified. Only returns false
// when called with check_compat
int _SplinePointCategorize(SplinePoint *sp, int flags) {
	enum pointtype geom, dg, cur;

	if ( flags & pconvert_flag_none )
		// No points selected for conversion -- keep type as is
		return true;
	if ( flags & pconvert_flag_smooth && sp->pointtype == pt_corner )
		// Convert only "smooth" points, not corners
		return true;

	geom = SplinePointCategory(sp);
	dg = SplinePointDowngrade(sp->pointtype, geom);

	if ( flags & pconvert_flag_incompat && sp->pointtype == dg )
		// Only convert points incompatible with current type
		return true;

	if ( flags & pconvert_flag_by_geom ) {
		if ( ! ( flags & pconvert_flag_hvcurve ) && geom == pt_hvcurve )
			sp->pointtype = pt_curve;
		else
			sp->pointtype = geom;
	} else if ( flags & pconvert_flag_downgrade ) {
		sp->pointtype = dg;
	} else if ( flags & pconvert_flag_force_type ) {
		if ( sp->pointtype != dg ) {
			cur = sp->pointtype;
			sp->pointtype = dg;
			/* SPChangePointType(sp,cur); */
		}
	} else if ( flags & pconvert_flag_check_compat ) {
		if ( sp->pointtype != dg )
			return false;
	}
	return true;
}

void SplinePointCategorize(SplinePoint *sp) {
	_SplinePointCategorize(sp, pconvert_flag_all|pconvert_flag_by_geom);
}

static void SplinePointReCategorize(SplinePoint *sp,int oldpt) {
    SplinePointCategorize(sp);
    if ( sp->pointtype!=oldpt ) {
	if ( sp->pointtype==pt_curve && oldpt==pt_hvcurve &&
		((sp->nextcp.x == sp->me.x && sp->nextcp.y != sp->me.y ) ||
		 (sp->nextcp.y == sp->me.y && sp->nextcp.x != sp->me.x )))
	    sp->pointtype = pt_hvcurve;
    }
}

void SplinesRemoveBetween(SplinePoint *from, SplinePoint *to, int type) {
    int tot;
    FitPoint *fp;
    SplinePoint *np, oldfrom;
    int oldfpt = from->pointtype, oldtpt = to->pointtype;
    Spline *sp;
    int order2 = from->next->order2;

    oldfrom = *from;
    fp = SplinesFigureFPsBetween(from,to,&tot);

    if ( type==1 )
	ApproximateSplineFromPointsSlopes(from,to,fp,tot-1,order2,mt_levien);
    else
	ApproximateSplineFromPoints(from,to,fp,tot-1,order2);

    /* Have to do the frees after the approximation because the approx */
    /*  uses the splines to determine slopes */
    for ( sp = oldfrom.next; ; ) {
	np = sp->to;
	SplineFree(sp);
	if ( np==to )
    break;
	sp = np->next;
	// SplinePointMDFree(sc,np);
    }
    
    free(fp);

    SplinePointReCategorize(from,oldfpt);
    SplinePointReCategorize(to,oldtpt);
}

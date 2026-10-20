// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef _SEEN_SPLINEFONT_H_
#define _SEEN_SPLINEFONT_H_

#include <glib.h>

G_BEGIN_DECLS

typedef double real;
typedef double bigreal;
typedef double extended;
typedef int BOOL;

#define chunkalloc(size)	calloc(1,size)
#define chunkfree(item,size)	free(item)

typedef struct basepoint {
    real x;
    real y;
} BasePoint;

typedef struct ipoint {
    int x;
    int y;
} IPoint;

enum pointtype { pt_curve, pt_corner, pt_tangent, pt_hvcurve };
typedef struct splinepoint {
    BasePoint me;
    BasePoint nextcp;		/* control point */
    BasePoint prevcp;		/* control point */
    unsigned int nonextcp:1;
    unsigned int noprevcp:1;
    unsigned int nextcpdef:1;
    unsigned int prevcpdef:1;
    unsigned int selected:1;	/* for UI */
    unsigned int nextcpselected: 2; /* Is the next BCP selected */
    unsigned int prevcpselected: 2; /* Is the prev BCP selected */
    unsigned int pointtype:2;
    unsigned int isintersection: 1;
    unsigned int flexy: 1;	/* When "freetype_markup" is on in charview.c:DrawPoint */
    unsigned int flexx: 1;	/* flexy means select nextcp, and flexx means draw circle around nextcp */
    unsigned int roundx: 1;	/* For true type hinting */
    unsigned int roundy: 1;	/* For true type hinting */
    unsigned int dontinterpolate: 1;	/* in ttf, don't imply point by interpolating between cps */
    unsigned int ticked: 1;
    unsigned int watched: 1;
	/* 1 bits left... */
    uint16_t ptindex;		/* Temporary value used by metafont routine */
    uint16_t ttfindex;		/* Truetype point index */
	/* Special values 0xffff => point implied by averaging control points */
	/*		  0xfffe => point created with no real number yet */
	/* (or perhaps point in context where no number is possible as in a glyph with points & refs) */
    uint16_t nextcpindex;		/* Truetype point index */
    struct spline *next;
    struct spline *prev;
    /* Inkscape: not used; HintMask *hintmask; */
	char* name;
} SplinePoint;


typedef struct spline1d {
    real a, b, c, d;
} Spline1D;

typedef struct spline {
    unsigned int islinear: 1;		/* No control points */
    unsigned int isquadratic: 1;	/* probably read in from ttf */
    unsigned int isticked: 1;
    unsigned int isneeded: 1;		/* Used in remove overlap */
    unsigned int isunneeded: 1;		/* Used in remove overlap */
    unsigned int exclude: 1;		/* Used in remove overlap variant: exclude */
    unsigned int ishorvert: 1;
    unsigned int knowncurved: 1;	/* We know that it curves */
    unsigned int knownlinear: 1;	/* it might have control points, but still traces out a line */
	/* If neither knownlinear nor curved then we haven't checked */
    unsigned int order2: 1;		/* It's a bezier curve with only one cp */
    unsigned int touched: 1;
    unsigned int leftedge: 1;
    unsigned int rightedge: 1;
    unsigned int acceptableextrema: 1;	/* This spline has extrema, but we don't care */
    SplinePoint *from;
    SplinePoint *to;
    Spline1D splines[2];		/* splines[0] is the x spline, splines[1] is y */
    struct linearapprox *approx;
    /* Possible optimizations:
	Precalculate bounding box
	Precalculate min/max/ points of inflection
    */
} Spline;

typedef struct splinepointlist {
    SplinePoint *first, *last;
    struct splinepointlist *next;
    /* Not used:   spiro_cp *spiros; */
    uint16_t spiro_cnt, spiro_max;
	/* These could be bit fields, but bytes are easier to access and we */
	/*  don't need the space (yet) */
    uint8_t ticked;
    uint8_t beziers_need_optimizer;	/* If the spiros have changed in spiro mode, then reverting to bezier mode might, someday, run a simplifier */
    uint8_t is_clip_path;			/* In type3/svg fonts */
    int start_offset; // This indicates which point is the canonical first for purposes of outputting to U. F. O..
    char *contour_name;
} SplinePointList, SplineSet;

typedef struct dbounds {
    real minx, maxx;
    real miny, maxy;
} DBounds;

typedef struct quartic {
    bigreal a,b,c,d,e;
} Quartic;


int RealWithin(real a,real b,real fudge);
BOOL RealNear(real a, real b);

Spline *SplineMake(SplinePoint *from, SplinePoint *to, int order2);
Spline *SplineMake2(SplinePoint *from, SplinePoint *to);
Spline *SplineMake3(SplinePoint *from, SplinePoint *to);
SplinePoint *SplinePointCreate(real x, real y);

void SplineRefigure3(Spline *spline);
void SplineRefigure(Spline *spline);
int SplineIsLinear(Spline *spline);
void SplineFindExtrema(const Spline1D *sp, extended *_t1, extended *_t2 );
bigreal SplineMinDistanceToPoint(Spline *s, BasePoint *p);

void SplinePointFree(SplinePoint *sp);
void SplineFree(Spline *spline);
void SplinePointListFree(SplinePointList *spl);

bigreal BPDot(BasePoint v1, BasePoint v2);
bigreal BPCross(BasePoint v1, BasePoint v2);
BasePoint BPRev(BasePoint v);

int _CubicSolve(const Spline1D *sp,bigreal sought, extended ts[3]);
int _QuarticSolve(Quartic *q,extended ts[4]);
int IntersectLines(BasePoint *inter,
	BasePoint *line1_1, BasePoint *line1_2,
	BasePoint *line2_1, BasePoint *line2_2);

#define IError(msg) g_warning(msg)
#define TRACE g_message

enum linelist_flags { cvli_onscreen=0x1, cvli_clipped=0x2 };

typedef struct linelist {
    IPoint here;
    struct linelist *next;
    /* The first two fields are constant for the linelist, the next ones */
    /*  refer to a particular screen. If some portion of the line from */
    /*  this point to the next one is on the screen then set cvli_onscreen */
    /*  if this point needs to be clipped then set cvli_clipped */
    /*  asend and asstart are the actual screen locations where this point */
    /*  intersects the clip edge. */
    enum linelist_flags flags;
    IPoint asend, asstart;
} LineList;

typedef struct linearapprox {
    real scale;
    unsigned int oneline: 1;
    unsigned int onepoint: 1;
    unsigned int any: 1;		/* refers to a particular screen */
    struct linelist *lines;
    struct linearapprox *next;
} LinearApprox;

void LinearApproxFree(LinearApprox *la);

int Within16RoundingErrors(bigreal v1, bigreal v2);

enum pconvert_flags {
	// Point selection (mutually exclusive)
	pconvert_flag_none = 0x01,
	pconvert_flag_all = 0x02,
	pconvert_flag_smooth = 0x04,
	pconvert_flag_incompat = 0x08,
	// Conversion modes (mutually exclusive)
	pconvert_flag_by_geom = 0x100,
	pconvert_flag_force_type = 0x200,
	pconvert_flag_downgrade = 0x400,
	pconvert_flag_check_compat = 0x0800,
	// Additional
	pconvert_flag_hvcurve = 0x4000
};

void SplinesRemoveBetween(SplinePoint *from, SplinePoint *to, int type);

G_END_DECLS

#endif // _SEEN_SPLINEFONT_H_

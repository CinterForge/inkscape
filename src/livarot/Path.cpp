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
#include "Path.h"

#include <2geom/pathvector.h>

#include "livarot/path-description.h"

/*
 * manipulation of the path data: path description and polyline
 * grunt work...
 * at the end of this file, 2 utilitary functions to get the point and tangent to path associated with a (command no;abcissis)
 */

Path::~Path()
{
    for (auto & i : descr_cmd) {
        delete i;
    }
}

// debug function do dump the path contents on stdout
void Path::Affiche()
{
    std::cout << "path: " << descr_cmd.size() << " commands." << std::endl;
    for (auto i : descr_cmd) {
        i->dump(std::cout);
        std::cout << std::endl;
    }

    std::cout << std::endl;
}

void Path::Reset()
{
    for (auto & i : descr_cmd) {
        delete i;
    }
    
    descr_cmd.clear();
    descr_flags = 0;

    forced_subdivisions.clear();
}

void Path::Copy(Path * who)
{
    ResetPoints();
    
    for (auto & i : descr_cmd) {
        delete i;
    }
        
    descr_cmd.clear();
        
    for (auto i : who->descr_cmd)
    {
        descr_cmd.push_back(i->clone());
    }

    forced_subdivisions = who->forced_subdivisions;
}

void Path::CloseSubpath()
{
    descr_flags &= ~(descr_doing_subpath);
}

int Path::ForcePoint()
{
    if ( (descr_flags & descr_doing_subpath) == 0 ) {
        return -1;
    }
    
    if (descr_cmd.empty()) {
        return -1;
    }

    descr_cmd.push_back(new PathDescrForced);
    return descr_cmd.size() - 1;
}


void Path::InsertForcePoint(int at)
{
    if ( at < 0 || at > int(descr_cmd.size()) ) {
	return;
    }
    
    if ( at == int(descr_cmd.size()) ) {
	ForcePoint();
	return;
    }
    
    descr_cmd.insert(descr_cmd.begin() + at, new PathDescrForced);
}

int Path::Close()
{
    if ( descr_flags & descr_doing_subpath ) {
        CloseSubpath();
    } else {
        // Nothing to close.
        return -1;
    }

    descr_cmd.push_back(new PathDescrClose);
    
    descr_flags &= ~(descr_doing_subpath);
    
    return descr_cmd.size() - 1;
}

int Path::MoveTo(Geom::Point const &iPt)
{
    if ( descr_flags & descr_doing_subpath ) {
	CloseSubpath();
    }
    
    descr_cmd.push_back(new PathDescrMoveTo(iPt));

    descr_flags |= descr_doing_subpath;
    return descr_cmd.size() - 1;
}

void Path::InsertMoveTo(Geom::Point const &iPt, int at)
{
    if ( at < 0 || at > int(descr_cmd.size()) ) {
        return;
    }
    
    if ( at == int(descr_cmd.size()) ) {
        MoveTo(iPt);
        return;
    }

  descr_cmd.insert(descr_cmd.begin() + at, new PathDescrMoveTo(iPt));
}

int Path::LineTo(Geom::Point const &iPt)
{
    if (!( descr_flags & descr_doing_subpath )) {
	return MoveTo (iPt);
    }
    
    descr_cmd.push_back(new PathDescrLineTo(iPt));
    return descr_cmd.size() - 1;
}

void Path::InsertLineTo(Geom::Point const &iPt, int at)
{
    if ( at < 0 || at > int(descr_cmd.size()) ) {
        return;
    }
    
    if ( at == int(descr_cmd.size()) ) {
        LineTo(iPt);
        return;
    }
    
    descr_cmd.insert(descr_cmd.begin() + at, new PathDescrLineTo(iPt));
}

int Path::CubicTo(Geom::Point const &iPt, Geom::Point const &iStD, Geom::Point const &iEnD)
{
    if ( (descr_flags & descr_doing_subpath) == 0) {
	return MoveTo (iPt);
    }

    descr_cmd.push_back(new PathDescrCubicTo(iPt, iStD, iEnD));
    return descr_cmd.size() - 1;
}


void Path::InsertCubicTo(Geom::Point const &iPt, Geom::Point const &iStD, Geom::Point const &iEnD, int at)
{
    if ( at < 0 || at > int(descr_cmd.size()) ) {
	return;
    }
    
    if ( at == int(descr_cmd.size()) ) {
	CubicTo(iPt,iStD,iEnD);
	return;
    }
  
    descr_cmd.insert(descr_cmd.begin() + at, new PathDescrCubicTo(iPt, iStD, iEnD));
}

int Path::ArcTo(Geom::Point const &iPt, double iRx, double iRy, double angle,
		bool iLargeArc, bool iClockwise)
{
    if ( (descr_flags & descr_doing_subpath) == 0 ) {
	return MoveTo(iPt);
    }

    descr_cmd.push_back(new PathDescrArcTo(iPt, iRx, iRy, angle, iLargeArc, iClockwise));
    return descr_cmd.size() - 1;
}


void Path::InsertArcTo(Geom::Point const &iPt, double iRx, double iRy, double angle,
		       bool iLargeArc, bool iClockwise, int at)
{
    if ( at < 0 || at > int(descr_cmd.size()) ) {
	return;
    }
    
    if ( at == int(descr_cmd.size()) ) {
	ArcTo(iPt, iRx, iRy, angle, iLargeArc, iClockwise);
	return;
    }
  
    descr_cmd.insert(descr_cmd.begin() + at, new PathDescrArcTo(iPt, iRx, iRy,
                                                                angle, iLargeArc, iClockwise));
}

/*
 * points of the polyline
 */
void
Path::SetBackData (bool nVal)
{
	if (! back) {
		if (nVal) {
			back = true;
			ResetPoints();
		}
	} else {
		if (! nVal) {
			back = false;
			ResetPoints();
		}
	}
}


void Path::ResetPoints()
{
    pts.clear();
}


int Path::AddPoint(Geom::Point const &iPt, bool mvto)
{
    if (back) {
        return AddPoint (iPt, -1, 0.0, mvto);
    }
  
    if ( !mvto && !pts.empty() && pts.back().p == iPt ) {
        return -1;
    }
    
    int const n = pts.size();
    pts.emplace_back(mvto ? polyline_moveto : polyline_lineto, iPt);
    return n;
}

int Path::AddPoint(Geom::Point const &iPt, int ip, double it, bool mvto)
{
    if (! back) {
        return AddPoint (iPt, mvto);
    }
    
    if ( !mvto && !pts.empty() && pts.back().p == iPt ) {
        return -1;
    }
    
    int const n = pts.size();
    pts.emplace_back(mvto ? polyline_moveto : polyline_lineto, iPt, ip, it);
    return n;
}

int Path::AddForcedPoint()
{
    if (pts.empty() || pts.back().isMoveTo != polyline_lineto) {
        return -1;
    }

    int const n = pts.size();

    if (back) {
        pts.emplace_back(polyline_forced, pts[n - 1].p, pts[n - 1].piece, pts[n - 1].t);
    } else {
        pts.emplace_back(polyline_forced, pts[n - 1].p);
    }

    return n;
}

void Path::PolylineBoundingBox(double &l, double &t, double &r, double &b)
{
  l = t = r = b = 0.0;
  if ( pts.empty() ) {
      return;
  }

  std::vector<path_lineto>::const_iterator i = pts.begin();
  l = r = i->p[Geom::X];
  t = b = i->p[Geom::Y];
  ++i;

  for (; i != pts.end(); ++i) {
      r = std::max(r, i->p[Geom::X]);
      l = std::min(l, i->p[Geom::X]);
      b = std::max(b, i->p[Geom::Y]);
      t = std::min(t, i->p[Geom::Y]);
  }
}


/**
 *    \param piece Index of a one of our commands.
 *    \param at Distance along the segment that corresponds to `piece' (0 <= at <= 1)
 *    \param pos Filled in with the point at `at' on `piece'.
 */

void Path::PointAt(int piece, double at, Geom::Point &pos)
{
    if (piece < 0 || piece >= int(descr_cmd.size())) {
	// this shouldn't happen: the piece we are asked for doesn't
	// exist in the path
	pos = Geom::Point(0,0);
	return;
    }
    
    PathDescr const *theD = descr_cmd[piece];
    int const typ = theD->getType();
    Geom::Point tgt;
    double len;
    double rad;
    
    if (typ == descr_moveto) {
	
	return PointAt (piece + 1, 0.0, pos);
	
    } else if (typ == descr_close || typ == descr_forced) {
	
	return PointAt (piece - 1, 1.0, pos);
	
    } else if (typ == descr_lineto) {
	
	PathDescrLineTo const *nData = dynamic_cast<PathDescrLineTo const *>(theD);
	TangentOnSegAt(at, PrevPoint (piece - 1), *nData, pos, tgt, len);
	
    } else if (typ == descr_arcto) {
	
	PathDescrArcTo const *nData = dynamic_cast<PathDescrArcTo const *>(theD);
	TangentOnArcAt(at,PrevPoint (piece - 1), *nData, pos, tgt, len, rad);
	
    } else if (typ == descr_cubicto) {
	
	PathDescrCubicTo const *nData = dynamic_cast<PathDescrCubicTo const *>(theD);
	TangentOnCubAt(at, PrevPoint (piece - 1), *nData, false, pos, tgt, len, rad);
    }
}

void Path::PointAndTangentAt(int piece, double at, Geom::Point &pos, Geom::Point &tgt)
{
    if (piece < 0 || piece >= int(descr_cmd.size())) {
	// this shouldn't happen: the piece we are asked for doesn't exist in the path
	pos = Geom::Point(0, 0);
	return;
    }
    
    PathDescr const *theD = descr_cmd[piece];
    int typ = theD->getType();
    double len;
    double rad;
    if (typ == descr_moveto) {
	
	return PointAndTangentAt(piece + 1, 0.0, pos, tgt);
	
    } else if (typ == descr_close ) {
	
	int cp = piece - 1;
	while ( cp >= 0 && (descr_cmd[cp]->getType()) != descr_moveto ) {
	    cp--;
	}
	if ( cp >= 0 ) {
	    PathDescrMoveTo *nData = dynamic_cast<PathDescrMoveTo *>(descr_cmd[cp]);
	    PathDescrLineTo dst(nData->p);
	    TangentOnSegAt(at, PrevPoint (piece - 1), dst, pos, tgt, len);
	}
	
    } else if ( typ == descr_forced) {
	
	return PointAndTangentAt(piece - 1, 1.0, pos,tgt);
	
    } else if (typ == descr_lineto) {

	PathDescrLineTo const *nData = dynamic_cast<PathDescrLineTo const *>(theD);
	TangentOnSegAt(at, PrevPoint (piece - 1), *nData, pos, tgt, len);
	
    } else if (typ == descr_arcto) {
	
	PathDescrArcTo const *nData = dynamic_cast<PathDescrArcTo const *>(theD);
	TangentOnArcAt (at,PrevPoint (piece - 1), *nData, pos, tgt, len, rad);
	
    } else if (typ == descr_cubicto) {
	
	PathDescrCubicTo const *nData = dynamic_cast<PathDescrCubicTo const *>(theD);
	TangentOnCubAt (at, PrevPoint (piece - 1), *nData, false, pos, tgt, len, rad);
    }
}

/**
 * Apply a transform in-place.
 *
 * Note: Converts to Geom::PathVector, applies the transform, and converts back.
 */
void Path::Transform(const Geom::Affine &trans)
{
    LoadPathVector(MakePathVector() * trans);
}

void Path::FastBBox(double &l,double &t,double &r,double &b)
{
    l = t = r = b = 0;
    bool empty = true;
    Geom::Point lastP(0, 0);
    
    for (auto & i : descr_cmd) {
	int const typ = i->getType();
	switch ( typ ) {
	case descr_lineto:
	{
	    PathDescrLineTo *nData = dynamic_cast<PathDescrLineTo *>(i);
	    if ( empty ) {
		l = r = nData->p[Geom::X];
		t = b = nData->p[Geom::Y];
		empty = false;
	    } else {
		if ( nData->p[Geom::X] < l ) {
		    l = nData->p[Geom::X];
		}
		if ( nData->p[Geom::X] > r ) {
		    r = nData->p[Geom::X];
		}
		if ( nData->p[Geom::Y] < t ) {
		    t = nData->p[Geom::Y];
		}
		if ( nData->p[Geom::Y] > b ) {
		    b = nData->p[Geom::Y];
		}
	    }
	    lastP = nData->p;
	}
        break;
	
	case descr_moveto:
	{
	    PathDescrMoveTo *nData = dynamic_cast<PathDescrMoveTo *>(i);
	    if ( empty ) {
		l = r = nData->p[Geom::X];
		t = b = nData->p[Geom::Y];
		empty = false;
	    } else {
		if ( nData->p[Geom::X] < l ) {
		    l = nData->p[Geom::X];
		}
		if ( nData->p[Geom::X] > r ) {
		    r = nData->p[Geom::X];
		}
		if ( nData->p[Geom::Y] < t ) {
		    t = nData->p[Geom::Y];
		}
		if ( nData->p[Geom::Y] > b ) {
		    b = nData->p[Geom::Y];
		}
	    }
	    lastP = nData->p;
	}
        break;
	
	case descr_arcto:
	{
	    PathDescrArcTo *nData  =  dynamic_cast<PathDescrArcTo *>(i);
	    if ( empty ) {
		l = r = nData->p[Geom::X];
		t = b = nData->p[Geom::Y];
		empty = false;
	    } else {
		if ( nData->p[Geom::X] < l ) {
		    l = nData->p[Geom::X];
		}
		if ( nData->p[Geom::X] > r ) {
		    r = nData->p[Geom::X];
		}
		if ( nData->p[Geom::Y] < t ) {
		    t = nData->p[Geom::Y];
		}
		if ( nData->p[Geom::Y] > b ) {
		    b = nData->p[Geom::Y];
		}
	    }
	    lastP = nData->p;
	}
        break;
	
	case descr_cubicto:
	{
	    PathDescrCubicTo *nData  =  dynamic_cast<PathDescrCubicTo *>(i);
	    if ( empty ) {
		l = r = nData->p[Geom::X];
		t = b = nData->p[Geom::Y];
		empty = false;
	    } else {
		if ( nData->p[Geom::X] < l ) {
		    l = nData->p[Geom::X];
		}
		if ( nData->p[Geom::X] > r ) {
		    r = nData->p[Geom::X];
		}
		if ( nData->p[Geom::Y] < t ) {
		    t = nData->p[Geom::Y];
		}
		if ( nData->p[Geom::Y] > b ) {
		    b = nData->p[Geom::Y];
		}
	    }
	    
/* bug 249665: "...the calculation of the bounding-box for cubic-paths
has some extra steps to make it work correctly in Win32 that unfortunately
are unnecessary in Linux, generating wrong results. This only shows in 
Type1 fonts because they use cubic-paths instead of the
bezier-paths used by True-Type fonts."
*/

#ifdef _WIN32
	    Geom::Point np = nData->p - nData->end;
	    if ( np[Geom::X] < l ) {
		l = np[Geom::X];
	    }
	    if ( np[Geom::X] > r ) {
		r = np[Geom::X];
	    }
	    if ( np[Geom::Y] < t ) {
		t = np[Geom::Y];
	    }
	    if ( np[Geom::Y] > b ) {
		b = np[Geom::Y];
	    }
	    
	    np = lastP + nData->start;
	    if ( np[Geom::X] < l ) {
		l = np[Geom::X];
	    }
	    if ( np[Geom::X] > r ) {
		r = np[Geom::X];
	    }
	    if ( np[Geom::Y] < t ) {
		t = np[Geom::Y];
	    }
	    if ( np[Geom::Y] > b ) {
		b = np[Geom::Y];
	    }
#endif

	    lastP = nData->p;
	}
        break;
	}
    }
}

std::string Path::svg_dump_path() const
{
    Inkscape::SVGOStringStream os;

    for (int i = 0; i < descr_cmd.size(); i++) {
        auto const p = i == 0 ? Geom::Point() : PrevPoint(i - 1);
        descr_cmd[i]->dumpSVG(os, p);
    }

    return os.str();
}

// Find out if the segment that corresponds to 'piece' is a straight line
bool Path::IsLineSegment(int piece)
{
    if (piece < 0 || piece >= int(descr_cmd.size())) {
        return false;
    }
    
    PathDescr const *theD = descr_cmd[piece];
    int const typ = theD->getType();
    
    return (typ == descr_lineto);
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

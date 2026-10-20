// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  PathCutting.cpp
 *  nlivarot
 *
 *  Created by fred on someday in 2004.
 *  public domain
 *
 *  Additional Code by Authors:
 *   Richard Hughes <cyreve@users.sf.net>
 *
 *  Copyright (C) 2005 Richard Hughes
 *
 *  Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cmath>
#include <cstdio>
#include <vector>

#include <glib.h>

#include <2geom/pathvector.h>
#include <2geom/point.h>
#include <2geom/affine.h>
#include <2geom/sbasis-to-bezier.h>
#include <2geom/curves.h>

#include "Path.h"
#include "livarot/path-description.h"
#include "helper/geom-curves.h"

void  Path::DashPolyline(float head,float tail,float body,int nbD, const float dashs[],bool stPlain,float stOffset)
{
  if ( nbD <= 0 || body <= 0.0001 ) return; // pas de tirets, en fait

  std::vector<path_lineto> orig_pts = pts;
  pts.clear();

  int       lastMI=-1;
  int curP = 0;
  int lastMP = -1;

  for (int i = 0; i < int(orig_pts.size()); i++) {
    if ( orig_pts[curP].isMoveTo == polyline_moveto ) {
      if ( lastMI >= 0 && lastMI < i-1 ) { // au moins 2 points
        DashSubPath(i-lastMI,lastMP, orig_pts, head,tail,body,nbD,dashs,stPlain,stOffset);
      }
      lastMI=i;
      lastMP=curP;
    }
    curP++;
  }
  if ( lastMI >= 0 && lastMI < int(orig_pts.size()) - 1 ) {
    DashSubPath(orig_pts.size() - lastMI, lastMP, orig_pts, head, tail, body, nbD, dashs, stPlain, stOffset);
  }
}

void  Path::DashPolyline(std::vector<double> array_values, double offset, float scale, float min_len)
{
    double dlen = 0.0;
    // Find total length
    for (auto & value : array_values) {
        dlen += value * scale;
    }

    if (dlen >= min_len) {
        // Extract out dash pattern (relative positions)
        double dash_offset = offset * scale;

        size_t n_dash = array_values.size();
        std::vector<double> dash(n_dash);
        for (unsigned i = 0; i < n_dash; i++) {
            dash[i] = array_values[i] * scale;
        }

        // Convert relative positions to absolute positions
        int nbD = n_dash;
        std::vector<float> dashes(n_dash);
        if (dlen > 0) {
            while (dash_offset >= dlen) dash_offset -= dlen;
        }
        dashes[0] = dash[0];
        for (int i = 1; i < nbD; ++i) {
            dashes[i] = dashes[i-1] + dash[i];
        }

        // modulo dlen
        DashPolyline(0.0, 0.0, dlen, nbD, dashes.data(), true, dash_offset);
    }
}


void Path::DashSubPath(int spL, int spP, std::vector<path_lineto> const &orig_pts, float head,float tail,float body,int nbD, const float dashs[],bool stPlain,float stOffset)
{
  if ( spL <= 0 || spP == -1 ) return;
  
  double      totLength=0;
  Geom::Point   lastP;
  lastP = orig_pts[spP].p;
  for (int i=1;i<spL;i++) {
    Geom::Point const n = orig_pts[spP + i].p;
    Geom::Point d=n-lastP;
    double    nl=Geom::L2(d);
    if ( nl > 0.0001 ) {
      totLength+=nl;
      lastP=n;
    }
  }
  
  if ( totLength <= head+tail ) return; // tout mange par la tete et la queue
  
  double    curLength=0;
  double    dashPos=0;
  int       dashInd=0;
  bool      dashPlain=false;
  double    lastT=0;
  int       lastPiece=-1;
  lastP = orig_pts[spP].p;
  for (int i=1;i<spL;i++) {
    Geom::Point   n;
    int         nPiece=-1;
    double      nT=0;
    if ( back ) {
      n = orig_pts[spP + i].p;
      nPiece = orig_pts[spP + i].piece;
      nT = orig_pts[spP + i].t;
    } else {
      n = orig_pts[spP + i].p;
    }
    Geom::Point d=n-lastP;
    double    nl=Geom::L2(d);
    if ( nl > 0.0001 ) {
      double   stLength=curLength;
      double   enLength=curLength+nl;
      // couper les bouts en trop
      if ( curLength <= head && curLength+nl > head ) {
        nl-=head-curLength;
        curLength=head;
        dashInd=0;
        dashPos=stOffset;
        bool nPlain=stPlain;
        while ( dashs[dashInd] < stOffset ) {
          dashInd++;
          nPlain=!(nPlain);
          if ( dashInd >= nbD ) {
            dashPos=0;
            dashInd=0;
            break;
          }
        }
        if ( nPlain == true && dashPlain == false ) {
          Geom::Point  p=(enLength-curLength)*lastP+(curLength-stLength)*n;
          p/=(enLength-stLength);
          if ( back ) {
            double pT=0;
            if ( nPiece == lastPiece ) {
              pT=(lastT*(enLength-curLength)+nT*(curLength-stLength))/(enLength-stLength);
            } else {
              pT=(nPiece*(curLength-stLength))/(enLength-stLength);
            }
            AddPoint(p,nPiece,pT,true);
          } else {
            AddPoint(p,true);
          }
        } else if ( nPlain == false && dashPlain == true ) {
        }
        dashPlain=nPlain;
      }
      // faire les tirets
      if ( curLength >= head /*&& curLength+nl <= totLength-tail*/ ) {
        while ( curLength <= totLength-tail && nl > 0 ) {
          if ( enLength <= totLength-tail ) nl=enLength-curLength; else nl=totLength-tail-curLength;
          double  leftInDash=body-dashPos;
          if ( dashInd < nbD ) {
            leftInDash=dashs[dashInd]-dashPos;
          }
          if ( leftInDash <= nl ) {
            bool nPlain=false;
            if ( dashInd < nbD ) {
              dashPos=dashs[dashInd];
              dashInd++;
              if ( dashPlain ) nPlain=false; else nPlain=true;
            } else {
              dashInd=0;
              dashPos=0;
              //nPlain=stPlain;
              nPlain=dashPlain;
            }
            if ( nPlain == true && dashPlain == false ) {
              Geom::Point  p=(enLength-curLength-leftInDash)*lastP+(curLength+leftInDash-stLength)*n;
              p/=(enLength-stLength);
              if ( back ) {
                double pT=0;
                if ( nPiece == lastPiece ) {
                  pT=(lastT*(enLength-curLength-leftInDash)+nT*(curLength+leftInDash-stLength))/(enLength-stLength);
                } else {
                  pT=(nPiece*(curLength+leftInDash-stLength))/(enLength-stLength);
                }
                AddPoint(p,nPiece,pT,true);
              } else {
                AddPoint(p,true);
              }
            } else if ( nPlain == false && dashPlain == true ) {
              Geom::Point  p=(enLength-curLength-leftInDash)*lastP+(curLength+leftInDash-stLength)*n;
              p/=(enLength-stLength);
              if ( back ) {
                double pT=0;
                if ( nPiece == lastPiece ) {
                  pT=(lastT*(enLength-curLength-leftInDash)+nT*(curLength+leftInDash-stLength))/(enLength-stLength);
                } else {
                  pT=(nPiece*(curLength+leftInDash-stLength))/(enLength-stLength);
                }
                AddPoint(p,nPiece,pT,false);
              } else {
                AddPoint(p,false);
              }
            }
            dashPlain=nPlain;
            
            curLength+=leftInDash;
            nl-=leftInDash;
          } else {
            dashPos+=nl;
            curLength+=nl;
            nl=0;
          }
        }
        if ( dashPlain ) {
          if ( back ) {
            AddPoint(n,nPiece,nT,false);
          } else {
            AddPoint(n,false);
          }
        }
        nl=enLength-curLength;
      }
      if ( curLength <= totLength-tail && curLength+nl > totLength-tail ) {
        nl=totLength-tail-curLength;
        dashInd=0;
        dashPos=0;
        bool nPlain=false;
        if ( nPlain == true && dashPlain == false ) {
        } else if ( nPlain == false && dashPlain == true ) {
          Geom::Point  p=(enLength-curLength)*lastP+(curLength-stLength)*n;
          p/=(enLength-stLength);
          if ( back ) {
            double pT=0;
            if ( nPiece == lastPiece ) {
              pT=(lastT*(enLength-curLength)+nT*(curLength-stLength))/(enLength-stLength);
            } else {
              pT=(nPiece*(curLength-stLength))/(enLength-stLength);
            }
            AddPoint(p,nPiece,pT,false);
          } else {
            AddPoint(p,false);
          }
        }
        dashPlain=nPlain;
      }
      // continuer
      curLength=enLength;
      lastP=n;
      lastPiece=nPiece;
      lastT=nT;
    }
  }
}

/**
 * Make a Geom::PathVector version of the path description.
 *
 * \return A PathVector copy of the path description
 */
Geom::PathVector Path::MakePathVector() const
{
    Geom::PathVector pv;

    Geom::Path *currentpath = nullptr;
    Geom::Point lastP;

    for (auto c : descr_cmd) {
        switch (c->getType()) {
            case descr_close: {
                currentpath->close(true);
                break;
            }
            case descr_lineto: {
                auto data = static_cast<PathDescrLineTo const *>(c);
                currentpath->appendNew<Geom::LineSegment>(data->p);
                lastP = data->p;
                break;
            }
            case descr_moveto: {
                auto data = static_cast<PathDescrMoveTo const *>(c);
                pv.push_back(Geom::Path());
                currentpath = &pv.back();
                currentpath->start(data->p);
                lastP = data->p;
                break;
            }
            case descr_arcto: {
                auto data = static_cast<PathDescrArcTo const *>(c);
                currentpath->appendNew<Geom::EllipticalArc>(data->rx, data->ry, Geom::rad_from_deg(data->angle), data->large, !data->clockwise, data->p);
                lastP = data->p;
                break;
            }
            case descr_cubicto: {
                auto data = static_cast<PathDescrCubicTo const *>(c);
                currentpath->appendNew<Geom::CubicBezier>(lastP + data->start / 3, data->p - data->end / 3, data->p);
                lastP = data->p;
                break;
            }
            default:
                break;
        }
    }

    return pv;
}

void Path::AddCurve(Geom::Curve const &c)
{
    if (dynamic_cast<Geom::LineSegment const *>(&c)) {
        LineTo(c.finalPoint());
    } else if (auto cubic = dynamic_cast<Geom::CubicBezier const *>(&c)) {
        if (is_straight_curve(*cubic)) {
            LineTo(c.finalPoint());
        } else {
            CubicTo((*cubic)[3],
                    3 * ((*cubic)[1] - (*cubic)[0]),
                    3 * ((*cubic)[3] - (*cubic)[2]));
        }
    } else if (auto arc = dynamic_cast<Geom::EllipticalArc const *>(&c)) {
        ArcTo(arc->finalPoint(),
              arc->rays().x(), arc->rays().y(),
              Geom::deg_from_rad(arc->rotationAngle()),
              arc->largeArc(), !arc->sweep());
    } else { 
        // This case handles sbasis as well as all other curve types.
        auto const sbasis_path = Geom::cubicbezierpath_from_sbasis(c.toSBasis(), 0.1);

        // Recurse to convert the new path resulting from the sbasis to svgd.
        for (auto const &iter : sbasis_path) {
            AddCurve(iter);
        }
    }
}

/**  append is false by default: it means that the path should be resetted. If it is true, the path is not resetted and Geom::Path will be appended as a new path
 */
void  Path::LoadPath(Geom::Path const &path, Geom::Affine const &tr, bool doTransformation, bool append)
{
    if (!append) {
        SetBackData (false);
        Reset();
    }
    if (path.empty())
        return;

    // TODO: this can be optimized by not generating a new path here, but doing the transform in AddCurve
    //       directly on the curve parameters

    Geom::Path const pathtr = doTransformation ? path * tr : path;

    MoveTo( pathtr.initialPoint() );

    for(const auto & cit : pathtr) {
        AddCurve(cit);
    }

    if (pathtr.closed()) {
        Close();
    }
}

void  Path::LoadPathVector(Geom::PathVector const &pv)
{
    LoadPathVector(pv, Geom::Affine(), false);
}

void Path::LoadPathVector(Geom::PathVector const &pv, std::vector<Geom::PathVectorTime> const &cuts)
{
    SetBackData(false);
    Reset();

    forced_subdivisions.reserve(cuts.size());
    auto it = cuts.begin();

    for (int i = 0, maxi = pv.size(); i < maxi; i++) {
        auto const &path = pv[i];

        if (path.empty()) {
            continue;
        }

        MoveTo(path.initialPoint());

        for (int j = 0, maxj = path.size(); j < maxj; j++) {
            auto const &curve = path[j];

            AddCurve(curve);

            while (it != cuts.end() && it->path_index == i && it->curve_index == j) {
                forced_subdivisions.push_back({ .piece = (int)descr_cmd.size() - 1, .t = it->t });
                ++it;
            }
        }

        if (path.closed()) {
            Close();
        }
    }

    assert(it == cuts.end());
}

void  Path::LoadPathVector(Geom::PathVector const &pv, Geom::Affine const &tr, bool doTransformation)
{
    SetBackData (false);
    Reset();

    for(const auto & it : pv) {
        LoadPath(it, tr, doTransformation, true);
    }
}

/**
 *    \return Length of the lines in the pts vector.
 */

double Path::Length()
{
    if ( pts.empty() ) {
        return 0;
    }

    Geom::Point lastP = pts[0].p;

    double len = 0;
    for (const auto & pt : pts) {

        if ( pt.isMoveTo != polyline_moveto ) {
            len += Geom::L2(pt.p - lastP);
        }

        lastP = pt.p;
    }
    
    return len;
}


double Path::Surface()
{
    if ( pts.empty() ) {
        return 0;
    }
    
    Geom::Point lastM = pts[0].p;
    Geom::Point lastP = lastM;

    double surf = 0;
    for (const auto & pt : pts) {

        if ( pt.isMoveTo == polyline_moveto ) {
            surf += Geom::cross(lastM, lastM - lastP);
            lastP = lastM = pt.p;
        } else {
            surf += Geom::cross(pt.p, pt.p - lastP);
            lastP = pt.p;
        }
        
    }
    
  return surf;
}


Path**      Path::SubPaths(int &outNb,bool killNoSurf)
{
  int      nbRes=0;
  Path**   res=nullptr;
  Path*    curAdd=nullptr;
  
  for (auto & i : descr_cmd) {
    int const typ = i->getType();
    switch ( typ ) {
      case descr_moveto:
        if ( curAdd ) {
          if ( curAdd->descr_cmd.size() > 1 ) {
            curAdd->Convert(1.0);
            double addSurf=curAdd->Surface();
            if ( fabs(addSurf) > 0.0001 || killNoSurf == false ) {
              res=(Path**)g_realloc(res,(nbRes+1)*sizeof(Path*));
              res[nbRes++]=curAdd;
            } else { 
              delete curAdd;
            }
          } else {
            delete curAdd;
          }
          curAdd=nullptr;
        }
        curAdd=new Path;
        curAdd->SetBackData(false);
        {
          PathDescrMoveTo *nData = dynamic_cast<PathDescrMoveTo *>(i);
          curAdd->MoveTo(nData->p);
        }
          break;
      case descr_close:
      {
        curAdd->Close();
      }
        break;        
      case descr_lineto:
      {
        PathDescrLineTo *nData = dynamic_cast<PathDescrLineTo *>(i);
        curAdd->LineTo(nData->p);
      }
        break;
      case descr_cubicto:
      {
        PathDescrCubicTo *nData = dynamic_cast<PathDescrCubicTo *>(i);
        curAdd->CubicTo(nData->p,nData->start,nData->end);
      }
        break;
      case descr_arcto:
      {
        PathDescrArcTo *nData = dynamic_cast<PathDescrArcTo *>(i);
        curAdd->ArcTo(nData->p,nData->rx,nData->ry,nData->angle,nData->large,nData->clockwise);
      }
        break;
      default:
        break;
    }
  }
  if ( curAdd ) {
    if ( curAdd->descr_cmd.size() > 1 ) {
      curAdd->Convert(1.0);
      double addSurf=curAdd->Surface();
      if ( fabs(addSurf) > 0.0001 || killNoSurf == false  ) {
        res=(Path**)g_realloc(res,(nbRes+1)*sizeof(Path*));
        res[nbRes++]=curAdd;
      } else {
        delete curAdd;
      }
    } else {
      delete curAdd;
    }
  }
  curAdd=nullptr;
  
  outNb=nbRes;
  return res;
}
Path**      Path::SubPathsWithNesting(int &outNb,bool killNoSurf,int nbNest,int* nesting,int* conts)
{
  int      nbRes=0;
  Path**   res=nullptr;
  Path*    curAdd=nullptr;
  bool     increment=false;
  
  for (int i=0;i<int(descr_cmd.size());i++) {
    int const typ = descr_cmd[i]->getType();
    switch ( typ ) {
      case descr_moveto:
      {
        if ( curAdd && increment == false ) {
          if ( curAdd->descr_cmd.size() > 1 ) {
            // sauvegarder descr_cmd[0]->associated
            int savA=curAdd->descr_cmd[0]->associated;
            curAdd->Convert(1.0);
            curAdd->descr_cmd[0]->associated=savA; // associated n'est pas utilise apres
            double addSurf=curAdd->Surface();
            if ( fabs(addSurf) > 0.0001 || killNoSurf == false ) {
              res=(Path**)g_realloc(res,(nbRes+1)*sizeof(Path*));
              res[nbRes++]=curAdd;
            } else { 
              delete curAdd;
            }
          } else {
            delete curAdd;
          }
          curAdd=nullptr;
        }
        Path*  hasParent=nullptr;
        for (int j=0;j<nbNest;j++) {
          if ( conts[j] == i && nesting[j] >= 0 ) {
            int  parentMvt=conts[nesting[j]];
            for (int k=0;k<nbRes;k++) {
              if ( res[k] && res[k]->descr_cmd.empty() == false && res[k]->descr_cmd[0]->associated == parentMvt ) {
                hasParent=res[k];
                break;
              }
            }
          }
          if ( conts[j] > i  ) break;
        }
        if ( hasParent ) {
          curAdd=hasParent;
          increment=true;
        } else {
          curAdd=new Path;
          curAdd->SetBackData(false);
          increment=false;
        }
        PathDescrMoveTo *nData = dynamic_cast<PathDescrMoveTo *>(descr_cmd[i]);
        int mNo=curAdd->MoveTo(nData->p);
        curAdd->descr_cmd[mNo]->associated=i;
        }
        break;
      case descr_close:
      {
        curAdd->Close();
      }
        break;        
      case descr_lineto:
      {
        PathDescrLineTo *nData = dynamic_cast<PathDescrLineTo *>(descr_cmd[i]);
        curAdd->LineTo(nData->p);
      }
        break;
      case descr_cubicto:
      {
        PathDescrCubicTo *nData = dynamic_cast<PathDescrCubicTo *>(descr_cmd[i]);
        curAdd->CubicTo(nData->p,nData->start,nData->end);
      }
        break;
      case descr_arcto:
      {
        PathDescrArcTo *nData = dynamic_cast<PathDescrArcTo *>(descr_cmd[i]);
        curAdd->ArcTo(nData->p,nData->rx,nData->ry,nData->angle,nData->large,nData->clockwise);
      }
        break;
      default:
        break;
    }
  }
  if ( curAdd && increment == false ) {
    if ( curAdd->descr_cmd.size() > 1 ) {
      curAdd->Convert(1.0);
      double addSurf=curAdd->Surface();
      if ( fabs(addSurf) > 0.0001 || killNoSurf == false  ) {
        res=(Path**)g_realloc(res,(nbRes+1)*sizeof(Path*));
        res[nbRes++]=curAdd;
      } else {
        delete curAdd;
      }
    } else {
      delete curAdd;
    }
  }
  curAdd=nullptr;
  
  outNb=nbRes;
  return res;
}


void Path::ConvertForcedToVoid()
{  
    for (int i=0; i < int(descr_cmd.size()); i++) {
        if ( descr_cmd[i]->getType() == descr_forced) {
            delete descr_cmd[i];
            descr_cmd.erase(descr_cmd.begin() + i);
        }
    }
}


void Path::ConvertForcedToMoveTo()
{  
    Geom::Point lastSeen(0, 0);
    Geom::Point lastMove(0, 0);
    
    {
        Geom::Point lastPos(0, 0);
        for (int i = int(descr_cmd.size()) - 1; i >= 0; i--) {
            int const typ = descr_cmd[i]->getType();
            switch ( typ ) {
            case descr_forced:
            {
                PathDescrForced *d = dynamic_cast<PathDescrForced *>(descr_cmd[i]);
                d->p = lastPos;
                break;
            }
            case descr_close:
            {
                PathDescrClose *d = dynamic_cast<PathDescrClose *>(descr_cmd[i]);
                d->p = lastPos;
                break;
            }
            case descr_moveto:
            {
                PathDescrMoveTo *d = dynamic_cast<PathDescrMoveTo *>(descr_cmd[i]);
                lastPos = d->p;
                break;
            }
            case descr_lineto:
            {
                PathDescrLineTo *d = dynamic_cast<PathDescrLineTo *>(descr_cmd[i]);
                lastPos = d->p;
                break;
            }
            case descr_arcto:
            {
                PathDescrArcTo *d = dynamic_cast<PathDescrArcTo *>(descr_cmd[i]);
                lastPos = d->p;
                break;
            }
            case descr_cubicto:
            {
                PathDescrCubicTo *d = dynamic_cast<PathDescrCubicTo *>(descr_cmd[i]);
                lastPos = d->p;
                break;
            }
            default:
                break;
            }
        }
    }

    bool hasMoved = false;
    for (int i = 0; i < int(descr_cmd.size()); i++) {
        int const typ = descr_cmd[i]->getType();
        switch ( typ ) {
        case descr_forced:
            if ( i < int(descr_cmd.size()) - 1 && hasMoved ) { // sinon il termine le chemin

                delete descr_cmd[i];
                descr_cmd[i] = new PathDescrMoveTo(lastSeen);
                lastMove = lastSeen;
                hasMoved = true;
            }
            break;
            
        case descr_moveto:
        {
          PathDescrMoveTo *nData = dynamic_cast<PathDescrMoveTo *>(descr_cmd[i]);
          lastMove = lastSeen = nData->p;
          hasMoved = true;
        }
        break;
      case descr_close:
      {
        lastSeen=lastMove;
      }
        break;        
      case descr_lineto:
      {
        PathDescrLineTo *nData = dynamic_cast<PathDescrLineTo *>(descr_cmd[i]);
        lastSeen=nData->p;
      }
        break;
      case descr_cubicto:
      {
        PathDescrCubicTo *nData = dynamic_cast<PathDescrCubicTo *>(descr_cmd[i]);
        lastSeen=nData->p;
     }
        break;
      case descr_arcto:
      {
        PathDescrArcTo *nData = dynamic_cast<PathDescrArcTo *>(descr_cmd[i]);
        lastSeen=nData->p;
      }
        break;
      default:
        break;
    }
  }
}
static int       CmpPosition(const void * p1, const void * p2) {
  Path::cut_position *cp1=(Path::cut_position*)p1;
  Path::cut_position *cp2=(Path::cut_position*)p2;
  if ( cp1->piece < cp2->piece ) return -1;
  if ( cp1->piece > cp2->piece ) return 1;
  if ( cp1->t < cp2->t ) return -1;
  if ( cp1->t > cp2->t ) return 1;
  return 0;
}
static int       CmpCurv(const void * p1, const void * p2) {
  double *cp1=(double*)p1;
  double *cp2=(double*)p2;
  if ( *cp1 < *cp2 ) return -1;
  if ( *cp1 > *cp2 ) return 1;
  return 0;
}


Path::cut_position* Path::CurvilignToPosition(int nbCv, double *cvAbs, int &nbCut)
{
    if ( nbCv <= 0 || pts.empty() || back == false ) {
        return nullptr;
    }
  
    qsort(cvAbs, nbCv, sizeof(double), CmpCurv);
  
    cut_position *res = nullptr;
    nbCut = 0;
    int curCv = 0;
  
    double len = 0;
    double lastT = 0;
    int lastPiece = -1;

    Geom::Point lastM = pts[0].p;
    Geom::Point lastP = lastM;

    for (const auto & pt : pts) {

        if ( pt.isMoveTo == polyline_moveto ) {

            lastP = lastM = pt.p;
            lastT = pt.t;
            lastPiece = pt.piece;

        } else {
            
            double const add = Geom::L2(pt.p - lastP);
            double curPos = len;
            double curAdd = add;
            
            while ( curAdd > 0.0001 && curCv < nbCv && curPos + curAdd >= cvAbs[curCv] ) {
                double const theta = (cvAbs[curCv] - len) / add;
                res = (cut_position*) g_realloc(res, (nbCut + 1) * sizeof(cut_position));
                res[nbCut].piece = pt.piece;
                res[nbCut].t = theta * pt.t + (1 - theta) * ( (lastPiece != pt.piece) ? 0 : lastT);
                nbCut++;
                curAdd -= cvAbs[curCv] - curPos;
                curPos = cvAbs[curCv];
                curCv++;
            }
            
            len += add;
            lastPiece = pt.piece;
            lastP = pt.p;
            lastT = pt.t;
        }
    }
    
    return res;
}

/* 
Moved from Layout-TNG-OutIter.cpp
TODO: clean up uses of the original function and remove

Original Comment:
"this function really belongs to Path. I'll probably move it there eventually,
hence the Path-esque coding style"

*/
template<typename T> inline static T square(T x) {return x*x;}
Path::cut_position Path::PointToCurvilignPosition(Geom::Point const &pos, unsigned seg) const
{
    // if the parameter "seg" == 0, then all segments will be considered
    // In however e.g. "seg" == 6 , then only the 6th segment will be considered 
 
    unsigned bestSeg = 0;
    double bestRangeSquared = DBL_MAX;
    double bestT = 0.0; // you need a sentinel, or make sure that you prime with correct values.

    for (unsigned i = 1 ; i < pts.size() ; i++) {
        if (pts[i].isMoveTo == polyline_moveto || (seg > 0 && i != seg)) continue;
        Geom::Point p1, p2, localPos;
        double thisRangeSquared;
        double t;

        if (pts[i - 1].p == pts[i].p) {
            thisRangeSquared = square(pts[i].p[Geom::X] - pos[Geom::X]) + square(pts[i].p[Geom::Y] - pos[Geom::Y]);
            t = 0.0;
        } else {
            // we rotate all our coordinates so we're always looking at a mostly vertical line.
            if (fabs(pts[i - 1].p[Geom::X] - pts[i].p[Geom::X]) < fabs(pts[i - 1].p[Geom::Y] - pts[i].p[Geom::Y])) {
                p1 = pts[i - 1].p;
                p2 = pts[i].p;
                localPos = pos;
            } else {
                p1 = pts[i - 1].p.cw();
                p2 = pts[i].p.cw();
                localPos = pos.cw();
            }
            double gradient = (p2[Geom::X] - p1[Geom::X]) / (p2[Geom::Y] - p1[Geom::Y]);
            double intersection = p1[Geom::X] - gradient * p1[Geom::Y];
            /*
              orthogonalGradient = -1.0 / gradient; // you are going to have numerical problems here.
              orthogonalIntersection = localPos[Geom::X] - orthogonalGradient * localPos[Geom::Y];
              nearestY = (orthogonalIntersection - intersection) / (gradient - orthogonalGradient);

              expand out nearestY fully :
              nearestY = (localPos[Geom::X] - (-1.0 / gradient) * localPos[Geom::Y] - intersection) / (gradient - (-1.0 / gradient));

              multiply top and bottom by gradient:
              nearestY = (localPos[Geom::X] * gradient - (-1.0) * localPos[Geom::Y] - intersection * gradient) / (gradient * gradient - (-1.0));

              and simplify to get:
            */
            double nearestY =  (localPos[Geom::X] * gradient + localPos[Geom::Y] - intersection * gradient)
                             / (gradient * gradient + 1.0);
            t = (nearestY - p1[Geom::Y]) / (p2[Geom::Y] - p1[Geom::Y]);
            if (t <= 0.0) {
                thisRangeSquared = square(p1[Geom::X] - localPos[Geom::X]) + square(p1[Geom::Y] - localPos[Geom::Y]);
                t = 0.0;
            } else if (t >= 1.0) {
                thisRangeSquared = square(p2[Geom::X] - localPos[Geom::X]) + square(p2[Geom::Y] - localPos[Geom::Y]);
                t = 1.0;
            } else {
                thisRangeSquared = square(nearestY * gradient + intersection - localPos[Geom::X]) + square(nearestY - localPos[Geom::Y]);
            }
        }

        if (thisRangeSquared < bestRangeSquared) {
            bestSeg = i;
            bestRangeSquared = thisRangeSquared;
            bestT = t;
        }
    }
    Path::cut_position result;
    if (bestSeg == 0) {
        result.piece = 0;
        result.t = 0.0;
    } else {
        result.piece = pts[bestSeg].piece;
        if (result.piece == pts[bestSeg - 1].piece) {
            result.t = pts[bestSeg - 1].t * (1.0 - bestT) + pts[bestSeg].t * bestT;
        } else {
            result.t = pts[bestSeg].t * bestT;
        }
    }
    return result;
}
/*
    this one also belongs to Path
    returns the length of the path up to the position indicated by t (0..1)

    TODO: clean up uses of the original function and remove

    should this take a cut_position as a parameter?
*/
double Path::PositionToLength(int piece, double t)
{
    double length = 0.0;
    for (unsigned i = 1 ; i < pts.size() ; i++) {
        if (pts[i].isMoveTo == polyline_moveto) continue;
        if (pts[i].piece == piece && t < pts[i].t) {
            length += Geom::L2((t - pts[i - 1].t) / (pts[i].t - pts[i - 1].t) * (pts[i].p - pts[i - 1].p));
            break;
        }
        length += Geom::L2(pts[i].p - pts[i - 1].p);
    }
    return length;
}

void Path::ConvertPositionsToForced(int nbPos, cut_position *poss)
{
    if ( nbPos <= 0 ) {
        return;
    }
    
    {
        Geom::Point lastPos(0, 0);
        for (int i = int(descr_cmd.size()) - 1; i >= 0; i--) {
            int const typ = descr_cmd[i]->getType();
            switch ( typ ) {
                
            case descr_forced:
            {
                PathDescrForced *d = dynamic_cast<PathDescrForced *>(descr_cmd[i]);
                d->p = lastPos;
                break;
            }
                
            case descr_close:
            {
                delete descr_cmd[i];
                descr_cmd[i] = new PathDescrLineTo(Geom::Point(0, 0));

                int fp = i - 1;
                while ( fp >= 0 && (descr_cmd[fp]->getType()) != descr_moveto ) {
                    fp--;
                }
                
                if ( fp >= 0 ) {
                    PathDescrMoveTo *oData = dynamic_cast<PathDescrMoveTo *>(descr_cmd[fp]);
                    dynamic_cast<PathDescrLineTo*>(descr_cmd[i])->p = oData->p;
                }
            }
            break;
            
        case descr_moveto:
        {
            PathDescrMoveTo *d = dynamic_cast<PathDescrMoveTo *>(descr_cmd[i]);
            lastPos = d->p;
            break;
        }
        case descr_lineto:
        {
            PathDescrLineTo *d = dynamic_cast<PathDescrLineTo *>(descr_cmd[i]);
            lastPos = d->p;
            break;
        }
        case descr_arcto:
        {
            PathDescrArcTo *d = dynamic_cast<PathDescrArcTo *>(descr_cmd[i]);
            lastPos = d->p;
            break;
        }
        case descr_cubicto:
        {
            PathDescrCubicTo *d = dynamic_cast<PathDescrCubicTo *>(descr_cmd[i]);
            lastPos = d->p;
            break;
        }
        default:
          break;
      }
    }
  }
  if (descr_cmd[0]->getType() == descr_moveto)
    descr_flags |= descr_doing_subpath;         // see LP Bug 166302

  qsort(poss, nbPos, sizeof(cut_position), CmpPosition);

  for (int curP=0;curP<nbPos;curP++) {
    int   cp=poss[curP].piece;
    if ( cp < 0 || cp >= int(descr_cmd.size()) ) break;
    float ct=poss[curP].t;
    if ( ct < 0 ) continue;
    if ( ct > 1 ) continue;
        
    int const typ = descr_cmd[cp]->getType();
    if ( typ == descr_moveto || typ == descr_forced || typ == descr_close ) {
      // ponctuel= rien a faire
    } else if ( typ == descr_lineto || typ == descr_arcto || typ == descr_cubicto ) {
      // facile: creation d'un morceau et d'un forced -> 2 commandes
      Geom::Point        theP;
      Geom::Point        theT;
      Geom::Point        startP;
      startP=PrevPoint(cp-1);
      if ( typ == descr_cubicto ) {
        double           len,rad;
        Geom::Point        stD,enD,endP;
        {
          PathDescrCubicTo *oData = dynamic_cast<PathDescrCubicTo *>(descr_cmd[cp]);
          stD=oData->start;
          enD=oData->end;
          endP=oData->p;
          TangentOnCubAt (ct, startP, *oData,true, theP,theT,len,rad);
        }
        
        theT*=len;
        
        InsertCubicTo(endP,(1-ct)*theT,(1-ct)*enD,cp+1);
        InsertForcePoint(cp+1);
        {
          PathDescrCubicTo *nData = dynamic_cast<PathDescrCubicTo *>(descr_cmd[cp]);
          nData->start=ct*stD;
          nData->end=ct*theT;
          nData->p=theP;
        }
        // decalages dans le tableau des positions de coupe
        for (int j=curP+1;j<nbPos;j++) {
          if ( poss[j].piece == cp ) {
            poss[j].piece+=2;
            poss[j].t=(poss[j].t-ct)/(1-ct);
          } else {
            poss[j].piece+=2;
          }
        }
      } else if ( typ == descr_lineto ) {
        Geom::Point        endP;
        {
          PathDescrLineTo *oData = dynamic_cast<PathDescrLineTo *>(descr_cmd[cp]);
          endP=oData->p;
        }

        theP=ct*endP+(1-ct)*startP;
        
        InsertLineTo(endP,cp+1);
        InsertForcePoint(cp+1);
        {
          PathDescrLineTo *nData = dynamic_cast<PathDescrLineTo *>(descr_cmd[cp]);
          nData->p=theP;
        }
        // decalages dans le tableau des positions de coupe
       for (int j=curP+1;j<nbPos;j++) {
          if ( poss[j].piece == cp ) {
            poss[j].piece+=2;
            poss[j].t=(poss[j].t-ct)/(1-ct);
          } else {
            poss[j].piece+=2;
          }
        }
      } else if ( typ == descr_arcto ) {
        Geom::Point        endP;
        double           rx,ry,angle;
        bool             clockw,large;
        double   delta=0;
        {
          PathDescrArcTo *oData = dynamic_cast<PathDescrArcTo *>(descr_cmd[cp]);
          endP=oData->p;
          rx=oData->rx;
          ry=oData->ry;
          angle=oData->angle;
          clockw=oData->clockwise;
          large=oData->large;
        }
        {
          double      sang,eang;
          ArcAngles(startP,endP,rx,ry,angle*M_PI/180.0,large,clockw,sang,eang);
          
          if (clockw) {
            if ( sang < eang ) sang += 2*M_PI;
            delta=eang-sang;
          } else {
            if ( sang > eang ) sang -= 2*M_PI;
            delta=eang-sang;
          }
          if ( delta < 0 ) delta=-delta;
        }
        
        PointAt (cp,ct, theP);
        
        if ( delta*(1-ct) > M_PI ) {
          InsertArcTo(endP,rx,ry,angle,true,clockw,cp+1);
        } else {
          InsertArcTo(endP,rx,ry,angle,false,clockw,cp+1);
        }
        InsertForcePoint(cp+1);
        {
          PathDescrArcTo *nData = dynamic_cast<PathDescrArcTo *>(descr_cmd[cp]);
          nData->p=theP;
          if ( delta*ct > M_PI ) {
            nData->large=true;
          } else {
            nData->large=false;
          }
        }
        // decalages dans le tableau des positions de coupe
        for (int j=curP+1;j<nbPos;j++) {
          if ( poss[j].piece == cp ) {
            poss[j].piece+=2;
            poss[j].t=(poss[j].t-ct)/(1-ct);
          } else {
            poss[j].piece+=2;
          }
        }
      }
    }
  }
}

void        Path::ConvertPositionsToMoveTo(int nbPos,cut_position* poss)
{
  ConvertPositionsToForced(nbPos,poss);
//  ConvertForcedToMoveTo();
  // on fait une version customizee a la place

  Path*  res=new Path;
  
  Geom::Point    lastP(0,0);
  for (int i=0;i<int(descr_cmd.size());i++) {
    int const typ = descr_cmd[i]->getType();
    if ( typ == descr_moveto ) {
      Geom::Point  np;
      {
        PathDescrMoveTo *nData = dynamic_cast<PathDescrMoveTo *>(descr_cmd[i]);
        np=nData->p;
      }
      Geom::Point  endP;
      bool       hasClose=false;
      int        hasForced=-1;
      bool       doesClose=false;
      int        j=i+1;
      for (;j<int(descr_cmd.size());j++) {
        int const ntyp = descr_cmd[j]->getType();
        if ( ntyp == descr_moveto ) {
          j--;
          break;
        } else if ( ntyp == descr_forced ) {
          if ( hasForced < 0 ) hasForced=j;
        } else if ( ntyp == descr_close ) {
          hasClose=true;
          break;
        } else if ( ntyp == descr_lineto ) {
          PathDescrLineTo *nData = dynamic_cast<PathDescrLineTo *>(descr_cmd[j]);
          endP=nData->p;
        } else if ( ntyp == descr_arcto ) {
          PathDescrArcTo *nData = dynamic_cast<PathDescrArcTo *>(descr_cmd[j]);
          endP=nData->p;
        } else if ( ntyp == descr_cubicto ) {
          PathDescrCubicTo *nData = dynamic_cast<PathDescrCubicTo *>(descr_cmd[j]);
          endP=nData->p;
        }
      }
      if ( Geom::LInfty(endP-np) < 0.00001 ) {
        doesClose=true;
      }
      if ( ( doesClose || hasClose ) && hasForced >= 0 ) {
 //       printf("nasty i=%i j=%i frc=%i\n",i,j,hasForced);
        // aghhh.
        Geom::Point   nMvtP=PrevPoint(hasForced);
        res->MoveTo(nMvtP);
        Geom::Point   nLastP=nMvtP;
        for (int k = hasForced + 1; k < j; k++) {
          int ntyp=descr_cmd[k]->getType();
          if ( ntyp == descr_moveto ) {
            // ne doit pas arriver
          } else if ( ntyp == descr_forced ) {
            res->MoveTo(nLastP);
          } else if ( ntyp == descr_close ) {
            // rien a faire ici; de plus il ne peut y en avoir qu'un
          } else if ( ntyp == descr_lineto ) {
            PathDescrLineTo *nData = dynamic_cast<PathDescrLineTo *>(descr_cmd[k]);
            res->LineTo(nData->p);
            nLastP=nData->p;
          } else if ( ntyp == descr_arcto ) {
            PathDescrArcTo *nData = dynamic_cast<PathDescrArcTo *>(descr_cmd[k]);
            res->ArcTo(nData->p,nData->rx,nData->ry,nData->angle,nData->large,nData->clockwise);
            nLastP=nData->p;
          } else if ( ntyp == descr_cubicto ) {
            PathDescrCubicTo *nData = dynamic_cast<PathDescrCubicTo *>(descr_cmd[k]);
            res->CubicTo(nData->p,nData->start,nData->end);
            nLastP=nData->p;
          }
        }
        if ( doesClose == false ) res->LineTo(np);
        nLastP=np;
        for (int k=i+1;k<hasForced;k++) {
          int ntyp=descr_cmd[k]->getType();
          if ( ntyp == descr_moveto ) {
            // ne doit pas arriver
          } else if ( ntyp == descr_forced ) {
            res->MoveTo(nLastP);
          } else if ( ntyp == descr_close ) {
            // rien a faire ici; de plus il ne peut y en avoir qu'un
          } else if ( ntyp == descr_lineto ) {
            PathDescrLineTo *nData = dynamic_cast<PathDescrLineTo *>(descr_cmd[k]);
            res->LineTo(nData->p);
            nLastP=nData->p;
          } else if ( ntyp == descr_arcto ) {
            PathDescrArcTo *nData = dynamic_cast<PathDescrArcTo *>(descr_cmd[k]);
            res->ArcTo(nData->p,nData->rx,nData->ry,nData->angle,nData->large,nData->clockwise);
            nLastP=nData->p;
          } else if ( ntyp == descr_cubicto ) {
            PathDescrCubicTo *nData = dynamic_cast<PathDescrCubicTo *>(descr_cmd[k]);
            res->CubicTo(nData->p,nData->start,nData->end);
            nLastP=nData->p;
          }
        }
        lastP=nMvtP;
        i=j;
      } else {
        // regular, just move on
        res->MoveTo(np);
        lastP=np;
      }
    } else if ( typ == descr_close ) {
      res->Close();
    } else if ( typ == descr_forced ) {
      res->MoveTo(lastP);
    } else if ( typ == descr_lineto ) {
      PathDescrLineTo *nData = dynamic_cast<PathDescrLineTo *>(descr_cmd[i]);
      res->LineTo(nData->p);
      lastP=nData->p;
    } else if ( typ == descr_arcto ) {
      PathDescrArcTo *nData = dynamic_cast<PathDescrArcTo *>(descr_cmd[i]);
      res->ArcTo(nData->p,nData->rx,nData->ry,nData->angle,nData->large,nData->clockwise);
      lastP=nData->p;
    } else if ( typ == descr_cubicto ) {
      PathDescrCubicTo *nData = dynamic_cast<PathDescrCubicTo *>(descr_cmd[i]);
      res->CubicTo(nData->p,nData->start,nData->end);
      lastP=nData->p;
    } else {
    }
  }

  Copy(res);
  delete res;
  return;
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

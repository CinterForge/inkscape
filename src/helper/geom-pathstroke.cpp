// SPDX-License-Identifier: GPL-2.0-or-later
/* Authors:
 *   Liam P. White
 *   Tavmjong Bah
 *   Alexander Brock
 *
 * Copyright (C) 2014-2015 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <iomanip>
#include <random>
#include <2geom/path-sink.h>
#include <2geom/sbasis-to-bezier.h> // cubicbezierpath_from_sbasis
#include <2geom/path-intersection.h>
#include <2geom/circle.h>
#include <2geom/sweeper.h>

#include "helper/geom-pathstroke.h"
#include "helper/geom.h"
#include "path/path-boolop.h"
#include "util/treeify.h"

namespace Geom {

static Point intersection_point(Point origin_a, Point vector_a, Point origin_b, Point vector_b)
{
    Coord denom = cross(vector_a, vector_b);
    if (!are_near(denom,0.)) {
        Coord t = (cross(vector_b, origin_a) + cross(origin_b, vector_b)) / denom;
        return origin_a + vector_a*t;
    }
    return Point(infinity(), infinity());
}

/**
* Find circle that touches inside of the curve, with radius matching the curvature, at time value \c t.
* Because this method internally uses unitTangentAt, t should be smaller than 1.0 (see unitTangentAt).
*/
static Circle touching_circle( D2<SBasis> const &curve, double t, double tol=0.01 )
{
    D2<SBasis> dM=derivative(curve);
    if ( are_near(L2sq(dM(t)), tol) ) {
        dM=derivative(dM);
    }
    if ( are_near(L2sq(dM(t)), tol) ) {   // try second time
        dM=derivative(dM);
    }
    Piecewise<D2<SBasis> > unitv = unitVector(dM,tol);
    Piecewise<SBasis> dMlength = dot(Piecewise<D2<SBasis> >(dM),unitv);
    Piecewise<SBasis> k = cross(derivative(unitv),unitv);
    k = divide(k,dMlength,tol,3);
    double curv = k(t); // note that this value is signed

    Geom::Point normal = unitTangentAt(curve, t).cw();
    double radius = 1/curv;
    Geom::Point center = curve(t) + radius*normal;
    return Geom::Circle(center, fabs(radius));
}


// Area of triangle given three corner points
static double area( Geom::Point a, Geom::Point b, Geom::Point c ) {

    using Geom::X;
    using Geom::Y;
    return( 0.5 * fabs( ( a[X]*(b[Y]-c[Y]) + b[X]*(c[Y]-a[Y]) + c[X]*(a[Y]-b[Y]) ) ) );
}

// Alternative touching circle routine directly using Beziers. Works only at end points.
static Circle touching_circle( CubicBezier const &curve, bool start ) {

    double k = 0;
    Geom::Point p;
    Geom::Point normal;
    if ( start ) {
        double distance = Geom::distance( curve[1], curve[0] );
        k = 4.0/3.0 * area( curve[0], curve[1], curve[2] ) /
            (distance * distance * distance);
        if( Geom::cross(curve[0]-curve[1], curve[1]-curve[2]) < 0 ) {
            k = -k;
        }
        p = curve[0];
        normal = Geom::Point(curve[1] - curve[0]).cw();
        normal.normalize();
        // std::cout << "Start k: " << k << " d: " << distance << " normal: " << normal << std::endl;
    } else {
        double distance = Geom::distance( curve[3], curve[2] );
        k = 4.0/3.0 * area( curve[1], curve[2], curve[3] ) /
            (distance * distance * distance);
        if( Geom::cross(curve[1]-curve[2], curve[2]-curve[3]) < 0 ) {
            k = -k;
        }
        p = curve[3];
        normal = Geom::Point(curve[3] - curve[2]).cw();
        normal.normalize();
        // std::cout << "End   k: " << k << " d: " << distance << " normal: " << normal << std::endl;
    }
    if( k == 0 ) {
        return Geom::Circle( Geom::Point(0,std::numeric_limits<float>::infinity()),
                             std::numeric_limits<float>::infinity());
    } else {
        double radius = 1/k;
        Geom::Point center = p + normal * radius;
        return Geom::Circle( center, fabs(radius) );
    }
}
}

namespace {

// Internal data structure

struct join_data {
    join_data(Geom::Path &_res, Geom::Path const&_outgoing, Geom::Point _in_tang, Geom::Point _out_tang, double _miter, double _width)
        : res(_res), outgoing(_outgoing), in_tang(_in_tang), out_tang(_out_tang), miter(_miter), width(_width) {};

    // contains the current path that is being built on
    Geom::Path &res;

    // contains the next curve to append
    Geom::Path const& outgoing;

    // input tangents
    Geom::Point in_tang;
    Geom::Point out_tang;

    // line parameters
    double miter;
    double width; // half stroke width
};

// Join functions must append the outgoing path

typedef void join_func(join_data jd);

void bevel_join(join_data jd)
{
    jd.res.appendNew<Geom::LineSegment>(jd.outgoing.initialPoint());
    jd.res.append(jd.outgoing);
}

void round_join(join_data jd)
{
    jd.res.appendNew<Geom::EllipticalArc>(jd.width, jd.width, 0, false, jd.width <= 0, jd.outgoing.initialPoint());
    jd.res.append(jd.outgoing);
}

void miter_join_internal(join_data const &jd, bool clip)
{
    using namespace Geom;

    Curve const& incoming = jd.res.back();
    Curve const& outgoing = jd.outgoing.front();
    Geom::Path &res = jd.res;
    double width = jd.width, miter = jd.miter;

    Point tang1 = jd.in_tang;
    Point tang2 = jd.out_tang;
    Point p = intersection_point(incoming.finalPoint(), tang1, outgoing.initialPoint(), tang2);

    bool satisfied = false;
    bool inc_ls = res.back_open().degreesOfFreedom() <= 4;

    if (p.isFinite()) {
        // check size of miter
        Point point_on_path = incoming.finalPoint() + rot90(tang1)*width;
        // SVG defines miter length as distance between inner intersection and outer intersection,
        // which is twice the distance from p to point_on_path but width is half stroke width.
        satisfied = distance(p, point_on_path) <= miter * width; 
        if (satisfied) {
            // miter OK, check to see if we can do a relocation
            if (inc_ls) {
                res.setFinal(p);
            } else {
                res.appendNew<LineSegment>(p);
            }
        } else if (clip) {
            // std::cout << "  Clipping ------------ " << std::endl;
            // miter needs clipping, find two points
            Point bisector_versor = Line(point_on_path, p).versor();
            Point point_limit = point_on_path + miter * width * bisector_versor;
            // std::cout << "     bisector_versor: " << bisector_versor << std::endl;
            // std::cout << "     point_limit: " << point_limit << std::endl;
            Point p1 = intersection_point(incoming.finalPoint(), tang1, point_limit, bisector_versor.cw());
            Point p2 = intersection_point(outgoing.initialPoint(), tang2, point_limit, bisector_versor.cw());
            // std::cout << "     p1: " << p1 << std::endl;
            // std::cout << "     p2: " << p2 << std::endl;
            if (inc_ls) {
                res.setFinal(p1);
            } else {
                res.appendNew<LineSegment>(p1);
            }
            res.appendNew<LineSegment>(p2);
        }
    }

    res.appendNew<LineSegment>(outgoing.initialPoint());

    // check if we can do another relocation
    bool out_ls = outgoing.degreesOfFreedom() <= 4;

    if ((satisfied || clip) && out_ls) {
        res.setFinal(outgoing.finalPoint());
    } else {
        res.append(outgoing);
    }

    // either way, add the rest of the path
    res.insert(res.end(), ++jd.outgoing.begin(), jd.outgoing.end());
}

void miter_join(join_data jd) { miter_join_internal(jd, false); }
void miter_clip_join(join_data jd) { miter_join_internal(jd, true); }

Geom::Point pick_solution(std::vector<Geom::ShapeIntersection> points, Geom::Point tang2, Geom::Point endPt)
{
    assert(points.size() == 2);
    Geom::Point sol;
    if ( dot(tang2, points[0].point() - endPt) > 0 ) {
        // points[0] is bad, choose points[1]
        sol = points[1];
    } else if ( dot(tang2, points[1].point() - endPt) > 0 ) { // points[0] could be good, now check points[1]
        // points[1] is bad, choose points[0]
        sol = points[0];
    } else {
        // both points are good, choose nearest
        sol = ( distanceSq(endPt, points[0].point()) < distanceSq(endPt, points[1].point()) )
            ? points[0].point() : points[1].point();
    }
    return sol;
}

// Arcs line join. If two circles don't intersect, expand inner circle.
Geom::Point expand_circle( Geom::Circle &inner_circle, Geom::Circle const &outer_circle, Geom::Point const &start_pt, Geom::Point const &start_tangent ) {
    // std::cout << "expand_circle:" << std::endl;
    // std::cout << "  outer_circle: radius: " << outer_circle.radius() << "  center: " << outer_circle.center() << std::endl;
    // std::cout << "  start: point: " << start_pt << "  tangent: " << start_tangent << std::endl;

    if( !(outer_circle.contains(start_pt) ) ) {
        // std::cout << "  WARNING: Outer circle does not contain starting point!" << std::endl;
        return Geom::Point(0,0);
    }

    Geom::Line secant1(start_pt, start_pt + start_tangent);
    std::vector<Geom::ShapeIntersection> chord1_pts = outer_circle.intersect(secant1);
    // std::cout << "  chord1: " << chord1_pts[0].point() << ", " << chord1_pts[1].point() << std::endl;
    Geom::LineSegment chord1(chord1_pts[0].point(), chord1_pts[1].point());

    Geom::Line bisector = make_bisector_line( chord1 );
    std::vector<Geom::ShapeIntersection> chord2_pts = outer_circle.intersect(bisector);
    // std::cout << "  chord2: " << chord2_pts[0].point() << ", " << chord2_pts[1].point() << std::endl;
    Geom::LineSegment chord2(chord2_pts[0].point(), chord2_pts[1].point());

    // Find D, point on chord2 and on circle closest to start point
    Geom::Coord d0 = Geom::distance(chord2_pts[0].point(),start_pt);
    Geom::Coord d1 = Geom::distance(chord2_pts[1].point(),start_pt);
    // std::cout << "  d0: " << d0 << " d1: " << d1 << std::endl;
    Geom::Point d = (d0 < d1) ? chord2_pts[0].point() : chord2_pts[1].point();
    Geom::Line da(d,start_pt);

    // Chord through start point and point D
    std::vector<Geom::ShapeIntersection> chord3_pts =  outer_circle.intersect(da);
    // std::cout << "  chord3: " << chord3_pts[0].point() << ", " << chord3_pts[1].point() << std::endl;

    // Find farthest point on chord3 and on circle (could be more robust)
    Geom::Coord d2 = Geom::distance(chord3_pts[0].point(),d);
    Geom::Coord d3 = Geom::distance(chord3_pts[1].point(),d);
    // std::cout << "  d2: " << d2 << " d3: " << d3 << std::endl;

    // Find point P, the intersection of outer circle and new inner circle
    Geom::Point p = (d2 > d3) ? chord3_pts[0].point() : chord3_pts[1].point();

    // Find center of new circle: it is at the intersection of the bisector
    // of the chord defined by the start point and point P and a line through
    // the start point and parallel to the first bisector.
    Geom::LineSegment chord4(start_pt,p);
    Geom::Line bisector2 = make_bisector_line( chord4 );
    Geom::Line diameter = make_parallel_line( start_pt, bisector );
    std::vector<Geom::ShapeIntersection> center_new = bisector2.intersect( diameter );
    // std::cout << "  center_new: " << center_new[0].point() << std::endl;
    Geom::Coord r_new = Geom::distance( center_new[0].point(), start_pt );
    // std::cout << "  r_new: " << r_new << std::endl;

    inner_circle.setCenter( center_new[0].point() );
    inner_circle.setRadius( r_new );
    return p;
}

// Arcs line join. If two circles don't intersect, adjust both circles so they just touch.
// Increase (decrease) the radius of circle 1 and decrease (increase) of circle 2 by the same amount keeping the given points and tangents fixed.
Geom::Point adjust_circles( Geom::Circle &circle1, Geom::Circle &circle2, Geom::Point const &point1, Geom::Point const &point2, Geom::Point const &tan1, Geom::Point const &tan2 ) {

    Geom::Point n1 = (circle1.center() - point1).normalized(); // Always points towards center
    Geom::Point n2 = (circle2.center() - point2).normalized();
    Geom::Point sum_n = n1 + n2;

    double r1 = circle1.radius();
    double r2 = circle2.radius();
    double delta_r = r2 - r1;
    Geom::Point c1 = circle1.center();
    Geom::Point c2 = circle2.center();
    Geom::Point delta_c = c2 - c1;

    // std::cout << "adjust_circles:" << std::endl;
    // std::cout << "    norm: " << n1 << "; " << n2 << std::endl;
    // std::cout << "    sum_n: " << sum_n << std::endl;
    // std::cout << "    delta_r: " << delta_r << std::endl;
    // std::cout << "    delta_c: " << delta_c << std::endl;

    // Quadratic equation
    double A = 4 - sum_n.length() * sum_n.length();
    double B = 4.0 * delta_r - 2.0 * Geom::dot( delta_c, sum_n );
    double C = delta_r * delta_r - delta_c.length() * delta_c.length();

    double s1 = 0;
    double s2 = 0;

    if( fabs(A) < 0.01 ) {
        // std::cout << "    A near zero! $$$$$$$$$$$$$$$$$$" << std::endl;
        if( B != 0 ) {
            s1 = -C/B;
            s2 = -s1;
        }
    } else {
        s1 = (-B + sqrt(B*B - 4*A*C))/(2*A);
        s2 = (-B - sqrt(B*B - 4*A*C))/(2*A);
    }

    double dr = (fabs(s1)<=fabs(s2)?s1:s2);

    // std::cout << "    A: " << A << std::endl;
    // std::cout << "    B: " << B << std::endl;
    // std::cout << "    C: " << C << std::endl;
    // std::cout << "    s1: " << s1 << " s2: " << s2 << " dr: " << dr << std::endl;

    circle1 = Geom::Circle( c1 - dr*n1, r1-dr );
    circle2 = Geom::Circle( c2 + dr*n2, r2+dr );

    // std::cout << "    C1: " << circle1 << std::endl;
    // std::cout << "    C2: " << circle2 << std::endl;
    // std::cout << "    d': " << Geom::Point( circle1.center() - circle2.center() ).length() << std::endl;

    Geom::Line bisector( circle1.center(), circle2.center() );
    std::vector<Geom::ShapeIntersection> points = circle1.intersect( bisector );
    Geom::Point p0 = points[0].point();
    Geom::Point p1 = points[1].point();
    // std::cout << "    points: " << p0 << "; " << p1 << std::endl;
    if( std::abs( Geom::distance( p0, circle2.center() ) - circle2.radius() ) <
        std::abs( Geom::distance( p1, circle2.center() ) - circle2.radius() ) ) {
        return p0;
    } else {
        return p1;
    }
}

void extrapolate_join_internal(join_data const &jd, int alternative)
{
    // std::cout << "\nextrapolate_join: entrance: alternative: " << alternative << std::endl;
    using namespace Geom;

    Geom::Path &res = jd.res;
    Geom::Curve const& incoming = res.back();
    Geom::Curve const& outgoing = jd.outgoing.front();
    Geom::Point startPt = incoming.finalPoint();
    Geom::Point endPt = outgoing.initialPoint();
    Geom::Point tang1 = jd.in_tang;
    Geom::Point tang2 = jd.out_tang;
    // width is half stroke-width
    double width = jd.width, miter = jd.miter;

    // std::cout << "  startPt: " << startPt << "  endPt: " << endPt << std::endl;
    // std::cout << "  tang1: " << tang1 << "  tang2: " << tang2 <<  std::endl;
    // std::cout << "    dot product: " << Geom::dot( tang1, tang2 ) <<  std::endl;
    // std::cout << "  width: " << width << std::endl;

    // CIRCLE CALCULATION TESTING
    Geom::Circle circle1 = touching_circle(Geom::reverse(incoming.toSBasis()), 0.);
    Geom::Circle circle2 = touching_circle(outgoing.toSBasis(), 0);
    // std::cout << "  circle1: " << circle1 << std::endl;
    // std::cout << "  circle2: " << circle2 << std::endl;
    if( Geom::CubicBezier const * in_bezier = dynamic_cast<Geom::CubicBezier const*>(&incoming) ) {
        Geom::Circle circle_test1 = touching_circle(*in_bezier, false);
        if( !Geom::are_near( circle1, circle_test1, 0.01 ) ) {
            // std::cout << "  Circle 1 error!!!!!!!!!!!!!!!!!" << std::endl;
            // std::cout << "           " << circle_test1 << std::endl;
        }
    }
    if( Geom::CubicBezier const * out_bezier = dynamic_cast<Geom::CubicBezier const*>(&outgoing) ) {
        Geom::Circle circle_test2 = touching_circle(*out_bezier, true);
        if( !Geom::are_near( circle2, circle_test2, 0.01 ) ) {
            // std::cout << "  Circle 2 error!!!!!!!!!!!!!!!!!" << std::endl;
            // std::cout << "           " << circle_test2 << std::endl;
        }
    }
    // END TESTING

    Geom::Point center1 = circle1.center();
    double side1 = tang1[Geom::X]*(startPt[Geom::Y]-center1[Geom::Y]) - tang1[Geom::Y]*(startPt[Geom::X]-center1[Geom::X]);
    // std::cout << "  side1: " << side1 << std::endl;

    bool inc_ls = !circle1.center().isFinite();
    bool out_ls = !circle2.center().isFinite();

    std::vector<Geom::ShapeIntersection> points;

    Geom::EllipticalArc *arc1 = nullptr;
    Geom::EllipticalArc *arc2 = nullptr;
    Geom::LineSegment *seg1 = nullptr;
    Geom::LineSegment *seg2 = nullptr;
    Geom::Point sol;
    Geom::Point p1;
    Geom::Point p2;

    if (!inc_ls && !out_ls) {
        // std::cout << " two circles" << std::endl;

        // See if tangent is backwards (radius < width/2 and circle is inside stroke).
        Geom::Point node_on_path = startPt + Geom::rot90(tang1)*width;
        // std::cout << "  node_on_path: " << node_on_path << "  -------------- " << std::endl;
        bool b1 = false;
        bool b2 = false;
        if (circle1.radius() < width &&  distance( circle1.center(), node_on_path ) < width) {
            b1 = true;
        }
        if (circle2.radius() < width &&  distance( circle2.center(), node_on_path ) < width) {
            b2 = true;
        }
        // std::cout << "  b1: " << (b1?"true":"false")
        //           << "  b2: " << (b2?"true":"false") << std::endl;

        // Two circles
        points = circle1.intersect(circle2);

        if (points.size() != 2) {
            // std::cout << "   Circles do not intersect, do backup" << std::endl;
            switch (alternative) {
                case 1:
                {
                    // Fallback to round if one path has radius smaller than half line width.
                    if(b1 || b2) {
                        // std::cout << "At one least path has radius smaller than half line width." << std::endl;
                        return( round_join(jd) );
                    }

                    Point p;
                    if( circle2.contains( startPt ) && !circle1.contains( endPt ) ) {
                        // std::cout << "Expand circle1" << std::endl;
                        p = expand_circle( circle1, circle2, startPt, tang1 );
                        points.emplace_back( 0, 0, p );
                        points.emplace_back( 0, 0, p );
                    } else if( circle1.contains( endPt ) && !circle2.contains( startPt ) ) {
                        // std::cout << "Expand circle2" << std::endl;
                        p = expand_circle( circle2, circle1, endPt, tang2 );
                        points.emplace_back( 0, 0, p );
                        points.emplace_back( 0, 0, p );
                    } else {
                        // std::cout << "Either both points inside or both outside" << std::endl;
                        return( miter_clip_join(jd) );
                    }
                    break;
                    
                }
                case 2:
                {
                    // Fallback to round if one path has radius smaller than half line width.
                    if(b1 || b2) {
                        // std::cout << "At one least path has radius smaller than half line width." << std::endl;
                        return( round_join(jd) );
                    }

                    if( ( circle2.contains( startPt ) && !circle1.contains( endPt ) ) ||
                        ( circle1.contains( endPt ) && !circle2.contains( startPt ) ) ) {
                        
                        Geom::Point apex = adjust_circles( circle1, circle2, startPt, endPt, tang1, tang2 );
                        points.emplace_back( 0, 0, apex );
                        points.emplace_back( 0, 0, apex );
                    } else {
                        // std::cout << "Either both points inside or both outside" << std::endl;
                        return( miter_clip_join(jd) );
                    }
                        
                    break;
                }
                case 3:
                    if( side1 > 0 ) {
                        Geom::Line secant(startPt, startPt + tang1);
                        points = circle2.intersect(secant);
                        circle1.setRadius(std::numeric_limits<float>::infinity());
                        circle1.setCenter(Geom::Point(0,std::numeric_limits<float>::infinity()));
                    } else {
                        Geom::Line secant(endPt, endPt + tang2);
                        points = circle1.intersect(secant);
                        circle2.setRadius(std::numeric_limits<float>::infinity());
                        circle2.setCenter(Geom::Point(0,std::numeric_limits<float>::infinity()));
                    }
                    break;


                case 0:
                default:
                    // Do nothing
                    break;
            }
        }

        if (points.size() == 2) {
            sol = pick_solution(points, tang2, endPt);
            if( circle1.radius() != std::numeric_limits<float>::infinity() ) {
                arc1 = circle1.arc(startPt, 0.5*(startPt+sol), sol);
            } else {
                seg1 = new Geom::LineSegment(startPt, sol);
            }
            if( circle2.radius() != std::numeric_limits<float>::infinity() ) {
                arc2 = circle2.arc(sol, 0.5*(sol+endPt), endPt);
            } else {
                seg2 = new Geom::LineSegment(sol, endPt);
            }
        }
    } else if (inc_ls && !out_ls) {
        // Line and circle
        // std::cout << " line circle" << std::endl;
        points = circle2.intersect(Line(incoming.initialPoint(), incoming.finalPoint()));
        if (points.size() == 2) {
            sol = pick_solution(points, tang2, endPt);
            arc2 = circle2.arc(sol, 0.5*(sol+endPt), endPt);
        }
    } else if (!inc_ls && out_ls) {
        // Circle and line
        // std::cout << " circle line" << std::endl;
        points = circle1.intersect(Line(outgoing.initialPoint(), outgoing.finalPoint()));
        if (points.size() == 2) {
            sol = pick_solution(points, tang2, endPt);
            arc1 = circle1.arc(startPt, 0.5*(sol+startPt), sol);
        }
    }
    if (points.size() != 2) {
        // std::cout << " no solutions" << std::endl;
        // no solutions available, fall back to miter
        return miter_join(jd);
    }

    // We have a solution, thus sol is defined.
    p1 = sol;
    
    // See if we need to clip. Miter length is measured along a circular arc that is tangent to the
    // bisector of the incoming and out going angles and passes through the end point (sol) of the
    // line join.

    // Center of circle is intersection of a line orthogonal to bisector and a line bisecting
    // a chord connecting the path end point (point_on_path) and the join end point (sol).
    Geom::Point point_on_path = startPt + Geom::rot90(tang1)*width;
    Geom::Line bisector = make_angle_bisector_line(startPt, point_on_path, endPt);
    Geom::Line ortho = make_orthogonal_line(point_on_path, bisector); 

    Geom::LineSegment chord(point_on_path, sol);
    Geom::Line bisector_chord =  make_bisector_line(chord);

    Geom::Line limit_line;
    double miter_limit = width * miter;
    bool clipped = false;

    if (are_parallel(bisector_chord, ortho)) {
        // No intersection (can happen if curvatures are equal but opposite)
        if (Geom::distance(point_on_path, sol) > miter_limit) {
            clipped = true;
            Geom::Point temp = bisector.versor();
            Geom::Point limit_point = point_on_path + miter_limit * temp; 
            limit_line = make_parallel_line( limit_point, ortho );
        }
    } else {
        Geom::Point center =
            Geom::intersection_point( bisector_chord.pointAt(0), bisector_chord.versor(),
                                      ortho.pointAt(0),          ortho.versor() );
        Geom::Coord radius = distance(center, point_on_path);
        Geom::Circle circle_center(center, radius);

        double limit_angle = miter_limit / radius;

        Geom::Ray start_ray(center, point_on_path);
        Geom::Ray end_ray(center, sol);
        Geom::Line limit_line(center, 0); // Angle set below

        if (Geom::cross(start_ray.versor(), end_ray.versor()) < 0) {
            limit_line.setAngle(start_ray.angle() - limit_angle);
        } else {
            limit_line.setAngle(start_ray.angle() + limit_angle);
        }

        Geom::EllipticalArc *arc_center = circle_center.arc(point_on_path, 0.5*(point_on_path + sol), sol);
        if (arc_center && arc_center->sweepAngle() > limit_angle) {
            // We need to clip
            clipped = true;

            if (!inc_ls) {
                // Incoming circular
                points = circle1.intersect(limit_line);
                if (points.size() == 2) {
                    p1 = pick_solution(points, tang2, endPt);
                    delete arc1;
                    arc1 = circle1.arc(startPt, 0.5*(p1+startPt), p1);
                }
            } else {
                p1 = Geom::intersection_point(startPt, tang1, limit_line.pointAt(0), limit_line.versor());
            }

            if (!out_ls) {
                // Outgoing circular
                points = circle2.intersect(limit_line);
                if (points.size() == 2) {
                    p2 = pick_solution(points, tang1, endPt);
                    delete arc2;
                    arc2 = circle2.arc(p2, 0.5*(p2+endPt), endPt);
                }
            } else {
                p2 = Geom::intersection_point(endPt, tang2, limit_line.pointAt(0), limit_line.versor());
            }
        }
    }    

    // Add initial
    if (arc1) {
        res.append(*arc1);
    } else if (seg1 ) {
        res.append(*seg1);
    } else {
        // Straight line segment: move last point
        res.setFinal(p1);
    }

    if (clipped) {
        res.appendNew<Geom::LineSegment>(p2);
    }

    // Add outgoing
    if (arc2) {
        res.append(*arc2);
        res.append(outgoing);
    } else if (seg2 ) {
        res.append(*seg2);
        res.append(outgoing);
    } else {
        // Straight line segment:
        res.appendNew<Geom::LineSegment>(outgoing.finalPoint());
    }

    // add the rest of the path
    res.insert(res.end(), ++jd.outgoing.begin(), jd.outgoing.end());

    delete arc1;
    delete arc2;
    delete seg1;
    delete seg2;
}

void extrapolate_join(     join_data jd) { extrapolate_join_internal(jd, 0); }
void extrapolate_join_alt1(join_data jd) { extrapolate_join_internal(jd, 1); }
void extrapolate_join_alt2(join_data jd) { extrapolate_join_internal(jd, 2); }
void extrapolate_join_alt3(join_data jd) { extrapolate_join_internal(jd, 3); }


void tangents(Geom::Point tang[2], Geom::Curve const& incoming, Geom::Curve const& outgoing)
{
    Geom::Point tang1 = Geom::unitTangentAt(reverse(incoming.toSBasis()), 0.);
    Geom::Point tang2 = outgoing.unitTangentAt(0.);
    tang[0] = tang1, tang[1] = tang2;
}

// Offsetting a line segment is mathematically stable and quick to do
Geom::LineSegment offset_line(Geom::LineSegment const& l, double width)
{
    Geom::Point tang1 = Geom::rot90(l.unitTangentAt(0));
    Geom::Point tang2 = Geom::rot90(unitTangentAt(reverse(l.toSBasis()), 0.));

    Geom::Point start = l.initialPoint() + tang1 * width;
    Geom::Point end = l.finalPoint() - tang2 * width;
    
    return Geom::LineSegment(start, end);
}

void get_cubic_data(Geom::CubicBezier const& bez, double time, double& len, double& rad)
{
    // get derivatives
    std::vector<Geom::Point> derivs = bez.pointAndDerivatives(time, 3);

    Geom::Point der1 = derivs[1]; // first deriv (tangent vector)
    Geom::Point der2 = derivs[2]; // second deriv (tangent's tangent)
    double l = Geom::L2(der1); // length

    len = rad = 0;

    // TODO: we might want to consider using Geom::touching_circle to determine the
    // curvature radius here. Less code duplication, but slower

    if (Geom::are_near(l, 0, 1e-4)) {
        l = Geom::L2(der2) / 2;
        Geom::Point der3 = derivs.at(3); // try second time
        if (Geom::are_near(l, 0, 1e-4)) {
            l = Geom::L2(der3);
            if (Geom::are_near(l, 0)) {
                return; // this isn't a segment...
            }
        rad = 1e8;
        } else {
            rad = -l * (Geom::dot(der2, der2) / Geom::cross(der2, der3));
        }
    } else {
        rad = -l * (Geom::dot(der1, der1) / Geom::cross(der1, der2));
    }
    len = l;
}

double _offset_cubic_stable_sub(
        Geom::CubicBezier const& bez,
        Geom::CubicBezier& c,
        const Geom::Point& start_normal,
        const Geom::Point& end_normal,
        const Geom::Point& start_new,
        const Geom::Point& end_new,
        const double start_rad,
        const double end_rad,
        const double start_len,
        const double end_len,
        const double width,
        const double width_correction) {
    using Geom::X;
    using Geom::Y;

    double start_off = 1, end_off = 1;
    // correction of the lengths of the tangent to the offset
    // start_off / end_off can also be negative. This is intended and
    // is the case when *_radius is negative and its absolute value smaller then width.
    if (!Geom::are_near(start_rad, 0)) {
        start_off += width / start_rad;
    }
    if (!Geom::are_near(end_rad, 0)) {
        end_off += width / end_rad;
    }

    // the correction factor should not change the sign of the factors
    // as it is only a scaling heuristic to make the approximation better.
    const auto correction_factor = 1 + width_correction / width;
    if (correction_factor > 0) {
        start_off *= correction_factor;
        end_off *= correction_factor;
    }

    start_off *= start_len;
    end_off *= end_len;

    Geom::Point mid1_new = start_normal.ccw()*start_off;
    mid1_new = Geom::Point(start_new[X] + mid1_new[X]/3., start_new[Y] + mid1_new[Y]/3.);
    Geom::Point mid2_new = end_normal.ccw()*end_off;
    mid2_new = Geom::Point(end_new[X] - mid2_new[X]/3., end_new[Y] - mid2_new[Y]/3.);

    // create the estimate curve
    c = Geom::CubicBezier(start_new, mid1_new, mid2_new, end_new);

    // check the tolerance for our estimate to be a parallel curve
    // both directions have to be checked, as we are computing a hausdorff distance with offset
    double worst_residual = 0;
    const auto min_offset_difference = [width, &worst_residual](Geom::CubicBezier const &bez1,
                                                                Geom::CubicBezier const &bez2, const double time) {
        const Geom::Point requested_point = bez1.pointAt(time);
        const Geom::Point closest_point = bez2.pointAt(bez2.nearestTime(requested_point));
        const double current_residual = (requested_point - closest_point).length() - std::abs(width);
        if (std::abs(current_residual) > std::abs(worst_residual)) {
            worst_residual = current_residual;
        }
    };
    for (size_t ii = 3; ii <= 7; ii += 2) {
        const double t = static_cast<double>(ii) / 10;

        min_offset_difference(bez, c, t);
        min_offset_difference(c, bez, t);
    }
    return worst_residual;
}

void offset_cubic(Geom::Path& p, Geom::CubicBezier const& bez, double width, double tol, size_t levels)
{
    using Geom::X;
    using Geom::Y;

    const Geom::Point start_pos = bez.initialPoint();
    const Geom::Point end_pos = bez.finalPoint();

    const Geom::Point start_normal = Geom::rot90(bez.unitTangentAt(0));
    const Geom::Point end_normal = -Geom::rot90(Geom::unitTangentAt(Geom::reverse(bez.toSBasis()), 0.));

    // offset the start and end control points out by the width
    const Geom::Point start_new = start_pos + start_normal*width;
    const Geom::Point end_new = end_pos + end_normal*width;

    // --------
    double start_rad, end_rad;
    double start_len, end_len; // tangent lengths
    get_cubic_data(bez, 0, start_len, start_rad);
    get_cubic_data(bez, 1, end_len, end_rad);


    Geom::CubicBezier c;

    double best_width_correction = 0;
    double best_residual = _offset_cubic_stable_sub(
                bez, c,
                start_normal, end_normal,
                start_new, end_new,
                start_rad, end_rad,
                start_len, end_len,
                width, best_width_correction);
    double stepsize = std::abs(width)/2;
    bool seen_success = false;
    double stepsize_threshold = 0;
    // std::cout << "Residual from " << best_residual << " ";
    size_t ii = 0;
    for (; ii < 100 && stepsize > stepsize_threshold; ++ii) {
        bool success = false;
        const double width_correction = best_width_correction - (best_residual > 0 ? 1 : -1) * stepsize;
        Geom::CubicBezier current_curve;
        const double residual = _offset_cubic_stable_sub(
                    bez, current_curve,
                    start_normal, end_normal,
                    start_new, end_new,
                    start_rad, end_rad,
                    start_len, end_len,
                    width, width_correction);
        if (std::abs(residual) < std::abs(best_residual)) {
            best_residual = residual;
            best_width_correction = width_correction;
            c = current_curve;
            success = true;
            if (std::abs(best_residual) < tol/4) {
                break;
            }
        }

        if (success) {
            if (!seen_success) {
                seen_success = true;
                //std::cout << "Stepsize factor: " << std::abs(width) / stepsize << std::endl;
                stepsize_threshold = stepsize / 1000;
            }
        }
        else {
            stepsize /= 2;
        }
        if (std::abs(best_width_correction) >= std::abs(width)/2) {
            //break; // Seems to prevent some numerical instabilities, not clear if useful
        }
    }

    // reached maximum recursive depth
    // don't bother with any more correction
    if (levels == 0) {
        try {
            p.append(c);
        }
        catch (...) {
            if ((p.finalPoint() - c.initialPoint()).length() < 1e-6) {
                c.setInitial(p.finalPoint());
            }
            else {
                auto line = Geom::LineSegment(p.finalPoint(), c.initialPoint());
                p.append(line);
            }
            p.append(c);
        }

        return;
    }

    // We find the point on (bez) for which the distance between
    // (c) and (bez) differs the most from the desired distance (width).
    // both directions have to be checked, as we are computing a hausdorff distance with offset
    double worst_err = std::abs(best_residual);
    double worst_time = .5;
    const auto min_offset_difference = [width, &worst_err, &worst_time](Geom::CubicBezier const &bez1,
                                                                        Geom::CubicBezier const &bez2,
                                                                        const double time) {
        const Geom::Point requested_point = bez1.pointAt(time);
        const Geom::Point closest_point = bez2.pointAt(bez2.nearestTime(requested_point));
        const double current_residual = std::abs((requested_point - closest_point).length() - std::abs(width));
        if (current_residual > worst_err) {
            worst_err = current_residual;
            worst_time = time;
        }
    };
    for (size_t ii = 1; ii <= 9; ++ii) {
        const double t = static_cast<double>(ii) / 10;

        min_offset_difference(bez, c, t);
        min_offset_difference(c, bez, t);
    }

    if (worst_err < tol) {
        if (Geom::are_near(start_new, p.finalPoint())) {
            p.setFinal(start_new); // if it isn't near, we throw
        }

        // we're good, curve is accurate enough
        p.append(c);
        return;
    } else {
        // split the curve in two
        std::pair<Geom::CubicBezier, Geom::CubicBezier> s = bez.subdivide(worst_time);
        offset_cubic(p, s.first, width, tol, levels - 1);
        offset_cubic(p, s.second, width, tol, levels - 1);
    }
}

void offset_quadratic(Geom::Path& p, Geom::QuadraticBezier const& bez, double width, double tol, size_t levels)
{
    // cheat
    // it's faster
    // seriously
    std::vector<Geom::Point> points = bez.controlPoints();
    Geom::Point b1 = points[0] + (2./3) * (points[1] - points[0]);
    Geom::Point b2 = b1 + (1./3) * (points[2] - points[0]);
    Geom::CubicBezier cub = Geom::CubicBezier(points[0], b1, b2, points[2]);
    offset_cubic(p, cub, width, tol, levels);
}

void offset_curve(Geom::Path& res, Geom::Curve const* current, double width, double tolerance)
{
    size_t levels = 8;

    if (current->isDegenerate()) return; // don't do anything

    // TODO: we can handle SVGEllipticalArc here as well, do that!

    if (Geom::BezierCurve const *b = dynamic_cast<Geom::BezierCurve const*>(current)) {
        size_t order = b->order();
        switch (order) {
            case 1:
                res.append(offset_line(static_cast<Geom::LineSegment const&>(*current), width));
                break;
            case 2: {
                Geom::QuadraticBezier const& q = static_cast<Geom::QuadraticBezier const&>(*current);
                offset_quadratic(res, q, width, tolerance, levels);
                break;
            }
            case 3: {
                Geom::CubicBezier const& cb = static_cast<Geom::CubicBezier const&>(*current);
                offset_cubic(res, cb, width, tolerance, levels);
                break;
            }
            default: {
                Geom::Path sbasis_path = Geom::cubicbezierpath_from_sbasis(current->toSBasis(), tolerance);
                for (const auto & i : sbasis_path)
                    offset_curve(res, &i, width, tolerance);
                break;
            }
        }
    } else {
        Geom::Path sbasis_path = Geom::cubicbezierpath_from_sbasis(current->toSBasis(), 0.1);
        for (const auto & i : sbasis_path)
            offset_curve(res, &i, width, tolerance);
    }
}

typedef void cap_func(Geom::PathBuilder& res, Geom::Path const& with_dir, Geom::Path const& against_dir, double width);

void flat_cap(Geom::PathBuilder& res, Geom::Path const&, Geom::Path const& against_dir, double)
{
    res.lineTo(against_dir.initialPoint());
}

void round_cap(Geom::PathBuilder& res, Geom::Path const&, Geom::Path const& against_dir, double width)
{
    res.arcTo(width / 2., width / 2., 0., true, false, against_dir.initialPoint());
}

void square_cap(Geom::PathBuilder& res, Geom::Path const& with_dir, Geom::Path const& against_dir, double width)
{
    width /= 2.;
    Geom::Point normal_1 = -Geom::unitTangentAt(Geom::reverse(with_dir.back().toSBasis()), 0.);
    Geom::Point normal_2 = -against_dir[0].unitTangentAt(0.);
    res.lineTo(with_dir.finalPoint() + normal_1*width);
    res.lineTo(against_dir.initialPoint() + normal_2*width);
    res.lineTo(against_dir.initialPoint());
}

void peak_cap(Geom::PathBuilder& res, Geom::Path const& with_dir, Geom::Path const& against_dir, double width)
{
    width /= 2.;
    Geom::Point normal_1 = -Geom::unitTangentAt(Geom::reverse(with_dir.back().toSBasis()), 0.);
    Geom::Point normal_2 = -against_dir[0].unitTangentAt(0.);
    Geom::Point midpoint = ((with_dir.finalPoint() + normal_1*width) + (against_dir.initialPoint() + normal_2*width)) * 0.5;
    res.lineTo(midpoint);
    res.lineTo(against_dir.initialPoint());
}

} // namespace

namespace Inkscape {

Geom::PathVector outline(
        Geom::Path const& input,
        double width,
        double miter,
        LineJoinType join,
        LineCapType butt,
        double tolerance)
{
    if (input.size() == 0) return Geom::PathVector(); // nope, don't even try

    Geom::PathBuilder res;
    Geom::Path with_dir = half_outline(input, width/2., miter, join, tolerance);
    Geom::Path against_dir = half_outline(input.reversed(), width/2., miter, join, tolerance);
    res.moveTo(with_dir[0].initialPoint());
    res.append(with_dir);

    cap_func *cf;
    switch (butt) {
        case BUTT_ROUND:
            cf = &round_cap;
            break;
        case BUTT_SQUARE:
            cf = &square_cap;
            break;
        case BUTT_PEAK:
            cf = &peak_cap;
            break;
        default:
            cf = &flat_cap;
    }

    // glue caps
    if (!input.closed()) {
        cf(res, with_dir, against_dir, width);
    } else {
        res.closePath();
        res.moveTo(against_dir.initialPoint());
    }

    res.append(against_dir);

    if (!input.closed()) {
        cf(res, against_dir, with_dir, width);
    }

    res.closePath();
    res.flush();
    return res.peek();
}

Geom::Path half_outline(
        Geom::Path const& input,
        double width,
        double miter,
        LineJoinType join,
        double tolerance)
{
    if (tolerance <= 0) {
        if (std::abs(width) > 0) {
            tolerance = std::abs(width) / 100;
        }
        else {
            tolerance = 1e-4;
        }
    }
    Geom::Path res;
    if (input.size() == 0) return res;

    Geom::Point tang1 = input[0].unitTangentAt(0);
    Geom::Point start = input.initialPoint() + tang1 * width;
    Geom::Path temp;
    Geom::Point tang[2];

    res.setStitching(true);
    temp.setStitching(true);

    res.start(start);

    // Do two curves at a time for efficiency, since the join function needs to know the outgoing curve as well
    const Geom::Curve &closingline = input.back_closed();
    const size_t k = (are_near(closingline.initialPoint(), closingline.finalPoint()) && input.closed() )
            ?input.size_open():input.size_default();
    
    for (size_t u = 0; u < k; u += 2) {
        temp.clear();

        offset_curve(temp, &input[u], width, tolerance);

        // on the first run through, there isn't a join
        if (u == 0) {
            res.append(temp);
        } else {
            tangents(tang, input[u-1], input[u]);
            outline_join(res, temp, tang[0], tang[1], width, miter, join);
        }

        // odd number of paths
        if (u < k - 1) {
            temp.clear();
            offset_curve(temp, &input[u+1], width, tolerance);
            tangents(tang, input[u], input[u+1]);
            outline_join(res, temp, tang[0], tang[1], width, miter, join);
        }
    }
    if (input.closed()) {
        Geom::Curve const &c1 = res.back();
        Geom::Curve const &c2 = res.front();
        temp.clear();
        temp.append(c1);
        Geom::Path temp2;
        temp2.append(c2);
        tangents(tang, input.back(), input.front());
        outline_join(temp, temp2, tang[0], tang[1], width, miter, join);
        res.erase(res.begin());
        res.erase_last();
        res.append(temp);
        res.close();
    }
    return res;
}

void outline_join(Geom::Path &res, Geom::Path const& temp, Geom::Point in_tang, Geom::Point out_tang, double width, double miter, Inkscape::LineJoinType join)
{
    if (res.size() == 0 || temp.size() == 0)
        return;
    Geom::Curve const& outgoing = temp.front();
    if (Geom::are_near(res.finalPoint(), outgoing.initialPoint(), 0.01)) {
        // if the points are /that/ close, just ignore this one
        res.setFinal(temp.initialPoint());
        res.append(temp);
        return;
    }

    join_data jd(res, temp, in_tang, out_tang, miter, width);
    if (!(Geom::cross(in_tang, out_tang) > 0)) {
        join = Inkscape::JOIN_BEVEL;
    }
    join_func *jf;
    switch (join) {
        case Inkscape::JOIN_BEVEL:
            jf = &bevel_join;
            break;
        case Inkscape::JOIN_ROUND:
            jf = &round_join;
            break;
        case Inkscape::JOIN_EXTRAPOLATE:
            jf = &extrapolate_join;
            break;
        case Inkscape::JOIN_EXTRAPOLATE1:
            jf = &extrapolate_join_alt1;
            break;
        case Inkscape::JOIN_EXTRAPOLATE2:
            jf = &extrapolate_join_alt2;
            break;
        case Inkscape::JOIN_EXTRAPOLATE3:
            jf = &extrapolate_join_alt3;
            break;
        case Inkscape::JOIN_MITER_CLIP:
            jf = &miter_clip_join;
            break;
        default:
            jf = &miter_join;
    }
    jf(jd);
}

/**
 * Check for an empty path.
 */
bool is_path_empty(Geom::Path const &path)
{
    double area;
    Geom::Point pt;
    Geom::centroid(path.toPwSb(), pt, area);
    return std::abs(area) < 1e-3;
}

/// Attempts to find a point in the interior of a filled path.
static std::optional<Geom::Point> find_interior_point(Geom::Path const &path, FillRule fill_rule, std::default_random_engine &gen)
{
    auto ran = [&] { return std::uniform_real_distribution()(gen); };

    auto const bounds = path.boundsFast();
    if (!bounds) {
        return {};
    }

    constexpr int max_iterations = 10; // arbitrary cutoff, typically one iteration needed
    for (int i = 0; i < max_iterations; i++) {
        // Pick a random horizontal line through the path.
        auto const y = Geom::lerp(ran(), bounds->top(), bounds->bottom());
        auto const line = Geom::LineSegment{Geom::Point{bounds->left() - bounds->width(), y}, Geom::Point{bounds->right() + bounds->width(), y}};

        // Intersect the line with the path, recording the intersection times on the line.
        std::vector<double> times;
        for (auto const &curve : path) {
            for (auto const &intersection : line.intersect(curve)) {
                times.emplace_back(intersection.first);
            }
        }

        // Interior-test the points exactly halfway between intersections.
        for (int j = 1; j < times.size(); j++) {
            auto const t = (times[j - 1] + times[j]) / 2;
            auto const p = line.pointAt(t);
            if (is_point_inside(fill_rule, path.winding(p))) {
                return p;
            }
        }
    }

    return {};
}

namespace {

/**
 * Given a pathvector @a pathv and fill rule @a fill_rule, compute pathv_fully_contains(pathv[i], pathv[j], fill_rule)
 * for all possible i != j. This class must be used with the Geom::Sweeper API to actually compute results. The results
 * are then available by calling contains(i, j);
 */
class PathContainmentSweeper
{
public:
    using ItemIterator = Geom::PathVector::const_iterator;

    PathContainmentSweeper(Geom::PathVector const &pathv, FillRule fill_rule, double precision = Geom::EPSILON)
        : _pathv{pathv}
        , _fill_rule{fill_rule}
        , _precision{precision}
        , _contains(_pathv.size() * _pathv.size(), false) // allocate space for two-dimensional array
    {}

    Geom::PathVector const &items() const { return _pathv; }

    Geom::Interval itemBounds(ItemIterator path) const
    {
        auto const r = path->boundsFast();
        return r ? (*r)[Geom::X] : Geom::Interval{};
    }

    void addActiveItem(ItemIterator incoming)
    {
        for (auto const &path : _active) {
            _checkPair(path, incoming);
            _checkPair(incoming, path);
        }
        _active.push_back(incoming);
    }

    void removeActiveItem(ItemIterator to_remove)
    {
        auto const it = std::find(_active.begin(), _active.end(), to_remove);
        std::swap(*it, _active.back());
        _active.pop_back();
    }

    //// Return the value of pathv_fully_contains(pathv[i], pathv[j], fill_rule).
    bool contains(int i, int j) const { return _contains[index(i, j)]; }

private:
    Geom::PathVector const &_pathv;
    FillRule const _fill_rule;
    double const _precision;

    std::vector<ItemIterator> _active;
    std::vector<bool> _contains;

    int index(int i, int j) const { return i * _pathv.size() + j; }

    void _checkPair(ItemIterator a, ItemIterator b)
    {
        if (pathv_fully_contains(*a, *b, _fill_rule)) {
            auto const ia = std::distance(_pathv.begin(), a);
            auto const ib = std::distance(_pathv.begin(), b);
            _contains[index(ia, ib)] = true;
        }
    }
};

/**
 * Given a pathvector and a tree structure on its subpaths representing how they are nested,
 * split the pathvector into its connected components when filled with the given fill rule.
 */
class PathContainmentTraverser
{
public:
    PathContainmentTraverser(Geom::PathVector &paths, Util::TreeifyResult const &tree, FillRule fill_rule)
        : _paths{paths}
        , _tree{tree}
        , _fill_rule{fill_rule}
    {
        while (_pos < tree.preorder.size()) {
            _visit(nullptr, 0, nullptr);
        }
    }

    std::vector<Geom::PathVector> &&moveResult() { return std::move(_result); }

private:
    // Input
    Geom::PathVector &_paths;
    Util::TreeifyResult const &_tree;
    FillRule const _fill_rule;

    // State
    std::default_random_engine _gen{std::random_device()()};
    int _pos = 0;

    // Output
    std::vector<Geom::PathVector> _result;

    void _visit(Geom::Path const *parent, int winding, Geom::PathVector *component)
    {
        // Visit the next node in the preorder traversal.
        int const x = _tree.preorder[_pos];
        _pos++;

        // Determine winding number at paths[x] of its parents in the tree.
        // Skip the computation for fill_justDont, as it would be unused.
        if (parent && _fill_rule != fill_justDont) {
            if (auto const p = find_interior_point(_paths[x], _fill_rule, _gen)) {
                winding += parent->winding(*p);
            }
        }

        // Determine if paths[x] is inside a hole. If so, we start a new component.
        bool const boundary = !is_point_inside(_fill_rule, winding);

        std::optional<Geom::PathVector> pathv;
        if (boundary) {
            pathv.emplace();
            component = &*pathv;
        }

        assert(component);
        component->push_back(std::move(_paths[x]));

        for (int i = 0; i < _tree.num_children[x]; i++) {
            _visit(&_paths[x], winding, component);
        }

        if (boundary) {
            _result.emplace_back(std::move(*pathv));
        }
    }
};

} // namespace

std::vector<Geom::PathVector> split_non_intersecting_paths(Geom::PathVector &&paths, FillRule fill_rule)
{
    auto path_containment = PathContainmentSweeper{paths, fill_nonZero};
    Geom::Sweeper{path_containment}.process();

    auto const tree = Util::treeify(paths.size(), [&] (int i, int j) {
        return path_containment.contains(i, j);
    });

    return PathContainmentTraverser(paths, tree, fill_rule).moveResult();
}

Geom::PathVector 
do_offset(Geom::PathVector const & path_in
        , double to_offset
        , double tolerance
        , double miter_limit
        , FillRule fillrule
        , Inkscape::LineJoinType join
        , Geom::Point point // knot on LPE
        , Geom::PathVector &helper_path
        , Geom::PathVector &mix_pathv_all)
{
    Geom::PathVector ret_closed;
    Geom::PathVector ret_open;
    Geom::PathVector open_pathv;
    Geom::PathVector closed_pathv;
    Geom::PathVector orig_pathv = pathv_to_linear_and_cubic_beziers(path_in);
    Geom::PathVector outline; // return path
    helper_path.insert(helper_path.end(), orig_pathv.begin(), orig_pathv.end());
    // Store separated open/closed paths
    for (auto &i : orig_pathv) {
        // this improve offset in near closed paths
        if (Geom::are_near(i.initialPoint(), i.finalPoint())) {
            i.close(true);
        }
        if (i.closed()) {
            closed_pathv.push_back(i);
        } else {
            open_pathv.push_back(i);
        }
    }
    // flatten order the direcions and remove self intersections
    // we use user fill rule to match original view
    // after flatten all elements has the same direction in his widding
    flatten(closed_pathv, fillrule);
    if (Geom::are_near(to_offset,0.0)) {
        // this is to keep reference to multiple pathvectors like in a group. Used by knot position in LPE Offset
        mix_pathv_all.insert(mix_pathv_all.end(), path_in.begin(), path_in.end());
        closed_pathv.insert(closed_pathv.end(), open_pathv.begin(), open_pathv.end());
        return closed_pathv;
    }
    if (to_offset < 0) {
        Geom::OptRect bbox = closed_pathv.boundsFast();
        if (bbox) {
            (*bbox).expandBy(to_offset / 2.0);
            if ((*bbox).hasZeroArea()) {
                closed_pathv.clear();
            }
        }
    }
    // this is to keep reference to multiple pathvectors like in a group. Used by knot position in LPE Offset
    mix_pathv_all.insert(mix_pathv_all.end(), closed_pathv.begin(), closed_pathv.end());
    Geom::PathVector outline_tmp; // full outline to operate
    double gap = to_offset > 0 ? 0 : 0.01;
    for (auto &input : closed_pathv) {
        // input dir is 1 on fill and 0 in holes. garanteed by flatten
        bool dir = Geom::path_direction(input);
        Geom::Path with_dir = half_outline(input, std::abs(to_offset) + gap, miter_limit, join, tolerance);
        if (to_offset > 0) {
            //we remove artifacts in a manual way not the best way but there is no other way without a clean offset line
            if (!dir) {
                auto bbox = input.boundsFast();
                if (bbox) {
                    double sizei = std::min((*bbox).width(),(*bbox).height());
                    if (sizei > to_offset * 2) {
                        outline_tmp.push_back(with_dir);
                    }
                }
            } else {
                auto with_dir_pv = Geom::PathVector(with_dir);
                flatten(with_dir_pv, fill_positive);
                for (auto path : with_dir_pv) {
                    auto bbox = path.boundsFast();
                    if (bbox) {
                        double sizei = std::min((*bbox).width(),(*bbox).height());
                        if (sizei > to_offset * 2) {
                            outline_tmp.push_back(path);
                        }
                    }
                }
            }
        } else {
            Geom::Path against_dir = half_outline(input.reversed(), std::abs(to_offset) + gap, miter_limit, join, tolerance);
            outline_tmp.push_back(with_dir);
            outline_tmp.push_back(against_dir);
            outline.push_back(input);
        }
    }
    if (!closed_pathv.empty()) {
        if (to_offset > 0) {
            outline.insert(outline.end(), outline_tmp.begin(), outline_tmp.end());
            // this make a union propely without calling boolops
            flatten(outline, fill_positive);
        } else {
            // this flatten in a fill_positive way that allow us erase it from the otiginal outline alwais (smaller)
            flatten(outline_tmp, fill_positive);
            // this can produce small satellites that become removed after new offset impletation work in 1.4
            outline = sp_pathvector_boolop(outline_tmp, outline, bool_op_diff, fill_nonZero, fill_nonZero);
        }
    }
    // this is to keep reference to multiple pathvectors like in a group. Used by knot position in LPE Offset
    mix_pathv_all.insert(mix_pathv_all.end(), open_pathv.begin(), open_pathv.end());
    for (auto &i : open_pathv) {
        Geom::Path tmp_a = half_outline(i, to_offset,  miter_limit, join, tolerance);
        if (point != Geom::Point(Geom::infinity(), Geom::infinity())) {
            Geom::Path tmp_b = half_outline(i.reversed(), to_offset,  miter_limit, join, tolerance);
            double distance_b = Geom::distance(point, tmp_a.pointAt(tmp_a.nearestTime(point)));
            double distance_a = Geom::distance(point, tmp_b.pointAt(tmp_b.nearestTime(point)));
            if (distance_b < distance_a) {
                outline.push_back(tmp_a);
            } else {
                outline.push_back(tmp_b);
            }
        } else {
            outline.push_back(tmp_a);
        }
    }
    return outline;
}

Geom::PathVector 
do_offset(Geom::PathVector const & path_in
        , double to_offset
        , double tolerance
        , double miter_limit
        , FillRule fillrule
        , Inkscape::LineJoinType join)
{
    Geom::PathVector not_used;
    return do_offset(path_in, to_offset, tolerance, miter_limit, fillrule, join, Geom::Point(Geom::infinity(), Geom::infinity()), not_used, not_used);
}

} // namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :

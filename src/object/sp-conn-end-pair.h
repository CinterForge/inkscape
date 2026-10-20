// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_CONN_END_PAIR
#define SEEN_SP_CONN_END_PAIR

/*
 * A class for handling connector endpoint movement and libavoid interaction.
 *
 * Authors:
 *   Peter Moulder <pmoulder@mail.csse.monash.edu.au>
 *
 *    * Copyright (C) 2004 Monash University
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <sigc++/sigc++.h>
#include <2geom/pathvector.h>

#include "attributes.h"

class SPConnEnd;
class SPPath;
class SPItem;
class SPObject;

namespace Avoid { class ConnRef; }
namespace Geom { class Point; }
namespace Inkscape::XML { class Node; }

class SPConnEndPair
{
public:
    explicit SPConnEndPair(SPPath *);
    ~SPConnEndPair();

    void release();
    void setAttr(const SPAttr key, char const *const value);
    void writeRepr(Inkscape::XML::Node *const repr) const;
    void getAttachedItems(SPItem *[2]) const;
    void getEndpoints(Geom::Point endPts[]) const;
    double getCurvature() const;
    SPConnEnd **getConnEnds();
    bool isOrthogonal() const;
    static Geom::PathVector createCurve(Avoid::ConnRef *connRef, double curvature);
    void tellLibavoidNewEndpoints(bool const processTransaction = false);
    bool reroutePathFromLibavoid();
    void makePathInvalid();
    void update();
    bool isAutoRoutingConn() const;
    void rerouteFromManipulation();

private:
    void _updateEndPoints();

    SPConnEnd *_connEnd[2];

    SPPath *_path;

    // libavoid's internal representation of the item.
    Avoid::ConnRef *_connRef;

    int _connType;
    double _connCurvature;

    // A sigc connection for transformed signal.
    sigc::connection _transformed_connection;
};


void sp_conn_end_pair_build(SPObject *object);


// _connType options:
enum {
    SP_CONNECTOR_NOAVOID,     // Basic connector - a straight line.
    SP_CONNECTOR_POLYLINE,    // Object avoiding polyline.
    SP_CONNECTOR_ORTHOGONAL   // Object avoiding orthogonal polyline (only horizontal and vertical segments).
};

#endif /* !SEEN_SP_CONN_END_PAIR */

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

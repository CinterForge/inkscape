// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Tavmjong Bah <tavmjong@free.fr>
 *
 * Copyrigt  (C) 2012 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_SP_MESH_ARRAY_H
#define SEEN_SP_MESH_ARRAY_H

/**
   A group of classes and functions for manipulating mesh gradients.

   A mesh is made up of an array of patches. Each patch has four sides and four corners. The sides can
   be shared between two patches and the corners between up to four.

   The order of the points for each side always goes from left to right or top to bottom.
   For sides 2 and 3 the points must be reversed when used (as in calls to cairo functions). 

   Two patches: (C=corner, S=side, H=handle, T=tensor)

                         C0   H1  H2 C1 C0 H1  H2  C1
                          + ---------- + ---------- +
                          |     S0     |     S0     |
                       H1 |  T0    T1  |H1 T0   T1  | H1
                          |S3        S1|S3        S1|
                       H2 |  T3    T2  |H2 T3   T2  | H2
                          |     S2     |     S2     |
                          + ---------- + ---------- +
                         C3   H1  H2 C2 C3 H1  H2   C2

   The mesh is stored internally as an array of nodes that includes the tensor nodes.

   Note: This code uses tensor points which are not part of the SVG2 plan at the moment.
   Including tensor points was motivated by a desire to experiment with their usefulness
   in smoothing color transitions. There doesn't seem to be much advantage for that
   purpose. However including them internally allows for storing all the points in
   an array which simplifies things like inserting new rows or columns.
*/

#include <memory>
#include <vector>
#include <2geom/point.h>

#include "colors/color.h"
// For color picking
#include "sp-item.h"

enum SPMeshType {
  SP_MESH_TYPE_COONS,
  SP_MESH_TYPE_BICUBIC
};

enum SPMeshGeometry {
  SP_MESH_GEOMETRY_NORMAL,
  SP_MESH_GEOMETRY_CONICAL
};

enum NodeType {
  MG_NODE_TYPE_UNKNOWN,
  MG_NODE_TYPE_CORNER,
  MG_NODE_TYPE_HANDLE,
  MG_NODE_TYPE_TENSOR
};

// Is a node along an edge?
enum NodeEdge {
  MG_NODE_EDGE_NONE,
  MG_NODE_EDGE_TOP = 1,
  MG_NODE_EDGE_LEFT = 2,
  MG_NODE_EDGE_BOTTOM = 4,
  MG_NODE_EDGE_RIGHT = 8
};

enum MeshCornerOperation {
  MG_CORNER_SIDE_TOGGLE,
  MG_CORNER_SIDE_ARC,
  MG_CORNER_TENSOR_TOGGLE,
  MG_CORNER_COLOR_SMOOTH,
  MG_CORNER_COLOR_PICK,
  MG_CORNER_INSERT
};

enum MeshNodeOperation {
  MG_NODE_NO_SCALE,
  MG_NODE_SCALE,
  MG_NODE_SCALE_HANDLE
};

class SPStop;

class SPMeshNode {
public:
  SPMeshNode() = default;

  NodeType node_type = MG_NODE_TYPE_UNKNOWN;
  unsigned node_edge = MG_NODE_EDGE_NONE;
  bool set = false;
  Geom::Point p;
  unsigned draggable = -1;  // index of on-screen node
  char path_type = 'u';
  std::optional<Inkscape::Colors::Color> color;
  SPStop *stop = nullptr; // Stop corresponding to node.
};

// I for Internal to distinguish it from the Object class
// This is a convenience class...
class SPMeshPatchI {
private:
  std::vector<std::vector< SPMeshNode* > > *nodes;
  int row;
  int col;

public:
  SPMeshPatchI( std::vector<std::vector< SPMeshNode* > > *n, int r, int c );
  Geom::Point getPoint( unsigned side, unsigned point );
  std::vector< Geom::Point > getPointsForSide( unsigned i );
  void        setPoint( unsigned side, unsigned point, Geom::Point const &p, bool set = true );
  char getPathType( unsigned i );
  void  setPathType( unsigned, char t );
  Geom::Point getTensorPoint( unsigned i );
  void        setTensorPoint( unsigned i, Geom::Point const &p );
  bool tensorIsSet();
  bool tensorIsSet( unsigned i );
  Geom::Point coonsTensorPoint( unsigned i );
  void    updateNodes();
  std::optional<Inkscape::Colors::Color> getColor(unsigned i);
  void    setColor(unsigned i, Inkscape::Colors::Color const &c);
  SPStop* getStopPtr( unsigned i );
  void    setStopPtr( unsigned i, SPStop* );
};

class SPMeshGradient;

// An array of mesh nodes.
class SPMeshNodeArray {
// Should be private
public:
  std::vector< std::vector< SPMeshNode* > > nodes;

public:
  // Draggables to nodes.
  std::vector< SPMeshNode* > corners;
  std::vector< SPMeshNode* > handles;
  std::vector< SPMeshNode* > tensors;

  friend class SPMeshPatchI;

  SPMeshNodeArray() { built = false; };
  SPMeshNodeArray( SPMeshGradient *mg );
  SPMeshNodeArray( const SPMeshNodeArray& rhs );
  SPMeshNodeArray& operator=(const SPMeshNodeArray& rhs);

  ~SPMeshNodeArray() { clear(); };
  bool built;

  void update_node_vectors();

  bool read( SPMeshGradient const *mg );
  void write( SPMeshGradient *mg );
  void create( SPMeshGradient *mg, SPItem *item, Geom::OptRect bbox );
  void clear();
  void print();

  // Fill 'smooth' with a smoothed version by subdividing each patch.
  void bicubic( SPMeshNodeArray* smooth, SPMeshType type);

  // Get size of patch
  unsigned patch_rows();
  unsigned patch_columns();

  SPMeshNode * node( unsigned i, unsigned j ) { return nodes[i][j]; }

  // Operations on corners
  bool adjacent_corners(unsigned i, unsigned j, SPMeshNode *n[4]);
  unsigned side_toggle  (std::vector<unsigned> const &);
  unsigned side_arc     (std::vector<unsigned> const &);
  unsigned tensor_toggle(std::vector<unsigned> const &);
  unsigned color_smooth (std::vector<unsigned> const &);
  unsigned color_pick   (std::vector<unsigned> const &, SPGradient *, SPItem *);
  unsigned insert       (std::vector<unsigned> const &);

  // Update other nodes in response to a node move.
  void update_handles(unsigned corner,
                      std::vector<unsigned> const &selected_corners,
                      Geom::Point const &old_p, MeshNodeOperation op);

  // Return outline path
  Geom::PathVector outline_path() const;

  // Transform array
  void transform(Geom::Affine const &m);

  // Transform mesh to fill box. Return true if not identity transform.
  bool fill_box(SPMeshGradient *mg, Geom::OptRect &box);

  // Find bounding box
  // Geom::OptRect findBoundingBox();

  void split_row( unsigned i, unsigned n );
  void split_column( unsigned j, unsigned n );
  void split_row( unsigned i, double coord );
  void split_column( unsigned j, double coord );
};

#endif /* !SEEN_SP_MESH_ARRAY_H */

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  c-basic-offset:2
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

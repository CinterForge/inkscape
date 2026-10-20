// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * KnotHolder - Hold SPKnot list and manage signals
 *
 * Author:
 *   Mitsuru Oka <oka326@parkcity.ne.jp>
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *
 * Copyright (C) 1999-2001 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2001 Mitsuru Oka
 * Copyright (C) 2008 Maximilian Albert
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 *
 */

#include <list>
#include <sigc++/scoped_connection.h>
#include <2geom/affine.h>

#ifndef SEEN_SP_KNOTHOLDER_H
#define SEEN_SP_KNOTHOLDER_H

namespace Geom {
class Point;
}

namespace Inkscape {

namespace UI {
class ShapeEditor;
} // namespace UI

namespace XML {
class Node;
} // namespace XML

namespace LivePathEffect {
class PowerStrokePointArrayParamKnotHolderEntity;
class NodeSatelliteArrayParam;
class FilletChamferKnotHolderEntity;
} // namespace LivePathEffect

} // namespace Inkscape

class KnotHolderEntity;
class SPItem;
class SPDesktop;
class SPKnot;

class KnotHolder {
public:
    KnotHolder(SPDesktop *desktop, SPItem *item);
    virtual ~KnotHolder();

    KnotHolder() = delete; // declared but not defined

    void update_knots();
    void unselect_knots();
    void knot_mousedown_handler(SPKnot *knot, unsigned int state);
    void knot_moved_handler(SPKnot *knot, Geom::Point const &p, unsigned int state);
    void knot_clicked_handler(SPKnot *knot, unsigned int state);
    void knot_grabbed_handler(SPKnot *knot, unsigned state);
    void knot_ungrabbed_handler(SPKnot *knot, unsigned int state);
    void transform_selected(Geom::Affine transform);
    void add(KnotHolderEntity *e);
    void remove(KnotHolderEntity *e);
    void add_pattern_knotholder();
    void add_hatch_knotholder();
    void add_filter_knotholder();
    void clear();

    void setEditTransform(Geom::Affine edit_transform);
    Geom::Affine getEditTransform() const { return _edit_transform; }

    bool knot_selected() const;
    bool knot_mouseover() const;

    SPDesktop *getDesktop() { return desktop; }
    SPItem *getItem() { return item; }
    bool is_dragging() const { return dragging; }

    bool set_item_clickpos(Geom::Point loc);
    void install_modification_watch();

    std::list<KnotHolderEntity *> entity;   // TODO: convert to std::unique_ptr
    friend class Inkscape::UI::ShapeEditor; // FIXME why?
    friend class Inkscape::LivePathEffect::NodeSatelliteArrayParam;                    // why?
    friend class Inkscape::LivePathEffect::PowerStrokePointArrayParamKnotHolderEntity; // why?
    friend class Inkscape::LivePathEffect::FilletChamferKnotHolderEntity; // why?

protected:

    SPDesktop *desktop;
    SPItem *item; // TODO: Remove this and keep the actual item (e.g., SPRect etc.) in the item-specific knotholders
    Inkscape::XML::Node *repr; ///< repr of the item, for setting and releasing listeners.

    bool local_change = false; ///< if true, no need to recreate knotholder if repr was changed.

    bool dragging = false;

    Geom::Affine _edit_transform;
    sigc::scoped_connection _watch_fill;
    sigc::scoped_connection _watch_stroke;
};

#endif // SEEN_SP_KNOTHOLDER_H

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

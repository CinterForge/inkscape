// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_PERSP3D_H
#define SEEN_PERSP3D_H

/*
 * Implementation of 3D perspectives as SPObjects
 *
 * Authors:
 *   Maximilian Albert <Anhalter42@gmx.de>
 *
 * Copyright (C) 2007  Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <list>
#include <map>
#include <vector>

#include "document.h"
#include "inkscape.h" // for SP_ACTIVE_DOCUMENT
#include "sp-object.h"
#include "transf_mat_3x4.h"
#include "xml/node-observer.h"

class SPBox3D;
class Persp3D;

namespace Inkscape {
class ObjectSet;
}

class Persp3DNodeObserver : public Inkscape::XML::NodeObserver
{
    friend class Persp3D;
    ~Persp3DNodeObserver() override = default; // can only exist as a direct base of Persp3D

    void notifyAttributeChanged(Inkscape::XML::Node &, GQuark, Inkscape::Util::ptr_shared, Inkscape::Util::ptr_shared) final;
};

class Persp3DImpl
{
public:
    Persp3DImpl();

    Proj::TransfMat3x4 tmat{Proj::TransfMat3x4()};

    // Also write the list of boxes into the xml repr and vice versa link boxes to their persp3d?
    std::vector<SPBox3D *> boxes;
    SPDocument *document{nullptr};

    // for debugging only
    int my_counter;
};

class Persp3D final
    : public SPObject
    , private Persp3DNodeObserver
{
public:
	Persp3D();
	~Persp3D() override;
    int tag() const override { return tag_of<decltype(*this)>; }

    std::unique_ptr<Persp3DImpl> perspective_impl;

protected:
	void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
	void release() override;

	void set(SPAttr key, char const* value) override;

	void update(SPCtx* ctx, unsigned int flags) override;

	Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, unsigned int flags) override;


public:
    // FIXME: Make more of these inline!
    static Persp3D * get_from_repr (Inkscape::XML::Node *repr) {
        return cast<Persp3D>(SP_ACTIVE_DOCUMENT->getObjectByRepr(repr));
    }
    Proj::Pt2 get_VP (Proj::Axis axis) const {
        return perspective_impl->tmat.column(axis);
    }
    Geom::Point get_PL_dir_from_pt (Geom::Point const &pt, Proj::Axis axis) const; // convenience wrapper around the following two
    Geom::Point get_finite_dir (Geom::Point const &pt, Proj::Axis axis) const;
    Geom::Point get_infinite_dir (Proj::Axis axis) const;
    double get_infinite_angle (Proj::Axis axis) const;
    static bool VP_is_finite (Persp3DImpl *persp_impl, Proj::Axis axis);
    void toggle_VP (Proj::Axis axis, bool set_undo = true);
    static void toggle_VPs (std::list<Persp3D *>, Proj::Axis axis);
    void set_VP_state (Proj::Axis axis, Proj::VPState state);
    void rotate_VP (Proj::Axis axis, double angle, bool alt_pressed); // angle is in degrees
    void apply_affine_transformation (Geom::Affine const &xform);

    void add_box (SPBox3D *box);
    void remove_box (SPBox3D *box);
    bool has_box (SPBox3D *box) const;

    void update_box_displays ();
    void update_box_reprs ();
    void update_z_orders ();
    unsigned int num_boxes () const { return perspective_impl->boxes.size(); }
    std::vector<SPBox3D *> list_of_boxes() const;

    bool perspectives_coincide(Persp3D const *rhs) const;
    void absorb(Persp3D *persp2);

    static Persp3D * create_xml_element (SPDocument *document);
    static Persp3D * document_first_persp (SPDocument *document);
    static Persp3D * create_copy(const Persp3D &copy_from);

    bool has_all_boxes_in_selection (Inkscape::ObjectSet *set) const;

    void print_debugging_info () const;
    static void print_debugging_info_all(SPDocument *doc);
    static void print_all_selected();

private:
    friend Persp3DNodeObserver; // for static_cast
    Persp3DNodeObserver &nodeObserver() { return *this; }
};

#endif // SEEN_PERSP3D_H

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

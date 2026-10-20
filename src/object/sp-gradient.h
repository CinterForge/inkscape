// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_GRADIENT_H
#define SEEN_SP_GRADIENT_H
/*
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyrigt  (C) 2010 Jon A. Cruz
 * Copyright (C) 2007 Johan Engelen
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <functional>
#include <vector>
#include <sigc++/connection.h>
#include <glibmm/ustring.h>
#include <2geom/affine.h>

#include "sp-paint-server.h"
#include "sp-paint-server-data.h"
#include "sp-mesh-array.h"

class SPGradientReference;
class SPStop;

enum SPGradientType {
    SP_GRADIENT_TYPE_UNKNOWN,
    SP_GRADIENT_TYPE_LINEAR,
    SP_GRADIENT_TYPE_RADIAL,
    SP_GRADIENT_TYPE_MESH
};

enum SPGradientState {
    SP_GRADIENT_STATE_UNKNOWN,
    SP_GRADIENT_STATE_VECTOR,
    SP_GRADIENT_STATE_PRIVATE
};

enum GrPointType {
    POINT_LG_BEGIN = 0,
    POINT_LG_END,
    POINT_LG_MID,
    POINT_RG_CENTER,
    POINT_RG_R1,
    POINT_RG_R2,
    POINT_RG_FOCUS,
    POINT_RG_MID1,
    POINT_RG_MID2,
    POINT_MG_CORNER,
    POINT_MG_HANDLE,
    POINT_MG_TENSOR,
    // insert new point types here.

    POINT_G_INVALID
};

namespace Inkscape {

enum PaintTarget {
    FOR_FILL,
    FOR_STROKE
};

/**
 * Convenience function to access a common vector of all enum values.
 */
std::vector<PaintTarget> const &allPaintTargets();

} // namespace Inkscape

/**
 * Gradient
 *
 * Implement spread, stops list
 * \todo fixme: Implement more here (Lauris)
 */
class SPGradient
    : public SPPaintServer
{
public:
    SPGradient();
    ~SPGradient() override;
    int tag() const override { return tag_of<decltype(*this)>; }

protected:
    /**
     * Returns the first of {src, src-\>ref-\>getObject(),
     * src-\>ref-\>getObject()-\>ref-\>getObject(),...}
     * for which \a match is true, or NULL if none found.
     *
     * The raison d'être of this routine is that it correctly handles cycles in the href chain (e.g., if
     * a gradient gives itself as its href, or if each of two gradients gives the other as its href).
     *
     * \pre is<SPGradient>(src).
     */
    template <typename G, typename Pred>   // G is SPGradient or SPGradient const
    static G *chase_hrefs(G *src, Pred match)
    {
        g_return_val_if_fail(src, nullptr);

        G *p1 = src, *p2 = src;
        bool do1 = false;
        for (;;) {
            if (match(p2)) {
                return p2;
            }

            p2 = p2->ref->getObject();   // SPGradient* converts to G*
            if (!p2) {
                return nullptr;
            }
            if (do1) {
                p1 = p1->ref->getObject();
            }
            do1 = !do1;

            if (p2 == p1) {
                return nullptr;          // cycle
            }
        }
    }

    /** SVGLength attributes (used by sp-linear-gradient.h and sp-radial-gradient.h) */
    template <typename Derived>
    static SVGLength find_attr_length(const Derived *src, SVGLength Derived::*attr);

    /** gradientUnits attribute */
    SPGradientUnits units;
    unsigned int units_set : 1;

public:
    /** gradientTransform attribute */
    Geom::Affine gradientTransform;
    unsigned int gradientTransform_set : 1;

private:
    /** spreadMethod attribute */
    SPGradientSpread spread;
    unsigned int spread_set : 1;

    /** Gradient stops */
    unsigned int has_stops : 1;

    /** Gradient patches */
    mutable unsigned int has_patches : 1;

    /** Pinned in swatches dialog */
    bool _pinned = false;

public:
    /** Reference (href) */
    SPGradientReference *ref;

    /** State in Inkscape gradient system */
    unsigned int state;
    
    /** Linear and Radial Gradients */

    /** Composed vector */
    mutable SPGradientVector vector;
    SPGradientVector const *getGradientVector() const override;
    virtual std::vector<double> getGradientGeom() const { return {}; }

    sigc::connection modified_connection;

    bool hasStops() const;

    SPStop* getFirstStop();
    int getStopCount() const;

    bool isEquivalent(SPGradient *b);
    bool isAligned(SPGradient *b);

    mutable SPGradientMesh mesh;
    SPGradientMesh const *getGradientMesh() const override;
    virtual std::vector<std::vector<SPGradientPatch>> getGradientPatches() const { return {}; }

    /** Mesh Gradients **************/

    /** Composed array (for mesh gradients) */
    mutable SPMeshNodeArray array;
    mutable SPMeshNodeArray array_smoothed; // Smoothed version of array
    
    bool hasPatches() const;

    /** All Gradients **************/
    bool isUnitsSet() const;
    SPGradientUnits getUnits() const override;
    void setUnits(SPGradientUnits units);


    bool isSpreadSet() const;
    SPGradientSpread getSpread() const override;

    bool isTransformSet() const;
    Geom::Affine getGradientTransform() const override { return gradientTransform; }

/**
 * Returns private vector of given gradient (the gradient at the end of the href chain which has
 * stops), optionally normalizing it.
 *
 * \pre is<SPGradient>(gradient).
 * \pre There exists a gradient in the chain that has stops.
 */
    SPGradient *getVector(bool force_private = false);
    SPGradient const *getVector(bool force_private = false) const
    {
        return const_cast<SPGradient *>(this)->getVector(force_private);
    }

 /**
 * Returns private mesh of given gradient (the gradient at the end of the href chain which has
 * patches), optionally normalizing it.
 */
    SPGradient *getArray(bool force_private = false) const;

    //static GType getType();

    /** Forces vector to be built, if not present (i.e. changed) */
    void ensureVector();

    /** Forces array (mesh) to be built, if not present (i.e. changed) */
    void ensureArray() const;

    /**
     * Set spread property of gradient and emit modified.
     */
    void setSpread(SPGradientSpread spread);

    SPGradientSpread fetchSpread() const;
    SPGradientUnits fetchUnits();
    Geom::Affine fetchTransform();

    void setSwatch(bool swatch = true);
    void setPinned(bool pinned = true);
    bool isPinned() const { return _pinned; }

    bool isSolid() const;

    static void gradientRefModified(SPObject *href, unsigned int flags, SPGradient *gradient);
    static void gradientRefChanged(SPObject *old_ref, SPObject *ref, SPGradient *gr);

    /* Gradient repr methods */
    void repr_write_vector();
    void repr_clear_vector();

    void forEachPreviewPatternStop(std::function<void (double offset, Inkscape::Colors::Color const &col)> const &callback);
    Inkscape::Colors::Color getPreviewAverageColor();
    cairo_pattern_t *create_preview_pattern(double width);

    /** Transforms to/from gradient position space in given environment */
    Geom::Affine get_g2d_matrix(Geom::Affine const &ctm,
                                Geom::Rect const &bbox) const;
    Geom::Affine get_gs2d_matrix(Geom::Affine const &ctm,
                                 Geom::Rect const &bbox) const;
    void set_gs2d_matrix(Geom::Affine const &ctm, Geom::Rect const &bbox,
                         Geom::Affine const &gs2d);

    Geom::OptRect getAllItemsBox() const;

private:
    bool invalidateVector();
    bool invalidateArray();
    void rebuildVector() const;
    void rebuildArray() const;

protected:
    void build(SPDocument *document, Inkscape::XML::Node *repr) override;
    void release() override;
    void modified(unsigned int flags) override;
    Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;

    void child_added(Inkscape::XML::Node *child, Inkscape::XML::Node *ref) override;
    void remove_child(Inkscape::XML::Node *child) override;

    void set(SPAttr key, char const *value) override;
};

void
sp_gradient_pattern_common_setup(cairo_pattern_t *cp,
                                 SPGradient *gr,
                                 Geom::OptRect const &bbox,
                                 double opacity);

#endif // SEEN_SP_GRADIENT_H

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

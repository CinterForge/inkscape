// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_CLIPPATH_H
#define SEEN_SP_CLIPPATH_H

/*
 * SVG <clipPath> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Abhishek Sharma
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2001-2002 authors
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <vector>
#include <2geom/rect.h>

#include "object-view.h"
#include "sp-object-group.h"
#include "uri-references.h"

namespace Inkscape {
class Drawing;
class DrawingItem;
class DrawingGroup;
} // namespace Inkscape

class SPClipPath final
    : public SPObjectGroup
{
public:
	SPClipPath();
    ~SPClipPath() override;
    int tag() const override { return tag_of<decltype(*this)>; }

    bool clippath_units() const { return clipPathUnits; }

    // Fixme: Hack used by cairo-renderer.
    Geom::OptRect get_last_bbox() const { return views.back().bbox; }

    static char const *create(std::vector<Inkscape::XML::Node*> &reprs, SPDocument *document);

    Inkscape::DrawingItem *show(Inkscape::Drawing &drawing, unsigned key, Geom::OptRect const &bbox);
    void hide(unsigned key);
    void setBBox(unsigned key, Geom::OptRect const &bbox);

    Geom::OptRect geometricBounds(Geom::Affine const &transform) const;
    Geom::OptRect visualBounds(Geom::Affine const &transform) const;
    Geom::PathVector getPathVector(Geom::Affine const &transform) const;
    SPText const *getTextObject() const;
    void transform_multiply(Geom::Affine postmul, bool set = true);
    void removeTransformsRecursively(SPObject const *root);

protected:
    void build(SPDocument *doc, Inkscape::XML::Node *repr) override;
	void release() override;
    void set(SPAttr key, char const *value) override;
    void update(SPCtx *ctx, unsigned flags) override;
    void modified(unsigned flags) override;
    Inkscape::XML::Node *write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, unsigned flags) override;

    void child_added(Inkscape::XML::Node *child, Inkscape::XML::Node *ref) override;

private:
    bool clipPathUnits_set : 1;
    bool clipPathUnits : 1;

    using View = ObjectView<Inkscape::DrawingGroup>;
    std::vector<View> views;
    void update_view(View &v);
};

class SPClipPathReference
    : public Inkscape::URIReference
{
public:
    SPClipPathReference(SPObject *obj)
        : URIReference(obj) {}

    SPClipPath *getObject() const
    {
        return static_cast<SPClipPath*>(URIReference::getObject());
    }

    sigc::connection modified_connection;

protected:
    /**
     * If the owner element of this reference (the element with <... clippath="...">)
     * is a child of the clippath it refers to, return false.
     * \return false if obj is not a clippath or if obj is a parent of this
     *         reference's owner element. True otherwise.
     */
    bool _acceptObject(SPObject *obj) const override;
};

#endif // SEEN_SP_CLIPPATH_H

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

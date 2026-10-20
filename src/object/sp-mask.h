// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_MASK_H
#define SEEN_SP_MASK_H

/*
 * SVG <mask> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2003 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <memory>
#include <vector>
#include <2geom/rect.h>

#include "object-view.h"
#include "sp-object-group.h"
#include "uri-references.h"
#include "xml/node.h"

namespace Inkscape {
class Drawing;
class DrawingItem;
class DrawingGroup;
} // namespace Inkscape

class SPMask final
    : public SPObjectGroup
{
public:
	SPMask();
	~SPMask() override;
    int tag() const override { return tag_of<decltype(*this)>; }

    bool mask_content_units() const { return maskContentUnits; }

    // Fixme: Hack used by cairo-renderer.
    Geom::OptRect get_last_bbox() const { return views.back().bbox; }

    static char const *create(std::vector<Inkscape::XML::Node*> &reprs, SPDocument *document);

    Inkscape::DrawingItem *show(Inkscape::Drawing &drawing, unsigned key, Geom::OptRect const &bbox);
    void hide(unsigned key);
    void setBBox(unsigned key, Geom::OptRect const &bbox);

    Geom::OptRect geometricBounds(Geom::Affine const &transform) const;
    Geom::OptRect visualBounds(Geom::Affine const &transform) const;

protected:
    void build(SPDocument *doc, Inkscape::XML::Node *repr) override;
	void release() override;
    void set(SPAttr key, char const *value) override;
    void update(SPCtx *ctx, unsigned flags) override;
    void modified(unsigned flags) override;
    Inkscape::XML::Node *write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, unsigned flags) override;

    void child_added(Inkscape::XML::Node *child, Inkscape::XML::Node *ref) override;

private:
    bool maskUnits_set : 1;
    bool maskUnits : 1;

    bool maskContentUnits_set : 1;
    bool maskContentUnits : 1;

    using View = ObjectView<Inkscape::DrawingGroup>;
    std::vector<View> views;
    void update_view(View &v);
};

class SPMaskReference
    : public Inkscape::URIReference
{
public:
    SPMaskReference(SPObject *obj)
        : URIReference(obj) {}

    SPMask *getObject() const
    {
        return static_cast<SPMask*>(URIReference::getObject());
	}

    sigc::connection modified_connection;

protected:
    /**
     * If the owner element of this reference (the element with <... mask="...">)
     * is a child of the mask it refers to, return false.
     * \return false if obj is not a mask or if obj is a parent of this
     *         reference's owner element. True otherwise.
     */
    bool _acceptObject(SPObject *obj) const override;
};

#endif // SEEN_SP_MASK_H

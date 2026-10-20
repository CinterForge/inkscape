// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
/*
 */

#include "sp-flowregion.h"

#include <glibmm/i18n.h>

#include "sp-desc.h"
#include "sp-shape.h"
#include "sp-text.h"
#include "sp-title.h"
#include "sp-use.h"
#include "style.h"

#include "path/path-curve.h"
#include "livarot/Path.h"
#include "livarot/Shape.h"
#include "xml/document.h"

class SPDesc;
class SPTitle;

static std::unique_ptr<Shape> shape_union(std::unique_ptr<Shape> base_shape, std::unique_ptr<Shape> add_shape);
static std::unique_ptr<Shape> extract_shape(SPItem *item);

void SPFlowregion::child_added(Inkscape::XML::Node *child, Inkscape::XML::Node *ref)
{
    SPItem::child_added(child, ref);

    this->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

/* fixme: hide (Lauris) */

void SPFlowregion::remove_child(Inkscape::XML::Node * child) {
	SPItem::remove_child(child);

	this->requestModified(SP_OBJECT_MODIFIED_FLAG);
}


void SPFlowregion::update(SPCtx *ctx, unsigned int flags) {
    SPItemCtx *ictx = reinterpret_cast<SPItemCtx *>(ctx);
    SPItemCtx cctx = *ictx;

    unsigned childflags = flags;
    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        childflags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }
    childflags &= SP_OBJECT_MODIFIED_CASCADE;

    std::vector<SPObject*>l;

    for (auto& child: children) {
        sp_object_ref(&child);
        l.push_back(&child);
    }

    for (auto child:l) {
        g_assert(child != nullptr);
        auto item = cast<SPItem>(child);

        if (childflags || (child->uflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            if (item) {
                SPItem const &chi = *item;
                cctx.i2doc = chi.transform * ictx->i2doc;
                cctx.i2vp = chi.transform * ictx->i2vp;
                child->updateDisplay((SPCtx *)&cctx, childflags);
            } else {
                child->updateDisplay(ctx, childflags);
            }
        }

        sp_object_unref(child);
    }

    SPItem::update(ctx, flags);

    updateComputed();
}

void SPFlowregion::updateComputed()
{
    computed.clear();
    for (auto &child : children) {
        if (auto item = cast<SPItem>(&child)) {
            computed.push_back(extract_shape(item));
        }
    }
}

void SPFlowregion::modified(guint flags) {
    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }

    flags &= SP_OBJECT_MODIFIED_CASCADE;

    std::vector<SPObject *>l;

    for (auto& child: children) {
        sp_object_ref(&child);
        l.push_back(&child);
    }

    for (auto child:l) {
        g_assert(child != nullptr);

        if (flags || (child->mflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            child->emitModified(flags);
        }

        sp_object_unref(child);
    }
}

Inkscape::XML::Node *SPFlowregion::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
    if (flags & SP_OBJECT_WRITE_BUILD) {
        if ( repr == nullptr ) {
            repr = xml_doc->createElement("svg:flowRegion");
        }

        std::vector<Inkscape::XML::Node *> l;
        for (auto& child: children) {
            if (!is<SPTitle>(&child) && !is<SPDesc>(&child)) {
                Inkscape::XML::Node *crepr = child.updateRepr(xml_doc, nullptr, flags);

                if (crepr) {
                    l.push_back(crepr);
                }
            }
        }

        for (auto i = l.rbegin(); i != l.rend(); ++i) {
            repr->addChild(*i, nullptr);
            Inkscape::GC::release(*i);
        }

        for (auto& child: children) {
            if (!is<SPTitle>(&child) && !is<SPDesc>(&child)) {
                child.updateRepr(flags);
            }
        }
    }

    SPItem::write(xml_doc, repr, flags);

    updateComputed(); // copied from update(), see LP Bug 1339305

    return repr;
}

const char* SPFlowregion::typeName() const {
    return "text-flow";
}

const char* SPFlowregion::displayName() const {
    // TRANSLATORS: "Flow region" is an area where text is allowed to flow
    return _("Flow Region");
}

void SPFlowregionExclude::child_added(Inkscape::XML::Node *child, Inkscape::XML::Node *ref) {
	SPItem::child_added(child, ref);

	this->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

/* fixme: hide (Lauris) */

void SPFlowregionExclude::remove_child(Inkscape::XML::Node * child) {
	SPItem::remove_child(child);

	this->requestModified(SP_OBJECT_MODIFIED_FLAG);
}


void SPFlowregionExclude::update(SPCtx *ctx, unsigned int flags) {
    SPItemCtx *ictx = reinterpret_cast<SPItemCtx *>(ctx);
    SPItemCtx cctx = *ictx;

    SPItem::update(ctx, flags);

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }

    flags &= SP_OBJECT_MODIFIED_CASCADE;

    std::vector<SPObject *> l;

    for (auto& child: children) {
        sp_object_ref(&child);
        l.push_back(&child);
    }

    for(auto child:l) {
        g_assert(child != nullptr);

        if (flags || (child->uflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            auto item = cast<SPItem>(child);
            if (item) {
                SPItem const &chi = *item;
                cctx.i2doc = chi.transform * ictx->i2doc;
                cctx.i2vp = chi.transform * ictx->i2vp;
                child->updateDisplay((SPCtx *)&cctx, flags);
            } else {
                child->updateDisplay(ctx, flags);
            }
        }

        sp_object_unref(child);
    }

    _updateComputed();
}

void SPFlowregionExclude::_updateComputed()
{
    _computed.reset();
    for (auto &child : children) {
        if (auto *item = cast<SPItem>(&child)) {
            _computed = shape_union(std::move(_computed), extract_shape(item));
        }
    }
}

void SPFlowregionExclude::modified(guint flags) {
    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }

    flags &= SP_OBJECT_MODIFIED_CASCADE;

    std::vector<SPObject*> l;

    for (auto& child: children) {
        sp_object_ref(&child);
        l.push_back(&child);
    }

    for (auto child:l) {
        g_assert(child != nullptr);

        if (flags || (child->mflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            child->emitModified(flags);
        }

        sp_object_unref(child);
    }
}

Inkscape::XML::Node *SPFlowregionExclude::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
    if (flags & SP_OBJECT_WRITE_BUILD) {
        if ( repr == nullptr ) {
            repr = xml_doc->createElement("svg:flowRegionExclude");
        }

        std::vector<Inkscape::XML::Node *> l;

        for (auto& child: children) {
            Inkscape::XML::Node *crepr = child.updateRepr(xml_doc, nullptr, flags);

            if (crepr) {
                l.push_back(crepr);
            }
        }

        for (auto i = l.rbegin(); i != l.rend(); ++i) { 
            repr->addChild(*i, nullptr);
            Inkscape::GC::release(*i);
        }

    } else {
        for (auto& child: children) {
            child.updateRepr(flags);
        }
    }

    SPItem::write(xml_doc, repr, flags);

    return repr;
}

const char* SPFlowregionExclude::typeName() const {
    return "text-flow";
}

const char* SPFlowregionExclude::displayName() const {
	/* TRANSLATORS: A region "cut out of" a flow region; text is not allowed to flow inside the
	 * flow excluded region.  flowRegionExclude in SVG 1.2: see
	 * http://www.w3.org/TR/2004/WD-SVG12-20041027/flow.html#flowRegion-elem and
	 * http://www.w3.org/TR/2004/WD-SVG12-20041027/flow.html#flowRegionExclude-elem. */
	return _("Flow Excluded Region");
}

static std::unique_ptr<Shape> shape_union(std::unique_ptr<Shape> base_shape, std::unique_ptr<Shape> add_shape)
{
    if (!base_shape) {
        base_shape = std::make_unique<Shape>();
    }
    if (!base_shape->hasEdges()) {
        base_shape->Copy(add_shape.get());
    } else if (add_shape && add_shape->hasEdges()) {
        auto temp = std::make_unique<Shape>();
        temp->Booleen(add_shape.get(), base_shape.get(), bool_op_union);
        base_shape = std::move(temp);
    }
    return base_shape;
}

static std::unique_ptr<Shape> extract_shape(SPItem *item)
{
    Geom::Affine tr_mat;
    auto *shape_source = item;
    if (auto use = cast<SPUse>(item)) {
        shape_source = use->child;
        tr_mat = use->getRelativeTransform(item->parent);
    } else {
        tr_mat = item->transform;
    }

    std::optional<Geom::PathVector> curve;
    if (auto shape = cast<SPShape>(shape_source)) {
        if (!shape->curve()) {
            shape->set_shape();
        }
        curve = ptr_to_opt(shape->curve());
    } else if (auto text = cast<SPText>(shape_source)) {
        curve = text->getNormalizedBpath();
    }

    if (!curve) {
        return {};
    }
    Path temp;
    temp.LoadPathVector(*curve, tr_mat, true);
    temp.Convert(0.25);

    Shape n_shp;
    temp.Fill(&n_shp, 0);

    auto result = std::make_unique<Shape>();
    SPStyle *style = shape_source->style;
    result->ConvertToShape(&n_shp,
                           (style && style->fill_rule.computed == SP_WIND_RULE_EVENODD) ? fill_oddEven : fill_nonZero);
    return result;
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

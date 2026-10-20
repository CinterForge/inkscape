// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * SVG <hatch> implementation
 */
/*
 * Authors:
 *   Tomasz Boczkowski <penginsbacon@gmail.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2014 Tomasz Boczkowski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-hatch.h"

#include <cstring>
#include <sigc++/functors/mem_fun.h>
#include <2geom/transforms.h>

#include "attributes.h"
#include "bad-uri-exception.h"
#include "display/drawing-pattern.h"
#include "display/drawing.h"
#include "document.h"
#include "object/uri.h"
#include "sp-defs.h"
#include "sp-hatch-path.h"
#include "sp-item.h"
#include "style.h"
#include "svg/svg.h"
#include "xml/href-attribute-helper.h"

SPHatchReference::SPHatchReference(SPHatch *obj)
    : URIReference(obj)
{}

SPHatch *SPHatchReference::getObject() const
{
    return cast_unsafe<SPHatch>(URIReference::getObject());
}

bool SPHatchReference::_acceptObject(SPObject *obj) const
{
    return is<SPHatch>(obj) && URIReference::_acceptObject(obj);
}

SPHatch::SPHatch()
    : ref{this}
{
    ref.changedSignal().connect(sigc::mem_fun(*this, &SPHatch::_onRefChanged));
}

SPHatch::~SPHatch() = default;

void SPHatch::build(SPDocument *doc, Inkscape::XML::Node *repr)
{
    SPPaintServer::build(doc, repr);

    readAttr(SPAttr::HATCHUNITS);
    readAttr(SPAttr::HATCHCONTENTUNITS);
    readAttr(SPAttr::TRANSFORM);
    readAttr(SPAttr::X);
    readAttr(SPAttr::Y);
    readAttr(SPAttr::PITCH);
    readAttr(SPAttr::ROTATE);
    readAttr(SPAttr::XLINK_HREF);
    readAttr(SPAttr::STYLE);

    // Register ourselves
    doc->addResource("hatch", this);
}

void SPHatch::release()
{
    if (document) {
        // Unregister ourselves
        document->removeResource("hatch", this);
    }

    auto children = hatchPaths();
    for (auto &v : views) {
        for (auto child : children) {
            child->hide(v.key);
        }
        v.drawingitem.reset();
    }
    views.clear();

    _modified_connection.disconnect();
    ref.detach();

    SPPaintServer::release();
}

void SPHatch::child_added(Inkscape::XML::Node* child, Inkscape::XML::Node* ref)
{
    SPObject::child_added(child, ref);

    auto path_child = cast<SPHatchPath>(document->getObjectByRepr(child));

    if (path_child) {
        for (auto &v : views) {
            Geom::OptInterval extents = _calculateStripExtents(v.bbox);
            auto ac = path_child->show(v.drawingitem->drawing(), v.key, extents);

            path_child->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            if (ac) {
                v.drawingitem->prependChild(ac);
            }
        }
    }
    //FIXME: notify all hatches that refer to this child set
}

void SPHatch::set(SPAttr key, char const *value)
{
    switch (key) {
    case SPAttr::HATCHUNITS:
        if (value) {
            if (!std::strcmp(value, "userSpaceOnUse")) {
                _hatch_units = HatchUnits::UserSpaceOnUse;
            } else {
                _hatch_units = HatchUnits::ObjectBoundingBox;
            }
        } else {
            _hatch_units = {};
        }

        requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        break;

    case SPAttr::HATCHCONTENTUNITS:
        if (value) {
            if (!std::strcmp(value, "userSpaceOnUse")) {
                _hatch_content_units = HatchUnits::UserSpaceOnUse;
            } else {
                _hatch_content_units = HatchUnits::ObjectBoundingBox;
            }
        } else {
            _hatch_content_units = {};
        }

        requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        break;

    case SPAttr::TRANSFORM: {
        Geom::Affine t;
        if (value && sp_svg_transform_read(value, &t)) {
            _hatch_transform = t;
        } else {
            _hatch_transform = {};
        }

        requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        break;
    }
    case SPAttr::X:
        _x.readOrUnset(value);
        requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        break;

    case SPAttr::Y:
        _y.readOrUnset(value);
        requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        break;

    case SPAttr::PITCH:
        _pitch.readOrUnset(value);
        requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        break;

    case SPAttr::ROTATE:
        _rotate.readOrUnset(value);
        requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        break;

    case SPAttr::XLINK_HREF:
        if (value && href == value) {
            // Href unchanged, do nothing.
        } else {
            href.clear();

            if (value) {
                // First, set the href field; it's only used in the "unchanged" check above.
                href = value;
                // Now do the attaching, which emits the changed signal.
                if (value) {
                    try {
                        ref.attach(Inkscape::URI(value));
                    } catch (Inkscape::BadURIException const &e) {
                        g_warning("%s", e.what());
                        ref.detach();
                    }
                } else {
                    ref.detach();
                }
            }
        }
        requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        break;

    default:
        if (SP_ATTRIBUTE_IS_CSS(key)) {
            style->clear(key);
            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
        } else {
            SPPaintServer::set(key, value);
        }
        break;
    }
}

bool SPHatch::_hasHatchPatchChildren(SPHatch const *hatch)
{
    return std::any_of(hatch->children.begin(),
                       hatch->children.end(),
                       [] (auto &c) { return is<SPHatchPath>(&c); });
}

Geom::Affine SPHatch::get_this_transform() const {
    return _hatch_transform.value_or(Geom::identity());
}

std::vector<SPHatchPath *> SPHatch::hatchPaths()
{
    auto const src = rootHatch();
    if (!src) {
        return {};
    }

    std::vector<SPHatchPath *> list;

    for (auto &child : src->children) {
        if (auto hatch_path = cast<SPHatchPath>(&child)) {
            list.push_back(hatch_path);
        }
    }

    return list;
}

std::vector<SPHatchPath const *> SPHatch::hatchPaths() const
{
    auto const src = rootHatch();
    if (!src) {
        return {};
    }

    std::vector<SPHatchPath const *> list;

    for (auto &child : src->children) {
        if (auto hatch_path = cast<SPHatchPath>(&child)) {
            list.push_back(hatch_path);
        }
    }

    return list;
}

// TODO: ::remove_child and ::order_changed handles - see SPPattern

void SPHatch::update(SPCtx* ctx, unsigned flags)
{
    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }

    flags &= SP_OBJECT_MODIFIED_CASCADE;

    std::vector<SPHatchPath *> children(hatchPaths());

    for (auto child : children) {
        sp_object_ref(child, nullptr);

        for (auto &v : views) {
            Geom::OptInterval strip_extents = _calculateStripExtents(v.bbox);
            child->setStripExtents(v.key, strip_extents);
        }

        if ((flags & SP_OBJECT_FLAGS_ALL) || (child->mflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            child->updateDisplay(ctx, flags);
        }

        sp_object_unref(child, nullptr);
    }

    for (auto &v : views) {
        _updateView(v);
    }
}

void SPHatch::modified(unsigned flags)
{
    flags = cascade_flags(flags);

    std::vector<SPHatchPath *> children(hatchPaths());

    for (auto child : children) {
        sp_object_ref(child, nullptr);

        if ((flags & SP_OBJECT_FLAGS_ALL) || (child->mflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            child->emitModified(flags);
        }

        sp_object_unref(child, nullptr);
    }
}

void SPHatch::_onRefChanged(SPObject *old_ref, SPObject *ref)
{
    if (old_ref) {
        _modified_connection.disconnect();
    }

    auto hatch = cast<SPHatch>(ref);
    if (hatch) {
        _modified_connection = ref->connectModified(sigc::mem_fun(*this, &SPHatch::_onRefModified));
    }

    if (!_hasHatchPatchChildren(this)) {
        SPHatch *old_shown = nullptr;
        SPHatch *new_shown = nullptr;
        std::vector<SPHatchPath *> oldhatchPaths;
        std::vector<SPHatchPath *> newhatchPaths;

        auto old_hatch = cast<SPHatch>(old_ref);
        if (old_hatch) {
            old_shown = old_hatch->rootHatch();
            oldhatchPaths = old_shown->hatchPaths();
        }
        if (hatch) {
            new_shown = hatch->rootHatch();
            newhatchPaths = new_shown->hatchPaths();
        }
        if (old_shown != new_shown) {

            for (auto &v : views) {
                Geom::OptInterval extents = _calculateStripExtents(v.bbox);

                for (auto child : oldhatchPaths) {
                    child->hide(v.key);
                }
                for (auto child : newhatchPaths) {
                    auto cai = child->show(v.drawingitem->drawing(), v.key, extents);
                    child->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
                    if (cai) {
                        v.drawingitem->appendChild(cai);
                    }

                }
            }
        }
    }

    _onRefModified(ref, 0);
}

void SPHatch::_onRefModified(SPObject *, unsigned)
{
    requestModified(SP_OBJECT_MODIFIED_FLAG);
}

SPHatch const *SPHatch::rootHatch() const
{
    for (auto p = this; p; p = p->ref.getObject()) {
        if (_hasHatchPatchChildren(p)) { // find the first one with hatch patch children
            return p;
        }
    }
    return this; // document is broken, we can't get to root; but at least we can return ourself which is supposedly a valid hatch
}

// Access functions that look up fields up the chain of referenced hatchs and return the first one which is set

SPHatch::HatchUnits SPHatch::hatchUnits() const
{
    for (auto hatch = this; hatch; hatch = hatch->ref.getObject()) {
        if (hatch->_hatch_units) {
            return *hatch->_hatch_units;
        }
    }
    return HatchUnits::ObjectBoundingBox;
}

SPHatch::HatchUnits SPHatch::hatchContentUnits() const
{
    for (auto hatch = this; hatch; hatch = hatch->ref.getObject()) {
        if (hatch->_hatch_content_units) {
            return *hatch->_hatch_content_units;
        }
    }
    return HatchUnits::UserSpaceOnUse;
}

Geom::Affine SPHatch::hatchTransform() const
{
    for (auto hatch = this; hatch; hatch = hatch->ref.getObject()) {
        if (hatch->_hatch_transform) {
            return *hatch->_hatch_transform;
        }
    }
    return Geom::identity();
}

double SPHatch::x() const
{
    for (auto hatch = this; hatch; hatch = hatch->ref.getObject()) {
        if (hatch->_x._set) {
            return hatch->_x.computed;
        }
    }
    return 0;
}

double SPHatch::y() const
{
    for (auto hatch = this; hatch; hatch = hatch->ref.getObject()) {
        if (hatch->_y._set) {
            return hatch->_y.computed;
        }
    }
    return 0;
}

double SPHatch::pitch() const
{
    for (auto hatch = this; hatch; hatch = hatch->ref.getObject()) {
        if (hatch->_pitch._set) {
            return hatch->_pitch.computed;
        }
    }
    return 0;
}

double SPHatch::rotate() const
{
    for (auto hatch = this; hatch; hatch = hatch->ref.getObject()) {
        if (hatch->_rotate._set) {
            return hatch->_rotate.computed;
        }
    }
    return 0;
}

int SPHatch::_countHrefs(SPObject *obj) const
{
    if (!obj) {
        return 1;
    }

    int i = 0;

    SPStyle *style = obj->style;
    if (style && style->fill.isPaintserver() && style->getFillPaintServer() == this) {
        i++;
    }
    if (style && style->stroke.isPaintserver() && style->getStrokePaintServer() == this) {
        i++;
    }

    for (auto &child : obj->children) {
        i += _countHrefs(&child);
    }

    return i;
}

SPHatch *SPHatch::clone_if_necessary(SPItem *item, char const *property)
{
    SPHatch *hatch = this;
    if (hatch->href.empty() || hatch->hrefcount > _countHrefs(item)) {
        Inkscape::XML::Document *xml_doc = document->getReprDoc();
        Inkscape::XML::Node *defsrepr = document->getDefs()->getRepr();

        Inkscape::XML::Node *repr = xml_doc->createElement("svg:hatch");
        repr->setAttribute("inkscape:collect", "always");
        Glib::ustring parent_ref = Glib::ustring::compose("#%1", getRepr()->attribute("id"));
        Inkscape::setHrefAttribute(*repr, parent_ref);

        defsrepr->addChild(repr, nullptr);
        char const *child_id = repr->attribute("id");
        SPObject *child = document->getObjectById(child_id);
        g_assert(is<SPHatch>(child));

        hatch = cast<SPHatch>(child);

        Glib::ustring href = Glib::ustring::compose("url(#%1)", hatch->getRepr()->attribute("id"));

        SPCSSAttr *css = sp_repr_css_attr_new();
        sp_repr_css_set_property(css, property, href.c_str());
        sp_repr_css_change_recursive(item->getRepr(), css, "style");
        sp_repr_css_attr_unref(css);
    }

    return hatch;
}

void SPHatch::transform_multiply(Geom::Affine const &postmul, bool set)
{
    _hatch_transform = set ? postmul : hatchTransform() * postmul;
    setAttributeOrRemoveIfEmpty("transform", sp_svg_transform_write(*_hatch_transform));
}

bool SPHatch::isValid() const
{
    if (pitch() <= 0) {
        return false;
    }

    auto const children = hatchPaths();
    if (children.empty()) {
        return false;
    }

    return std::all_of(children.begin(),
                       children.end(),
                       [] (auto c) { return c->isValid(); });
}

Inkscape::DrawingPattern *SPHatch::show(Inkscape::Drawing &drawing, unsigned key, Geom::OptRect const &bbox)
{
    views.emplace_back(make_drawingitem<Inkscape::DrawingPattern>(drawing), bbox, key);
    auto &v = views.back();
    auto ai = v.drawingitem.get();

    auto children = hatchPaths();

    Geom::OptInterval extents = _calculateStripExtents(bbox);
    for (auto child : children) {
        Inkscape::DrawingItem *cai = child->show(drawing, key, extents);
        if (cai) {
            ai->appendChild(cai);
        }
    }

    _updateView(v);

    return ai;
}

void SPHatch::hide(unsigned key)
{
    auto const children = hatchPaths();

    for (auto child : children) {
        child->hide(key);
    }

    auto it = std::find_if(views.begin(), views.end(), [=] (auto &v) {
        return v.key == key;
    });

    if (it != views.end()) {
        views.erase(it);
        return;
    }

    g_assert_not_reached();
}

Geom::Interval SPHatch::bounds() const
{
    Geom::Interval result;
    auto children = hatchPaths();

    for (auto child : children) {
        if (result.extent() == 0) {
            result = child->bounds();
        } else {
            result |= child->bounds();
        }
    }
    return result;
}

SPHatch::RenderInfo SPHatch::calculateRenderInfo(unsigned key) const
{
    for (auto const &v : views) {
        if (v.key == key) {
            return _calculateRenderInfo(v);
        }
    }
    g_assert_not_reached();
    return {};
}

void SPHatch::_updateView(View &view)
{
    RenderInfo info = _calculateRenderInfo(view);
    //The rendering of hatch overflow is implemented by repeated drawing
    //of hatch paths over one strip. Within each iteration paths are moved by pitch value.
    //The movement progresses from right to left. This gives the same result
    //as drawing whole strips in left-to-right order.

    view.drawingitem->setChildTransform(info.child_transform);
    view.drawingitem->setPatternToUserTransform(info.pattern_to_user_transform);
    view.drawingitem->setTileRect(info.tile_rect);
    view.drawingitem->setStyle(style);
    view.drawingitem->setOverflow(info.overflow_initial_transform, info.overflow_steps, info.overflow_step_transform);
}

SPHatch::RenderInfo SPHatch::_calculateRenderInfo(View const &view) const
{
    auto const extents = _calculateStripExtents(view.bbox);
    if (!extents) {
        return {};
    }

    double tile_x = x();
    double tile_y = y();
    double tile_width = pitch();
    double tile_height = extents->max() - extents->min();
    double tile_rotate = rotate();
    double tile_render_y = extents->min();

    if (view.bbox && hatchUnits() == HatchUnits::ObjectBoundingBox) {
        tile_x *= view.bbox->width();
        tile_y *= view.bbox->height();
        tile_width *= view.bbox->width();
    }

    // Extent calculated using content units, need to correct.
    if (view.bbox && hatchContentUnits() == HatchUnits::ObjectBoundingBox) {
        tile_height *= view.bbox->height();
        tile_render_y *= view.bbox->height();
    }

    // Pattern size in hatch space
    Geom::Rect hatch_tile = Geom::Rect::from_xywh(0, tile_render_y, tile_width, tile_height);

    // Content to bbox
    Geom::Affine content2ps;
    if (view.bbox && hatchContentUnits() == HatchUnits::ObjectBoundingBox) {
        content2ps = Geom::Affine(view.bbox->width(), 0.0, 0.0, view.bbox->height(), 0, 0);
    }

    // Tile (hatch space) to user.
    Geom::Affine ps2user = Geom::Translate(tile_x, tile_y) * Geom::Rotate::from_degrees(tile_rotate) * hatchTransform();

    RenderInfo info;
    info.child_transform = content2ps;
    info.pattern_to_user_transform = ps2user;
    info.tile_rect = hatch_tile;

    if (style->overflow.computed == SP_CSS_OVERFLOW_VISIBLE) {
        Geom::Interval bounds = this->bounds();
        double pitch = this->pitch();
        if (view.bbox) {
            if (hatchUnits() == HatchUnits::ObjectBoundingBox) {
                pitch *= view.bbox->width();
            }
            if (hatchContentUnits() == HatchUnits::ObjectBoundingBox) {
                bounds *= view.bbox->width();
            }
        }
        double overflow_right_strip = floor(bounds.max() / pitch) * pitch;
        info.overflow_steps = ceil((overflow_right_strip - bounds.min()) / pitch) + 1;
        info.overflow_step_transform = Geom::Translate(pitch, 0.0);
        info.overflow_initial_transform = Geom::Translate(-overflow_right_strip, 0.0);
    } else {
        info.overflow_steps = 1;
    }

    return info;
}

//calculates strip extents in content space
Geom::OptInterval SPHatch::_calculateStripExtents(Geom::OptRect const &bbox) const
{
    if (bbox.hasZeroArea()) {
        return {};
    }

    double tile_x = x();
    double tile_y = y();
    double tile_rotate = rotate();

    Geom::Affine ps2user = Geom::Translate(tile_x, tile_y) * Geom::Rotate::from_degrees(tile_rotate) * hatchTransform();
    Geom::Affine user2ps = ps2user.inverse();

    Geom::Interval extents;
    for (int i = 0; i < 4; ++i) {
        Geom::Point corner = bbox->corner(i);
        Geom::Point corner_ps  =  corner * user2ps;
        if (i == 0 || corner_ps.y() < extents.min()) {
            extents.setMin(corner_ps.y());
        }
        if (i == 0 || corner_ps.y() > extents.max()) {
            extents.setMax(corner_ps.y());
        }
    }

    if (hatchContentUnits() == HatchUnits::ObjectBoundingBox) {
        extents /= bbox->height();
    }

    return extents;
}

void SPHatch::setBBox(unsigned key, Geom::OptRect const &bbox)
{
    for (auto &v : views) {
        if (v.key == key) {
            v.bbox = bbox;
            break;
        }
    }
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

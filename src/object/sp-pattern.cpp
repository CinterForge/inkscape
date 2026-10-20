// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SVG <pattern> implementation
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-pattern.h"

#include <cstring>
#include <string>
#include <glibmm.h>
#include <2geom/transforms.h>

#include "attributes.h"
#include "bad-uri-exception.h"
#include "display/cairo-utils.h"
#include "display/drawing-context.h"
#include "display/drawing-image.h"
#include "display/drawing-pattern.h"
#include "display/drawing.h"
#include "document.h"
#include "object/uri.h"
#include "sp-defs.h"
#include "sp-factory.h"
#include "sp-item.h"
#include "svg/svg.h"
#include "xml/href-attribute-helper.h"

SPPatternReference::SPPatternReference(SPPattern *owner)
    : URIReference(owner)
{
}

SPPattern *SPPatternReference::getObject() const
{
    return static_cast<SPPattern*>(URIReference::getObject());
}

bool SPPatternReference::_acceptObject(SPObject *obj) const
{
    return is<SPPattern>(obj) && URIReference::_acceptObject(obj);
}

/*
 *
 */

SPPattern::SPPattern()
    : ref(this)
    , _pattern_units(UNITS_OBJECTBOUNDINGBOX)
    , _pattern_units_set(false)
    , _pattern_content_units(UNITS_USERSPACEONUSE)
    , _pattern_content_units_set(false)
    , _pattern_transform_set(false)
    , shown(nullptr)
{
    ref.changedSignal().connect(sigc::mem_fun(*this, &SPPattern::_onRefChanged));
}

SPPattern::~SPPattern() = default;

void SPPattern::build(SPDocument *doc, Inkscape::XML::Node *repr)
{
    SPPaintServer::build(doc, repr);

    readAttr(SPAttr::PATTERNUNITS);
    readAttr(SPAttr::PATTERNCONTENTUNITS);
    readAttr(SPAttr::PATTERNTRANSFORM);
    readAttr(SPAttr::X);
    readAttr(SPAttr::Y);
    readAttr(SPAttr::WIDTH);
    readAttr(SPAttr::HEIGHT);
    readAttr(SPAttr::VIEWBOX);
    readAttr(SPAttr::PRESERVEASPECTRATIO);
    readAttr(SPAttr::XLINK_HREF);
    readAttr(SPAttr::STYLE);

    doc->addResource("pattern", this);
}

void SPPattern::release()
{
    if (document) {
        document->removeResource("pattern", this);
    }

    // Should have been unattached by their owners on the release signal.
    assert(attached_views.empty());

    set_shown(nullptr);
    views.clear();

    _modified_connection.disconnect();
    ref.detach();

    SPPaintServer::release();
}

void SPPattern::set(SPAttr key, char const *value)
{
    switch (key) {
        case SPAttr::PATTERNUNITS:
            _pattern_units = UNITS_OBJECTBOUNDINGBOX;
            _pattern_units_set = false;

            if (value) {
                if (!std::strcmp(value, "userSpaceOnUse")) {
                    _pattern_units = UNITS_USERSPACEONUSE;
                    _pattern_units_set = true;
                } else if (!std::strcmp(value, "objectBoundingBox")) {
                    _pattern_units_set = true;
                }
            }

            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;

        case SPAttr::PATTERNCONTENTUNITS:
            _pattern_content_units = UNITS_USERSPACEONUSE;
            _pattern_content_units_set = false;

            if (value) {
                if (!std::strcmp(value, "userSpaceOnUse")) {
                    _pattern_content_units_set = true;
                } else if (!std::strcmp(value, "objectBoundingBox")) {
                    _pattern_content_units = UNITS_OBJECTBOUNDINGBOX;
                    _pattern_content_units_set = true;
                }
            }

            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;

        case SPAttr::PATTERNTRANSFORM: {
            _pattern_transform = Geom::identity();
            _pattern_transform_set = false;

            if (value) {
                Geom::Affine t;
                if (sp_svg_transform_read(value, &t)) {
                    _pattern_transform = t;
                    _pattern_transform_set = true;
                }
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

        case SPAttr::WIDTH:
            _width.readOrUnset(value);
            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;

        case SPAttr::HEIGHT:
            _height.readOrUnset(value);
            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;

        case SPAttr::VIEWBOX:
            set_viewBox(value);
            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
            break;

        case SPAttr::PRESERVEASPECTRATIO:
            set_preserveAspectRatio(value);
            requestModified(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
            break;

        case SPAttr::XLINK_HREF:
            if (!value) {
                if (href.empty()) {
                    break;
                }
                href.clear();
                ref.detach();
            } else {
                if (href == value) {
                    break;
                }
                href = value;

                // Attempt to attach ref, which emits the changed signal.
                try {
                    ref.attach(Inkscape::URI(href.data()));
                } catch (Inkscape::BadURIException const &e) {
                    g_warning("%s", e.what());
                    href.clear();
                    ref.detach();
                }
            }
            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;

        default:
            SPPaintServer::set(key, value);
            break;
    }
}

void SPPattern::update(SPCtx *ctx, unsigned flags)
{
    auto const cflags = cascade_flags(flags);

    for (auto c : childList(true)) {
        if (cflags || (c->uflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            c->updateDisplay(ctx, cflags);
        }
        sp_object_unref(c, nullptr);
    }

    for (auto &v : views) {
        update_view(v);
    }
}

void SPPattern::update_view(View &v)
{
    // * "width" and "height" determine tile size.
    // * "viewBox" (if defined) or "patternContentUnits" determines placement of content inside tile.
    // * "x", "y", and "patternTransform" transform tile to user space after tile is generated.

    // These functions recursively search up the tree to find the values.
    double tile_x = x();
    double tile_y = y();
    double tile_width = width();
    double tile_height = height();
    if (v.bbox && patternUnits() == UNITS_OBJECTBOUNDINGBOX) {
        tile_x *= v.bbox->width();
        tile_y *= v.bbox->height();
        tile_width *= v.bbox->width();
        tile_height *= v.bbox->height();
    }

    // Pattern size in pattern space
    auto pattern_tile = Geom::Rect::from_xywh(0, 0, tile_width, tile_height);

    // Content to tile (pattern space)
    Geom::Affine content2ps;
    if (auto effective_view_box = viewbox()) {
        // viewBox to pattern server (using SPViewBox)
        viewBox = *effective_view_box;
        c2p.setIdentity();
        apply_viewbox(pattern_tile);
        content2ps = c2p;
    }
    else {
        // Content to bbox
        if (v.bbox && patternContentUnits() == UNITS_OBJECTBOUNDINGBOX) {
            content2ps = Geom::Affine(v.bbox->width(), 0.0, 0.0, v.bbox->height(), 0, 0);
        }
    }

    // Tile (pattern space) to user.
    Geom::Affine ps2user = Geom::Translate(tile_x, tile_y) * getTransform();

    v.drawingitem->setTileRect(pattern_tile);
    v.drawingitem->setChildTransform(content2ps);
    v.drawingitem->setPatternToUserTransform(ps2user);
}

void SPPattern::modified(unsigned flags)
{
    auto const cflags = cascade_flags(flags);

    for (auto c : childList(true)) {
        if (auto lpeitem = cast<SPLPEItem>(c)) {
            sp_lpe_item_enable_path_effects(lpeitem, false);
        }
        if (cflags || (c->mflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            c->emitModified(cflags);
        }
        sp_object_unref(c);
    }

    set_shown(rootPattern());
}

// The following three functions are based on SPGroup.

void SPPattern::child_added(Inkscape::XML::Node *child, Inkscape::XML::Node *ref)
{
    SPPaintServer::child_added(child, ref);

    auto last_child = lastChild();
    if (last_child && last_child->getRepr() == child) {
        if (auto item = cast<SPItem>(last_child)) {
            for (auto &v : attached_views) {
                auto ac = item->invoke_show(v.drawingitem->drawing(), v.key, SP_ITEM_SHOW_DISPLAY);
                if (ac) {
                    v.drawingitem->appendChild(ac);
                }
            }
        }
    } else {
        if (auto item = cast<SPItem>(get_child_by_repr(child))) {
            unsigned position = item->pos_in_parent();
            for (auto &v : attached_views) {
                auto ac = item->invoke_show(v.drawingitem->drawing(), v.key, SP_ITEM_SHOW_DISPLAY);
                if (ac) {
                    v.drawingitem->prependChild(ac);
                    ac->setZOrder(position);
                }
            }
        }
    }

    requestModified(SP_OBJECT_MODIFIED_FLAG);
}

void SPPattern::remove_child(Inkscape::XML::Node *child)
{
    SPPaintServer::remove_child(child);
    // no need to do anything as child will automatically remove itself
    requestModified(SP_OBJECT_MODIFIED_FLAG);
}

void SPPattern::order_changed(Inkscape::XML::Node *child, Inkscape::XML::Node *old_prev, Inkscape::XML::Node *new_prev)
{
    SPPaintServer::order_changed(child, old_prev, new_prev);

    if (auto item = cast<SPItem>(get_child_by_repr(child))) {
        unsigned position = item->pos_in_parent();
        for (auto &v : attached_views) {
            auto ac = item->get_arenaitem(v.key);
            ac->setZOrder(position);
        }
    }

    requestModified(SP_OBJECT_MODIFIED_FLAG);
}

void SPPattern::_onRefChanged(SPObject *old_ref, SPObject *ref)
{
    if (old_ref) {
        _modified_connection.disconnect();
    }

    if (is<SPPattern>(ref)) {
        _modified_connection = ref->connectModified(sigc::mem_fun(*this, &SPPattern::_onRefModified));
    }

    _onRefModified(ref, 0);
}

void SPPattern::_onRefModified(SPObject */*ref*/, unsigned /*flags*/)
{
    requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void SPPattern::set_shown(SPPattern *new_shown)
{
    if (shown == new_shown) {
        return;
    }

    if (shown) {
        for (auto &v : views) {
            shown->unattach_view(v.drawingitem.get());
        }

        shown_released_connection.disconnect();
    }

    shown = new_shown;

    if (shown) {
        for (auto &v : views) {
            shown->attach_view(v.drawingitem.get(), v.key);
        }

        shown_released_connection = shown->connectRelease([this] (auto) {
            set_shown(nullptr);
        });
    }
}

void SPPattern::attach_view(Inkscape::DrawingPattern *di, unsigned key)
{
    attached_views.push_back({di, key});

    for (auto &c : children) {
        if (auto child = cast<SPItem>(&c)) {
            auto item = child->invoke_show(di->drawing(), key, SP_ITEM_SHOW_DISPLAY);
            di->appendChild(item);

            if (auto image = cast<Inkscape::DrawingImage>(item)) {
                // Avoid gaps between pattern images
                image->setExtend(CAIRO_EXTEND_PAD);
            }
        }
    }
}

void SPPattern::unattach_view(Inkscape::DrawingPattern *di)
{
    auto it = std::find_if(attached_views.begin(), attached_views.end(), [di] (auto const &v) {
        return v.drawingitem == di;
    });
    assert(it != attached_views.end());

    for (auto &c : children) {
        if (auto child = cast<SPItem>(&c)) {
            child->invoke_hide(it->key);
        }
    }

    attached_views.erase(it);
}

unsigned SPPattern::_countHrefs(SPObject *o) const
{
    if (!o)
        return 1;

    guint i = 0;

    SPStyle *style = o->style;
    if (style && style->fill.isPaintserver() && is<SPPattern>(SP_STYLE_FILL_SERVER(style)) &&
        cast<SPPattern>(SP_STYLE_FILL_SERVER(style)) == this) {
        i++;
    }
    if (style && style->stroke.isPaintserver() && is<SPPattern>(SP_STYLE_STROKE_SERVER(style)) &&
        cast<SPPattern>(SP_STYLE_STROKE_SERVER(style)) == this) {
        i++;
    }

    for (auto& child: o->children) {
        i += _countHrefs(&child);
    }

    return i;
}

SPPattern *SPPattern::_chain() const
{
    Inkscape::XML::Document *xml_doc = document->getReprDoc();
    Inkscape::XML::Node *defsrepr = document->getDefs()->getRepr();

    Inkscape::XML::Node *repr = xml_doc->createElement("svg:pattern");
    repr->setAttribute("inkscape:collect", "always");
    Glib::ustring parent_ref = Glib::ustring::compose("#%1", getRepr()->attribute("id"));
    Inkscape::setHrefAttribute(*repr, parent_ref);
    // this attribute is used to express uniform pattern scaling in pattern editor, so keep it
    repr->setAttribute("preserveAspectRatio", getRepr()->attribute("preserveAspectRatio"));

    defsrepr->appendChild(repr);
    SPObject *child = document->getObjectByRepr(repr);
    assert(child == document->getObjectById(repr->attribute("id")));
    g_assert(is<SPPattern>(child));

    return cast<SPPattern>(child);
}

SPPattern *SPPattern::clone_if_necessary(SPItem *item, const gchar *property)
{
    SPPattern *pattern = this;
    if (pattern->href.empty() || pattern->hrefcount > _countHrefs(item)) {
        pattern = _chain();
        Glib::ustring href = Glib::ustring::compose("url(#%1)", pattern->getRepr()->attribute("id"));

        SPCSSAttr *css = sp_repr_css_attr_new();
        sp_repr_css_set_property(css, property, href.c_str());
        sp_repr_css_change_recursive(item->getRepr(), css, "style");
    }
    return pattern;
}

// do not remove identity transform in pattern elements; when patterns are referenced then linking
// pattern transform overrides root/referenced pattern transform; if it disappears then root transform
// takes over and that's not what we want
static std::string write_transform(const Geom::Affine& transform) {
    if (transform.isIdentity()) {
        return "scale(1)";
    }
    return sp_svg_transform_write(transform);
}

void SPPattern::transform_multiply(Geom::Affine postmul, bool set)
{
    // this formula is for a different interpretation of pattern transforms as described in (*) in sp-pattern.cpp
    // for it to work, we also need    sp_object_read_attr( item, "transform");
    // pattern->patternTransform = premul * item->transform * pattern->patternTransform * item->transform.inverse() *
    // postmul;

    // otherwise the formula is much simpler
    if (set) {
        _pattern_transform = postmul;
    }
    else {
        _pattern_transform = getTransform() * postmul;
    }
    _pattern_transform_set = true;

    setAttributeOrRemoveIfEmpty("patternTransform", write_transform(_pattern_transform));
}

char const *SPPattern::produce(std::vector<Inkscape::XML::Node*> const &reprs, Geom::Rect const &bounds,
                               SPDocument *document, Geom::Affine const &transform, Geom::Affine const &move)
{
    Inkscape::XML::Document *xml_doc = document->getReprDoc();
    Inkscape::XML::Node *defsrepr = document->getDefs()->getRepr();

    Inkscape::XML::Node *repr = xml_doc->createElement("svg:pattern");
    repr->setAttribute("patternUnits", "userSpaceOnUse");
    repr->setAttributeSvgDouble("width", bounds.dimensions()[Geom::X]);
    repr->setAttributeSvgDouble("height", bounds.dimensions()[Geom::Y]);
    repr->setAttributeOrRemoveIfEmpty("patternTransform", write_transform(transform));
    // by default use uniform scaling
    repr->setAttribute("preserveAspectRatio", "xMidYMid");
    defsrepr->appendChild(repr);
    const gchar *pd = repr->attribute("id");
    SPObject *pat_object = document->getObjectById(pd);
    bool can_colorize = false;

    for (auto node : reprs) {
        auto copy = cast<SPItem>(pat_object->appendChildRepr(node));

        if (!repr->attribute("inkscape:label") && node->attribute("inkscape:label")) {
            repr->setAttribute("inkscape:label", node->attribute("inkscape:label"));
        }

        // if some elements have undefined color or solid black, then their fill color is customizable
        if (copy->style && copy->style->isSet(SPAttr::FILL)) {
            if (auto paint = copy->style->getFillOrStroke(true)) {
                if (paint->isColor() && paint->getColor().toRGBA() == 255) { // black color set?
                    can_colorize = true;
                    // remove black fill, it will be inherited from pattern
                    paint->clear();
                }
            }
        }
        else {
            // no color - it will be inherited
            can_colorize = true;
        }

        Geom::Affine dup_transform;
        if (!sp_svg_transform_read(node->attribute("transform"), &dup_transform))
            dup_transform = Geom::identity();
        dup_transform *= move;

        copy->doWriteTransform(dup_transform, nullptr, false);
    }

    if (can_colorize && pat_object->style) {
        // add black fill style to the pattern object - it will tell pattern editor to enable color selector
        pat_object->style->readIfUnset(SPAttr::FILL, "black");
    }

    Inkscape::GC::release(repr);
    return pd;
}

SPPattern const *SPPattern::rootPattern() const
{
    for (auto p = this; p; p = p->ref.getObject()) {
        if (p->firstChild()) { // find the first one with children
            return p;
        }
    }
    return this; // document is broken, we can't get to root; but at least we can return ourself which is supposedly a valid pattern
}

SPPattern *SPPattern::rootPattern()
{
    return const_cast<SPPattern*>(std::as_const(*this).rootPattern());
}

// Access functions that look up fields up the chain of referenced patterns and return the first one which is set

SPPattern::PatternUnits SPPattern::patternUnits() const
{
    for (auto p = this; p; p = p->ref.getObject()) {
        if (p->_pattern_units_set)
            return p->_pattern_units;
    }
    return _pattern_units;
}

SPPattern::PatternUnits SPPattern::patternContentUnits() const
{
    for (auto p = this; p; p = p->ref.getObject()) {
        if (p->_pattern_content_units_set)
            return p->_pattern_content_units;
    }
    return _pattern_content_units;
}

Geom::Affine const &SPPattern::getTransform() const
{
    for (auto p = this; p; p = p->ref.getObject()) {
        if (p->_pattern_transform_set)
            return p->_pattern_transform;
    }
    return _pattern_transform;
}

const Geom::Affine& SPPattern::get_this_transform() const {
    return _pattern_transform;
}

double SPPattern::x() const
{
    for (auto p = this; p; p = p->ref.getObject()) {
        if (p->_x._set)
            return p->_x.computed;
    }
    return 0;
}

double SPPattern::y() const
{
    for (auto p = this; p; p = p->ref.getObject()) {
        if (p->_y._set)
            return p->_y.computed;
    }
    return 0;
}

double SPPattern::width() const
{
    for (auto p = this; p; p = p->ref.getObject()) {
        if (p->_width._set)
            return p->_width.computed;
    }
    return 0;
}

double SPPattern::height() const
{
    for (auto p = this; p; p = p->ref.getObject()) {
        if (p->_height._set)
            return p->_height.computed;
    }
    return 0;
}

Geom::OptRect SPPattern::viewbox() const
{
    Geom::OptRect viewbox;
    for (auto p = this; p; p = p->ref.getObject()) {
        if (p->viewBox_set) {
            viewbox = p->viewBox;
            break;
        }
    }
    return viewbox;
}

bool SPPattern::hasItemChildren() const
{
    return std::any_of(children.begin(), children.end(),
                       [](SPObject const &child) -> bool { return is<SPItem>(&child); });
}

bool SPPattern::isValid() const
{
    return width() > 0 && height() > 0;
}

Inkscape::DrawingPattern *SPPattern::show(Inkscape::Drawing &drawing, unsigned key, Geom::OptRect const &bbox)
{
    views.emplace_back(make_drawingitem<Inkscape::DrawingPattern>(drawing), bbox, key);
    auto &v = views.back();
    auto root = v.drawingitem.get();

    if (shown) {
        shown->attach_view(root, key);
    }

    root->setStyle(style);

    update_view(v);

    return root;
}

void SPPattern::hide(unsigned key)
{
    auto it = std::find_if(views.begin(), views.end(), [=] (auto &v) {
        return v.key == key;
    });

    if (it == views.end()) {
        return;
    }

    if (shown) {
        shown->unattach_view(it->drawingitem.get());
    }

    views.erase(it);
}

void SPPattern::setBBox(unsigned key, Geom::OptRect const &bbox)
{
    auto it = std::find_if(views.begin(), views.end(), [=] (auto &v) {
        return v.key == key;
    });
    assert(it != views.end());
    auto &v = *it;

    v.bbox = bbox;
    update_view(v);
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

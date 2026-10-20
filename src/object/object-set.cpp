// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Multiindex container for selection
 *
 * Authors:
 *   Adrian Boguszewski
 *
 * Copyright (C) 2016 Adrian Boguszewski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "object-set.h"

#include <glib.h>
#include <sigc++/sigc++.h>

#include "box3d.h"
#include "persp3d.h"
#include "preferences.h"

#include "desktop.h"
#include "document.h"

namespace Inkscape {

ObjectSet::ObjectSet(SPDesktop *desktop)
    : _desktop(desktop)
{
    if (desktop) {
        _document = desktop->getDocument();
    }
}

bool ObjectSet::add(SPObject *object, bool nosignal)
{
    g_return_val_if_fail(object != nullptr, false);

    // any ancestor is in the set - do nothing
    if (_anyAncestorIsInSet(object)) {
        return false;
    }

    // very nice function, but changes selection behavior (probably needs new selection option to deal with it)
    // check if there is mutual ancestor for some elements, which can replace all of them in the set
//    object = _getMutualAncestor(object);

    // remove all descendants from the set
    _removeDescendantsFromSet(object);

    _add(object);
    if (!nosignal)
        _emitChanged();
    return true;
}

void ObjectSet::add(XML::Node *repr)
{
    if (document() && repr) {
        SPObject *obj = document()->getObjectByRepr(repr);
        assert(obj == document()->getObjectById(repr->attribute("id")));
        add(obj);
    }
}

bool ObjectSet::remove(SPObject* object) {
    g_return_val_if_fail(object != nullptr, false);

    // object is the top of subtree
    if (includes(object)) {
        _remove(object);
        _emitChanged();
        return true;
    }

    // any ancestor of object is in the set
    if (_anyAncestorIsInSet(object)) {
        _removeAncestorsFromSet(object);
        _emitChanged();
        return true;
    }

    // no object nor any parent in the set
    return false;
}

void ObjectSet::_emitChanged(bool persist_selection_context /*= false*/) {
    _sibling_state.clear();
}

bool ObjectSet::includes(SPObject *object, bool anyAncestor) {
    g_return_val_if_fail(object != nullptr, false);
    if (anyAncestor) {
        return _anyAncestorIsInSet(object);
    } else {
        return _container.get<hashed>().find(object) != _container.get<hashed>().end();
    }
}

bool ObjectSet::includes(Inkscape::XML::Node *node, bool anyAncestor)
{
    if (node) {
        return includes(document()->getObjectByRepr(node), anyAncestor);
    }
    return false;
}

SPObject * 
ObjectSet::includesAncestor(SPObject *object) {
    g_return_val_if_fail(object != nullptr, nullptr);
    SPObject* o = object;
    while (o != nullptr) {
        if (includes(o)) {
            return o;
        }
        o = o->parent;
    }
    return nullptr;
}

void ObjectSet::clear() {
    _clear();
    _emitChanged();
}

int ObjectSet::size() {
    return _container.size();
}

bool ObjectSet::_anyAncestorIsInSet(SPObject *object) {
    SPObject* o = object;
    while (o != nullptr) {
        if (includes(o)) {
            return true;
        }
        o = o->parent;
    }

    return false;
}

void ObjectSet::_removeDescendantsFromSet(SPObject *object) {
    for (auto& child: object->children) {
        if (includes(&child)) {
            _remove(&child);
            // there is certainly no children of this child in the set
            continue;
        }

        _removeDescendantsFromSet(&child);
    }
}

void ObjectSet::_disconnect(SPObject *object) {
    _releaseConnections[object].disconnect();
    _releaseConnections.erase(object);
    _remove3DBoxesRecursively(object);
    _releaseSignals(object);
}

void ObjectSet::_remove(SPObject *object) {
    _disconnect(object);
    _container.get<hashed>().erase(object);
}

void ObjectSet::_add(SPObject *object) {
    _releaseConnections[object] =
        object->connectRelease(sigc::hide_return(sigc::mem_fun(*this, static_cast<bool (ObjectSet::*)(SPObject*)>(&ObjectSet::remove))));
    _container.push_back(object);
    _add3DBoxesRecursively(object);
    _connectSignals(object);
}

void ObjectSet::_clear() {
    for (auto object: _container)
        _disconnect(object);
    _container.clear();
}

SPObject *ObjectSet::_getMutualAncestor(SPObject *object) {
    SPObject *o = object;

    bool flag = true;
    while (o->parent != nullptr) {
        for (auto &child: o->parent->children) {
            if(&child != o && !includes(&child)) {
                flag = false;
                break;
            }
        }
        if (!flag) {
            break;
        }
        o = o->parent;
    }
    return o;
}

void ObjectSet::_removeAncestorsFromSet(SPObject *object) {
    SPObject* o = object;
    while (o->parent != nullptr) {
        for (auto &child: o->parent->children) {
            if (&child != o) {
                _add(&child);
            }
        }
        if (includes(o->parent)) {
            _remove(o->parent);
            break;
        }
        o = o->parent;
    }
}

ObjectSet::~ObjectSet() {
    _clear();
}

void ObjectSet::toggle(SPObject *obj) {
    if (includes(obj)) {
        remove(obj);
    } else {
        add(obj);
    }
}

bool ObjectSet::isEmpty() {
    return _container.empty();
}

SPObject *ObjectSet::single() {
    return _container.size() == 1 ? _container.front() : nullptr;
}

SPItem *ObjectSet::singleItem() {
    return _container.size() == 1 ? cast<SPItem>(_container.front()) : nullptr;
}

SPItem *ObjectSet::firstItem() const
{
    return !_container.empty() ? cast<SPItem>(_container.front()) : nullptr;
}

SPItem *ObjectSet::lastItem() const
{
    return !_container.empty() ? cast<SPItem>(_container.back()) : nullptr;
}

SPItem *ObjectSet::smallestItem(CompareSize compare) {
    return _sizeistItem(true, compare);
}

SPItem *ObjectSet::largestItem(CompareSize compare) {
    return _sizeistItem(false, compare);
}

SPItem *ObjectSet::_sizeistItem(bool sml, CompareSize compare) {
    auto items = this->items();
    gdouble max = sml ? 1e18 : 0;
    SPItem *ist = nullptr;

    for (auto *item : items) {
        Geom::OptRect obox = item->documentPreferredBounds();
        if (!obox || obox.empty()) {
            continue;
        }

        Geom::Rect bbox = *obox;

        gdouble size = compare == AREA ? bbox.area() :
                       (compare == VERTICAL ? bbox.height() : bbox.width());
        size = sml ? size : size * -1;
        if (size < max) {
            max = size;
            ist = item;
        }
    }

    return ist;
}

Inkscape::XML::Node *ObjectSet::singleRepr() {
    SPObject *obj = single();
    return obj ? obj->getRepr() : nullptr;
}

Inkscape::XML::Node *ObjectSet::topRepr() const
{
    auto nodes = const_cast<ObjectSet *>(this)->xmlNodes();

    if (nodes.empty()) {
        return nullptr;
    }

#ifdef _LIBCPP_VERSION
    // workaround for
    // static_assert(__is_cpp17_forward_iterator<_ForwardIterator>::value
    auto n = std::vector<Inkscape::XML::Node *>(nodes.begin(), nodes.end());
#else
    auto &n = nodes;
#endif

    return *std::max_element(n.begin(), n.end(), sp_repr_compare_position_bool);
}

void ObjectSet::set(SPObject *object, bool persist_selection_context) {
    _clear();
    _add(object);
    _emitChanged(persist_selection_context);
}

void ObjectSet::set(XML::Node *repr)
{
    if (document() && repr) {
        SPObject *obj = document()->getObjectByRepr(repr);
        assert(obj == document()->getObjectById(repr->attribute("id")));
        set(obj);
    }
}

void ObjectSet::setReprList(std::vector<XML::Node*> const &list) {
    if(!document())
        return;
    clear();
    for (auto iter = list.rbegin(); iter != list.rend(); ++iter) {
#if 0
        // This can fail when pasting a clone into a new document
        SPObject *obj = document()->getObjectByRepr(*iter);
        assert(obj == document()->getObjectById((*iter)->attribute("id")));
#else
        SPObject *obj = document()->getObjectById((*iter)->attribute("id"));
#endif
        if (obj) {
            add(obj, true);
        }
    }
    _emitChanged();
}

void ObjectSet::enforceIds()
{
    bool idAssigned = false;
    auto items = this->items();
    for (auto *item : items) {
        if (!item->getId()) {
            // Selected object does not have an ID, so assign it a unique ID
            auto id = item->generate_unique_id();
            item->setAttribute("id", id);
            idAssigned = true;
        }
    }
    if (idAssigned) {
        SPDocument *document = _desktop->getDocument();
        if (document) {
            document->setModifiedSinceSave(true);
        }
    }
}

Geom::OptRect ObjectSet::bounds(SPItem::BBoxType type) const
{
    return (type == SPItem::GEOMETRIC_BBOX) ?
           geometricBounds() : visualBounds();
}

Geom::OptRect ObjectSet::geometricBounds() const
{
    auto items = const_cast<ObjectSet *>(this)->items();

    Geom::OptRect bbox;
    for (auto *item : items) {
        bbox.unionWith(item->desktopGeometricBounds());
    }
    return bbox;
}

Geom::OptRect ObjectSet::visualBounds() const
{
    auto items = const_cast<ObjectSet *>(this)->items();

    Geom::OptRect bbox;
    for (auto *item : items) {
        bbox.unionWith(item->desktopVisualBounds());
    }
    return bbox;
}

Geom::OptRect ObjectSet::strokedBounds() const
{
    auto items = const_cast<ObjectSet *>(this)->items();

    Geom::OptRect bbox;
    for (auto *item : items) {
        bbox.unionWith(item->visualBounds(item->i2doc_affine(), false, true, true));
    }
    if (bbox) {
        *bbox *= _desktop->getDocument()->doc2dt();
    }
    return bbox;
}

Geom::OptRect ObjectSet::preferredBounds() const
{
    if (Inkscape::Preferences::get()->getInt("/tools/bounding_box") == 0) {
        return bounds(SPItem::VISUAL_BBOX);
    } else {
        return bounds(SPItem::GEOMETRIC_BBOX);
    }
}

Geom::OptRect ObjectSet::documentBounds(SPItem::BBoxType type) const
{
    Geom::OptRect bbox;
    auto items = const_cast<ObjectSet *>(this)->items();
    if (items.empty()) return bbox;

    for (auto *item : items) {
        bbox |= item->documentBounds(type);
    }

    return bbox;
}

Geom::OptRect ObjectSet::documentPreferredBounds() const
{
    if (Inkscape::Preferences::get()->getInt("/tools/bounding_box") == 0) {
        return documentBounds(SPItem::VISUAL_BBOX);
    } else {
        return documentBounds(SPItem::GEOMETRIC_BBOX);
    }
}

// If we have a selection of multiple items, then the center of the first item
// will be returned; this is also the case in SelTrans::centerRequest()
std::optional<Geom::Point> ObjectSet::center() const {
    auto items = const_cast<ObjectSet *>(this)->items();
    if (!items.empty()) {
        SPItem *first = items.back(); // from the first item in selection
        if (first->isCenterSet()) { // only if set explicitly
            return first->getCenter();
        }
    }
    Geom::OptRect bbox = preferredBounds();
    if (bbox) {
        return bbox->midpoint();
    } else {
        return std::optional<Geom::Point>();
    }
}

std::list<Persp3D *> const ObjectSet::perspList() {
    std::list<Persp3D *> pl;
    for (auto & _3dboxe : _3dboxes) {
        Persp3D *persp = _3dboxe->get_perspective();
        if (std::find(pl.begin(), pl.end(), persp) == pl.end())
            pl.push_back(persp);
    }
    return pl;
}

std::list<SPBox3D *> const ObjectSet::box3DList(Persp3D *persp) {
    std::list<SPBox3D *> boxes;
    if (persp) {
        for (auto box : _3dboxes) {
            if (persp == box->get_perspective()) {
                boxes.push_back(box);
            }
        }
    } else {
        boxes = _3dboxes;
    }
    return boxes;
}

void ObjectSet::_add3DBoxesRecursively(SPObject *obj) {
    std::list<SPBox3D *> boxes = SPBox3D::extract_boxes(obj);

    for (auto box : boxes) {
        _3dboxes.push_back(box);
    }
}

void ObjectSet::_remove3DBoxesRecursively(SPObject *obj) {
    std::list<SPBox3D *> boxes = SPBox3D::extract_boxes(obj);

    for (auto box : boxes) {
        std::list<SPBox3D *>::iterator b = std::find(_3dboxes.begin(), _3dboxes.end(), box);
        if (b == _3dboxes.end()) {
            g_warning ("Warning! Trying to remove unselected box from selection.");
            return;
        }
        _3dboxes.erase(b);
    }
}

} // namespace Inkscape

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

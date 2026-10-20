// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * The reference corresponding to the inkscape:live-effect attribute
 *
 * Copyright (C) 2007 Johan Engelen
 *
 * Release under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "lpeobject-reference.h"

#include <cstring>

#include "bad-uri-exception.h"
#include "live_effects/effect.h"
#include "live_effects/lpeobject.h"
#include "object/uri.h"
#include "object/sp-lpe-item.h"  // sp_lpe_item_update_patheffect

namespace Inkscape {

namespace LivePathEffect {

static void lpeobjectreference_href_changed(SPObject *old_ref, SPObject *ref, LPEObjectReference *lpeobjref);
static void lpeobjectreference_release_self(SPObject *release, LPEObjectReference *lpeobjref);
static void lpeobjectreference_release_owner(SPObject *release, LPEObjectReference *lpeobjref);

LPEObjectReference::LPEObjectReference(SPObject* i_owner) : URIReference(i_owner)
{
    owner=i_owner;
    lpeobject_href = nullptr;
    lpeobject_repr = nullptr;
    lpeobject = nullptr;
    _changed_connection = changedSignal().connect(sigc::bind(sigc::ptr_fun(lpeobjectreference_href_changed), this)); // listening to myself, this should be virtual instead
    _owner_release_connection = owner->connectRelease(sigc::bind(sigc::ptr_fun(&lpeobjectreference_release_owner), this));

}

LPEObjectReference::~LPEObjectReference()
{
    _owner_release_connection.disconnect(); // to do before unlinking
    _changed_connection.disconnect(); // to do before unlinking
    unlink();
}

bool LPEObjectReference::_acceptObject(SPObject * const obj) const
{
    auto lpobj = cast<LivePathEffectObject>(obj);
    if (lpobj) {
        return URIReference::_acceptObject(obj);
    } else {
        return false;
    }
}

void
LPEObjectReference::link(const char *to)
{
    if (!to || !to[0]) {
        unlink();
    } else {
        if ( !lpeobject_href || ( strcmp(to, lpeobject_href) != 0 ) ) {
            if (lpeobject_href) {
                g_free(lpeobject_href);
            }
            lpeobject_href = g_strdup(to);
            try {
                attach(Inkscape::URI(to));
            } catch (Inkscape::BadURIException &e) {
                /* TODO: Proper error handling as per
                 * http://www.w3.org/TR/SVG11/implnote.html#ErrorProcessing.
                 */
                g_warning("%s", e.what());
                detach();
            }
        }
    }
}

void
LPEObjectReference::unlink()
{
    /* if (lpeobject && lpeobject->isOnClipboard()) {
        return;
    } */
    lpeobject_repr = nullptr;
    lpeobject = nullptr;
    if (lpeobject_href) {
        g_free(lpeobject_href);
        lpeobject_href = nullptr;
    }
    detach();
}

void
LPEObjectReference::start_listening(LivePathEffectObject* to)
{
    if ( to == nullptr ) {
        return;
    }
    lpeobject = to;
    lpeobject_repr = to->getRepr();
    _release_connection = to->connectRelease(sigc::bind(sigc::ptr_fun(&lpeobjectreference_release_self), this));
}

static void
lpeobjectreference_href_changed(SPObject */*old_ref*/, SPObject */*ref*/, LPEObjectReference *lpeobjref)
{
    auto refobj = cast<LivePathEffectObject>( lpeobjref->getObject() );
    if ( refobj ) {
        lpeobjref->start_listening(refobj);
    }
    if (lpeobjref->owner) {
        lpeobjref->owner->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
    }
}

static void
lpeobjectreference_release_self(SPObject */*release*/, LPEObjectReference *lpeobjref)
{
    lpeobjref->unlink();
}

static void
lpeobjectreference_release_owner(SPObject */*release*/, LPEObjectReference *lpeobjref)
{
    lpeobjref->unlink();
    lpeobjref->owner = nullptr;
}

} //namespace LivePathEffect

} // namespace inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "helper-observer.h"

#include "object/sp-object.h"

namespace Inkscape {
namespace XML {

SignalObserver::~SignalObserver()
{
    set(nullptr); // if _oldsel!=nullptr, remove observer and decrease refcount
}

// Add this observer to the SPObject and remove it from any previous object
void SignalObserver::set(SPObject* o)
{
  // XML Tree being used directly in this function in the following code
  //   while it shouldn't be
  // Pointer to object is stored, so refcounting should be increased/decreased
    if(_oldsel) {
        if (_oldsel->getRepr()) {
            _oldsel->getRepr()->removeObserver(*this);
        }
        sp_object_unref(_oldsel);
        _oldsel = nullptr;
    }
    if(o) {
        if (o->getRepr()) {
            o->getRepr()->addObserver(*this);
            sp_object_ref(o);
            _oldsel = o;
        }
    }
}

void SignalObserver::notifyChildAdded(XML::Node&, XML::Node&, XML::Node*)
{ signal_changed()(ChildAdded, {}); }

void SignalObserver::notifyChildRemoved(XML::Node&, XML::Node&, XML::Node*)
{ signal_changed()(ChildRemoved, {}); }

void SignalObserver::notifyChildOrderChanged(XML::Node&, XML::Node&, XML::Node*, XML::Node*)
{ signal_changed()(Order, {}); }

void SignalObserver::notifyContentChanged(XML::Node&, Util::ptr_shared, Util::ptr_shared)
{}

void SignalObserver::notifyAttributeChanged(XML::Node&, GQuark key, Util::ptr_shared, Util::ptr_shared) {
    auto attr = g_quark_to_string(key);
    signal_changed()(Attribute, attr);
}

void SignalObserver::notifyElementNameChanged(Node&, GQuark, GQuark)
{
    signal_changed()(ElementName, {});
}

} //namespace XML
} //namespace Inkscape


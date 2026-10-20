// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_XML_HELPER_OBSERVER
#define SEEN_XML_HELPER_OBSERVER

#include <sigc++/signal.h>

#include "node-observer.h"
#include "node.h"

class SPObject;

namespace Inkscape::XML {

class Node;

// Very simple observer that just emits a signal if anything happens to a node
class SignalObserver : public NodeObserver
{
public:
    ~SignalObserver() override;

    // Add this observer to the SPObject and remove it from any previous object
    void set(SPObject* o);
    void notifyChildAdded(Node&, Node&, Node*) override;
    void notifyChildRemoved(Node&, Node&, Node*) override;
    void notifyChildOrderChanged(Node&, Node&, Node*, Node*) override;
    void notifyContentChanged(Node&, Util::ptr_shared, Util::ptr_shared) override;
    void notifyAttributeChanged(Node&, GQuark, Util::ptr_shared, Util::ptr_shared) override;
    void notifyElementNameChanged(Node&, GQuark, GQuark) override;
    enum Change { ChildAdded, ChildRemoved, Order, Attribute, Content, ElementName };
    sigc::signal<void (Change, const char*)>& signal_changed() { return _signal_changed; }

private:
    sigc::signal<void (Change, const char*)> _signal_changed;
    SPObject *_oldsel = nullptr;
};

} // namespace Inkscape::XML

#endif // SEEN_XML_HELPER_OBSERVER

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :

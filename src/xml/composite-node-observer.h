// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Inkscape::XML::CompositeNodeObserver - combine multiple observers
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Author
 * Copyright 2005 MenTaLguY <mental@rydia.net>
 * 
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_XML_COMPOSITE_NODE_OBSERVER_H
#define SEEN_INKSCAPE_XML_COMPOSITE_NODE_OBSERVER_H

#include "inkgc/gc-alloc.h"
#include "inkgc/gc-managed.h"
#include "xml/node-observer.h"

#include <vector>

namespace Inkscape {

namespace XML {

/**
 * @brief An observer that relays notifications to multiple other observers
 *
 * This special observer keeps a list of other observer objects and sends
 * the notifications it receives to all of them. The implementation of the class
 * allows an observer to remove itself from this object during a method call.
 * For the documentation of callback methods, see NodeObserver.
 */
class CompositeNodeObserver : public NodeObserver, public GC::Managed<> {
public:
    struct ObserverRecord
    {
        explicit ObserverRecord(NodeObserver *o) : observer(o), marked(false) {}

        NodeObserver *observer;
        bool marked; //< if marked for removal
    };
    using ObserverRecordList = std::vector<ObserverRecord, Inkscape::GC::Alloc<ObserverRecord, Inkscape::GC::ATOMIC>>;

    CompositeNodeObserver()
    : _iterating(0), _active_marked(0), _pending_marked(0) {}

    /**
     * @brief Add an observer to the list
     * @param observer The observer object to add
     */
    void add(NodeObserver &observer);
    /**
     * @brief Remove an observer from the list
     * @param observer The observer object to remove
     */
    void remove(NodeObserver &observer);
    
    void notifyChildAdded(Node &node, Node &child, Node *prev) override;

    void notifyChildRemoved(Node &node, Node &child, Node *prev) override;

    void notifyChildOrderChanged(Node &node, Node &child,
                                 Node *old_prev, Node *new_prev) override;

    void notifyContentChanged(Node &node,
                              Util::ptr_shared old_content,
                              Util::ptr_shared new_content) override;

    void notifyAttributeChanged(Node &node, GQuark name,
                                Util::ptr_shared old_value,
                                Util::ptr_shared new_value) override;

    void notifyElementNameChanged(Node& node, GQuark old_name, GQuark new_name) override;

private:
    unsigned _iterating;
    ObserverRecordList _active;
    unsigned _active_marked;
    ObserverRecordList _pending;
    unsigned _pending_marked;

    void _startIteration() { ++_iterating; }
    void _finishIteration();
};

} // namespace XML
} // namespace Inkscape

#endif
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

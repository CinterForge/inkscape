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
 * Inkscape::XML::CompositeNodeObserver - combine multiple observers
 *
 * Copyright 2005 MenTaLguY <mental@rydia.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * See the file COPYING for details.
 *
 */

#include <algorithm>
#include <cstring>
#include <glib.h>

#include "xml/composite-node-observer.h"
#include "debug/event-tracker.h"
#include "debug/simple-event.h"

namespace Inkscape {

namespace XML {

void CompositeNodeObserver::notifyChildAdded(Node &node, Node &child, Node *prev)
{
    _startIteration();
    for (auto & iter : _active)
    {
        if (!iter.marked) {
            iter.observer->notifyChildAdded(node, child, prev);
        }
    }
    _finishIteration();
}

void CompositeNodeObserver::notifyChildRemoved(Node &node, Node &child,
                                                           Node *prev)
{
    _startIteration();
    for (auto & iter : _active)
    {
        if (!iter.marked) {
            iter.observer->notifyChildRemoved(node, child, prev);
        }
    }
    _finishIteration();
}

void CompositeNodeObserver::notifyChildOrderChanged(Node &node, Node &child,
                                                                Node *old_prev,
                                                                Node *new_prev)
{
    _startIteration();
    for (auto & iter : _active)
    {
        if (!iter.marked) {
            iter.observer->notifyChildOrderChanged(node, child, old_prev, new_prev);
        }
    }
    _finishIteration();
}

void CompositeNodeObserver::notifyContentChanged(
    Node &node,
    Util::ptr_shared old_content, Util::ptr_shared new_content
) {
    _startIteration();
    for (auto & iter : _active)
    {
        if (!iter.marked) {
            iter.observer->notifyContentChanged(node, old_content, new_content);
        }
    }
    _finishIteration();
}

void CompositeNodeObserver::notifyAttributeChanged(
    Node &node, GQuark name,
    Util::ptr_shared old_value, Util::ptr_shared new_value
) {
    _startIteration();
    for (auto & iter : _active)
    {
        if (!iter.marked) {
            iter.observer->notifyAttributeChanged(node, name, old_value, new_value);
        }
    }
    _finishIteration();
}

void CompositeNodeObserver::notifyElementNameChanged(Node& node, GQuark old_name, GQuark new_name)
{
    _startIteration();
    for (auto& iter : _active) {
        if (!iter.marked) {
            iter.observer->notifyElementNameChanged(node, old_name, new_name);
        }
    }
    _finishIteration();
}

void CompositeNodeObserver::add(NodeObserver &observer) {
    if (_iterating) {
        _pending.emplace_back(&observer);
    } else {
        _active.emplace_back(&observer);
    }
}

namespace {

typedef CompositeNodeObserver::ObserverRecord ObserverRecord;
typedef CompositeNodeObserver::ObserverRecordList ObserverRecordList;

template <typename ObserverPredicate>
struct unmarked_record_satisfying {
    ObserverPredicate predicate;
    unmarked_record_satisfying(ObserverPredicate p) : predicate(p) {}
    bool operator()(ObserverRecord const &record) {
        return !record.marked && predicate(record.observer);
    }
};

template <typename Predicate>
bool mark_one(ObserverRecordList &observers, unsigned &marked_count,
              Predicate p)
{
    auto found = std::find_if(
        observers.begin(), observers.end(),
        unmarked_record_satisfying<Predicate>(p)
    );

    if ( found != observers.end() ) {
        ++marked_count;
        found->marked = true;
        return true;
    } else {
        return false;
    }
}

template <typename Predicate>
bool remove_one(ObserverRecordList &observers, unsigned &/*marked_count*/,
                Predicate p)
{
    auto found = std::find_if(
        observers.begin(), observers.end(),
        unmarked_record_satisfying<Predicate>(p)
    );

    if ( found != observers.end() ) {
        // for O(1) removal
        if (observers.size() > 3) {
            *found = std::move(observers.back());
            observers.pop_back();
        } else {
            observers.erase(found);
        }
        return true;
    } else {
        return false;
    }
}

bool is_marked(ObserverRecord const &record) { return record.marked; }

void remove_all_marked(ObserverRecordList &observers, unsigned &marked_count)
{
    if (marked_count) {
        g_assert(!observers.empty());

        auto newEnd = std::remove_if(observers.begin(), observers.end(), is_marked);
        observers.erase(newEnd, observers.end());
        marked_count = 0;
    }
}

}

void CompositeNodeObserver::_finishIteration() {
    if (!--_iterating) {
        remove_all_marked(_active, _active_marked);
        remove_all_marked(_pending, _pending_marked);
        _active.insert(_active.end(), _pending.begin(), _pending.end());
        _pending.clear();

        g_assert(_pending.empty());
    }
}

namespace {

struct eql_observer {
    NodeObserver const *observer;
    eql_observer(NodeObserver const *o) : observer(o) {}
    bool operator()(NodeObserver const *other) {
        return observer == other;
    }
};

}

void CompositeNodeObserver::remove(NodeObserver &observer) {
    eql_observer p(&observer);
    if (_iterating) {
        mark_one(_active, _active_marked, p) ||
        mark_one(_pending, _pending_marked, p);
    } else {
        remove_one(_active, _active_marked, p) ||
        remove_one(_pending, _pending_marked, p);
    }
}
    
} // namespace XML
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

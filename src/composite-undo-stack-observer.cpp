// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Heavily inspired by Inkscape::XML::CompositeNodeObserver.
 *
 * Authors:
 * David Yip <yipdw@rose-hulman.edu>
 *
 * Copyright (c) 2005 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <algorithm>

#include "composite-undo-stack-observer.h"
#include "xml/event.h"

namespace Inkscape {

void
CompositeUndoStackObserver::add(UndoStackObserver& observer)
{
	if (!this->_iterating) {
		_active.emplace_back(observer);
	} else {
		_pending.emplace_back(observer);
	}
}

void
CompositeUndoStackObserver::remove(UndoStackObserver& observer)
{
	if (!this->_iterating) {
		// logical-or operator short-circuits
		this->_remove_one(this->_active, observer) || this->_remove_one(this->_pending, observer);
	} else {
		this->_mark_one(this->_active, observer) || this->_mark_one(this->_pending, observer);
	}
}

void
CompositeUndoStackObserver::notifyUndoEvent(Event* log)
{
	this->_lock();
	for (auto &i : _active) {
		if (!i.to_remove) {
			i.issueUndo(log);
		}
	}
	this->_unlock();
}

void
CompositeUndoStackObserver::notifyRedoEvent(Event* log)
{

	this->_lock();
	for (auto &i : _active) {
		if (!i.to_remove) {
			i.issueRedo(log);
		}
	}
	this->_unlock();
}

void
CompositeUndoStackObserver::notifyUndoCommitEvent(Event* log)
{
	this->_lock();
	for (auto &i : _active) {
		if (!i.to_remove) {
			i.issueUndoCommit(log);
		}
	}
	this->_unlock();
}

void
CompositeUndoStackObserver::notifyUndoExpired(Event *log)
{
       _lock();
       for (auto &i : _active) {
               if (!i.to_remove) {
                       i.issueUndoExpired(log);
               }
       }
       _unlock();
}

void
CompositeUndoStackObserver::notifyClearUndoEvent()
{
	this->_lock();
	for (auto &i : _active) {
		if (!i.to_remove) {
			i.issueClearUndo();
		}
	}
	this->_unlock();
}

void
CompositeUndoStackObserver::notifyClearRedoEvent()
{
	this->_lock();
	for (auto &i : _active) {
		if (!i.to_remove) {
			i.issueClearRedo();
		}
	}
	this->_unlock();
}

bool
CompositeUndoStackObserver::_remove_one(UndoObserverRecordList& list, UndoStackObserver& o)
{
	UndoStackObserverRecord eq_comp(o);

	auto i = std::find(list.begin(), list.end(), eq_comp);

	if (i != list.end()) {
		list.erase(i);
		return true;
	} else {
		return false;
	}
}

bool
CompositeUndoStackObserver::_mark_one(UndoObserverRecordList& list, UndoStackObserver& o)
{
	UndoStackObserverRecord eq_comp(o);

	auto i = std::find(list.begin(), list.end(), eq_comp);

	if (i != list.end()) {
		i->to_remove = true;
		return true;
	} else {
		return false;
	}
}

void
CompositeUndoStackObserver::_unlock()
{
	if (!--_iterating) {
		// Remove marked observers
		const auto pred = [](UndoStackObserverRecord const &i) -> bool { return i.to_remove; };

		auto newEnd = std::remove_if(_active.begin(), _active.end(), pred);
		_active.erase(newEnd, _active.end());

		newEnd = std::remove_if(_pending.begin(), _pending.end(), pred);
		_pending.erase(newEnd, _pending.end());

		// Merge pending and active
		_active.insert(_active.end(), _pending.begin(), _pending.end());
		_pending.clear();
	}
}

}

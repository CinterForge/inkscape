// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Gustav Broberg <broberg@kth.se>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (c) 2014 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "event-log.h"

#include <glibmm/i18n.h>

#include "document-undo.h"
#include "document.h"
#include "actions/actions-undo-document.h"

namespace Inkscape {

EventLog::EventModelColumns const &EventLog::getColumns()
{
    static const EventModelColumns columns;
    return columns;
}

EventLog::EventLog(SPDocument *document)
    : _document{document}
    , _event_list_store{Gtk::TreeStore::create(getColumns())}
{
    // add initial pseudo event
    Gtk::TreeRow curr_row = *_event_list_store->append();
    _curr_event = _last_saved = _first_event = _last_event = curr_row.get_iter();
    
    auto &_columns = getColumns();
    curr_row[_columns.description] = _("[No more changes]");
    curr_row[_columns.icon_name] = "document-new";
    curr_row[_columns.child_count] = 0;
}

EventLog::~EventLog() = default;

void EventLog::notifyUndoEvent(Event *log)
{
    if (_blocker.pending()) {
        return;
    }

    auto &_columns = getColumns();

    // make sure the supplied event matches the next undoable event
    g_return_if_fail(_getUndoEvent() && (*(_getUndoEvent()))[_columns.event] == log);

    // if we're on the first child event...
    if (_curr_event->parent() && _curr_event == _curr_event->parent()->children().begin()) {
        // ...back up to the parent
        _curr_event = _curr_event->parent();
        _curr_event_parent = (iterator)nullptr;
    } else {
        --_curr_event;

        // if we're entering a branch, move to the end of it
        if (!_curr_event->children().empty()) {
            _curr_event_parent = _curr_event;
            _curr_event = _curr_event->children().end();
            --_curr_event;
        }
    }

    _checkForVirginity();
    updateUndoVerbs();

    _row_changed.emit();
}

void EventLog::notifyRedoEvent(Event *log)
{
    if (_blocker.pending()) {
        return;
    }

    auto &_columns = getColumns();

    // make sure the supplied event matches the next redoable event
    g_return_if_fail(_getRedoEvent() && (*(_getRedoEvent()))[_columns.event] == log);

    // if we're on a parent event...
    if (!_curr_event->children().empty()) {
        // ...move to its first child
        _curr_event_parent = _curr_event;
        _curr_event = _curr_event->children().begin();
    } else {
        ++_curr_event;

        // if we are about to leave a branch...
        if (_curr_event->parent() && _curr_event == _curr_event->parent()->children().end()) {
            // ...and move to the next event at parent level
            _curr_event = _curr_event->parent();
            _curr_event_parent = (iterator)nullptr;
            ++_curr_event;
        }
    }

    _checkForVirginity();
    updateUndoVerbs();

    _row_changed.emit();
}

void EventLog::notifyUndoCommitEvent(Event *log)
{
    auto icon_name = log->icon_name;

    Gtk::TreeRow curr_row;
    auto &_columns = getColumns();

    // if the new event is of the same type as the previous then create a new branch
    if (icon_name == Glib::ustring{(*_curr_event)[_columns.icon_name]}) {
        if (!_curr_event_parent) {
            _curr_event_parent = _curr_event;
        }
        curr_row = *_event_list_store->append(_curr_event_parent->children());
        (*_curr_event_parent)[_columns.child_count] = _curr_event_parent->children().size() + 1;
    } else {
        curr_row = *_event_list_store->append();
        curr_row[_columns.child_count] = 1;
        _curr_event_parent = iterator{nullptr};
    }

    _curr_event = _last_event = curr_row.get_iter();

    curr_row[_columns.event] = log;
    curr_row[_columns.icon_name] = icon_name;
    curr_row[_columns.description] = log->description;

    _checkForVirginity();
    updateUndoVerbs();

    _row_changed.emit();
}

void EventLog::notifyUndoExpired(Event *log)
{
    auto &columns = getColumns();

    if (_event_list_store->children().size() == 1) {
        return; // Nothing to do, nothing in the undo log
    }

    // We only have to look at one item because we never expire from the middle.
    iterator iter = _event_list_store->children().begin();

    // Skip first item, it's the non-event label
    if (iter == _first_event) {
        iter++;
    }

    assert((*iter)[columns.event] == log);

    iterator to_remove;
    if (iter->children().size() > 0) {
        // Move first child's log to parent as the parent is being deleted
        to_remove = iter->children().begin();

        Event *child_log = (*to_remove)[columns.event];
        Glib::ustring desc = (*to_remove)[columns.description];
        (*iter)[columns.event] = child_log;
        (*iter)[columns.description] = desc;
    } else {
        to_remove = iter;
    }

    // This should never happen as we should never expire undo items from the middle.
    assert(to_remove->children().size() == 0);

    if (auto parent = to_remove->parent()) {
        (*parent)[columns.child_count] = to_remove->parent()->children().size() - 1;
    }

    _event_list_store->erase(to_remove);

    // Tell the user about the forgotten undo stack
    if ((*_first_event)[columns.child_count] == 0) {
        (*_first_event)[columns.description] = _("[Changes forgotten]");
    }
    (*_first_event)[columns.child_count] = (*_first_event)[columns.child_count] + 1;
}

void EventLog::notifyClearUndoEvent()
{
    updateUndoVerbs();
}

void EventLog::notifyClearRedoEvent()
{
    _clearRedo();
    updateUndoVerbs();
}

// Enable/disable undo/redo GUI items.
void EventLog::updateUndoVerbs()
{
    if (_document) {
        enable_undo_actions(_document, static_cast<bool>(_getUndoEvent()), static_cast<bool>(_getRedoEvent()));
    }
}

void EventLog::seekTo(iterator target)
{
    if (_blocker.pending()) {
        return;
    }
    auto guard = _blocker.block();

    assert(target);

    if (_event_list_store->get_path(target) < _event_list_store->get_path(_curr_event)) {

        // An event before the current one has been selected. Undo to the selected event.
        while (_curr_event != target) {
            DocumentUndo::undo(_document);

            if (_curr_event->parent() && _curr_event == _curr_event->parent()->children().begin()) {
                _curr_event = _curr_event->parent();
                _curr_event_parent = {};
            } else {
                --_curr_event;
                if (!_curr_event->children().empty()) {
                    _curr_event_parent = _curr_event;
                    _curr_event = _curr_event->children().end();
                    --_curr_event;
                }
            }
        }

    } else {

        // An event after the current one has been selected. Redo to the selected event.
        while (target != _curr_event) {
            DocumentUndo::redo(_document);

            if (!_curr_event->children().empty()) {
                _curr_event_parent = _curr_event;
                _curr_event = _curr_event->children().begin();
            } else {
                ++_curr_event;
                if (_curr_event->parent() && _curr_event == _curr_event->parent()->children().end()) {
                    _curr_event = _curr_event->parent();
                    ++_curr_event;
                    _curr_event_parent = {};
                }
            }
        }
    }

    assert(_curr_event == target);

    _checkForVirginity();
    updateUndoVerbs();

    _row_changed.emit();
}

EventLog::const_iterator EventLog::_getUndoEvent() const
{
    if (_curr_event == _event_list_store->children().begin()) {
        return const_iterator{nullptr};
    }
    return _curr_event;
}

EventLog::const_iterator EventLog::_getRedoEvent() const
{
    if (_curr_event == _last_event) {
        return const_iterator{nullptr};
    }

    if (!_curr_event->children().empty()) {
        return _curr_event->children().begin();
    }

    auto redo_event = _curr_event;
    ++redo_event;

    if (redo_event->parent() && redo_event == redo_event->parent()->children().end()) {
        redo_event = redo_event->parent();
        ++redo_event;
    }

    return redo_event;
}

void EventLog::_clearRedo()
{
    auto guard = _blocker.block();

    if (_last_event == _curr_event) {
        return;
    }

    auto &_columns = getColumns();

    _last_event = _curr_event;

    if (!_last_event->children().empty()) {
        _last_event = _last_event->children().begin();
    } else {
        ++_last_event;
    }

    while ( _last_event != _event_list_store->children().end() ) {
        if (_last_event->parent()) {
            while (_last_event != _last_event->parent()->children().end()) {
                _last_event = _event_list_store->erase(_last_event);
            }
            _last_event = _last_event->parent();

            (*_last_event)[_columns.child_count] = _last_event->children().size() + 1;

            ++_last_event;
        } else {
            _last_event = _event_list_store->erase(_last_event);
        }
    }
}

// Mark document as untouched if we reach a state where the document was previously saved.
void EventLog::_checkForVirginity()
{
    g_return_if_fail(_document);
    if (_curr_event == _last_saved) {
        _document->setModifiedSinceSave(false);
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

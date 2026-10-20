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

#ifndef INKSCAPE_EVENT_LOG_H
#define INKSCAPE_EVENT_LOG_H

#include <gtkmm/treestore.h>
#include <glibmm/refptr.h>
#include <gtkmm/treeselection.h>
#include <gtkmm/treeview.h>
#include <sigc++/trackable.h>

#include "ui/operation-blocker.h"
#include "undo-stack-observer.h"
#include "event.h"

class SPDocument;

namespace Inkscape {

/**
 * A simple log for maintaining a history of committed, undone and redone events along with their
 * type. It implements the UndoStackObserver and should be registered with a
 * CompositeUndoStackObserver for each document. The event log is then notified on all commit, undo
 * and redo events and will store a representation of them in an internal Gtk::TreeStore.
 *
 * Consecutive events of the same type are grouped with the first event as a parent and following
 * as its children.
 *
 * If a Gtk::TreeView is connected to the event log, the TreeView's selection and its nodes
 * expanded/collapsed state will be updated as events are committed, undone and redone. Whenever
 * this happens, the event log will block the TreeView's callbacks to prevent circular updates.
 */
class EventLog
    : public UndoStackObserver
    , public sigc::trackable
{
public:
    using iterator = Gtk::TreeModel::iterator;
    using const_iterator = Gtk::TreeModel::const_iterator;

    explicit EventLog(SPDocument *document);
    ~EventLog() override;

    EventLog(EventLog &&) = delete;
    EventLog &operator=(EventLog &&) = delete;

    /**
     * Event datatype
     */
    struct EventModelColumns : public Gtk::TreeModelColumnRecord
    {
        Gtk::TreeModelColumn<Event *> event;
        Gtk::TreeModelColumn<Glib::ustring> icon_name;
        Gtk::TreeModelColumn<Glib::ustring> description;
        Gtk::TreeModelColumn<int> child_count;

        EventModelColumns()
        {
            add(event);
            add(icon_name);
            add(description);
            add(child_count);
        }
    };

    // Implementation of Inkscape::UndoStackObserver methods

    /**
     * Modifies the log's entries and the view's selection when triggered.
     */
    void notifyUndoEvent(Event *log) override;
    void notifyRedoEvent(Event *log) override;
    void notifyUndoCommitEvent(Event *log) override;
    void notifyUndoExpired(Event *log) override;
    void notifyClearUndoEvent() override;
    void notifyClearRedoEvent() override;

    // Accessor functions

    Glib::RefPtr<Gtk::TreeModel> getEventListStore() const { return _event_list_store; }
    static const EventModelColumns& getColumns();
    iterator getCurrEvent() const { return _curr_event; }

    void rememberFileSave() { _last_saved = _curr_event; }

    /// Update the sensitivity of undo and redo actions.
    void updateUndoVerbs();

    /// Seek the document to a given item in the undo history.
    void seekTo(iterator target);

    /// Emitted when the current event changed.
    sigc::connection connectRowChanged(sigc::slot<void ()> slot) { return _row_changed.connect(std::move(slot)); }

private:
    SPDocument *_document;       //< document that is logged

    Glib::RefPtr<Gtk::TreeStore> _event_list_store;

    iterator _first_event;       //< first non-event in _event_list_store
    iterator _curr_event;        //< current event in _event_list_store
    iterator _last_event;        //< end position in _event_list_store
    iterator _curr_event_parent{nullptr}; //< parent to current event, if any
    iterator _last_saved;        //< position where last document save occurred

    OperationBlocker _blocker;
    sigc::signal<void ()> _row_changed;

    // Helper functions

    const_iterator _getUndoEvent() const; //< returns the current undoable event or NULL if none
    const_iterator _getRedoEvent() const; //< returns the current redoable event or NULL if none

    void _clearRedo();  //< erase all previously undone events

    void _checkForVirginity(); //< marks the document as untouched if undo/redo reaches a previously saved state
};

} // namespace Inkscape

#endif // INKSCAPE_EVENT_LOG_H

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

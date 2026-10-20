// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Undo/Redo stack implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   MenTaLguY <mental@rydia.net>
 *   Abhishek Sharma
 *
 * Copyright (C) 2007  MenTaLguY <mental@rydia.net>
 * Copyright (C) 1999-2003 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 *
 * Using the split document model gives sodipodi a very simple and clean
 * undo implementation. Whenever mutation occurs in the XML tree,
 * SPObject invokes one of the five corresponding handlers of its
 * container document. This writes down a generic description of the
 * given action, and appends it to the recent action list, kept by the
 * document. There will be as many action records as there are mutation
 * events, which are all kept and processed together in the undo
 * stack. Two methods exist to indicate that the given action is completed:
 *
 * \verbatim
   void sp_document_done( SPDocument *document );
   void sp_document_maybe_done( SPDocument *document, const unsigned char *key ) \endverbatim
 *
 * Both move the recent action list into the undo stack and clear the
 * list afterwards.  While the first method does an unconditional push,
 * the second one first checks the key of the most recent stack entry. If
 * the keys are identical, the current action list is appended to the
 * existing stack entry, instead of pushing it onto its own.  This
 * behaviour can be used to collect multi-step actions (like winding the
 * Gtk spinbutton) from the UI into a single undoable step.
 *
 * For controls implemented by Sodipodi itself, implementing undo as a
 * single step is usually done in a more efficient way. Most controls have
 * the abstract model of grab, drag, release, and change user
 * action. During the grab phase, all modifications are done to the
 * SPObject directly - i.e. they do not change XML tree, and thus do not
 * generate undo actions either.  Only at the release phase (normally
 * associated with releasing the mousebutton), changes are written back
 * to the XML tree, thus generating only a single set of undo actions.
 * (Lauris Kaplinski)
 */

#include "document-undo.h"

#include <glibmm/ustring.h>                 // for ustring, operator==
#include <vector>                           // for vector

#include "document.h"                       // for SPDocument
#include "event.h"                          // for Event
#include "inkscape.h"                       // for Application, INKSCAPE
#include "composite-undo-stack-observer.h"  // for CompositeUndoStackObserver

#include "debug/event-tracker.h"            // for EventTracker
#include "debug/event.h"                    // for Event
#include "debug/simple-event.h"             // for SimpleEvent
#include "debug/timestamp.h"                // for timestamp
#include "object/sp-lpe-item.h"             // for sp_lpe_item_update_pathef...
#include "object/sp-root.h"                 // for SPRoot
#include "preferences.h"
#include "xml/event-fns.h"                  // for sp_repr_begin_transaction

namespace Inkscape::XML {
class Event;
} // namespace Inkscape::XML

/*
 * Undo & redo
 */

void Inkscape::DocumentUndo::setUndoSensitive(SPDocument *doc, bool sensitive)
{
	g_assert (doc != nullptr);

	if ( sensitive == doc->sensitive )
		return;

	if (sensitive) {
		sp_repr_begin_transaction (doc->rdoc);
	} else {
		doc->partial = sp_repr_coalesce_log (
			doc->partial,
			sp_repr_commit_undoable (doc->rdoc)
		);
	}

	doc->sensitive = sensitive;
}

bool Inkscape::DocumentUndo::getUndoSensitive(SPDocument const *document) {
	g_assert(document != nullptr);

	return document->sensitive;
}

void Inkscape::DocumentUndo::done(SPDocument *doc,
                                  Inkscape::Util::Internal::ContextString event_description,
                                  Glib::ustring const &icon_name,
                                  unsigned int object_modified_tag)
{
    if (doc->sensitive) {
        maybeDone(doc, nullptr, event_description, icon_name, object_modified_tag);
    }
}

void Inkscape::DocumentUndo::resetKey( SPDocument *doc )
{
    doc->actionkey.clear();
}

void Inkscape::DocumentUndo::setKeyExpires(SPDocument *doc, double seconds)
{
    doc->action_expires = seconds;
}

namespace {

using Inkscape::Debug::Event;
using Inkscape::Debug::SimpleEvent;
using Inkscape::Debug::timestamp;
using Inkscape::Util::Internal::ContextString;

typedef SimpleEvent<Event::INTERACTION> InteractionEvent;

class CommitEvent : public InteractionEvent {
public:

    CommitEvent(SPDocument *doc, const gchar *key, const gchar* event_description, const gchar *icon_name)
    : InteractionEvent("commit")
    {
        _addProperty("timestamp", timestamp());
        _addProperty("document", doc->serial());

        if (key) {
            _addProperty("merge-key", key);
        }

        if (event_description) {
            _addProperty("description", event_description);
        }

        if (icon_name) {
            _addProperty("icon-name", icon_name);
        }
    }
};

}

// 'key' is used to coalesce changes of the same type.
// 'event_description' and 'icon_name' are used in the Undo History dialog.
void Inkscape::DocumentUndo::maybeDone(SPDocument *doc,
                                       const gchar *key,
                                       ContextString event_description,
                                       Glib::ustring const &icon_name,
                                       unsigned int object_modified_tag)
{
	g_assert (doc != nullptr);
    g_assert (doc->sensitive);
    if ( key && !*key ) {
        g_warning("Blank undo key specified.");
    }

    bool limit_undo = Inkscape::Preferences::get()->getBool("/options/undo/limit");
    auto undo_size = Inkscape::Preferences::get()->getInt("/options/undo/size", 200);

    // Undo size zero will cause crashes when changing the preference during an active document
    assert(undo_size > 0);

    doc->before_commit_signal.emit();
    // This is only used for output to debug log file (and not for undo).
    Inkscape::Debug::EventTracker<CommitEvent> tracker(doc, key, event_description.c_str(), icon_name.c_str());

    doc->collectOrphans();
    doc->ensureUpToDate(object_modified_tag);

    DocumentUndo::clearRedo(doc);

    Inkscape::XML::Event *log = sp_repr_coalesce_log (doc->partial, sp_repr_commit_undoable (doc->rdoc));
    doc->partial = nullptr;

    if (!log) {
        sp_repr_begin_transaction (doc->rdoc);
        return;
    }

    bool expired = doc->undo_timer && std::chrono::steady_clock::now() - *doc->undo_timer > std::chrono::duration<double>(doc->action_expires);
    if (key && !expired && !doc->actionkey.empty() && (doc->actionkey == key) && !doc->undo.empty()) {
        doc->undo.back()->event = sp_repr_coalesce_log(doc->undo.back()->event, log);
    } else {
        Inkscape::Event *event = new Inkscape::Event(log, event_description.c_str(), icon_name);
        doc->undo.push_back(event);
        doc->undoStackObservers.notifyUndoCommitEvent(event);
    }

    if (key) {
        doc->actionkey = key;
        // Action key will expire in 10 seconds by default
        doc->undo_timer = std::chrono::steady_clock::now();
        doc->action_expires = 10.0;
    } else {
        doc->actionkey.clear();
        doc->undo_timer = {};
    }

    doc->virgin = FALSE;
    doc->setModifiedSinceSave();
    sp_repr_begin_transaction (doc->rdoc);
    doc->commit_signal.emit();

    // Keeping the undo stack to a reasonable size is done when we're not maybeDone.
    // Note: Redo does not need the same controls since in theory it should never be
    // able to get larger than the undo size as it's only populated with undo items.
    if (!key) {
        // We remove undo items from the front of the stack
        while (limit_undo && (int)doc->undo.size() > undo_size) {
            Inkscape::Event *e = doc->undo.front();
            doc->undoStackObservers.notifyUndoExpired(e);
            doc->undo.pop_front();
            delete e;
        }
    }
}

void Inkscape::DocumentUndo::cancel(SPDocument *doc)
{
    g_assert (doc != nullptr);
    g_assert (doc->sensitive);
    done(doc, ContextString("undozone"), "");
    // ensure there is something to undo (extension crash can do nothing)
    if (!doc->undo.empty() && doc->undo.back()->description == "undozone") {
        undo(doc);
        clearRedo(doc);
    }
}

// Member function for friend access to SPDocument privates.
void Inkscape::DocumentUndo::finish_incomplete_transaction(SPDocument &doc) {
    Inkscape::XML::Event *log=sp_repr_commit_undoable(doc.rdoc);
    if (log || doc.partial) {
        g_warning ("Incomplete undo transaction (added to next undo):");
        doc.partial = sp_repr_coalesce_log(doc.partial, log);
        if (!doc.undo.empty()) {
            Inkscape::Event* undo_stack_top = doc.undo.back();
            undo_stack_top->event = sp_repr_coalesce_log(undo_stack_top->event, doc.partial);
        } else {
            sp_repr_free_log(doc.partial);
        }
        doc.partial = nullptr;
	}
}

// Member function for friend access to SPDocument privates.
void Inkscape::DocumentUndo::perform_document_update(SPDocument &doc) {
    sp_repr_begin_transaction(doc.rdoc);
    doc.ensureUpToDate();

    Inkscape::XML::Event *update_log=sp_repr_commit_undoable(doc.rdoc);
    doc.emitReconstructionFinish();

    if (update_log != nullptr) {
        g_warning("Document was modified while being updated after undo operation");
        sp_repr_debug_print_log(update_log);

        //Coalesce the update changes with the last action performed by user
        if (!doc.undo.empty()) {
            Inkscape::Event* undo_stack_top = doc.undo.back();
            undo_stack_top->event = sp_repr_coalesce_log(undo_stack_top->event, update_log);
        } else {
            sp_repr_free_log(update_log);
        }
    }
}

gboolean Inkscape::DocumentUndo::undo(SPDocument *doc)
{
    using Inkscape::Debug::EventTracker;
    using Inkscape::Debug::SimpleEvent;

    gboolean ret;

    EventTracker<SimpleEvent<Inkscape::Debug::Event::DOCUMENT> > tracker("undo");
    g_assert (doc != nullptr);
    g_assert (doc->sensitive);

    doc->sensitive = FALSE;
    doc->seeking = true;

    doc->actionkey.clear();

    finish_incomplete_transaction(*doc);
    if (! doc->undo.empty()) {
        Inkscape::Event *log = doc->undo.back();
        doc->undo.pop_back();
        sp_repr_undo_log (log->event);
        perform_document_update(*doc);
        doc->redo.push_back(log);
        doc->setModifiedSinceSave();
        doc->undoStackObservers.notifyUndoEvent(log);
        ret = TRUE;
    } else {
	    ret = FALSE;
    }

    sp_repr_begin_transaction (doc->rdoc);
    doc->update_lpobjs();
    doc->sensitive = TRUE;
    doc->seeking = false;
    return ret;
}

gboolean Inkscape::DocumentUndo::redo(SPDocument *doc)
{
	using Inkscape::Debug::EventTracker;
	using Inkscape::Debug::SimpleEvent;

	gboolean ret;

	EventTracker<SimpleEvent<Inkscape::Debug::Event::DOCUMENT> > tracker("redo");

    g_assert (doc != nullptr);
    g_assert (doc->sensitive);
    doc->sensitive = FALSE;
    doc->seeking = true;
	doc->actionkey.clear();

    finish_incomplete_transaction(*doc);
    if (! doc->redo.empty()) {
        Inkscape::Event *log = doc->redo.back();
		doc->redo.pop_back();
		sp_repr_replay_log (log->event);
        doc->undo.push_back(log);
        perform_document_update(*doc);

        doc->setModifiedSinceSave();
        doc->undoStackObservers.notifyRedoEvent(log);
		ret = TRUE;
	} else {
		ret = FALSE;
	}

	sp_repr_begin_transaction (doc->rdoc);
    doc->update_lpobjs();
	doc->sensitive = TRUE;
    doc->seeking = false;
	if (ret) {
        doc->emitReconstructionFinish();
    }
	return ret;
}

void Inkscape::DocumentUndo::clearUndo(SPDocument *doc)
{
    if (! doc->undo.empty())
        doc->undoStackObservers.notifyClearUndoEvent();
    while (! doc->undo.empty()) {
        Inkscape::Event *e = doc->undo.back();
        doc->undo.pop_back();
        delete e;
    }
}

void Inkscape::DocumentUndo::clearRedo(SPDocument *doc)
{
        if (!doc->redo.empty())
                doc->undoStackObservers.notifyClearRedoEvent();

    while (! doc->redo.empty()) {
        Inkscape::Event *e = doc->redo.back();
        doc->redo.pop_back();
        delete e;
    }
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

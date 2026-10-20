// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief Spellcheck dialog
 */
/* Authors:
 *   bulia byak <bulia@users.sf.net>
 *
 * Copyright (C) 2009 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_DIALOG_SPELLCHECK_H
#define INKSCAPE_UI_DIALOG_SPELLCHECK_H

#include <set>
#include <vector>

#include <libspelling.h>

#include "text-editing.h"
#include "ui/dialog/dialog-base.h"
#include "display/control/canvas-item-ptr.h"
#include "util/gobjectptr.h"
#include <sigc++/scoped_connection.h>

namespace Gtk {
class Builder;
class Button;
class ColumnView;
class DropDown;
class Label;
class SingleSelection;
class StringList;
} // namespace Gtk

class SPObject;
class SPItem;
class SPCanvasItem;

namespace Inkscape {
class Preferences;
class CanvasItemRect;
} // namespace Inkscape {

namespace Inkscape::UI::Dialog {

/**
 *
 * A dialog widget to checking spelling of text elements in the document
 * Uses gspell and one of the languages set in the users preference file
 *
 */
class SpellCheck : public DialogBase
{
public:
    SpellCheck();
    ~SpellCheck() override;

private:
    SpellCheck(Glib::RefPtr<Gtk::Builder> const &builder);

    void documentReplaced() override;

    /**
     * Remove the highlight rectangle form the canvas
     */
    void clearRects();

    /**
     * Release handlers to the selected item
     */
    void disconnect();

    /**
     * Returns a list of all the text items in the SPObject
     */
    void allTextItems(SPObject *r, std::vector<SPItem *> &l, bool hidden, bool locked);

    /**
     * Is text inside the SPOject's tree
     */
    bool textIsValid(SPObject *root, SPItem *text);

    /**
     * Compare the visual bounds of 2 SPItems referred to by a and b
     */
    SPItem *getText(SPObject *root);
    void nextText();

    /**
     * Cleanup after spellcheck is finished
     */
    void finished();

    /**
     * Find the next word to spell check
     */
    bool nextWord();
    void deleteLastRect();
    void doSpellcheck();

    /**
     * Update speller from language combobox
     * @return true if update was successful
     */
    bool updateSpeller();

    /**
     * Accept button clicked
     */
    void onAccept();

    /**
     * Ignore button clicked
     */
    void onIgnore();

    /**
     * Ignore once button clicked
     */
    void onIgnoreOnce();

    /**
     * Add button clicked
     */
    void onAdd();

    /**
     * Stop button clicked
     */
    void onStop();

    /**
     * Start button clicked
     */
    void onStart();

    /**
     * Language selection changed
     */
    void onLanguageChanged();

    /**
     * Selected object modified on canvas
     */
    void onObjModified();

    /**
     * Selected object removed from canvas
     */
    void onObjReleased();

    /**
     * Selection in suggestions text view changed
     */
    void onTreeSelectionChange();

    Preferences &_prefs;

    SPObject *_root = nullptr;

    SpellingProvider *_provider = nullptr;
    Util::GObjectPtr<SpellingChecker> _checker;

    /**
     * list of canvasitems (currently just rects) that mark misspelled things on canvas
     */
    std::vector<CanvasItemPtr<CanvasItemRect>> _rects;

    /**
     * list of text objects we have already checked in this session
     */
    std::set<SPItem *> _seen_objects;

    /**
     *  the object currently being checked
     */
    SPItem *_text = nullptr;

    /**
     * current objects layout
     */
    Text::Layout const *_layout = nullptr;

    /**
     *  iterators for the start and end of the current word
     */
    Text::Layout::iterator _begin_w;
    Text::Layout::iterator _end_w;

    /**
     *  the word we're checking
     */
    Glib::ustring _word;

    /**
     *  counters for the number of stops and dictionary adds
     */
    int _stops = 0;
    int _adds = 0;

    /**
     *  true if we are in the middle of a check
     */
    bool _working = false;

    /**
     *  connect to the object being checked in case it is modified or deleted by user
     */
    sigc::scoped_connection _modified_connection;
    sigc::scoped_connection _release_connection;

    /**
     *  true if the spell checker dialog has changed text, to suppress modified callback
     */
    bool _local_change = false;

    struct LanguagePair
    {
        std::string name;
        std::string code;
    };

    std::vector<LanguagePair> _langs;

    // Dialog widgets
    Gtk::Label &banner_label;
    Gtk::ColumnView &column_view;
    Gtk::Button &accept_button;
    Gtk::Button &ignoreonce_button;
    Gtk::Button &ignore_button;
    Gtk::Button &add_button;
    Gtk::Button &pref_button;
    Gtk::DropDown &dictionary_combo;
    Gtk::Button &stop_button;
    Gtk::Button &start_button;

    Glib::RefPtr<Gtk::StringList> corrections;
    Glib::RefPtr<Gtk::SingleSelection> selection_model;
};

} // namespace Inkscape::UI::Dialog

#endif // INKSCAPE_UI_DIALOG_SPELLCHECK_H

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

// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Spellcheck dialog.
 */
/* Authors:
 *   bulia byak <bulia@users.sf.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2009 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "spellcheck.h"

#include <glibmm/i18n.h>
#include <gtkmm/dropdown.h>
#include <gtkmm/columnview.h>
#include <gtkmm/singleselection.h>
#include <gtkmm/stringlist.h>

#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "inkscape.h"
#include "layer-manager.h"
#include "selection.h"
#include "selection-chemistry.h"
#include "text-editing.h"
#include "display/control/canvas-item-rect.h"
#include "object/sp-defs.h"
#include "object/sp-flowtext.h"
#include "object/sp-object.h"
#include "object/sp-root.h"
#include "object/sp-string.h"
#include "object/sp-text.h"
#include "ui/builder-utils.h"
#include "ui/dialog/dialog-container.h"
#include "ui/dialog/inkscape-preferences.h" // for PREFS_PAGE_SPELLCHECK
#include "ui/icon-names.h"
#include "ui/tools/text-tool.h"
#include "ui/libspelling-wrapper.h"

namespace Inkscape::UI::Dialog {
namespace {

void show_spellcheck_preferences_dialog()
{
    Preferences::get()->setInt("/dialogs/preferences/page", PREFS_PAGE_SPELLCHECK);
    SP_ACTIVE_DESKTOP->getContainer()->new_dialog("Preferences");
}

// Returns a < b
bool compare_bboxes(SPItem const *a, SPItem const *b)
{
    auto bbox1 = a->documentVisualBounds();
    auto bbox2 = b->documentVisualBounds();
    if (!bbox1 || !bbox2) {
        return true;
    }

    // vector between top left corners
    auto diff = bbox1->min() - bbox2->min();

    return diff.y() == 0 ? diff.x() < 0 : diff.y() < 0;
}

} // namespace

SpellCheck::SpellCheck()
    : SpellCheck(UI::create_builder("dialog-spellcheck.ui"))
{}

// Note: Macro can be replaced using cpp2 metaclasses.
#define BUILD(name) name{UI::get_widget<std::remove_reference_t<decltype(name)>>(builder, #name)}

SpellCheck::SpellCheck(Glib::RefPtr<Gtk::Builder> const &builder)
    : DialogBase("/dialogs/spellcheck/", "Spellcheck")
    , _prefs{*Preferences::get()}
    , BUILD(banner_label)
    , BUILD(column_view)
    , BUILD(accept_button)
    , BUILD(ignoreonce_button)
    , BUILD(ignore_button)
    , BUILD(add_button)
    , BUILD(pref_button)
    , BUILD(dictionary_combo)
    , BUILD(stop_button)
    , BUILD(start_button)
{
    append(UI::get_widget<Gtk::Box>(builder, "main_box"));

    _provider = spelling_provider_get_default();
    list_language_names_and_codes(_provider,
                                  [&](auto name, auto code) { _langs.push_back({.name = name, .code = code}); });

    if (_langs.empty()) {
        banner_label.set_markup(Glib::ustring::compose("<i>%1</i>", _("No dictionaries installed")));
    }

    corrections = Gtk::StringList::create();
    selection_model = Gtk::SingleSelection::create(corrections);
    column_view.set_model(selection_model);

    if (!_langs.empty()) {
        auto list = Gtk::StringList::create();
        for (auto const &pair : _langs) {
            list->append(pair.name);
        }
        dictionary_combo.set_model(list);

        auto lookup_lang_code = [this] (Glib::ustring const &code) -> std::optional<int> {
            auto it = std::find_if(begin(_langs), end(_langs), [&] (auto &pair) {
                return pair.code == code.raw();
            });
            if (it == end(_langs)) {
                return {};
            }
            return std::distance(begin(_langs), it);
        };

        // Set previously set language (or the first item)
        dictionary_combo.set_selected(lookup_lang_code(_prefs.getString("/dialogs/spellcheck/lang")).value_or(0));
    }

    /*
     * Signal handlers
     */
    accept_button.signal_clicked().connect(sigc::mem_fun(*this, &SpellCheck::onAccept));
    ignoreonce_button.signal_clicked().connect(sigc::mem_fun(*this, &SpellCheck::onIgnoreOnce));
    ignore_button.signal_clicked().connect(sigc::mem_fun(*this, &SpellCheck::onIgnore));
    add_button.signal_clicked().connect(sigc::mem_fun(*this, &SpellCheck::onAdd));
    start_button.signal_clicked().connect(sigc::mem_fun(*this, &SpellCheck::onStart));
    stop_button.signal_clicked().connect(sigc::mem_fun(*this, &SpellCheck::onStop));
    selection_model->property_selected().signal_changed().connect(sigc::mem_fun(*this, &SpellCheck::onTreeSelectionChange));
    dictionary_combo.property_selected().signal_changed().connect(sigc::mem_fun(*this, &SpellCheck::onLanguageChanged));
    pref_button.signal_clicked().connect(sigc::ptr_fun(show_spellcheck_preferences_dialog));

    column_view.set_sensitive(false);
    accept_button.set_sensitive(false);
    ignore_button.set_sensitive(false);
    ignoreonce_button.set_sensitive(false);
    add_button.set_sensitive(false);
    stop_button.set_sensitive(false);
}

SpellCheck::~SpellCheck() = default;

void SpellCheck::documentReplaced()
{
    if (_working) {
        // Stop and start on the new desktop
        finished();
        onStart();
    }
}

void SpellCheck::clearRects()
{
    _rects.clear();
}

void SpellCheck::disconnect()
{
    _release_connection.disconnect();
    _modified_connection.disconnect();
}

void SpellCheck::allTextItems(SPObject *r, std::vector<SPItem *> &l, bool hidden, bool locked)
{
    if (is<SPDefs>(r)) {
        return; // we're not interested in items in defs
    }

    if (!std::strcmp(r->getRepr()->name(), "svg:metadata")) {
        return; // we're not interested in metadata
    }

    if (auto desktop = getDesktop()) {
        for (auto &child: r->children) {
            if (auto item = cast<SPItem>(&child)) {
                if (!child.cloned && !desktop->layerManager().isLayer(item)) {
                    if ((hidden || !desktop->itemIsHidden(item)) && (locked || !item->isLocked())) {
                        if (is<SPText>(item) || is<SPFlowtext>(item)) {
                            l.push_back(item);
                        }
                    }
                }
            }
            allTextItems(&child, l, hidden, locked);
        }
    }
}

bool SpellCheck::textIsValid(SPObject *root, SPItem *text)
{
    std::vector<SPItem *> l;
    allTextItems(root, l, false, true);
    return std::find(l.begin(), l.end(), text) != l.end();
}

// We regenerate and resort the list every time, because user could have changed it while the
// dialog was waiting
SPItem *SpellCheck::getText(SPObject *root)
{
    std::vector<SPItem *> l;
    allTextItems(root, l, false, true);
    std::sort(l.begin(), l.end(), compare_bboxes);

    for (auto item : l) {
        if (_seen_objects.insert(item).second) {
            return item;
        }
    }

    return nullptr;
}

void SpellCheck::nextText()
{
    disconnect();

    _text = getText(_root);
    if (_text) {
        _modified_connection = _text->connectModified([this] (auto, auto) { onObjModified(); });
        _release_connection = _text->connectRelease([this] (auto) { onObjReleased(); });

        _layout = te_get_layout(_text);
        _begin_w = _layout->begin();
    }

    _end_w = _begin_w;
    _word.clear();
}

bool SpellCheck::updateSpeller()
{
    _checker.reset();

    auto i = dictionary_combo.get_selected();
    if (i != GTK_INVALID_LIST_POSITION) {
        _checker = GObjectPtr(spelling_checker_new(_provider, _langs.at(i).code.c_str()));
    }

    return !!_checker;
}

void SpellCheck::onStart()
{
    if (!getDocument())
        return;

    start_button.set_sensitive(false);

    _stops = 0;
    _adds = 0;
    clearRects();

    if (!updateSpeller())
        return;

    _root = getDocument()->getRoot();

    // empty the list of objects we've checked
    _seen_objects.clear();

    // grab first text
    nextText();

    _working = true;

    doSpellcheck();
}

void SpellCheck::finished()
{
    clearRects();
    disconnect();

    corrections->splice(0, corrections->get_n_items(), {});
    column_view.set_sensitive(false);
    accept_button.set_sensitive(false);
    ignore_button.set_sensitive(false);
    ignoreonce_button.set_sensitive(false);
    add_button.set_sensitive(false);
    stop_button.set_sensitive(false);
    start_button.set_sensitive(true);

    banner_label.set_markup(
        _stops
        ? Glib::ustring::compose(_("<b>Finished</b>, <b>%1</b> words added to dictionary"), _adds)
        : _("<b>Finished</b>, nothing suspicious found")
    );

    _seen_objects.clear();

    _root = nullptr;

    _working = false;
}

bool SpellCheck::nextWord()
{
    auto desktop = getDesktop();
    if (!_working || !desktop)
        return false;

    if (!_text) {
        finished();
        return false;
    }
    _word.clear();

    while (_word.size() == 0) {
        _begin_w = _end_w;

        if (!_layout || _begin_w == _layout->end()) {
            nextText();
            return false;
        }

        if (!_layout->isStartOfWord(_begin_w)) {
            _begin_w.nextStartOfWord();
        }

        _end_w = _begin_w;
        _end_w.nextEndOfWord();
        _word = sp_te_get_string_multiline(_text, _begin_w, _end_w);
    }

    // try to link this word with the next if separated by '
    SPObject *char_item = nullptr;
    Glib::ustring::iterator text_iter;
    _layout->getSourceOfCharacter(_end_w, &char_item, &text_iter);
    if (is<SPString>(char_item)) {
        int this_char = *text_iter;
        if (this_char == '\'' || this_char == 0x2019) {
            auto end_t = _end_w;
            end_t.nextCharacter();
            _layout->getSourceOfCharacter(end_t, &char_item, &text_iter);
            if (is<SPString>(char_item)) {
                int this_char = *text_iter;
                if (g_ascii_isalpha(this_char)) { // 's
                    _end_w.nextEndOfWord();
                    _word = sp_te_get_string_multiline (_text, _begin_w, _end_w);
                }
            }
        }
    }

    // skip words containing digits
    if (_prefs.getInt(_prefs_path + "ignorenumbers") != 0) {
        bool digits = false;
        for (unsigned int i : _word) {
            if (g_unichar_isdigit(i)) {
               digits = true;
               break;
            }
        }
        if (digits) {
            return false;
        }
    }

    // skip ALL-CAPS words
    if (_prefs.getInt(_prefs_path + "ignoreallcaps") != 0) {
        bool allcaps = true;
        for (unsigned int i : _word) {
            if (!g_unichar_isupper(i)) {
               allcaps = false;
               break;
            }
        }
        if (allcaps) {
            return false;
        }
    }

    bool found = false;

    if (_checker) {
        found = spelling_checker_check_word(_checker.get(), _word.c_str(), _word.length());
    }

    if (!found) {
        _stops++;

        // display it in window
        banner_label.set_markup(Glib::ustring::compose(_("Not in dictionary: <b>%1</b>"), _word));

        column_view.set_sensitive(true);
        ignore_button.set_sensitive(true);
        ignoreonce_button.set_sensitive(true);
        add_button.set_sensitive(true);
        stop_button.set_sensitive(true);

        // draw rect
        auto points = _layout->createSelectionShape(_begin_w, _end_w, _text->i2dt_affine());
        // We may not have a single quad if this is a clipped part of text on path;
        // in that case skip drawing the rect
        if (points.size() >= 4) {
            // expand slightly
            auto area = Geom::Rect::from_range(points.begin(), points.end());
            double mindim = std::min(area.width(), area.height());
            area.expandBy(std::max(0.05 * mindim, 1.0));

            // Create canvas item rect with red stroke. (TODO: a quad could allow non-axis aligned rects.)
            auto rect = new Inkscape::CanvasItemRect(desktop->getCanvasSketch(), area);
            rect->set_stroke(0xff0000ff);
            rect->set_visible(true);
            _rects.emplace_back(rect);

            // scroll to make it all visible
            Geom::Point const center = desktop->current_center();
            area.expandBy(0.5 * mindim);
            Geom::Point scrollto;
            double dist = 0;
            for (unsigned corner = 0; corner < 4; corner ++) {
                if (Geom::L2(area.corner(corner) - center) > dist) {
                    dist = Geom::L2(area.corner(corner) - center);
                    scrollto = area.corner(corner);
                }
            }
            desktop->scroll_to_point(scrollto);
        }

        // select text; if in Text tool, position cursor to the beginning of word
        // unless it is already in the word
        if (desktop->getSelection()->singleItem() != _text) {
            desktop->getSelection()->set (_text);
        }

        if (auto const text_tool = dynamic_cast<Tools::TextTool *>(desktop->getTool())) {
            auto cursor = get_cursor_position(*text_tool, _text);
            if (!cursor) { // some other text is selected there
                desktop->getSelection()->set(_text);
            } else if (*cursor <= _begin_w || *cursor >= _end_w) {
                text_tool->placeCursor(_text, _begin_w);
            }
        }

        // get corrections
        auto new_corrections = list_corrections(_checker.get(), _word.c_str());
        corrections->splice(0, corrections->get_n_items(), new_corrections);

        // select first correction
        if (!new_corrections.empty()) {
            selection_model->property_selected().set_value(0);
        }

        accept_button.set_sensitive(!new_corrections.empty());

        return true;
    }

    return false;
}

void SpellCheck::deleteLastRect()
{
    if (!_rects.empty()) {
        _rects.pop_back();
    }
}

void SpellCheck::doSpellcheck()
{
    if (_langs.empty()) {
        return;
    }

    banner_label.set_markup(_("<i>Checking...</i>"));

    while (_working)
        if (nextWord())
            break;
}

void SpellCheck::onTreeSelectionChange()
{
    accept_button.set_sensitive(true);
}

void SpellCheck::onObjModified()
{
    if (_local_change) { // this was a change by this dialog, i.e. an Accept, skip it
        _local_change = false;
        return;
    }

    if (_working && _root) {
        // user may have edited the text we're checking; try to do the most sensible thing in this
        // situation

        // just in case, re-get text's layout
        _layout = te_get_layout(_text);

        // re-get the word
        _layout->validateIterator(&_begin_w);
        _end_w = _begin_w;
        _end_w.nextEndOfWord();
        Glib::ustring word_new = sp_te_get_string_multiline(_text, _begin_w, _end_w);
        if (word_new != _word) {
            _end_w = _begin_w;
            deleteLastRect();
            doSpellcheck(); // recheck this word and go ahead if it's ok
        }
    }
}

void SpellCheck::onObjReleased()
{
    if (_working && _root) {
        // the text object was deleted
        deleteLastRect();
        nextText();
        doSpellcheck(); // get next text and continue
    }
}

void SpellCheck::onAccept()
{
    // insert chosen correction

    auto index = selection_model->get_selected();
    if (index != GTK_INVALID_LIST_POSITION) {
        auto corr = corrections->get_string(index);
        if (!corr.empty()) {
            _local_change = true;
            sp_te_replace(_text, _begin_w, _end_w, corr.c_str());
            // find the end of the word anew
            _end_w = _begin_w;
            _end_w.nextEndOfWord();
            DocumentUndo::done(getDocument(), RC_("Undo", "Fix spelling"), INKSCAPE_ICON("draw-text"));
        }
    }

    deleteLastRect();
    doSpellcheck();
}

void SpellCheck::onIgnore()
{
    if (_checker) {
        spelling_checker_ignore_word(_checker.get(), _word.c_str());
    }

    deleteLastRect();
    doSpellcheck();
}

void SpellCheck::onIgnoreOnce()
{
    deleteLastRect();
    doSpellcheck();
}

void SpellCheck::onAdd()
{
    _adds++;

    if (_checker) {
        spelling_checker_add_word(_checker.get(), _word.c_str());
    }

    deleteLastRect();
    doSpellcheck();
}

void SpellCheck::onStop()
{
    finished();
}

void SpellCheck::onLanguageChanged()
{
    // First, save language for next load
    auto index = dictionary_combo.get_selected();
    if (index == GTK_INVALID_LIST_POSITION) {
        return;
    }
    _prefs.setString("/dialogs/spellcheck/lang", _langs.at(index).code);

    if (!_working) {
        onStart();
        return;
    }

    if (!updateSpeller()) {
        return;
    }

    // recheck current word
    _end_w = _begin_w;
    deleteLastRect();
    doSpellcheck();
}

} // namespace Inkscape::UI::Dialog

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

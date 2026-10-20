// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Widget that listens and modifies repr attributes.
 */
/* Authors:
 *  Lauris Kaplinski <lauris@ximian.com>
 *  Abhishek Sharma
 *  Kris De Gussem <Kris.DeGussem@gmail.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 * Copyright (C) 2002,2011-2012 authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-attribute-widget.h"

#include <glibmm/i18n.h>
#include <gtkmm/entry.h>
#include <gtkmm/enums.h>
#include <gtkmm/grid.h>
#include <gtkmm/object.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/textview.h>
#include <gtkmm/label.h>
#include <sigc++/adaptors/bind.h>

#include "document-undo.h"
#include "object/sp-object.h"
#include "preferences.h"
#include "ui/syntax.h"
#include "xml/node.h"

using Inkscape::DocumentUndo;
using Inkscape::UI::Syntax::SyntaxMode;

void SPAttributeTable::EntryWidget::set_text(const Glib::ustring& text) {
    if (_entry) {
        _entry->set_text(text);
    }
    else {
        _text_view->get_buffer()->set_text(text);
    }
}

Glib::ustring SPAttributeTable::EntryWidget::get_text() const {
    return _entry ?  _entry->get_text() : _text_view->get_buffer()->get_text();
}

const Gtk::Widget* SPAttributeTable::EntryWidget::get_widget() const {
    return _entry ?  static_cast<Gtk::Widget*>(_entry) : static_cast<Gtk::Widget*>(_text_view);
}

static constexpr int XPAD = 4;
static constexpr int YPAD = 2;

SPAttributeTable::SPAttributeTable(std::vector<Glib::ustring> const &labels,
                                   std::vector<Glib::ustring> const &attributes)
{
    create(labels, attributes);
}

void SPAttributeTable::create(const std::vector<Glib::ustring>& labels, const std::vector<Glib::ustring>& attributes) {
    // build rows
    _attributes = attributes;
    _entries.clear();
    _textviews.clear();
    _entries.reserve(attributes.size());

    append(_table);

    auto theme = Inkscape::Preferences::get()->getString("/theme/syntax-color-theme", "-none-");

    for (std::size_t i = 0; i < attributes.size(); ++i) {
        auto const ll = Gtk::make_managed<Gtk::Label>(_(labels[i].c_str()));
        ll->set_halign(Gtk::Align::START);
        ll->set_valign(Gtk::Align::CENTER);
        ll->set_vexpand(false);
        ll->set_margin_end(XPAD);
        ll->set_margin_top(YPAD);
        ll->set_margin_bottom(YPAD);
        _table.attach(*ll, 0, i, 1, 1);

        EntryWidget entry;
        if (_syntax != SyntaxMode::PlainText) {
            auto edit = Inkscape::UI::Syntax::TextEditView::create(_syntax);
            edit->setStyle(theme);
            auto& tv = edit->getTextView();
            entry._text_view = &tv;
            tv.set_wrap_mode(Gtk::WrapMode::WORD);
            auto wnd = Gtk::make_managed<Gtk::ScrolledWindow>();
            wnd->set_hexpand();
            wnd->set_vexpand(false);
            wnd->set_margin_start(XPAD);
            wnd->set_margin_top(YPAD);
            wnd->set_margin_bottom(YPAD);
            wnd->set_child(tv);
            wnd->set_has_frame(true);
            wnd->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
            _table.attach(*wnd, 1, i, 1, 1);

            tv.get_buffer()->signal_end_user_action().connect([i, this](){
                attribute_table_entry_changed(i);
            });
            _textviews.push_back(std::move(edit));
        }
        else {
            auto const ee = Gtk::make_managed<Gtk::Entry>();
            entry._entry = ee;
            ee->set_hexpand();
            ee->set_vexpand(false);
            ee->set_margin_start(XPAD);
            ee->set_margin_top(YPAD);
            ee->set_margin_bottom(YPAD);
            _table.attach(*ee, 1, i, 1, 1);

            ee->signal_changed().connect([i, this](){
                attribute_table_entry_changed(i);
            });
        }

        _entries.push_back(std::move(entry));
    }
}

void SPAttributeTable::change_object(SPObject *object)
{
    if (_object == object) return;

    if (_object) {
        modified_connection.disconnect();
        release_connection.disconnect();
        _object = nullptr;
    }

    _object = object;

    blocked = true;

    if (object) {
        // Set up object
        modified_connection = object->connectModified([this](SPObject* object, unsigned int flags){
            attribute_table_object_modified(object, flags);
        });
        release_connection  = object->connectRelease([this](SPObject* object){
            attribute_table_object_release(object);
        });
    }

    for (std::size_t i = 0; i < _attributes.size(); ++i) {
        auto const val = object ? object->getRepr()->attribute(_attributes[i].c_str()) : nullptr;
        _entries[i].set_text(val ? val : "");
    }

    blocked = false;
}

void SPAttributeTable::reread_properties()
{
    if (blocked) return;

    blocked = true;
    for (std::size_t i = 0; i < _attributes.size(); ++i) {
        auto const val = _object->getRepr()->attribute(_attributes[i].c_str());
        _entries.at(i).set_text(val ? val : "");
    }
    blocked = false;
}

void SPAttributeTable::set_modified_tag(unsigned int tag) {
    _modified_tag = tag;
}

/**
 * Callback for a modification of the selected object (size, color, properties, etc.).
 *
 * attribute_table_object_modified rereads the object properties
 * and shows the values in the entry boxes. It is a callback from a
 * connection of the SPObject.
 */
void SPAttributeTable::attribute_table_object_modified(SPObject */*object*/, unsigned const flags) {
    if (!(flags & SP_OBJECT_MODIFIED_FLAG)) return;

    for (std::size_t i = 0; i < _attributes.size(); ++i) {
        auto& e = _entries.at(i);
        auto const val = _object->getRepr()->attribute(_attributes[i].c_str());
        auto const new_text = Glib::ustring{val ? val : ""};
        if (e.get_text() != new_text) {
            // We are different
            blocked = true;
            e.set_text(new_text);
            blocked = false;
        }
    }
}

/**
 * Callback for user input in one of the entries.
 *
 * attribute_table_entry_changed set the object property
 * to the new value and updates history. It is a callback from
 * the entries created by SPAttributeTable.
 */
void SPAttributeTable::attribute_table_entry_changed(size_t index) {
    if (blocked) return;

    if (index >= _attributes.size() || index >= _entries.size()) {
        g_warning("file %s: line %d: Entry signalled change, but there is no such entry", __FILE__, __LINE__);
        return;
    }

    auto& e = _entries[index];
    blocked = true;
    if (_object) {
        auto text = e.get_text();
        auto attr = _object->getRepr()->attribute(_attributes[index].c_str());
        if (!attr || text != attr) {
            _object->getRepr()->setAttribute(_attributes[index], text);
            if (_modified_tag) {
                _object->requestModified(SP_OBJECT_MODIFIED_FLAG | _modified_tag);
            }
            DocumentUndo::done(_object->document, RC_("Undo", "Set attribute"), "");
        }
    }
    blocked = false;
}

/**
 * Callback for the deletion of the selected object.
 *
 * attribute_table_object_release invalidates all data of 
 * SPAttributeTable and clears the widget.
 */
void SPAttributeTable::attribute_table_object_release(SPObject */*object*/) {
    change_object(nullptr);
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

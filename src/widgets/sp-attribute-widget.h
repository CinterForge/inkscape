// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Widget that listens and modifies repr attributes.
 */
/* Authors:
 *  Lauris Kaplinski <lauris@kaplinski.com>
 *  Abhishek Sharma
 *  Kris De Gussem <Kris.DeGussem@gmail.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 * Copyright (C) 2002,2011-2012 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_DIALOGS_SP_ATTRIBUTE_WIDGET_H
#define SEEN_DIALOGS_SP_ATTRIBUTE_WIDGET_H

#include <gtkmm/textview.h>
#include <memory>
#include <vector>
#include <glibmm/ustring.h>
#include <gtkmm/box.h>
#include <gtkmm/grid.h>

#include <sigc++/scoped_connection.h>
#include "ui/syntax.h"

namespace Gtk {
class Entry;
class Grid;
} // namespace Gtk

namespace Inkscape::XML {
class Node;
} // namespace Inkscape::XML

class  SPObject;

/**
 * A base class for dialogs to enter the value of several properties.
 *
 * SPAttributeTable is used if you want to alter several properties of
 * an object. For each property, it creates an entry next to a label and
 * positiones these labels and entries one by one below each other.
 */
class SPAttributeTable final : public Gtk::Box {
public:
    /**
     * Constructor defaulting to no content. Call create() afterwards.
     */
    SPAttributeTable(Inkscape::UI::Syntax::SyntaxMode mode = Inkscape::UI::Syntax::SyntaxMode::PlainText): _syntax(mode) {}

    /**
     * Constructor referring to a specific object.
     *
     * This constructor initializes all data fields and creates the necessary widgets.
     * 
     * @param labels list of labels to be shown for the different attributes.
     * @param attributes list of attributes whose value can be edited.
     */
    SPAttributeTable(std::vector<Glib::ustring> const &labels,
                     std::vector<Glib::ustring> const &attributes);

    // create all widgets
    void create(const std::vector<Glib::ustring>& labels, const std::vector<Glib::ustring>& attributes);

    /**
     * Update values in entry boxes on change of object.
     *
     * change_object updates the values of the entry boxes in case the user
     * of Inkscape selects an other object.
     * change_object is a subset of set_object and should only be called by
     * the parent class (holding the SPAttributeTable instance). This function
     * should only be called when the number of properties/entries nor
     * the labels do not change.
     * 
     * @param object the SPObject to which this instance is referring to. It should be the object that is currently selected and whose properties are being shown by this SPAttribuTable instance.
     */
    void change_object(SPObject *object);

    /**
     * Reads the object attributes.
     * 
     * Reads the object attributes and shows the new object attributes in the
     * entry boxes. Caution: function should only be used when which there is
     * no change in which objects are selected.
     */
    void reread_properties();

    // Set one of the modification flags (SP_OBJECT_USER_MODIFIED_TAG_N) to destinguish sources of mofification requests
    void set_modified_tag(unsigned int tag);

    Gtk::Grid& get_grid() { return _table; }

private:
    /**
     * Stores pointer to the selected object.
     */
    SPObject *_object = nullptr;

    /**
     * Indicates whether SPAttributeTable is processing callbacks and whether it should accept any updating.
     */
    bool blocked = false;

    struct EntryWidget {
        EntryWidget() = default;
        EntryWidget(EntryWidget& e) = default;
        EntryWidget(EntryWidget&& e) = default;

        Glib::ustring get_text() const;
        void set_text(const Glib::ustring& text);
        const Gtk::Widget* get_widget() const;

        Gtk::Entry* _entry = nullptr;
        Gtk::TextView* _text_view = nullptr;
    };

    /**
     * Container widget for the dynamically created child widgets (labels and entry boxes).
     */
    Gtk::Grid _table;

    /**
     * List of attributes.
     * 
     * _attributes stores the attribute names of the selected object that
     * are valid and can be modified through this widget.
     */
    std::vector<Glib::ustring> _attributes;

    /**
     * List of pointers to the respective entry boxes.
     * 
     * _entries stores pointers to the dynamically created entry boxes in which
     * the user can midify the attributes of the selected object.
     */
    std::vector<EntryWidget> _entries;
    std::vector<std::unique_ptr<Inkscape::UI::Syntax::TextEditView>> _textviews;

    /**
     * Sets the callback for a modification of the selection.
     */
    sigc::scoped_connection modified_connection;

    /**
     * Sets the callback for the deletion of the selected object.
     */
    sigc::scoped_connection release_connection;

    Inkscape::UI::Syntax::SyntaxMode _syntax;

    void attribute_table_entry_changed(size_t index);
    void attribute_table_object_modified(SPObject* object, unsigned flags);
    void attribute_table_object_release(SPObject* object);

    unsigned int _modified_tag = 0;
};

#endif // SEEN_DIALOGS_SP_ATTRIBUTE_WIDGET_H

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

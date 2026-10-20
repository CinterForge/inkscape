// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * XML editor.
 */
/* Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   MenTaLguY <mental@rydia.net>
 *   bulia byak <buliabyak@users.sf.net>
 *   Johan Engelen <goejendaagh@zonnet.nl>
 *   David Turner
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *   Mike Kowalski
 *
 * Copyright (C) l99-2006 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 *
 */

#include "xml-tree.h"

#include <glibmm/i18n.h>
#include <glibmm/main.h>
#include <gtkmm/entry.h>
#include <gtkmm/menubutton.h>
#include <gtkmm/paned.h>
#include <gtkmm/scrolledwindow.h>

#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "layer-manager.h"
#include "object/sp-root.h"
#include "selection.h"
#include "ui/builder-utils.h"
#include "ui/icon-names.h"
#include "ui/widget/xml-treeview.h"
#include "util/trim.h"

namespace {

/**
 * Set the orientation of `paned` to vertical or horizontal, and make the first child resizable
 * if vertical, and the second child resizable if horizontal.
 * @pre `paned` has two children
 */
void paned_set_vertical(Gtk::Paned &paned, bool vertical)
{
    auto& first = *paned.get_start_child();
    auto& second = *paned.get_end_child();
    const int space = 1;
    paned.set_resize_start_child(vertical);
    first.set_margin_bottom(vertical ? space : 0);
    first.set_margin_end(vertical ? 0 : space);
    second.set_margin_top(vertical ? space : 0);
    second.set_margin_start(vertical ? 0 : space);
    paned.set_resize_end_child(!vertical);
    paned.set_orientation(vertical ? Gtk::Orientation::VERTICAL : Gtk::Orientation::HORIZONTAL);
}

} // namespace

namespace Inkscape::UI::Dialog {

XmlTree::XmlTree()
    : DialogBase("/dialogs/xml/", "XMLEditor")
    , _builder(create_builder("dialog-xml.glade"))
    , _paned(get_widget<Gtk::Paned>(_builder, "pane"))
    , xml_element_new_button(get_widget<Gtk::Button>(_builder, "new-elem"))
    , xml_text_new_button(get_widget<Gtk::Button>(_builder, "new-text"))
    , xml_node_delete_button(get_widget<Gtk::Button>(_builder, "del"))
    , xml_node_duplicate_button(get_widget<Gtk::Button>(_builder, "dup"))
    , unindent_node_button(get_widget<Gtk::Button>(_builder, "unindent"))
    , indent_node_button(get_widget<Gtk::Button>(_builder, "indent"))
    , lower_node_button(get_widget<Gtk::Button>(_builder, "lower"))
    , raise_node_button(get_widget<Gtk::Button>(_builder, "raise"))
    , _syntax_theme("/theme/syntax-color-theme")
    , _mono_font("/dialogs/xml/mono-font", false)
{
    /* tree view */
    _xml_treeview = Gtk::make_managed<Inkscape::UI::Widget::XmlTreeView>();
    _xml_treeview->set_tooltip_text( _("Drag to reorder nodes") );
    _xml_treeview->set_search_column(-1);

    Gtk::ScrolledWindow& tree_scroller = get_widget<Gtk::ScrolledWindow>(_builder, "tree-wnd");
    tree_scroller.set_child(*_xml_treeview);
    fix_inner_scroll(tree_scroller);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    /* attributes */
    attributes = Gtk::make_managed<AttrDialog>();
    attributes->set_margin_top(0);
    attributes->set_margin_bottom(0);
    attributes->set_margin_start(0);
    attributes->set_margin_end(0);
    attributes->get_scrolled_window().set_has_frame(true);
    attributes->set_visible(true);
    attributes->get_status_box().set_visible(false);
    _paned.set_end_child(*attributes);
    _paned.set_resize_end_child(true);

    /* Signal handlers */
    _xml_treeview->get_selection()->signal_changed().connect([this]() {
        if (blocked || !getDesktop()) {
            return;
        }
        if (!_tree_select_idle) {
            // Defer the update after all events have been processed.
            _tree_select_idle = Glib::signal_idle().connect(sigc::mem_fun(*this, &XmlTree::deferred_on_tree_select_row));
        }
    });

    xml_element_new_button.signal_clicked().connect(sigc::mem_fun(*this, &XmlTree::cmd_new_element_node));
    xml_text_new_button.signal_clicked().connect(sigc::mem_fun(*this, &XmlTree::cmd_new_text_node));
    xml_node_duplicate_button.signal_clicked().connect(sigc::mem_fun(*this, &XmlTree::cmd_duplicate_node));
    xml_node_delete_button.signal_clicked().connect(sigc::mem_fun(*this, &XmlTree::cmd_delete_node));
    unindent_node_button.signal_clicked().connect(sigc::mem_fun(*this, &XmlTree::cmd_unindent_node));
    indent_node_button.signal_clicked().connect(sigc::mem_fun(*this, &XmlTree::cmd_indent_node));
    raise_node_button.signal_clicked().connect(sigc::mem_fun(*this, &XmlTree::cmd_raise_node));
    lower_node_button.signal_clicked().connect(sigc::mem_fun(*this, &XmlTree::cmd_lower_node));

    set_name("XMLAndAttributesDialog");
    set_spacing(0);

    int panedpos = prefs->getInt("/dialogs/xml/panedpos", 200);
    _paned.property_position() = panedpos;
    _paned.property_position().signal_changed().connect(sigc::mem_fun(*this, &XmlTree::_resized));

    append(_bin);
    _bin.set_child(get_widget<Gtk::Box>(_builder, "main"));
    _bin.set_expand(true);

    int min_width = 0, dummy;
    measure(Gtk::Orientation::HORIZONTAL, -1, min_width, dummy, dummy, dummy);

    auto auto_arrange_panels = [=, this] (int width, int height) {
        // skip bogus sizes
        if (width < 10 || height < 10) return;

        // minimal width times fudge factor to arrive at "narrow" dialog with automatic vertical layout:
        bool const narrow = width < min_width * 1.5;
        paned_set_vertical(_paned, narrow);
    };

    auto arrange_panels = [=, this] (DialogLayout layout, int width, int height) {
        switch (layout) {
            case Auto:
                auto_arrange_panels(width, height);
                break;
            case Horizontal:
                paned_set_vertical(_paned, false);
                break;
            case Vertical:
                paned_set_vertical(_paned, true);
                break;
        }
    };

    _bin.connectBeforeResize([=, this] (int width, int height, int) {
        arrange_panels(_layout, width, height);
    });

    auto& popup = get_widget<Gtk::MenuButton>(_builder, "layout-btn");
    popup.set_has_tooltip();
    popup.signal_query_tooltip().connect([this](int x, int y, bool kbd, const Glib::RefPtr<Gtk::Tooltip>& tooltip){
        auto tip = "";
        switch (_layout) {
            case Auto: tip = _("Automatic panel layout:\nchanges with dialog size");
                break;
            case Horizontal: tip = _("Horizontal panel layout");
                break;
            case Vertical: tip = _("Vertical panel layout");
                break;
        }
        tooltip->set_text(tip);
        return true;
    }, true);

    auto set_layout = [=, this] (DialogLayout layout) {
        Glib::ustring icon = "layout-auto";
        if (layout == Horizontal) {
            icon = "layout-horizontal";
        } else if (layout == Vertical) {
            icon = "layout-vertical";
        }
        get_widget<Gtk::MenuButton>(_builder, "layout-btn").set_icon_name(icon + "-symbolic");
        prefs->setInt("/dialogs/xml/layout", layout);
        arrange_panels(layout, get_width(), get_height());
        _layout = layout;
    };

    _layout = static_cast<DialogLayout>(prefs->getIntLimited("/dialogs/xml/layout", Auto, Auto, Vertical));
    auto action_group = Gio::SimpleActionGroup::create();
    auto action = action_group->add_action_radio_integer("layout", _layout);
    action->property_state().signal_changed().connect([=, &popup]
    {
        popup.set_active(false);
        int target; action->get_state(target); set_layout(static_cast<DialogLayout>(target));
    });
    insert_action_group("xml-tree", std::move(action_group));
    set_layout(_layout);

    // establish initial layout to prevent unwanted panels resize in auto layout mode
    paned_set_vertical(_paned, true);

    _syntax_theme.action = [this]() {
        setSyntaxStyle(Inkscape::UI::Syntax::build_xml_styles(_syntax_theme));
        // rebuild tree to change markup
        rebuildTree();
    };

    setSyntaxStyle(Inkscape::UI::Syntax::build_xml_styles(_syntax_theme));

    _mono_font.action = [this]() {
        Glib::ustring mono("mono-font");
        if (_mono_font) {
            _xml_treeview->add_css_class(mono);
        } else {
            _xml_treeview->remove_css_class(mono);
        }
        attributes->set_mono_font(_mono_font);
    };
    _mono_font.action();

    auto renderer = _xml_treeview->get_renderer();
    renderer->signal_editing_canceled().connect([this]() {
        stopNodeEditing(false, "", "");
    });

    renderer->signal_edited().connect([this](const Glib::ustring& path, const Glib::ustring& name) {
        stopNodeEditing(true, path, name);
    });

    renderer->signal_editing_started().connect([this](Gtk::CellEditable* cell, const Glib::ustring& path) {
        startNodeEditing(cell, path);
    });
}

XmlTree::~XmlTree()
{
    unsetDocument();
}

void XmlTree::rebuildTree()
{
    if (auto document = getDocument()) {
        _xml_treeview->build_tree(document);
        set_tree_select(document->getReprRoot());
    }
}

void XmlTree::_resized()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setInt("/dialogs/xml/panedpos", _paned.property_position());
}

void XmlTree::unsetDocument()
{
    _tree_select_idle.disconnect();
}

void XmlTree::documentReplaced()
{
    unsetDocument();
    if (auto document = getDocument()) {
        // TODO: Why is this a document property?
        document->setXMLDialogSelectedObject(nullptr);

        _xml_treeview->build_tree(document);
        set_tree_select(document->getReprRoot());
    } else {
        _xml_treeview->build_tree(nullptr);
        set_tree_select(nullptr);
    }
}

void XmlTree::selectionChanged(Selection *selection)
{
    if (!blocked++) {
        Inkscape::XML::Node *node = get_dt_select();
        set_tree_select(node);
    }
    blocked--;
}

void XmlTree::set_tree_select(Inkscape::XML::Node *repr, bool edit)
{
    selected_repr = repr; // Can be nullptr.

    if (auto document = getDocument()) {
        document->setXMLDialogSelectedObject(nullptr);
    }

    _xml_treeview->select_node(repr, edit);
    propagate_tree_select(repr);
    on_tree_select_row_enable(repr);
}


// Update attributes panel, repr can be nullptr.
void XmlTree::propagate_tree_select(Inkscape::XML::Node *repr)
{
    if (repr &&
       (repr->type() == Inkscape::XML::NodeType::ELEMENT_NODE ||
        repr->type() == Inkscape::XML::NodeType::TEXT_NODE ||
        repr->type() == Inkscape::XML::NodeType::COMMENT_NODE))
    {
        attributes->setRepr(repr);
    } else {
        attributes->setRepr(nullptr);
    }
}


Inkscape::XML::Node *XmlTree::get_dt_select()
{
    if (auto selection = getSelection()) {
        return selection->singleRepr();
    }
    return nullptr;
}


/**
 * Like SPDesktop::isLayer(), but ignores SPGroup::effectiveLayerMode().
 */
static bool isRealLayer(SPObject const *object)
{
    auto group = cast<SPGroup>(object);
    return group && group->layerMode() == SPGroup::LAYER;
}

void XmlTree::set_dt_select(Inkscape::XML::Node *repr)
{
    auto document = getDocument();
    if (!document)
        return;

    SPObject *object;
    if (repr) {
        while ( ( repr->type() != Inkscape::XML::NodeType::ELEMENT_NODE )
                && repr->parent() )
        {
            repr = repr->parent();
        } // end of while loop

        object = document->getObjectByRepr(repr);
    } else {
        object = nullptr;
    }

    blocked++;

    if (!object || !in_dt_coordsys(*object)) {
        // object not on canvas
    } else if (isRealLayer(object)) {
        getDesktop()->layerManager().setCurrentLayer(object);
    } else {
        if (is<SPGroup>(object->parent)) {
            getDesktop()->layerManager().setCurrentLayer(object->parent);
        }

        getSelection()->set(cast<SPItem>(object));
    }

    document->setXMLDialogSelectedObject(object);
    blocked--;
}

bool XmlTree::deferred_on_tree_select_row()
{
    auto selection = _xml_treeview->get_selection();
    auto iter = selection->get_selected();

    selected_repr = iter ? _xml_treeview->get_repr(*iter) : nullptr;

    propagate_tree_select(selected_repr);
    set_dt_select(selected_repr);
    on_tree_select_row_enable(selected_repr);

    return false;
}

void XmlTree::_set_status_message(Inkscape::MessageType /*type*/, const gchar *message, GtkWidget *widget)
{
    if (widget) {
        gtk_label_set_markup(GTK_LABEL(widget), message ? message : "");
    }
}

void XmlTree::on_tree_select_row_enable(Inkscape::XML::Node *node)
{
    if (!node) {
        on_tree_unselect_row_disable();
        return;
    }

    // If mutable and not top node svg:svg :
    bool is_mutable = xml_tree_node_mutable(node);
    bool is_root = !(selected_repr && node->parent() && node->parent()->parent());
    xml_node_duplicate_button.set_sensitive(is_mutable);
    xml_node_delete_button.set_sensitive(!is_root && is_mutable);

    // If element node:
    bool is_element = (node->type() == Inkscape::XML::NodeType::ELEMENT_NODE);
    xml_element_new_button.set_sensitive(is_element);
    xml_text_new_button.set_sensitive(   is_element);

    // If unindentable (not child of top svg:svg):
    bool unindentable = false;
    auto parent=node->parent();
    if (parent) {
        auto grandparent = parent->parent();
        if (grandparent && grandparent->parent()) {
            unindentable = true; // XML tree root is actually 'xml' and not 'svg:svg'!
        }
    }
    unindent_node_button.set_sensitive(unindentable);

    // If indentable:
    bool indentable = false;
    if (xml_tree_node_mutable(node)) {
        Inkscape::XML::Node *prev;

        if (parent && node != parent->firstChild()) {
            assert (parent->firstChild());

            // Skip to the child just before the current node.
            for ( prev = parent->firstChild() ;
                  prev && prev->next() != node ;
                  prev = prev->next() ){};

            if (prev && (prev->type() == Inkscape::XML::NodeType::ELEMENT_NODE)) {
                indentable = true;
            }
        }
    }
    indent_node_button.set_sensitive(indentable);

    // If not first child:
    if (parent && node != parent->firstChild()) {
        raise_node_button.set_sensitive(true);
    } else {
        raise_node_button.set_sensitive(false);
    }

    // If not last child:
    if (parent && node->next()) {
        lower_node_button.set_sensitive(true);
    } else {
        lower_node_button.set_sensitive(false);
    }
}

bool XmlTree::xml_tree_node_mutable(Inkscape::XML::Node* node)
{
    assert(node);

    // Top-level is immutable, obviously.
    auto parent = node->parent();
    if (!parent) {
        return false;
    }

    // If not in base level (where namedview, defs, etc go), we're mutable.
    if (parent->parent()) {
        return true;
    }

    // Don't let "defs" or "namedview" disappear.
    if ( !strcmp(node->name(),"svg:defs") ||
         !strcmp(node->name(),"sodipodi:namedview") ) {
        return false;
    }

    // Everyone else is okay, I guess.  :)
    return true;
}

void XmlTree::on_tree_unselect_row_disable()
{
    xml_text_new_button.set_sensitive(false);
    xml_element_new_button.set_sensitive(false);
    xml_node_delete_button.set_sensitive(false);
    xml_node_duplicate_button.set_sensitive(false);
    unindent_node_button.set_sensitive(false);
    indent_node_button.set_sensitive(false);
    raise_node_button.set_sensitive(false);
    lower_node_button.set_sensitive(false);
}

void XmlTree::onCreateNameChanged()
{
    Glib::ustring text = name_entry->get_text();
    /* TODO: need to do checking a little more rigorous than this */
    create_button->set_sensitive(!text.empty());
}

void XmlTree::cmd_new_element_node()
{
    auto document = getDocument();
    if (!document)
        return;

    // Enable in-place node name editing.
    auto renderer = _xml_treeview->get_renderer();
    renderer->property_editable() = true;

    auto dummy = ""; // this element has no corresponding SP* object and its construction is silent
    auto xml_doc = document->getReprDoc();
    _dummy = xml_doc->createElement(dummy); // create dummy placeholder so we can have a new temporary row in xml tree

    assert (selected_repr);
    _node_parent = selected_repr;   // remember where the node is inserted
    selected_repr->appendChild(_dummy);
    set_tree_select(_dummy, true); // enter in-place node name editing
}

void XmlTree::startNodeEditing(Gtk::CellEditable* cell, const Glib::ustring& path)
{
    if (!cell) {
        return;
    }
    // remove dummy element name so user can start with an empty name
    auto entry = dynamic_cast<Gtk::Entry *>(cell);
    entry->get_buffer()->set_text("");
}

void XmlTree::stopNodeEditing(bool ok, const Glib::ustring& path, Glib::ustring element)
{
    auto renderer = _xml_treeview->get_renderer();
    renderer->property_editable() = false;

    auto document = getDocument();
    if (!document) {
        return;
    }
    // delete dummy node
    if (_dummy) {
        document->setXMLDialogSelectedObject(nullptr);

        auto parent = _dummy->parent();
        sp_repr_unparent(_dummy);
        if (parent) {
            auto parentobject = document->getObjectByRepr(parent);
            if (parentobject) {
                parentobject->requestDisplayUpdate(SP_OBJECT_CHILD_MODIFIED_FLAG);
            }
        }

        _dummy = nullptr;
    }

    Util::trim(element);
    if (!ok || element.empty() || !_node_parent) {
        return;
    }

    Inkscape::XML::Document* xml_doc = document->getReprDoc();
    // Extract tag name
    {
        static auto const extract_tagname = Glib::Regex::create("^<?\\s*(\\w[\\w:\\-\\d]*)");
        Glib::MatchInfo match_info;
        extract_tagname->match(element, match_info);
        if (!match_info.matches()) {
            return;
        }
        element = match_info.fetch(1);
    }

    // prepend "svg:" namespace if none is given
    if (element.find(':') == Glib::ustring::npos) {
        element = "svg:" + element;
    }
    auto repr = xml_doc->createElement(element.c_str());
    _node_parent->appendChild(repr);
    set_dt_select(repr);
    set_tree_select(repr, true);
    _node_parent = nullptr;

    DocumentUndo::done(document, RC_("Undo/XML Editor", "Create new element node"), INKSCAPE_ICON("dialog-xml-editor"));
}

void XmlTree::cmd_new_text_node()
{
    auto document = getDocument();
    if (!document)
        return;

    g_assert(selected_repr != nullptr);

    Inkscape::XML::Document *xml_doc = document->getReprDoc();
    Inkscape::XML::Node *text = xml_doc->createTextNode("");
    selected_repr->appendChild(text);

    DocumentUndo::done(document, RC_("Undo/XML Editor", "Create new text node"), INKSCAPE_ICON("dialog-xml-editor"));

    set_tree_select(text);
    set_dt_select(text);
}

void XmlTree::cmd_duplicate_node()
{
    auto document = getDocument();
    if (!document)
        return;

    g_assert(selected_repr != nullptr);

    Inkscape::XML::Node *parent = selected_repr->parent();
    Inkscape::XML::Node *dup = selected_repr->duplicate(parent->document());
    parent->addChild(dup, selected_repr);

    DocumentUndo::done(document, RC_("Undo/XML Editor", "Duplicate node"), INKSCAPE_ICON("dialog-xml-editor"));

    _xml_treeview->select_node(dup);
}

void XmlTree::cmd_delete_node()
{
    auto document = getDocument();
    if (!document)
        return;

    g_assert(selected_repr != nullptr);

    document->setXMLDialogSelectedObject(nullptr);

    Inkscape::XML::Node *parent = selected_repr->parent();
    if (!parent || !parent->parent()) {
        return;
    }

    sp_repr_unparent(selected_repr);
    selected_repr = nullptr;

    if (parent) {
        auto parentobject = document->getObjectByRepr(parent);
        if (parentobject) {
            parentobject->requestDisplayUpdate(SP_OBJECT_CHILD_MODIFIED_FLAG);
        }
    }

    DocumentUndo::done(document, RC_("Undo/XML Editor", "Delete node"), INKSCAPE_ICON("dialog-xml-editor"));
}

void XmlTree::cmd_raise_node()
{
    auto document = getDocument();
    if (!document)
        return;

    g_assert(selected_repr != nullptr);

    Inkscape::XML::Node *parent = selected_repr->parent();
    g_return_if_fail(parent != nullptr);
    g_return_if_fail(parent->firstChild() != selected_repr);

    Inkscape::XML::Node *ref = nullptr;
    Inkscape::XML::Node *before = parent->firstChild();
    while (before && (before->next() != selected_repr)) {
        ref = before;
        before = before->next();
    }

    parent->changeOrder(selected_repr, ref);

    DocumentUndo::done(document, RC_("Undo/XML Editor", "Raise node"), INKSCAPE_ICON("dialog-xml-editor"));

    set_tree_select(selected_repr);
    set_dt_select(selected_repr);
}



void XmlTree::cmd_lower_node()
{
    auto document = getDocument();
    if (!document)
        return;

    g_assert(selected_repr != nullptr);

    g_return_if_fail(selected_repr->next() != nullptr);
    Inkscape::XML::Node *parent = selected_repr->parent();

    parent->changeOrder(selected_repr, selected_repr->next());

    DocumentUndo::done(document, RC_("Undo/XML Editor", "Lower node"), INKSCAPE_ICON("dialog-xml-editor"));

    set_tree_select(selected_repr);
    set_dt_select(selected_repr);
}

void XmlTree::cmd_indent_node()
{
    auto document = getDocument();
    if (!document)
        return;

    assert (selected_repr);
    Inkscape::XML::Node *repr = selected_repr;

    Inkscape::XML::Node *parent = repr->parent();
    g_return_if_fail(parent != nullptr);
    g_return_if_fail(parent->firstChild() != repr);

    Inkscape::XML::Node* prev = parent->firstChild();
    while (prev && (prev->next() != repr)) {
        prev = prev->next();
    }
    g_return_if_fail(prev != nullptr);
    g_return_if_fail(prev->type() == Inkscape::XML::NodeType::ELEMENT_NODE);

    Inkscape::XML::Node* ref = nullptr;
    if (prev->firstChild()) {
        for( ref = prev->firstChild() ; ref->next() ; ref = ref->next() ){};
    }

    parent->removeChild(repr);
    prev->addChild(repr, ref);

    DocumentUndo::done(document, RC_("Undo/XML Editor", "Indent node"), INKSCAPE_ICON("dialog-xml-editor"));
    set_tree_select(repr);
    set_dt_select(repr);

} // end of cmd_indent_node()



void XmlTree::cmd_unindent_node()
{
    auto document = getDocument();
    if (!document) {
        return;
    }

    Inkscape::XML::Node *repr = selected_repr;
    g_assert(repr != nullptr);

    Inkscape::XML::Node *parent = repr->parent();
    g_return_if_fail(parent);

    Inkscape::XML::Node *grandparent = parent->parent();
    g_return_if_fail(grandparent);

    parent->removeChild(repr);
    grandparent->addChild(repr, parent);

    DocumentUndo::done(document, RC_("Undo/XML Editor", "Unindent node"), INKSCAPE_ICON("dialog-xml-editor"));

    set_tree_select(repr);
    set_dt_select(repr);

} // end of cmd_unindent_node()

/** Returns true iff \a item is suitable to be included in the selection, in particular
    whether it has a bounding box in the desktop coordinate system for rendering resize handles.

    Descendents of <defs> nodes (markers etc.) return false, for example.
*/
bool XmlTree::in_dt_coordsys(SPObject const &item)
{
    /* Definition based on sp_item_i2doc_affine. */
    SPObject const *child = &item;
    while (is<SPItem>(child)) {
        SPObject const * const parent = child->parent;
        if (parent == nullptr) {
            g_assert(is<SPRoot>(child));
            if (child == &item) {
                // item is root
                return false;
            }
            return true;
        }
        child = parent;
    }
    g_assert(!is<SPRoot>(child));
    return false;
}

void XmlTree::desktopReplaced() {
    // subdialog does not receive desktopReplace calls, we need to propagate desktop change
    if (attributes) {
        attributes->setDesktop(getDesktop());
    }
}

void XmlTree::setSyntaxStyle(Inkscape::UI::Syntax::XMLStyles const &new_style)
{
    _xml_treeview->set_style(new_style);
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

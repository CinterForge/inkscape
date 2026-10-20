// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * A widget showing the XML tree.
 *
 * Authors:
 *   Tavmjong Bah
 *   MenTaLguY <mental@rydia.net> (Original C version)
 *
 * Copyright (C)
 *   Tavmjong Bah 2024
 *   MenTaLguY 2002
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 *
 * TODO:
 *   Lazy load.
 */

#include "xml-treeview.h"

#include <glibmm/property.h>
#include <gtkmm/dragsource.h>
#include <gtkmm/droptarget.h>
#include <gtkmm/treerowreference.h>
#include <gtkmm/treestore.h>

#include "document.h"
#include "object/sp-defs.h" // D&D in <defs>, <glyph>, etc.
#include "object/sp-glyph.h"
#include "object/sp-mask.h"
#include "object/sp-pattern.h"
#include "object/sp-root.h" // -> SPGroup -> SPLPEItem -> SPItem
#include "object/sp-text.h"
#include "object/sp-tspan.h"
#include "util/value-utils.h"
#include "xml/simple-node.h"

using namespace Inkscape::Util;

namespace Inkscape::UI::Widget {
namespace {

struct XMLDnDRow
{
    XML::Node *node;
};

} // namespace

class ModelColumns final : public Gtk::TreeModel::ColumnRecord
{
public:
    ModelColumns()
    {
        add(node);
        add(markup);
        add(text);
    }

    Gtk::TreeModelColumn<Inkscape::XML::Node*> node;
    Gtk::TreeModelColumn<Glib::ustring> markup;
    Gtk::TreeModelColumn<Glib::ustring> text;
};

/************ NodeWatcher ************/

class NodeWatcher : public Inkscape::XML::NodeObserver
{
public:
    NodeWatcher() = delete;
    NodeWatcher(XmlTreeView *xml_tree_view, Inkscape::XML::Node *node, Gtk::TreeRow *row);
    ~NodeWatcher() override;

    std::unordered_map<Inkscape::XML::Node const *,
                       std::unique_ptr<NodeWatcher>> child_watchers;

private:
    void update_row();

    // Treeview routines
    Gtk::TreeNodeChildren get_children() const;
    void add_child (Inkscape::XML::Node *child); // Add a NodeWatcher for a child.
    void add_children();  // Add children to this node. (Maybe not needed.)

    // XML routines
    void move_child(Inkscape::XML::Node &child, Inkscape::XML::Node *sibling);

    Gtk::TreeModel::iterator get_child_iterator(Inkscape::XML::Node *node) const;

    // Notifiers
    void notifyContentChanged(Inkscape::XML::Node & /* node */,
                              Inkscape::Util::ptr_shared /* old_content */,
                              Inkscape::Util::ptr_shared new_content) override
    {
        update_row();
    }

    void notifyChildAdded(Inkscape::XML::Node &node,
                          Inkscape::XML::Node &child,
                          Inkscape::XML::Node *prev) override
    {
        assert (this->node == &node);

        add_child(&child);
        move_child(child, prev);
    }

    void notifyChildRemoved(Inkscape::XML::Node &node,
                            Inkscape::XML::Node &child,
                            Inkscape::XML::Node *) override
    {
        assert (this->node == &node);

        if (child_watchers.erase(&child) > 0) {
            return;
        }

        std::cerr << "NodeWatcher::notifyChildRemoved: failed to remove child!"
                  << std::endl;
    }

    void notifyChildOrderChanged(Inkscape::XML::Node &parent,
                                 Inkscape::XML::Node &child,
                                 Inkscape::XML::Node * /* old parent */,
                                 Inkscape::XML::Node *new_prev) override
    {
        assert (this->node == &parent);

        move_child(child, new_prev);
    }
        
    void notifyAttributeChanged(Inkscape::XML::Node &node,
                                GQuark key,
                                Inkscape::Util::ptr_shared,
                                Inkscape::Util::ptr_shared) override
    {
        // Only worry about 'id' or 'inkscape::label' changes.
        auto const attribute = g_quark_to_string(key);
        if (std::strcmp(attribute, "id") == 0 ||
            std::strcmp(attribute, "inkscape:label") == 0) {
            update_row();
        }
    }

    void notifyElementNameChanged(Inkscape::XML::Node &node,
                                  GQuark,
                                  GQuark) override
    {
        update_row();
    }

    // Variables
    Inkscape::XML::Node* node;
    XmlTreeView *xml_tree_view;
    Gtk::TreeModel::RowReference row_ref;
};

NodeWatcher::NodeWatcher(XmlTreeView *xml_tree_view, Inkscape::XML::Node *node, Gtk::TreeRow *row)
    : xml_tree_view(xml_tree_view)
    , node(node)
    , row_ref()
{
    if (row != nullptr) {
        assert(row->children().empty());

        auto store = xml_tree_view->store;

        auto row_iter = row->get_iter();
        auto path = store->get_path(row_iter);
        row_ref = Gtk::TreeModel::RowReference(store, path);

        update_row();
    }

    node->addObserver(*this);

    add_children(); // Recursively add all descendents.
}

NodeWatcher::~NodeWatcher()
{
    node->removeObserver(*this);
    Gtk::TreeModel::Path path;
    if (bool(row_ref) && (path = row_ref.get_path())) {
        if (auto iter = xml_tree_view->store->get_iter(path)) {
            xml_tree_view->store->erase(iter);
        }
    }
    child_watchers.clear();
}

void
NodeWatcher::update_row()
{
    Glib::ustring start;
    Glib::ustring end;
    using Inkscape::XML::NodeType;
    switch (node->type()) {
        case NodeType::ELEMENT_NODE:  start = "<";    end = ">";     break;
        case NodeType::TEXT_NODE:     start = "\"";   end = "\"";    break;
        case NodeType::COMMENT_NODE:  start = "<!--"; end = "-->";   break;
        case NodeType::PI_NODE:       start = "<?";   end = "?>";    break;
        case NodeType::DOCUMENT_NODE: break;
        default: std::cerr << "NodeWatcher::NodeWatcher: unhandled NodeType!" << std::endl;
    }

    Glib::ustring content;
    Glib::ustring text;
    Glib::ustring markup;
    switch (node->type()) {
        case NodeType::ELEMENT_NODE: {

            content = node->name();

            // Remove namespace "svg", it's just visual noise.
            auto pos = content.find("svg:");
            if (pos != Glib::ustring::npos) {
                content.erase(pos, 4);
            }

            // Markup text with color.
            xml_tree_view->formatter->openTag(content.c_str());

            // char const *id = node->attribute("id");
            if (char const *id = node->attribute("id")) {
                content += " id=\"";
                content += id;
                content += "\"";
                xml_tree_view->formatter->addAttribute("id", id);
            }

            if (char const *label = node->attribute("inkscape::label")) {
                content += " inkscape:label=\"";
                content += label;
                content += "\"";
                xml_tree_view->formatter->addAttribute("inkscape:label", label);
            }

            text = start + content + end;
            markup = xml_tree_view->formatter->finishTag();

            break;
        }
        case NodeType::TEXT_NODE:
        case NodeType::COMMENT_NODE:
        case NodeType::PI_NODE:
        {
            if (auto simple_node = dynamic_cast<Inkscape::XML::SimpleNode*>(node)) {
                if (simple_node->content()) {
                    content = simple_node->content();
                }
            }
            text = start + content + end;
            markup = xml_tree_view->formatter->formatContent(text.c_str(), false);
            break;
        }
        case NodeType::DOCUMENT_NODE: break;
        default: std::cerr << "NodeWatcher::NodeWatcher: unhandled NodeType!" << std::endl;
    }
    
    auto path = row_ref.get_path();
    if (!path) {
        std::cerr << "NodeWatcher::update_row: no path!" << std::endl;
        return;
    }
    auto row_iter = xml_tree_view->store->get_iter(path);
    if (!row_iter) {
        std::cerr << "NodeWatcher::update_row: no row_iter!" << std::endl;
        return;
    }

    (*row_iter)[xml_tree_view->model_columns->node  ] = node;
    (*row_iter)[xml_tree_view->model_columns->text  ] = text;
    (*row_iter)[xml_tree_view->model_columns->markup] = markup;
}


Gtk::TreeNodeChildren
NodeWatcher::get_children() const
{
    Gtk::TreeModel::Path path;
    if (row_ref && (path = row_ref.get_path())) {
        return xml_tree_view->store->get_iter(path)->children();
    }
    assert (!row_ref);
    return xml_tree_view->store->children();
}

void
NodeWatcher::add_child(Inkscape::XML::Node *child)
{
    assert (child);

    auto children = get_children();
    Gtk::TreeModel::Row row = *(xml_tree_view->store->append(children));

    auto &watcher = child_watchers[child];
    assert (!watcher);
    watcher.reset(new NodeWatcher(xml_tree_view, child, &row));
}

void
NodeWatcher::add_children()
{
    for (auto *child = node->firstChild(); child != nullptr; child = child->next()) {
        add_child(child);
    }
}

/**
 * Changes TreeStore in response to XML changes.
 */
void
NodeWatcher::move_child(Inkscape::XML::Node &child, Inkscape::XML::Node *sibling)
{
    auto child_iter = get_child_iterator(&child);
    if (!child_iter) {
        std::cerr << "NodeWatcher::move_child: no child iterator!" << std::endl;
        return;
    }

    auto sibling_iter = get_child_iterator(sibling); // Can be null.
    // move() puts the child before the sibling, but we need it before...
    if (sibling_iter) {
        sibling_iter++;
    } else {
        sibling_iter = get_children().begin(); // Messy!
    }
    xml_tree_view->store->move(     child_iter, sibling_iter);
}

Gtk::TreeModel::iterator
NodeWatcher::get_child_iterator(Inkscape::XML::Node *node) const
{
    auto child_rows = get_children();

    if (!node) {
        return child_rows.end();
    }

    for (auto &row : child_rows) {
        if (xml_tree_view->get_repr(row) == node) {
            return row.get_iter();
        }
    }

    std::cerr << "NodeWatcher::get_child_iterator: failed to find interator!" << std::endl;

    return child_rows.begin();
}

/************ NodeRenderer ***********/

class NodeRenderer : public Gtk::CellRendererText {
public:
    NodeRenderer()
        : Glib::ObjectBase(typeid(CellRendererText))
        , Gtk::CellRendererText()
        , property_plain_text(*this, "plain", "-") {}

    Glib::Property<Glib::ustring> property_plain_text;

    void snapshot_vfunc(Glib::RefPtr<Gtk::Snapshot> const &snapshot,
                        Gtk::Widget &widget,
                        const Gdk::Rectangle &background_area,
                        const Gdk::Rectangle &cell_area,
                        Gtk::CellRendererState flags) override
    {
        if ((bool)(flags & Gtk::CellRendererState::SELECTED)) {
            // Use plain text instead of marked-up text to render selected nodes, for legibility.
            property_text() = property_plain_text.get_value();
        }
        Gtk::CellRendererText::snapshot_vfunc(snapshot, widget, background_area, cell_area, flags);
    }
};

/************ XmlTreeView ************/

XmlTreeView::XmlTreeView()
{
    set_name("XmlTreeView");
    set_headers_visible(false);
    set_reorderable(false); // Don't interfere with D&D via controllers!
    set_enable_search(true);

    model_columns = std::make_unique<ModelColumns>();
    store = Gtk::TreeStore::create(*model_columns);
    set_model(store);

    // Text rendering
    formatter = std::unique_ptr<Inkscape::UI::Syntax::XMLFormatter>(new Inkscape::UI::Syntax::XMLFormatter());
    text_renderer = Gtk::make_managed<NodeRenderer>();
    auto text_column = Gtk::make_managed<Gtk::TreeViewColumn>();
    text_column->pack_start(*text_renderer, true);
    text_column->set_expand(true);
    text_column->add_attribute(*text_renderer, "markup", model_columns->markup);
    text_column->add_attribute(*text_renderer, "plain",  model_columns->text);
    append_column(*text_column);

    enable_model_drag_source ();
    auto const drag = Gtk::DragSource::create();
    drag->set_actions(Gdk::DragAction::MOVE);
    drag->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    drag->signal_prepare().connect([this, &drag = *drag](auto &&...args) { return on_prepare(drag, args...); }, false); // before
    add_controller(drag);

    auto const drop = Gtk::DropTarget::create(GlibValue::type<XMLDnDRow>(), Gdk::DragAction::MOVE);
    drop->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    drop->signal_motion().connect(sigc::mem_fun(*this, &XmlTreeView::on_drag_motion), false); // before
    drop->signal_drop().connect(sigc::mem_fun(*this, &XmlTreeView::on_drag_drop), false); // before
    add_controller(drop);
}

XmlTreeView::~XmlTreeView() = default;

// Build TreeView model, starting with root.
void
XmlTreeView::build_tree(SPDocument* document_in)
{
    document = document_in;

    root_watcher.reset();

    if (!document) {
        return;
    }

    auto root = document->getReprRoot();
    if (!root) {
        std::cerr << "XMLTreeView::set_root_watcher: No XML root!" << std::endl;
        return;
    }

    Gtk::TreeModel::Row row = *(store->prepend());
    root_watcher = std::make_unique<NodeWatcher>(this, root, &row);
}

Inkscape::XML::Node*
XmlTreeView::get_repr(Gtk::TreeModel::ConstRow const &row) const
{
    return row[model_columns->node];
}

/**
 * Select node in tree, if edit, move cursor.
 */
void
XmlTreeView::select_node(Inkscape::XML::Node *node, bool edit)
{
    auto selection = get_selection();

    if (node) {
        store->foreach_iter([this, node, edit, selection](const Gtk::TreeModel::iterator &it) {
            if ((*it)[model_columns->node] == node) {

                // Ensure node is shown
                auto path = store->get_path(it);
                expand_to_path(path);
                auto column = get_column(0);
                scroll_to_cell(path, *column, 0.66, 0.0);

                selection->unselect_all();
                selection->select(it);
                set_cursor(path, *column, edit);

                return true; // stop
            }
            return false; // continue
        });
    } else {
        // clear cursor to prevent Gtk from moving it on us if its row is deleted
        set_cursor(Gtk::TreePath());
        selection->unselect_all();
    }
}

/*
 * Set style for formatting tree.
 */
void
XmlTreeView::set_style(Inkscape::UI::Syntax::XMLStyles const &new_style)
{
    if (formatter) {
        formatter->setStyle(new_style);
    }
}

Glib::RefPtr<Gdk::ContentProvider>
XmlTreeView::on_prepare(Gtk::DragSource &controller, double x, double y)
{
    Gtk::TreeModel::Path path;
    Gtk::TreeView::DropPosition pos;
    get_dest_row_at_pos(x, y, path, pos);

    // Glib::ustring drag_label;
    Inkscape::XML::Node *node = nullptr;

    if (path) {
        // Don't drag root element <svg::svg/>!
        if (path.to_string() == "0") { // Gtkmm missing get_depth() function!
            return nullptr;
        }

        static GQuark const CODE_sodipodi_namedview = g_quark_from_static_string("sodipodi:namedview");
        static GQuark const CODE_svg_defs           = g_quark_from_static_string("svg:defs");
        
        if (auto row_iter = store->get_iter(path)) {
            node = (*row_iter)[model_columns->node];

            // Don't drag, document holds pointers to these elements which must stay valid.
            if (node->code() == CODE_sodipodi_namedview ||
                node->code() == CODE_svg_defs) {
                return nullptr;
            }

        } else {
            return nullptr;
        }

        // Set icon (or else icon is determined by provider value).
        auto surface = create_row_drag_icon(path);
        controller.set_icon(surface, x, 12);
    }

    return Gdk::ContentProvider::create(GlibValue::create<XMLDnDRow>(XMLDnDRow{node}));
}

Gdk::DragAction XmlTreeView::on_drag_motion(double const x, double const y)
{
    Gtk::TreeModel::Path path;
    Gtk::TreeView::DropPosition pos;
    get_dest_row_at_pos(x, y, path, pos);

    if (path) {
        if (auto row_iter = store->get_iter(path)) {
            // std::cout << "  Over " << (*row_iter)[model_columns->text] << std::endl;

            Inkscape::XML::Node *node = (*row_iter)[model_columns->node];
            bool const drop_into =
                pos != Gtk::TreeView::DropPosition::BEFORE &&
                pos != Gtk::TreeView::DropPosition::AFTER;

            // Only xml element nodes can have children.
            if (drop_into && node->type() != Inkscape::XML::NodeType::ELEMENT_NODE) {
                unset_drag_dest_row();
                return Gdk::DragAction{};
            }

            return Gdk::DragAction::MOVE;
        }
    }

    return Gdk::DragAction::MOVE; // Drag at bottom moves object to end.
}

bool XmlTreeView::on_drag_drop(Glib::ValueBase const &value, double x, double y)
{
    auto pointer = GlibValue::get<XMLDnDRow>(value);
    assert(pointer);
    auto node = pointer->node;
    assert(node);

    Glib:: ustring      id =  (     node->attribute("id") ?      node->attribute("id") : "Not element");

    Gtk::TreeModel::Path path;
    Gtk::TreeView::DropPosition pos;
    get_dest_row_at_pos(x, y, path, pos);

    if (!path) {
        if (is_blank_at_pos(x, y)) {
            // Move to end, why is this so hard?

            // Find first child of "svg:svg".
            path = Gtk::TreePath("0:0");
            auto row_iter = store->get_iter(path);
            Inkscape::XML::Node *child_node = (*row_iter)[model_columns->node];
            // Now count children.
            int n = 0;
            while (child_node->next()) {
                child_node = child_node->next();
                ++n;
            }
            Glib::ustring path_index = "0:" + std::to_string(n);
            path = Gtk::TreePath(path_index);
            pos = Gtk::TreeView::DropPosition::AFTER;
        } else {
            return true;
        }
    }

    auto row_iter = store->get_iter(path);
    assert (row_iter);

    Inkscape::XML::Node *drop_node = (*row_iter)[model_columns->node];

    if (node == drop_node) {
        // Don't drop onto self!
        return false;
    }

    bool const drop_into =
        pos != Gtk::TreeView::DropPosition::BEFORE && // 0
        pos != Gtk::TreeView::DropPosition::AFTER;    // 1

    auto parent_node = node->parent();
    auto drop_parent_node = drop_node->parent();

    // Glib:: ustring drop_id =  (drop_node->attribute("id") ? drop_node->attribute("id") : "Not element");
    // std::cout << "  node: " << node->name() << " (" << id << ")"
    //           << "  parent: " << parent_node->name()
    //           << "  drop node: " << drop_node->name() << "(" << drop_id << ")"
    //           << "  drop parent: " << drop_parent_node->name()
    //           << "  pos: " << (int)pos
    //           << "  drop_into: " << std::boolalpha << drop_into
    //           <<std::endl;

    if (drop_into) {
        // Only drop into containers!
        assert (document);
        auto item = document->getObjectByRepr(drop_node);
        if (item && (
                is<SPDefs>   (item) ||
                is<SPGlyph>  (item) ||
                is<SPGroup>  (item) || // (SPRoot, SPMarker, SPBox3D, SPSwitch, SPAnchor, SPSymbol)
                is<SPMask>   (item) ||
                is<SPPattern>(item) ||
                is<SPTSpan>  (item) ||
                is<SPText>   (item)
                )) {
            parent_node->removeChild(node);
            drop_node->addChild(node, nullptr);
        }
    } else {
        if (pos == Gtk::TreeView::DropPosition::BEFORE) {
            drop_node = drop_node->prev();
        }
        if (parent_node == drop_parent_node) {
            parent_node->changeOrder(node, drop_node);
        } else {
            parent_node->removeChild(node);
            drop_parent_node->addChild(node, drop_node);
        }
    }

    // while (node->parent()) { node = node->parent(); }
    // node->recursivePrintTree(0);

    return true;
}

} // namespace Inkscape::UI::Widget

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

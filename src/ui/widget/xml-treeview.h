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
 */

#ifndef SEEN_XML_TREEVIEW_H
#define SEEN_XML_TREEVIEW_H

#include <gtkmm/treeview.h>

#include "ui/syntax.h"  // XMLFormatter

namespace Gtk {
class TreeStore;
class DragSource;
class DropTarget;
} // namespace Gtk

namespace Inkscape::XML {
class Node;
} // namespace Inkscape::XML

namespace Inkscape::UI::Syntax {
class XMLFormatter;
} // namespace Inkscape::UI::Syntax

class SPDocument;

namespace Inkscape::UI::Widget {

class ModelColumns;
class NodeWatcher;

class XmlTreeView : public Gtk::TreeView
{
public:
    XmlTreeView();
    ~XmlTreeView() override;

    void build_tree(SPDocument* document); // set_root_watcher()
    Inkscape::XML::Node* get_repr(Gtk::TreeModel::ConstRow const &row) const;
    void select_node(Inkscape::XML::Node *node, bool edit = false);
    void set_style(Inkscape::UI::Syntax::XMLStyles const &new_style);
    Gtk::CellRendererText *get_renderer() { return text_renderer; }

private:

    friend class NodeWatcher;

    SPDocument* document = nullptr;
    Glib::RefPtr<Gtk::TreeStore> store;
    std::unique_ptr<ModelColumns> model_columns;
    std::unique_ptr<NodeWatcher> root_watcher;
    std::unique_ptr<Inkscape::UI::Syntax::XMLFormatter> formatter;
    Gtk::CellRendererText *text_renderer = nullptr;

    // ==== Controllers ====
    Glib::RefPtr<Gdk::ContentProvider> on_prepare(Gtk::DragSource &controller, double x, double y);
    Gdk::DragAction on_drag_motion(double x, double y);
    bool on_drag_drop(Glib::ValueBase const &/*value*/, double x, double y);
};

} // namespace Inkscape::UI::Widget

#endif // SEEN_XML_TREEVIEW_H

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

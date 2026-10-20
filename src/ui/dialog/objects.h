// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * A simple dialog for objects UI.
 *
 * Authors:
 *   Theodore Janeczko
 *   Tavmjong Bah
 *
 * Copyright (C) Theodore Janeczko 2012 <flutterguy317@gmail.com>
 *               Tavmjong Bah 2017
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_OBJECTS_PANEL_H
#define SEEN_OBJECTS_PANEL_H

#include <gtkmm/gesture.h>
#include <gtkmm/treerowreference.h>
#include <gtkmm/treeview.h>

#include "object/weakptr.h"
#include "selection.h"
#include "style-enums.h"
#include "ui/dialog/dialog-base.h"
#include "ui/widget/color-notebook.h"
#include "ui/widget/generic/popover-bin.h"
#include "ui/widget/preferences-widget.h"

namespace Glib {
class ValueBase;
} // namespace Glib

namespace Gdk {
class Drag;
} // namespace Gdk

namespace Gtk {
class Builder;
class CheckButton;
class DragSource;
class DropTarget;
class EventControllerKey;
class EventControllerMotion;
class GestureClick;
class Popover;
class Scale;
class SearchEntry2;
class TreeStore;
} // namespace Gtk

class SPItem;
class SPObject;

namespace Inkscape::UI {

namespace Widget { class ImageToggler; }

namespace Dialog {

class ObjectsPanel;
class ObjectWatcher;

enum {COL_LABEL, COL_VISIBLE, COL_LOCKED};

using SelectionState = int;
enum SelectionStates : SelectionState {
    SELECTED_NOT = 0,      // Object is NOT in desktop's selection
    SELECTED_OBJECT = 1,   // Object is in the desktop's selection
    LAYER_FOCUSED = 2,     // This layer is the desktop's focused layer
    LAYER_FOCUS_CHILD = 4, // This object is a child of the focused layer
    GROUP_SELECT_CHILD = 8 // This object is a child of the selected object
};

/**
 * A panel that displays objects.
 */
class ObjectsPanel : public DialogBase
{
public:
    ObjectsPanel();
    ~ObjectsPanel() override;

    class ModelColumns;

private:
    void desktopReplaced() override;
    void documentReplaced() override;
    void layerChanged(SPObject *obj);
    void selectionChanged(Selection *selected) override;
    ObjectWatcher *unpackToObject(SPObject *item);

    // Accessed by ObjectWatcher directly (friend class)
    SPObject* getObject(Inkscape::XML::Node *node);
    ObjectWatcher* getWatcher(Inkscape::XML::Node *node);
    ObjectWatcher *getRootWatcher() const { return root_watcher.get(); };
    bool showChildInTree(SPItem *item);

    Inkscape::XML::Node *getRepr(Gtk::TreeModel::ConstRow const &row) const;
    SPItem *getItem(Gtk::TreeModel::ConstRow const &row) const;
    std::optional<Gtk::TreeRow> getRow(SPItem *item) const;

    bool isDummy(Gtk::TreeModel::ConstRow const &row) const { return !getRepr(row); }
    bool removeDummyChildren(Gtk::TreeModel::Row row);
    bool cleanDummyChildren (Gtk::TreeModel::Row row);

    Glib::RefPtr<Gtk::TreeStore> _store;
    std::unique_ptr<ModelColumns> _model;

    void setRootWatcher();

    Glib::RefPtr<Gtk::Builder> _builder;
    Inkscape::PrefObserver _watch_object_mode;
    std::unique_ptr<ObjectWatcher> root_watcher;
    SPItem *current_item = nullptr;
    Gtk::TreeModel::Path _initial_path;
    bool _start_new_range = true;
    std::vector<SPWeakPtr<SPObject>> _prev_range;

    sigc::scoped_connection layer_changed;
    SPObject *_layer;
    Gtk::TreeModel::RowReference _hovered_row_ref;
    Gdk::RGBA _hovered_row_color;
    Gdk::RGBA _hovered_row_old_color;

    //Show icons in the context menu
    bool _show_contextmenu_icons;

    bool _is_editing;
    bool _scroll_lock = false;

    std::vector<Gtk::Widget*> _watching;
    std::vector<Gtk::Widget*> _watchingNonTop;
    std::vector<Gtk::Widget*> _watchingNonBottom;

    class TreeViewWithCssChanged;
    TreeViewWithCssChanged &_tree;
    Gtk::CellRendererText *_text_renderer = nullptr;
    Gtk::TreeView::Column *_name_column = nullptr;
    Gtk::TreeView::Column *_blend_mode_column = nullptr;
    Gtk::TreeView::Column *_eye_column = nullptr;
    Gtk::TreeView::Column *_lock_column = nullptr;
    Gtk::TreeView::Column *_color_tag_column = nullptr;
    Gtk::Box _buttonsRow;
    Gtk::Box _buttonsPrimary;
    Gtk::Box _buttonsSecondary;
    Gtk::SearchEntry2& _searchBox;
    Gtk::ScrolledWindow _scroller;
    Gtk::Box _page;
    sigc::scoped_connection _tree_style;
    Gtk::TreeRow _clicked_item_row;
    UI::Widget::PopoverBin _popoverbin;

    void _activateAction(const std::string& layerAction, const std::string& selectionAction);

    bool blendModePopup(int x, int y, Gtk::TreeModel::Row row);
    bool colorTagPopup(int x, int y, Gtk::TreeModel::Row row);
    bool toggleVisible(Gdk::ModifierType state, Gtk::TreeModel::Row row);
    bool toggleLocked (Gdk::ModifierType state, Gtk::TreeModel::Row row);

    enum class EventType {pressed, released};
    Gtk::EventSequenceState on_click(Gtk::GestureClick const &gesture,
                                     int n_press, double x, double y,
                                     EventType type);
    bool on_tree_key_pressed(Gtk::EventControllerKey const &controller,
                             unsigned keyval, unsigned keycode, Gdk::ModifierType state);
    bool on_window_key(Gtk::EventControllerKey const &controller,
                       unsigned keyval, unsigned keycode, Gdk::ModifierType state,
                       EventType type);
    void on_motion_enter(double x, double y);
    void on_motion_leave();
    void on_motion_motion(Gtk::EventControllerMotion const *controller, double x, double y);

    void _searchActivated();

    void _handleEdited(const Glib::ustring& path, const Glib::ustring& new_text);
    void _handleTransparentHover(bool enabled);
    void _generateTranslucentItems(SPItem *parent);
    void _updateIconVisibility();

    bool select_row( Glib::RefPtr<Gtk::TreeModel> const & model, Gtk::TreeModel::Path const & path, bool b );

    Glib::RefPtr<Gdk::ContentProvider> on_prepare(Gtk::DragSource &controller, double x, double y);
    void on_drag_begin(Glib::RefPtr<Gdk::Drag> const &drag);
    void on_drag_end(Glib::RefPtr<Gdk::Drag> const &drag, bool delete_data);
    Gdk::DragAction on_drag_motion(double x, double y);
    bool on_drag_drop(Glib::ValueBase const &value, double x, double y);

    void drag_end_impl();

    void selectRange(Gtk::TreeModel::Path start, Gtk::TreeModel::Path end);
    bool selectCursorItem(Gdk::ModifierType state);
    SPItem *_getCursorItem(Gtk::TreeViewColumn *column);

    friend class ObjectWatcher;

    int _msg_id;
    unsigned _translucency_key = 0;
    Gtk::Popover& _settings_menu;
    Gtk::Popover& _object_menu;
    std::shared_ptr<Colors::ColorSet> _colors;
    UI::Widget::ColorNotebook* _color_selector = nullptr;

    Gtk::Scale& _opacity_slider;
    std::map<SPBlendMode, Gtk::CheckButton *> _blend_items;
    std::map<SPBlendMode, Glib::ustring> _blend_mode_names;
    Inkscape::UI::Widget::ImageToggler* _item_state_toggler;

    // Special column dragging mode
    Gtk::TreeViewColumn* _drag_column = nullptr;

    UI::Widget::PrefCheckButton& _setting_layers;
    UI::Widget::PrefCheckButton& _setting_track;
    bool _drag_flip;

    bool _selectionChanged();
    sigc::scoped_connection _idle_connection;
};

} //namespace Dialog

} //namespace Inkscape::UI

#endif // SEEN_OBJECTS_PANEL_H

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

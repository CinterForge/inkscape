// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Combobox for selecting dash patterns - implementation.
 */
/* Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *   Mike Kowalski
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_SP_MARKER_COMBO_BOX_H
#define SEEN_SP_MARKER_COMBO_BOX_H

#include <giomm/liststore.h>
#include <gtkmm/box.h>
#include <gtkmm/menubutton.h>
#include <gtkmm/popover.h>
#include <gtkmm/cellrendererpixbuf.h>

#include "display/drawing.h"
#include "document.h"
#include "ui/operation-blocker.h"
#include "ui/widget-vfuncs-class-init.h"
#include "ui/widget/ink-property-grid.h"
#include "ui/widget/generic/snapshot-widget.h"
#include "ui/widget/generic/spin-button.h"
#include "ui/widget/recolor-art.h"
#include "ui/widget/recolor-art-manager.h"

namespace Gtk {
class Builder;
class Button;
class CheckButton;
class FlowBox;
class Image;
class Label;
class MenuButton;
class SpinButton;
class ToggleButton;
} // namespace Gtk

class SPDocument;
class SPMarker;
class SPObject;

namespace Inkscape::UI::Widget {

class Bin;

/**
 * ComboBox-like class for selecting stroke markers.
 */
class MarkerComboBox : public Gtk::MenuButton {
public:
    MarkerComboBox(Glib::ustring id, int loc);

    void setDocument(SPDocument *);
    void setDesktop(SPDesktop *desktop);
    void set_current(SPObject *marker);
    std::string get_active_marker_uri();
    bool in_update() const { return _update.pending(); };
    const char* get_id() const { return _combo_id.c_str(); };
    int get_loc() const { return _loc; };

    sigc::connection connect_changed(sigc::slot<void ()> slot);
    sigc::connection connect_edit   (sigc::slot<void ()> slot);
    // set a flat look
    void set_flat(bool flat);

private:
    struct MarkerItem : Glib::Object {
        SPDocument* source = nullptr;
        std::string id;
        std::string label;
        bool stock = false;
        bool history = false;
        int width = 0;
        int height = 0;

        bool operator == (const MarkerItem& item) const;

        static Glib::RefPtr<MarkerItem> create() {
            return Glib::make_refptr_for_instance(new MarkerItem());
        }

    protected:
        MarkerItem() = default;
    };

    SPMarker* get_current() const;
    Glib::ustring _current_marker_id;

    sigc::signal<void ()> _signal_changed;
    sigc::signal<void ()> _signal_edit;
    Glib::RefPtr<Gtk::Builder> _builder;
    Gtk::FlowBox& _marker_list;
    Gtk::Label& _marker_name;
    Glib::RefPtr<Gio::ListStore<MarkerItem>> _marker_store;
    std::vector<Glib::RefPtr<MarkerItem>> _stock_items;
    std::vector<Glib::RefPtr<MarkerItem>> _history_items;
    std::map<Gtk::Widget*, Glib::RefPtr<MarkerItem>> _widgets_to_markers;
    SnapshotWidget& _preview;
    bool _preview_no_alloc = true;
    Gtk::Button& _link_scale;
    InkSpinButton& _angle_btn;
    InkSpinButton& _scale_x;
    InkSpinButton& _scale_y;
    Gtk::CheckButton& _scale_with_stroke;
    InkSpinButton& _offset_x;
    InkSpinButton& _offset_y;
    InkSpinButton& _marker_alpha;
    Gtk::ToggleButton& _orient_auto_rev;
    Gtk::ToggleButton& _orient_auto;
    Gtk::ToggleButton& _orient_angle;
    Gtk::Button& _orient_flip_horz;
    SnapshotWidget _current_img;
    Gtk::Button& _edit_marker;
    bool _scale_linked = true;
    Glib::ustring _combo_id;
    int _loc;
    OperationBlocker _update;
    SPDocument *_document = nullptr;
    std::unique_ptr<SPDocument> _sandbox;
    InkPropertyGrid _grid;
    WidgetGroup _widgets;
    Gtk::CellRendererPixbuf _image_renderer;

    SPDesktop *_desktop = nullptr;
    sigc::scoped_connection _selection_changed_connection;
    Gtk::MenuButton *_recolorButtonTrigger = nullptr;

    class MarkerColumns : public Gtk::TreeModel::ColumnRecord
    {
    public:
        Gtk::TreeModelColumn<Glib::ustring> label;
        Gtk::TreeModelColumn<const gchar *> marker;   // ustring doesn't work here on windows due to unicode
        Gtk::TreeModelColumn<gboolean> stock;
        Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>> pixbuf;
        Gtk::TreeModelColumn<gboolean> history;
        Gtk::TreeModelColumn<gboolean> separator;

        MarkerColumns() {
            add(label); add(stock);  add(marker);  add(history); add(separator); add(pixbuf);
        }
    };
    MarkerColumns marker_columns;

    void update_ui(SPMarker *marker, bool select);
    void update_widgets_from_marker(SPMarker* marker);
    void update_store();
    Glib::RefPtr<MarkerItem> add_separator(bool filler);
    void update_scale_link();
    Glib::RefPtr<MarkerItem> get_active();
    Glib::RefPtr<MarkerItem> find_marker_item(SPMarker* marker);
    void update_preview(Glib::RefPtr<MarkerItem> marker_item);
    void update_menu_btn();
    void set_active(Glib::RefPtr<MarkerItem> item);
    void init_combo();
    void marker_list_from_doc(SPDocument* source, bool history);
    std::vector<SPMarker*> get_marker_list(SPDocument* source);
    void add_markers(std::vector<SPMarker *> const& marker_list, SPDocument *source, bool history);
    Cairo::RefPtr<Cairo::ImageSurface> create_marker_image(Geom::IntPoint pixel_size, gchar const *mname,
        SPDocument *source, Inkscape::Drawing &drawing, double scale, bool add_cross);
    void refresh_after_markers_modified();
    void draw_small_preview(const Glib::RefPtr<Gtk::Snapshot>& ctx, int width, int height, SPMarker* marker);
    void draw_big_preview(const Glib::RefPtr<Gtk::Snapshot>& snapshot, int width, int height);
    sigc::scoped_connection modified_connection;
    sigc::scoped_connection _idle;
    bool _is_up_to_date = false;
    Cairo::RefPtr<Cairo::ImageSurface> marker_to_image(Geom::IntPoint size, SPMarker* marker);
};

} // namespace Inkscape::UI::Widget

#endif // SEEN_SP_MARKER_COMBO_BOX_H

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

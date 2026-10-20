// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Combobox for selecting dash patterns - implementation.
 */
/* Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "marker-combo-box.h"

#include <cairo.h>
#include <optional>
#include <sstream>
#include <utility>
#include <glibmm/i18n.h>
#include <glibmm/main.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/flowbox.h>
#include <gtkmm/grid.h>
#include <gtkmm/popover.h>
#include <gtkmm/snapshot.h>
#include <gtkmm/togglebutton.h>

#include "helper/stock-items.h"
#include "io/resource.h"
#include "object/sp-defs.h"
#include "object/sp-marker-loc.h"
#include "object/sp-marker.h"
#include "object/sp-root.h"
#include "svg/css-ostringstream.h"
#include "ui/builder-utils.h"
#include "ui/svg-renderer.h"
#include "ui/util.h"
#include "ui/widget/generic/snapshot-widget.h"
#include "ui/widget/popover-utils.h"
#include "util/object-renderer.h"
#include "util/static-doc.h"

// size of marker image in a list
static constexpr int ITEM_WIDTH  = 35;
static constexpr int ITEM_HEIGHT = 28;

namespace Inkscape::UI::Widget {

namespace {

// create a "no marker is assigned" image
Cairo::RefPtr<Cairo::ImageSurface> create_separator(double alpha, int width, int height, int device_scale, int location) {
    width *= device_scale;
    height *= device_scale;
    auto surface = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, width, height);
    auto ctx = Cairo::Context::create(surface);
    auto x = 0.0;
    ctx->set_source_rgba(0.5, 0.5, 0.5, alpha);
    auto mid = height / 2;
    ctx->move_to(x, mid);
    ctx->line_to(x + width, mid);
    auto stroke = 2.0 * device_scale;
    ctx->set_line_width(stroke);
    ctx->stroke();
    auto h = 5 * device_scale;
    if (location == SP_MARKER_LOC_START) {
        ctx->move_to(x + stroke / 2, mid - h);
        ctx->line_to(x + stroke / 2, mid + h);
        ctx->stroke();
    }
    else if (location == SP_MARKER_LOC_END) {
        ctx->move_to(x + width - stroke / 2, mid - h);
        ctx->line_to(x + width - stroke / 2, mid + h);
        ctx->stroke();
    }
    surface->flush();
    surface->set_device_scale(device_scale, device_scale);
    return surface;
}

// empty images; "no marker" for start/middle/end markers
std::map<int, Cairo::RefPtr<Cairo::ImageSurface>> g_image_none;
// error extracting/rendering marker; "bad marker"
Cairo::RefPtr<Cairo::ImageSurface> g_bad_marker;

Glib::ustring get_attrib(SPMarker* marker, const char* attrib) {
    auto value = marker->getAttribute(attrib);
    return value ? value : "";
}

double get_attrib_num(SPMarker* marker, const char* attrib, double default_value = 0) {
    auto val = get_attrib(marker, attrib);
    return val.empty() ? default_value : strtod(val.c_str(), nullptr);
}

// find a marker object by ID in a document
SPMarker* find_marker(SPDocument* document, const Glib::ustring& marker_id) {
    if (!document) return nullptr;

    SPDefs* defs = document->getDefs();
    if (!defs) return nullptr;

    for (auto& child : defs->children) {
        if (is<SPMarker>(&child)) {
            auto marker = cast<SPMarker>(&child);
            auto id = marker->getId();
            if (id && marker_id == id) {
                // found it
                return marker;
            }
        }
    }

    // not found
    return nullptr;
}

void draw_marker_snapshot(const Glib::RefPtr<Gtk::Snapshot>& snapshot, int width, int height, int scale_factor, Glib::RefPtr<Gdk::Texture> texture) {
    if (width <= 1 || height <= 1 || !texture) return;

    Geom::Point img_size(texture->get_width(), texture->get_height());
    if (img_size.x() <= 0 || img_size.y() <= 0) return;
    // image physical pixels to display logical pixels:
    img_size /= scale_factor;

    Geom::Point size(width, height);
    // scale image to fit
    auto scale = size / img_size;
    auto uniform = std::min(scale.x(), scale.y());
    auto rescaled = img_size * uniform;
    // center it
    auto center = (size - rescaled) / 2;

    snapshot->translate(Gdk::Graphene::Point(center.x(), center.y()));
    snapshot->scale(uniform, uniform);
    snapshot->append_texture(texture, Gdk::Graphene::Rect(0, 0, img_size.x(), img_size.y()));
}

} // namespace

MarkerComboBox::MarkerComboBox(Glib::ustring id, int l) :
    _combo_id(std::move(id)),
    _loc(l),
    _builder(create_builder("marker-popup.glade")),
    _marker_list(get_widget<Gtk::FlowBox>(_builder, "flowbox")),
    _preview(get_derived_widget<SnapshotWidget>(_builder, "preview")),
    _marker_name(get_widget<Gtk::Label>(_builder, "marker-id")),
    _link_scale(get_widget<Gtk::Button>(_builder, "link-scale")),
    _scale_x(get_widget<InkSpinButton>(_builder, "scale-x")),
    _scale_y(get_widget<InkSpinButton>(_builder, "scale-y")),
    _scale_with_stroke(get_widget<Gtk::CheckButton>(_builder, "scale-with-stroke")),
    _angle_btn(get_widget<InkSpinButton>(_builder, "angle")),
    _offset_x(get_widget<InkSpinButton>(_builder, "offset-x")),
    _offset_y(get_widget<InkSpinButton>(_builder, "offset-y")),
    _marker_alpha(get_widget<InkSpinButton>(_builder, "alpha")),
    _orient_auto_rev(get_widget<Gtk::ToggleButton>(_builder, "orient-auto-rev")),
    _orient_auto(get_widget<Gtk::ToggleButton>(_builder, "orient-auto")),
    _orient_angle(get_widget<Gtk::ToggleButton>(_builder, "orient-angle")),
    _orient_flip_horz(get_widget<Gtk::Button>(_builder, "btn-horz-flip")),
    _edit_marker(get_widget<Gtk::Button>(_builder, "edit-marker")),
    _recolorButtonTrigger(Gtk::make_managed<Gtk::MenuButton>())
{
    set_name("MarkerComboBox");

    set_hexpand();
    set_always_show_arrow();
    set_popover(get_widget<Gtk::Popover>(_builder, "popover"));
    _current_img.set_snapshot_func([this](auto&& ...args) {
        draw_small_preview(args..., get_current());
    });
    set_child(_current_img);

    _preview.set_snapshot_func([this](auto&& ...args) {
        draw_big_preview(args...);
    });

    auto& input_grid = get_widget<Gtk::Grid>(_builder, "input-grid");
    _widgets = reparent_properties(input_grid, _grid, true, false, 1);
    get_widget<Gtk::Box>(_builder, "main-box").append(_grid);
    input_grid.set_visible(false);

    if (!g_image_none[_loc]) {
        auto device_scale = get_scale_factor();
        g_image_none[_loc] = create_separator(1, ITEM_WIDTH, ITEM_HEIGHT, device_scale, _loc);
    }

    if (!g_bad_marker) {
        auto path = Inkscape::IO::Resource::get_filename(Inkscape::IO::Resource::UIS, "bad-marker.svg");
        Inkscape::svg_renderer renderer(path.c_str());
        g_bad_marker = renderer.render_surface(1.0);
    }

    if (_loc == SP_MARKER_LOC_START) {
        set_tooltip_text(_("Start marker is drawn on the first node of a path"));
    }
    else if (_loc == SP_MARKER_LOC_MID) {
        set_tooltip_text(_("Middle markers are drawn on every node of the path except the first and last nodes"));
    }
    else if (_loc == SP_MARKER_LOC_END) {
        set_tooltip_text(_("End marker is drawn on the last node of a path"));
    }

    _marker_store = Gio::ListStore<MarkerItem>::create();
    _marker_list.bind_list_store(_marker_store, [this](const Glib::RefPtr<MarkerItem>& item) {
        auto image = Gtk::make_managed<SnapshotWidget>();
        image->set_size_request(ITEM_WIDTH, ITEM_HEIGHT);
        image->set_snapshot_func([this, item](const Glib::RefPtr<Gtk::Snapshot>& snapshot, int width, int height) {
            auto marker = find_marker(item->source, item->id);
            auto surface = marker ? marker_to_image({width, height}, marker) : g_image_none[_loc];
            draw_marker_snapshot(snapshot, width, height, get_scale_factor(), to_texture(surface));
        });
        auto const box = Gtk::make_managed<Gtk::FlowBoxChild>();
        box->set_child(*image);
        image->set_size_request(item->width, item->height);
        box->add_css_class("marker-item-box");
        _widgets_to_markers[image] = item;
        box->set_size_request(item->width, item->height);
        // removing the ability to focus from all items to prevent crash when user edits "Offset Y" and presses a tab key
        // to move to the next widget; not ideal, as it limits navigation, but lesser evil
        box->set_focusable(false);
        Glib::ustring tip = item->stock ? _("Stock marker:") : _("Document marker:");
        box->set_tooltip_text(tip + "\n" + item->label);
        return box;
    });

    _sandbox = Inkscape::ink_markers_preview_doc(_combo_id);

    set_sensitive(true);

    _marker_list.signal_selected_children_changed().connect([this]{
        auto item = get_active();
        if (!item && !_marker_list.get_selected_children().empty()) {
            _marker_list.unselect_all();
        }
    });

    _marker_list.signal_child_activated().connect([this](Gtk::FlowBoxChild* box){
        if (box->get_sensitive()) _signal_changed.emit();
    });

    auto set_orient = [this](bool enable_angle, const char* value) {
        if (_update.pending()) return;
        _angle_btn.set_sensitive(enable_angle);
        sp_marker_set_orient(get_current(), value);
    };
    _orient_auto_rev.signal_toggled().connect([=]{ set_orient(false, "auto-start-reverse"); });
    _orient_auto.signal_toggled().connect([=]    { set_orient(false, "auto"); });
    _orient_angle.signal_toggled().connect([=, this] {
        Inkscape::CSSOStringStream os;
        os << _angle_btn.get_value();
        set_orient(true, os.str().c_str());
    });
    _orient_flip_horz.signal_clicked().connect([this]()  { sp_marker_flip_horizontally(get_current()); });

    _angle_btn.signal_value_changed().connect([this](auto angle) {
        if (_update.pending() || !_angle_btn.is_sensitive()) return;
        Inkscape::CSSOStringStream os;
        os << angle;
        sp_marker_set_orient(get_current(), os.str().c_str());
    });

    auto set_scale = [this](bool changeWidth) {
        if (_update.pending()) return;

        if (auto marker = get_current()) {
            auto sx = _scale_x.get_value();
            auto sy = _scale_y.get_value();
            auto width  = get_attrib_num(marker, "markerWidth");
            auto height = get_attrib_num(marker, "markerHeight");
            if (_scale_linked && width > 0.0 && height > 0.0) {
                auto scoped(_update.block());
                if (changeWidth) {
                    // scale height proportionally
                    sy = height * (sx / width);
                    _scale_y.set_value(sy);
                }
                else {
                    // scale width proportionally
                    sx = width * (sy / height);
                    _scale_x.set_value(sx);
                }
            }
            sp_marker_set_size(marker, sx, sy);
        }
    };

    // delay setting scale to idle time; if invoked by focus change due to new marker selection,
    // it leads to marker list rebuild and apparent flowbox content corruption
    auto idle_set_scale = [=, this](bool changeWidth) {
        if (_update.pending()) return;

        if (auto orig_marker = get_current()) {
            _idle = Glib::signal_idle().connect([=, this]{
                if (auto marker = get_current()) {
                    if (marker == orig_marker) {
                        set_scale(changeWidth);
                    }
                }
                return false; // don't call again
            });
        }
    };

    _link_scale.signal_clicked().connect([this]{
        if (_update.pending()) return;
        _scale_linked = !_scale_linked;
        sp_marker_set_uniform_scale(get_current(), _scale_linked);
        update_scale_link();
    });

    _scale_x.signal_value_changed().connect([=](auto) { idle_set_scale(true); });
    _scale_y.signal_value_changed().connect([=](auto) { idle_set_scale(false); });

    _scale_with_stroke.signal_toggled().connect([this]{
        if (_update.pending()) return;
        sp_marker_scale_with_stroke(get_current(), _scale_with_stroke.get_active());
    });

    auto set_offset = [this]{
        if (_update.pending()) return;
        sp_marker_set_offset(get_current(), _offset_x.get_value(), _offset_y.get_value());
    };
    _offset_x.signal_value_changed().connect([=](auto) { set_offset(); });
    _offset_y.signal_value_changed().connect([=](auto) { set_offset(); });

    _marker_alpha.signal_value_changed().connect([this](double alpha) {
        if (_update.pending()) return;
        // change opacity
        sp_marker_set_opacity(get_current(), alpha);
    });
    // request to edit marker on canvas; close the popup to get it out of the way and call marker edit tool
    _edit_marker.signal_clicked().connect([this]{ get_popover()->popdown(); _signal_edit(); });
    // clear marker - unassign it
    get_widget<Gtk::Button>(_builder, "clear-marker").signal_clicked().connect([this] {
        _marker_list.unselect_all();
        _signal_changed.emit();
    });

    auto& popover = *get_popover();
    set_popover(popover);
    Utils::wrap_in_scrolled_window(popover, 150);

    // before showing popover refresh marker attributes
    get_popover()->signal_show().connect([this]{
        update_ui(get_current(), false);
        if (auto popover = get_popover()) {
            Utils::smart_position(*popover, *this);
        }
    }, false);

    init_combo();
    update_scale_link();
    update_menu_btn();

    _recolorButtonTrigger->set_label(_("Recolor Marker"));
    _recolorButtonTrigger->set_hexpand(true);
    _recolorButtonTrigger->set_vexpand(false);
    _recolorButtonTrigger->set_size_request(180);
    _recolorButtonTrigger->set_halign(Gtk::Align::FILL);
    _recolorButtonTrigger->set_valign(Gtk::Align::START);
    _recolorButtonTrigger->set_margin_top(8);
    _recolorButtonTrigger->set_direction(Gtk::ArrowType::NONE);
    _recolorButtonTrigger->set_visible(false);
    _grid.add_full_row(_recolorButtonTrigger);

    _recolorButtonTrigger->set_create_popup_func([this] {
        auto &mgr = RecolorArtManager::get();
        mgr.reparentPopoverTo(*_recolorButtonTrigger);
        mgr.widget.showForObject(_desktop, get_current());
    });
}

void MarkerComboBox::update_widgets_from_marker(SPMarker* marker) {
    _widgets.set_sensitive(marker != nullptr);

    if (marker) {
        _scale_x.set_value(get_attrib_num(marker, "markerWidth"));
        _scale_y.set_value(get_attrib_num(marker, "markerHeight"));
        auto units = get_attrib(marker, "markerUnits");
        _scale_with_stroke.set_active(units == "strokeWidth" || units == "");
        auto aspect = get_attrib(marker, "preserveAspectRatio");
        _scale_linked = aspect != "none";
        update_scale_link();
    // marker->setAttribute("markerUnits", scale_with_stroke ? "strokeWidth" : "userSpaceOnUse");
        _offset_x.set_value(get_attrib_num(marker, "refX"));
        _offset_y.set_value(get_attrib_num(marker, "refY"));
        _marker_alpha.set_value(get_attrib_num(marker, "fill-opacity", 100.0));
        auto orient = get_attrib(marker, "orient");

        // try parsing as a number
        _angle_btn.set_value(strtod(orient.c_str(), nullptr));
        if (orient == "auto-start-reverse") {
            _orient_auto_rev.set_active();
            _angle_btn.set_sensitive(false);
        }
        else if (orient == "auto") {
            _orient_auto.set_active();
            _angle_btn.set_sensitive(false);
        }
        else {
            _orient_angle.set_active();
            _angle_btn.set_sensitive(true);
        }

        bool should_show = RecolorArtManager::checkMarkerObject(marker);
        _recolorButtonTrigger->set_visible(should_show);
    }
}

void MarkerComboBox::update_scale_link() {
    _link_scale.set_icon_name(_scale_linked ? "entries-linked-symbolic" : "entries-unlinked-symbolic");
}

// update marker image inside the menu button
void MarkerComboBox::update_menu_btn() {
    _current_img.queue_draw();
}

// update the marker preview image in the popover panel
void MarkerComboBox::update_preview(Glib::RefPtr<MarkerItem> item) {
    Glib::ustring label;

    if (!item) {
        // TRANSLATORS: None - no marker selected for a path
        label = _("None");
    }
    else if (item->source && !item->id.empty()) {
        label = _(item->label.c_str());
    }

    _preview.queue_draw();

    std::ostringstream ost;
    ost << "<small>" << label.raw() << "</small>";
    _marker_name.set_markup(ost.str().c_str());
}

bool MarkerComboBox::MarkerItem::operator == (const MarkerItem& item) const {
    return
        id == item.id &&
        label == item.label &&
        stock == item.stock &&
        history == item.history &&
        source == item.source &&
        width == item.width &&
        height == item.height;
}

SPMarker* MarkerComboBox::get_current() const {
    // find current marker
    return find_marker(_document, _current_marker_id);
}

void MarkerComboBox::set_active(Glib::RefPtr<MarkerItem> item) {
    bool selected = false;
    if (item) {
        for (auto &widget : children(_marker_list)) {
            if (auto box = dynamic_cast<Gtk::FlowBoxChild*>(&widget)) {
                if (auto marker = _widgets_to_markers[box->get_child()]) {
                    if (*marker == *item) {
                        _marker_list.select_child(*box);
                        selected = true;
                    }
                }
            }
        }
    }

    if (!selected) {
        _marker_list.unselect_all();
    }
}

Glib::RefPtr<MarkerComboBox::MarkerItem> MarkerComboBox::find_marker_item(SPMarker* marker) {
    std::string id;
    if (marker != nullptr) {
        if (auto markname = marker->getRepr()->attribute("id")) {
            id = markname;
        }
    }

    Glib::RefPtr<MarkerItem> marker_item;
    if (!id.empty()) {
        for (auto&& item : _history_items) {
            if (item->id == id) {
                marker_item = item;
                break;
            }
        }
    }

    return marker_item;
}

Glib::RefPtr<MarkerComboBox::MarkerItem> MarkerComboBox::get_active() {
    auto empty = Glib::RefPtr<MarkerItem>();
    auto sel = _marker_list.get_selected_children();
    if (sel.size() == 1) {
        return _widgets_to_markers[sel.front()->get_child()];
    }
    else {
        return empty;
    }
}

void MarkerComboBox::setDesktop(SPDesktop *desktop)
{
    if (_desktop == desktop) {
        return;
    }

    RecolorArtManager::get().popover.popdown();

    _desktop = desktop;
}

void MarkerComboBox::setDocument(SPDocument *document)
{
    if (_document != document) {
        if (_document) {
            modified_connection.disconnect();
        }

        _document = document;

        if (_document) {
            modified_connection = _document->getDefs()->connectModified([this](SPObject*, unsigned int){
                if (get_popover()->is_visible()) {
                    refresh_after_markers_modified();
                }
                else {
                    _is_up_to_date = false;
                }
            });
        }

        _current_marker_id = "";
        refresh_after_markers_modified();
    }
}

/**
 * This function is invoked after the document "defs" section changes.
 * It will change when the current marker's attributes are modified in this popup,
 * and this function will refresh the recent list and a preview to reflect the changes.
 * It would be more efficient if there was a way to determine what has changed
 * and perform only more targeted update.
 */
void MarkerComboBox::refresh_after_markers_modified() {
    if (_update.pending()) return;

    auto scoped(_update.block());

    /*
     * Seems to be no way to get notified of changes just to markers,
     * so listen to changes in all defs and check if the number of markers has changed here
     * to avoid unnecessary refreshes when things like gradients change
    */
   // TODO: detect changes to markers; ignore changes to everything else;
   // simple count check doesn't cut it, so just do it unconditionally for now
    marker_list_from_doc(_document, true);

    auto marker = find_marker_item(get_current());
    update_menu_btn();
    update_preview(marker);
    _is_up_to_date = true;
}

Cairo::RefPtr<Cairo::ImageSurface> MarkerComboBox::marker_to_image(Geom::IntPoint size, SPMarker* marker) {
    if (!marker) return g_bad_marker;

    Inkscape::Drawing drawing;
    unsigned const visionkey = SPItem::display_key_new(1);
    drawing.setRoot(_sandbox->getRoot()->invoke_show(drawing, visionkey, SP_ITEM_SHOW_DISPLAY));
    auto surface = create_marker_image(size, marker->getId(), marker->document, drawing, 1.50, false);
    _sandbox->getRoot()->invoke_hide(visionkey);
    return surface ? surface : g_bad_marker;
}

// big preview in the popup
void MarkerComboBox::draw_big_preview(const Glib::RefPtr<Gtk::Snapshot>& snapshot, int width, int height) {
    auto item = find_marker_item(get_current());
    if (!item || !item->source || item->id.empty()) return;

    Inkscape::Drawing drawing;
    unsigned const visionkey = SPItem::display_key_new(1);
    drawing.setRoot(_sandbox->getRoot()->invoke_show(drawing, visionkey, SP_ITEM_SHOW_DISPLAY));
    // generate preview
    auto surface = create_marker_image({width, height}, item->id.c_str(), item->source, drawing, 2.60, true);
    _sandbox->getRoot()->invoke_hide(visionkey);

    draw_marker_snapshot(snapshot, width, height, get_scale_factor(), to_texture(surface));
}

// small preview inside the MenuButton
void MarkerComboBox::draw_small_preview(const Glib::RefPtr<Gtk::Snapshot>& snapshot, int width, int height, SPMarker* marker) {
    auto surface = marker ? marker_to_image({ITEM_WIDTH, ITEM_HEIGHT}, marker) : g_image_none[_loc];
    draw_marker_snapshot(snapshot, width, height, get_scale_factor(), to_texture(surface));
}

/**
 * Init the combobox widget to display markers from markers.svg
 */
void MarkerComboBox::init_combo() {
    auto const markers_doc = Util::cache_static_doc([] {
        // find and load markers.svg
        using namespace Inkscape::IO::Resource;
        auto markers_source = get_path_string(SYSTEM, MARKERS, "markers.svg");
        return SPDocument::createNewDoc(markers_source.c_str());
    });

    // load markers from markers.svg
    if (markers_doc) {
        marker_list_from_doc(markers_doc, false);
    }

    refresh_after_markers_modified();
}

/**
 * Sets the current marker in the marker combobox.
 */
void MarkerComboBox::set_current(SPObject *marker)
{
    auto sp_marker = cast<SPMarker>(marker);

    bool reselect = sp_marker != get_current();

    auto id = marker ? marker->getId() : nullptr;
    _current_marker_id = id ? id : "";

    if (get_popover()->is_visible()) {
        update_ui(sp_marker, reselect);
    }
    else {
        // if popup is hidden, there's no need to rebuild the store;
        // update menu button only
        update_menu_btn();
    }

    auto& mgr = RecolorArtManager::get();
    if (mgr.popover.is_visible() && RecolorArtManager::checkMarkerObject(get_current())) {
        mgr.widget.showForObject(_desktop, get_current());
    }
}

void MarkerComboBox::update_ui(SPMarker* marker, bool select) {
    if (!_is_up_to_date) {
        refresh_after_markers_modified();
    }

    auto scoped(_update.block());

    auto marker_item = find_marker_item(marker);

    if (select) {
        set_active(marker_item);
    }

    update_widgets_from_marker(marker);
    update_menu_btn();
    update_preview(marker_item);
}

/**
 * Return a uri string representing the current selected marker used for setting the marker style in the document
 */
std::string MarkerComboBox::get_active_marker_uri()
{
    /* Get Marker */
    auto item = get_active();
    if (!item) {
        return std::string();
    }

    std::string marker;

    if (item->id != "none") {
        bool stockid = item->stock;

        std::string markurn = stockid ? "urn:inkscape:marker:" + item->id : item->id;
        auto mark = cast<SPMarker>(get_stock_item(markurn.c_str(), stockid));

        if (mark) {
            Inkscape::XML::Node* repr = mark->getRepr();
            auto id = repr->attribute("id");
            if (id) {
                std::ostringstream ost;
                ost << "url(#" << id << ")";
                marker = ost.str();
            }
            if (stockid) {
                mark->getRepr()->setAttribute("inkscape:collect", "always");
            }
            // adjust marker's attributes (or add missing ones) to stay in sync with marker tool
            sp_validate_marker(mark, _document);
        }
    } else {
        marker = item->id;
    }

    return marker;
}

/**
 * Pick up all markers from the source and add items to the list/store.
 * If 'history' is true, then update recently used in-document portion of the list;
 * otherwise update a list of stock markers, which is displayed after recent ones
 */
void MarkerComboBox::marker_list_from_doc(SPDocument* source, bool history) {
    std::vector<SPMarker*> markers = get_marker_list(source);
    if (history) {
        _history_items.clear();
    }
    else {
        _stock_items.clear();
    }
    add_markers(markers, source, history);
    update_store();
}

void MarkerComboBox::update_store() {
    _marker_store->freeze_notify();

    auto selected = get_active();

    _marker_store->remove_all();
    _widgets_to_markers.clear();

    // recent and user-defined markers come first
    _marker_store->splice(0, 0, _history_items);
    // stock markers
    _marker_store->splice(_marker_store->get_n_items(), 0, _stock_items);

    _marker_store->thaw_notify();

    // reselect current
    set_active(selected);
}
/**
 *  Returns a vector of markers in the defs of the given source document as a vector.
 *  Returns empty vector if there are no markers in the document.
 *  If validate is true then it runs each marker through the validation routine that alters some attributes.
 */
std::vector<SPMarker*> MarkerComboBox::get_marker_list(SPDocument* source)
{
    std::vector<SPMarker *> ml;
    if (source == nullptr) return ml;

    SPDefs *defs = source->getDefs();
    if (!defs) {
        return ml;
    }

    for (auto& child: defs->children) {
        if (is<SPMarker>(&child)) {
            auto marker = cast<SPMarker>(&child);
            ml.push_back(marker);
        }
    }
    return ml;
}

/**
 * Adds markers in marker_list to the combo
 */
void MarkerComboBox::add_markers(std::vector<SPMarker *> const& marker_list, SPDocument *source, bool history) {
    for (auto i:marker_list) {

        Inkscape::XML::Node *repr = i->getRepr();
        gchar const *markid = repr->attribute("inkscape:stockid") ? repr->attribute("inkscape:stockid") : repr->attribute("id");

        auto item = MarkerItem::create();
        item->source = source;
        if (auto id = repr->attribute("id")) {
            item->id = id;
        }
        item->label = markid ? markid : "";
        item->stock = !history;
        item->history = history;
        item->width = ITEM_WIDTH;
        item->height = ITEM_HEIGHT;

        if (history) {
            _history_items.emplace_back(std::move(item));
        }
        else {
            _stock_items.emplace_back(std::move(item));
        }
    }
}

/**
 * Creates a copy of the marker named mname, determines its visible and renderable
 * area in the bounding box, and then renders it. This allows us to fill in
 * preview images of each marker in the marker combobox.
 */
Cairo::RefPtr<Cairo::ImageSurface>
MarkerComboBox::create_marker_image(Geom::IntPoint pixel_size, gchar const *mname,
    SPDocument *source, Inkscape::Drawing &drawing, double scale, bool add_cross)
{
    auto const fg = get_color();
    bool no_clip = true;
    return Inkscape::create_marker_image(_combo_id, _sandbox.get(), fg, pixel_size, mname, source,
        drawing, {}, no_clip, scale, get_scale_factor(), add_cross);
}

sigc::connection MarkerComboBox::connect_changed(sigc::slot<void ()> slot)
{
    return _signal_changed.connect(std::move(slot));
}

sigc::connection MarkerComboBox::connect_edit(sigc::slot<void ()> slot)
{
    return _signal_edit.connect(std::move(slot));
}

void MarkerComboBox::set_flat(bool flat) {
    set_always_show_arrow(!flat);
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

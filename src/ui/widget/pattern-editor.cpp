// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Pattern editor widget for the "Fill and Stroke" and "Object Properties" dialogs.
 *
 * Copyright (C) 2022-2025 Michael Kowalski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "pattern-editor.h"

#include <iomanip>
#include <glibmm/i18n.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/flowbox.h>
#include <gtkmm/grid.h>
#include <gtkmm/paned.h>
#include <gtkmm/scale.h>
#include <gtkmm/searchentry2.h>
#include <gtkmm/treemodel.h>

#include "document.h"
#include "object/sp-pattern.h"
#include "object/sp-hatch.h"
#include "preferences.h"
#include "pattern-manager.h"
#include "pattern-manipulation.h"
#include "ui/builder-utils.h"
#include "ui/monitor.h"
#include "ui/util.h"
#include "util-string/string-compare.h"

namespace Inkscape::UI::Widget {

using namespace Inkscape::IO;

namespace {
// default size of pattern image in a list
constexpr int ITEM_WIDTH = 45;

// tile size slider functions
int slider_to_tile(double index) {
    return 30 + static_cast<int>(index) * 5;
}
double tile_to_slider(int tile) {
    return (tile - 30) / 5.0;
}

int get_max_list_height() {
    static int max_height = std::max(500, get_monitor_geometry_primary().get_height() - 400);
    return max_height;
}

} // namespace

PatternEditor::PatternEditor(const char* prefs, PatternManager& manager) :
    _manager(manager),
    _builder(create_builder("pattern-edit.glade")),
    _offset_x(get_widget<InkSpinButton>(_builder, "offset-x")),
    _offset_y(get_widget<InkSpinButton>(_builder, "offset-y")),
    _scale_x(get_widget<InkSpinButton>(_builder, "scale-x")),
    _scale_y(get_widget<InkSpinButton>(_builder, "scale-y")),
    _angle_btn(get_widget<InkSpinButton>(_builder, "angle")),
    _gap_x_spin(get_widget<InkSpinButton>(_builder, "gap-x-spin")),
    _gap_y_spin(get_widget<InkSpinButton>(_builder, "gap-y-spin")),
    _pitch_spin(get_widget<InkSpinButton>(_builder, "pitch-spin")),
    _stroke_spin(get_widget<InkSpinButton>(_builder, "stroke-spin")),
    _gap_label(get_widget<Gtk::Label>(_builder, "gap-label")),
    _pitch_label(get_widget<Gtk::Label>(_builder, "pitch-label")),
    _stroke_label(get_widget<Gtk::Label>(_builder, "stroke-label")),
    _edit_btn(get_widget<Gtk::Button>(_builder, "edit-pattern")),
    _preview(get_widget<Gtk::DrawingArea>(_builder, "preview")),
    _paned(get_widget<Gtk::Paned>(_builder, "paned")),
    _input_grid(get_widget<Gtk::Grid>(_builder, "input-grid")),
    _stock_gallery(get_widget<Gtk::FlowBox>(_builder, "flowbox")),
    _doc_gallery(get_widget<Gtk::FlowBox>(_builder, "doc-flowbox")),
    _link_scale(get_widget<Gtk::Button>(_builder, "link-scale")),
    _name_box(get_widget<Gtk::Entry>(_builder, "pattern-name")),
    _combo_set(get_widget<Gtk::ComboBoxText>(_builder, "pattern-combo")),
    _search_box(get_widget<Gtk::SearchEntry2>(_builder, "search")),
    _tile_slider(get_widget<Gtk::Scale>(_builder, "tile-slider")),
    _show_names(get_widget<Gtk::CheckButton>(_builder, "show-names")),
    _color_picker(get_derived_widget<ColorPicker>(_builder, "color-btn", _("Pattern color"), false)),
    _separator(get_derived_widget<ResizingSeparator>(_builder, "resize")),
    _prefs(prefs)
{
    _separator.set_orientation(ResizingSeparator::Orientation::Vertical);
    reparent_properties(_input_grid, _main);
    _main.set_hexpand();

    _color_picker.connectChanged([this](Colors::Color const &color){
        if (_update.pending()) return;
        _signal_color_changed.emit(color);
    });

    _preview.set_draw_func([this](auto&& ...args){ draw_preview(args...); });

    _tile_size = Preferences::get()->getIntLimited(_prefs + "/tileSize", ITEM_WIDTH, 30, 1000);
    _tile_slider.set_value(tile_to_slider(_tile_size));
    _tile_slider.signal_change_value().connect([this](Gtk::ScrollType st, double value){
        if (_update.pending()) return true;
        auto scoped(_update.block());
        auto size = slider_to_tile(value);
        if (size != _tile_size) {
            _tile_slider.set_value(tile_to_slider(size));
            // change pattern tile size
            _tile_size = size;
            update_pattern_tiles();
            Preferences::get()->setInt(_prefs + "/tileSize", size);
        }
        return true;
    }, true);

    auto show_labels = Preferences::get()->getBool(_prefs + "/showLabels", false);
    _show_names.set_active(show_labels);
    _show_names.signal_toggled().connect([this]{
        // toggle pattern labels
        _stock_pattern_store.store.refresh();
        _doc_pattern_store.store.refresh();
        Preferences::get()->setBool(_prefs + "/showLabels", _show_names.get_active());
    });

    for (auto spin : {&_gap_x_spin, &_gap_y_spin, &_pitch_spin, &_stroke_spin}) {
        spin->signal_value_changed().connect([spin, this](double value){
            if (_update.pending() || !spin->is_sensitive()) return;
            _signal_changed.emit();
        });
    }

    _angle_btn.signal_value_changed().connect([this](double angle) {
        if (_update.pending() || !_angle_btn.is_sensitive()) return;
        auto scoped(_update.block());
        _signal_changed.emit();
    });

    _link_scale.signal_clicked().connect([this]{
        if (_update.pending()) return;
        auto scoped(_update.block());
        _scale_linked = !_scale_linked;
        if (_scale_linked) {
            // this is simplistic
            _scale_x.set_value(_scale_y.get_value());
        }
        update_scale_link();
        if (_uniform_supported) _signal_changed.emit();
    });

    for (auto el : {&_scale_x, &_scale_y, &_offset_x, &_offset_y}) {
        el->signal_value_changed().connect([el, this](double value) {
            if (_update.pending()) return;
            if (_scale_linked && (el == &_scale_x || el == &_scale_y)) {
                auto scoped(_update.block());
                // enforce uniform scaling
                (el == &_scale_x) ? _scale_y.set_value(value) : _scale_x.set_value(value);
            }
            _signal_changed.emit();
        });
    }

    _name_box.signal_changed().connect([this]{
        if (_update.pending()) return;

        _signal_changed.emit();
    });

    _search_box.signal_search_changed().connect([this]{
        if (_update.pending()) return;

        // filter patterns
        _filter_text = _search_box.get_text();
        apply_filter(false);
        apply_filter(true);
    });

    bind_store(_doc_gallery, _doc_pattern_store);
    bind_store(_stock_gallery, _stock_pattern_store);

    _stock_gallery.signal_child_activated().connect([this](Gtk::FlowBoxChild* box){
        if (_update.pending()) return;
        auto scoped(_update.block());
        auto pat = _stock_pattern_store.widgets_to_pattern[box];
        update_ui(pat);
        _doc_gallery.unselect_all();
        _signal_changed.emit();
    });

    _doc_gallery.signal_child_activated().connect([this](Gtk::FlowBoxChild* box){
        if (_update.pending()) return;
        auto scoped(_update.block());
        auto pat = _doc_pattern_store.widgets_to_pattern[box];
        update_ui(pat);
        _stock_gallery.unselect_all();
        _signal_changed.emit();
    });

    _edit_btn.signal_clicked().connect([this]{
        _signal_edit.emit();
    });

    _paned.set_position(Preferences::get()->getIntLimited(_prefs + "/handlePos", 50, 10, 9999));
    _paned.property_position().signal_changed().connect([this](){
        Preferences::get()->setInt(_prefs + "/handlePos", _paned.get_position());
    });

    _list_height = Preferences::get()->getIntLimited(_prefs + "/listHeight", _list_height, 100, get_max_list_height());
    _separator.resize(&_paned, Geom::Point(-1, get_max_list_height()));
    _separator.get_signal_resized().connect([this](Geom::Point size) {
        _list_height = size.y();
        Preferences::get()->setInt(_prefs + "/listHeight", _list_height);
    });
    _paned.set_size_request(-1, _list_height);

    update_scale_link();
    set_vexpand();
    append(_main);
}

void PatternEditor::bind_store(Gtk::FlowBox& list, PatternStore& pat) {
    pat.store.set_filter([this](const Glib::RefPtr<PatternItem>& p){
        if (!p) return false;
        if (_filter_text.empty()) return true;

        auto name = Glib::ustring(p->label).lowercase();
        auto expr = _filter_text.lowercase();
        auto pos = name.find(expr);
        return pos != Glib::ustring::npos;
    });

    list.bind_list_store(pat.store.get_store(), [&pat, this](const Glib::RefPtr<PatternItem>& item){
        auto const box = Gtk::make_managed<Box>(Gtk::Orientation::VERTICAL);
        auto const image = Gtk::make_managed<Gtk::Image>(to_texture(item->pix));
        image->set_size_request(_tile_size, _tile_size);
        image->set_pixel_size(_tile_size);
        box->append(*image);
        auto name = Glib::ustring(item->label.c_str());
        if (_show_names.get_active()) {
            auto const label = Gtk::make_managed<Gtk::Label>(name);
            label->add_css_class("small-font");
            // limit label size to tile size
            label->set_ellipsize(Pango::EllipsizeMode::END);
            label->set_max_width_chars(0);
            label->set_size_request(_tile_size);
            box->append(*label);
        }
        image->set_tooltip_text(name);

        auto const cbox = Gtk::make_managed<Gtk::FlowBoxChild>();
        cbox->set_child(*box);
        cbox->add_css_class("pattern-item-box");
        pat.widgets_to_pattern[cbox] = item;
        return cbox;
    });
}

void PatternEditor::select_pattern_set(int index) {
    auto sets = _manager.get_categories()->children();
    if (index >= 0 && index < sets.size()) {
        auto row = sets[index];
        if (auto category = row.get_value(_manager.columns.category)) {
            set_stock_patterns(category->patterns);
        }
    }
}

void PatternEditor::update_scale_link() {
    _link_scale.set_icon_name(_scale_linked ? "entries-linked-symbolic" : "entries-unlinked-symbolic");
}

void PatternEditor::update_widgets_from_pattern(Glib::RefPtr<PatternItem>& pattern) {
    _input_grid.set_sensitive(!!pattern);

    static auto const empty = PatternItem::create();
    auto const &item = pattern ? *pattern : *empty;

    if (_name_box.get_text().raw() != item.label) {
        _name_box.set_text(item.label.c_str());
    }

    auto scale_x = item.transform.xAxis().length();
    auto scale_y = item.transform.yAxis().length();
    _scale_x.set_value(scale_x);
    _scale_y.set_value(scale_y);

    // TODO if needed
    // auto units = get_attrib(pattern, "patternUnits");

    // if uniform scale attribute is not supported, then we simulate it by comparing scale values
    _scale_linked = item.uniform_scale.value_or(Geom::are_near(scale_x, scale_y));
    _uniform_supported = item.uniform_scale.has_value();
    update_scale_link();

    _offset_x.set_value(item.offset.x());
    _offset_y.set_value(item.offset.y());

    // show rotation in degrees
    auto degrees = item.rotation.has_value() ? *item.rotation : 180.0 / M_PI * Geom::atan2(item.transform.xAxis());
    _angle_btn.set_value(degrees);

    if (item.pitch.has_value()) {
        _pitch_spin.set_value(item.pitch.value());
    }
    else {
        _gap_x_spin.set_value(item.gap[Geom::X]);
        _gap_y_spin.set_value(item.gap[Geom::Y]);
    }
    _pitch_spin.set_visible(item.pitch.has_value());
    _pitch_label.set_visible(item.pitch.has_value());
    _gap_x_spin.set_visible(!item.pitch.has_value());
    _gap_y_spin.set_visible(!item.pitch.has_value());
    _gap_label.set_visible(!item.pitch.has_value());

    _stroke_spin.set_value(item.stroke.value_or(0));
    _stroke_spin.set_visible(item.stroke.has_value());
    _stroke_label.set_visible(item.stroke.has_value());

    // is coloring possible?
    if (item.color.has_value()) {
        _color_picker.setColor(*item.color);
        _color_picker.set_sensitive();
    }
    else {
        _color_picker.setColor(Colors::Color(0x0));
        _color_picker.set_sensitive(false);
        _color_picker.close();
    }

    // pattern/hatch tile editing
    _edit_btn.set_sensitive(item.editable);
}

void PatternEditor::update_ui(Glib::RefPtr<PatternItem> pattern) {
    update_widgets_from_pattern(pattern);
}

// sort patterns in-place by name/id
void sort_patterns(std::vector<Glib::RefPtr<PatternItem>>& list) {
    std::sort(list.begin(), list.end(), [](Glib::RefPtr<PatternItem>& a, Glib::RefPtr<PatternItem>& b) {
        if (!a || !b) return false;
        if (a->label == b->label) {
            return a->id < b->id;
        }
        return natural_compare(a->label, b->label);
    });
}

// given a pattern/hatch, create a PatternItem instance that describes it;
// the input pattern/hatch can be a link or a root pattern/hatch
Glib::RefPtr<PatternItem> create_pattern_item(PatternManager& manager, SPPaintServer* paint, int tile_size, double scale) {
    auto item = manager.get_item(paint);
    if (item && scale > 0) {
        // check if any existing cached image needs to be regenerated
        if (!item->pix || tile_size != item->pix->get_width() || scale != item->pix->get_device_scale()) {
            item->pix = manager.get_image(paint, tile_size, tile_size, scale);
        }
    }
    return item;
}

void PatternEditor::set_initial_selection() {
    auto [id, doc] = get_selected();
    if (id.empty()) return;

    auto scoped(_update.block());
    auto element = doc ? doc->getObjectById(id) : (_current_document ? _current_document->getObjectById(id) : nullptr);
    if (auto paint = cast<SPPaintServer>(element)) {
        auto item = create_pattern_item(_manager, paint, 0, 0);
        update_widgets_from_pattern(item);
    }
}

// update editor UI
void PatternEditor::set_selected(SPPattern* pattern) {
    // current 'pattern' (should be a link)
    auto offset = pattern ? pattern->getTransform().translation() : Geom::Point();
    _set_selected(pattern, pattern ? pattern->rootPattern() : nullptr, offset);
}

void PatternEditor::set_selected(SPHatch* hatch) {
    auto offset = Geom::Point(); // no need to preserve 'transform' offset; hatch has dedicated x, y attributes that we change
    _set_selected(hatch, hatch ? hatch->rootHatch() : nullptr, offset);
}

void PatternEditor::_set_selected(SPPaintServer* link_paint, SPPaintServer* root_paint, Geom::Point offset) {
    auto scoped(_update.block());

    _stock_gallery.unselect_all();

    if (root_paint && root_paint != link_paint) {
        _current_pattern.id = root_paint->getId();
        _current_pattern.link_id = link_paint->getId();
        _current_pattern.offset = offset;
    }
    else {
        _current_pattern.id.clear();
        _current_pattern.link_id.clear();
        _current_pattern.offset = {};
    }

    auto item = create_pattern_item(_manager, link_paint, 0, 0);

    update_widgets_from_pattern(item);

    auto list = update_doc_pattern_list(root_paint ? root_paint->document : nullptr);
    if (root_paint) {
        // patch up a tile image on a list of document root patterns, it might have changed;
        // color attribute, for instance, is being set directly on the root pattern;
        // other attributes are per-object, so should not be taken into account when rendering a tile
        for (auto& pattern_item : list) {
            if (pattern_item->id == item->id && pattern_item->collection == nullptr) {
                // update preview
                const double device_scale = get_scale_factor();
                pattern_item->pix = _manager.get_image(root_paint, _tile_size, _tile_size, device_scale);
                item->pix = pattern_item->pix;
                break;
            }
        }
    }

    set_active(_doc_gallery, _doc_pattern_store, item);
    // draw a large preview of the selected pattern
    _preview.queue_draw();
}

// generate preview images for patterns
std::vector<Glib::RefPtr<PatternItem>> create_pattern_items(PatternManager& manager, const std::vector<SPPaintServer*>& list, int tile_size, double device_scale) {
    std::vector<Glib::RefPtr<PatternItem>> output;
    output.reserve(list.size());

    for (auto pat : list) {
        if (auto item = create_pattern_item(manager, pat, tile_size, device_scale)) {
            output.push_back(item);
        }
    }

    return output;
}

// populate the store with document patterns if a list has changed, minimize the amount of work by using cached previews
std::vector<Glib::RefPtr<PatternItem>> PatternEditor::update_doc_pattern_list(SPDocument* document) {
    auto list = sp_get_pattern_list(document);
    auto hatch = sp_get_hatch_list(document);
    list.insert(list.begin(), hatch.begin(), hatch.end());
    const double device_scale = get_scale_factor();
    // create pattern items (cheap), but skip preview generation (expensive)
    auto patterns = create_pattern_items(_manager, list, 0, 0);
    bool modified = false;
    for (auto&& item : patterns) {
        auto it = _cached_items.find(item->id);
        if (it != end(_cached_items)) {
            // reuse cached preview image
            if (!item->pix) item->pix = it->second->pix;
        }
        else {
            if (!item->pix) {
                // generate a preview for a newly added pattern
                item->pix = _manager.get_image(cast<SPPaintServer>(document->getObjectById(item->id)), _tile_size, _tile_size, device_scale);
            }
            modified = true;
            _cached_items[item->id] = item;
        }
    }

    update_store(patterns, _doc_gallery, _doc_pattern_store);

    return patterns;
}

void PatternEditor::set_document(SPDocument* document) {
    _current_document = document;
    _cached_items.clear();
    update_doc_pattern_list(document);
    set_initial_selection();
}

// populate store with stock patterns
void PatternEditor::set_stock_patterns(const std::vector<SPPaintServer*>& list) {
    const double device_scale = get_scale_factor();
    auto patterns = create_pattern_items(_manager, list, _tile_size, device_scale);
    sort_patterns(patterns);
    update_store(patterns, _stock_gallery, _stock_pattern_store);
}

void PatternEditor::apply_filter(bool stock) {
    auto scoped(_update.block());
    if (!stock) {
        _doc_pattern_store.store.apply_filter();
    }
    else {
        _stock_pattern_store.store.apply_filter();
    }
}

void PatternEditor::update_store(const std::vector<Glib::RefPtr<PatternItem>>& list, Gtk::FlowBox& gallery, PatternStore& pat) {
    auto selected = get_active(gallery, pat);
    if (pat.store.assign(list)) {
        // reselect current
        set_active(gallery, pat, selected);
    }
}

Glib::RefPtr<PatternItem> PatternEditor::get_active(Gtk::FlowBox& gallery, PatternStore& pat) {
    auto empty = Glib::RefPtr<PatternItem>();

    auto sel = gallery.get_selected_children();
    if (sel.size() == 1) {
        return pat.widgets_to_pattern[sel.front()];
    }
    else {
        return empty;
    }
}

std::pair<Glib::RefPtr<PatternItem>, SPDocument*> PatternEditor::get_active() {
    SPDocument* stock = nullptr;
    auto sel = get_active(_doc_gallery, _doc_pattern_store);
    if (!sel) {
        sel = get_active(_stock_gallery, _stock_pattern_store);
        stock = sel ? sel->collection : nullptr;
    }
    return std::make_pair(sel, stock);
}

void PatternEditor::set_active(Gtk::FlowBox& gallery, PatternStore& pat, Glib::RefPtr<PatternItem> item) {
    bool selected = false;
    if (item) {
        for (auto &widget : children(gallery)) {
            if (auto box = dynamic_cast<Gtk::FlowBoxChild*>(&widget)) {
                if (auto pattern = pat.widgets_to_pattern[box]) {
                    if (pattern->id == item->id && pattern->collection == item->collection) {
                        gallery.select_child(*box);
                        if (item->pix) {
                            // update preview, it might be stale
                            for_each_descendant(*box, [&](Widget &widget){
                                if (auto const image = dynamic_cast<Gtk::Image *>(&widget)) {
                                    image->set_pixel_size(_tile_size);
                                    image->set(to_texture(item->pix));
                                    return ForEachResult::_break;
                                }
                                return ForEachResult::_continue;
                            });
                        }
                        selected = true;
                    }
                }
            }
        }
    }

    if (!selected) {
        gallery.unselect_all();
    }
}

std::pair<std::string, SPDocument*> PatternEditor::get_selected() {
    // document patterns first
    auto id = get_selected_doc_pattern();
    if (!id.empty()) {
        return std::make_pair(id, nullptr);
    }
    // stock patterns next
    return get_selected_stock_pattern();
}

std::string PatternEditor::get_selected_doc_pattern() {
    initial_select();
    if (auto sel = get_active(_doc_gallery, _doc_pattern_store)) {
        // for the current document, if selection hasn't changed return linked pattern ID
        // so that we can modify its properties (transform, offset, gap)
        if (sel->id == _current_pattern.id.raw()) {
            return _current_pattern.link_id;
        }
        // different pattern from the current document selected; use its root pattern
        // as a starting point; a link pattern will be injected by adjust_pattern()
        return sel->id;
    }
    return std::string{};
}

std::pair<std::string, SPDocument*> PatternEditor::get_selected_stock_pattern() {
    initial_select();
    auto sel = get_active(_stock_gallery, _stock_pattern_store);
    if (sel) {
        // return pattern ID and stock document it comes from
        return std::make_pair(sel->id, sel->collection);
    }
    else {
        // if nothing is selected, pick the first stock pattern, so we have something to assign
        // to selected object(s); without it, pattern editing will not be activated
        if (auto first = _stock_pattern_store.store.get_store()->get_item(0)) {
            return std::make_pair(first->id, first->collection);
        }

        // no stock patterns available; that's not good, transition to pattern fill won't work
        return std::make_pair("", nullptr);
    }
}

std::optional<Colors::Color> PatternEditor::get_selected_color() {
    auto pat = get_active();
    if (pat.first && pat.first->color.has_value()) {
        return _color_picker.get_current_color();
    }
    return {}; // color is not supported
}

Geom::Point PatternEditor::get_selected_offset() {
    return Geom::Point(_offset_x.get_value(), _offset_y.get_value());
}

Geom::Affine PatternEditor::get_selected_transform() {
    Geom::Affine matrix;

    matrix *= Geom::Scale(_scale_x.get_value(), _scale_y.get_value());
    auto pat = get_active();
    if (pat.first && !pat.first->rotation.has_value()) {
        // bake rotation into transform, unless current item has dedicated rotation attribute (hatch)
        matrix *= Geom::Rotate(_angle_btn.get_value() / 180.0 * M_PI);
    }
    matrix.setTranslation(_current_pattern.offset);
    return matrix;
}

double PatternEditor::get_selected_rotation() {
    return _angle_btn.get_value();
}

double PatternEditor::get_selected_pitch() {
    return _pitch_spin.get_value();
}

double PatternEditor::get_selected_thickness() {
    return _stroke_spin.get_value();
}

bool PatternEditor::is_selected_scale_uniform() {
    return _scale_linked;
}

Geom::Scale PatternEditor::get_selected_gap() {
    return Geom::Scale(_gap_x_spin.get_value(), _gap_y_spin.get_value());
}

Glib::ustring PatternEditor::get_label() {
    return _name_box.get_text();
}

static SPPaintServer* get_pattern(const PatternItem& item, SPDocument* document) {
    auto doc = item.collection ? item.collection : document;
    if (!doc) return nullptr;

    return cast<SPPaintServer>(doc->getObjectById(item.id));
}

void regenerate_tile_images(PatternManager& manager, PatternStore& pat_store, int tile_size, double device_scale, SPDocument* current) {
    auto& patterns = pat_store.store.get_items();
    for (auto& item : patterns) {
        if (auto pattern = get_pattern(*item.get(), current)) {
            item->pix = manager.get_image(pattern, tile_size, tile_size, device_scale);
        }
    }
    pat_store.store.refresh();
}

void PatternEditor::update_pattern_tiles() {
    const double device_scale = get_scale_factor();
    regenerate_tile_images(_manager, _doc_pattern_store, _tile_size, device_scale, _current_document);
    regenerate_tile_images(_manager, _stock_pattern_store, _tile_size, device_scale, nullptr);
}

void PatternEditor::draw_preview(const Cairo::RefPtr<Cairo::Context>& ctx, int width, int height) {
    if (!width || !height || _current_pattern.link_id.empty() || !_current_document) return;

    auto link_pattern = cast<SPPaintServer>(_current_document->getObjectById(_current_pattern.link_id.raw()));
    if (!link_pattern) return;

    const double device_scale = get_scale_factor();
    // use white for checkerboard since most stock patterns are black
    unsigned int background = 0xffffffff;
    auto surface = _manager.get_preview(link_pattern, width, height, background, device_scale);
    ctx->set_source(surface, 0, 0);
    ctx->paint();
}

void PatternEditor::on_map() {
    Box::on_map();

    initial_select();
}

// delay populating patterns until they are being used; it's expensive to read stock patterns
void PatternEditor::initial_select() {
    if (_initial_selection_done) return;

    // populate our combo box with all pattern categories
    auto pattern_categories = _manager.get_categories()->children();
    int cat_count = pattern_categories.size();
    for (auto row : pattern_categories) {
        auto name = row.get_value(_manager.columns.name);
        _combo_set.append(name);
    }

    get_widget<Gtk::Button>(_builder, "previous").signal_clicked().connect([this](){
        int previous = _combo_set.get_active_row_number() - 1;
        if (previous >= 0) _combo_set.set_active(previous);
    });
    get_widget<Gtk::Button>(_builder, "next").signal_clicked().connect([cat_count, this](){
        auto next = _combo_set.get_active_row_number() + 1;
        if (next < cat_count) _combo_set.set_active(next);
    });
    _combo_set.signal_changed().connect([this](){
        // select a pattern category to show
        auto index = _combo_set.get_active_row_number();
        select_pattern_set(index);
        Preferences::get()->setInt(_prefs + "/currentSet", index);
    });

    // current pattern category
    _combo_set.set_active(Preferences::get()->getIntLimited(_prefs + "/currentSet", 0, 0, std::max(cat_count - 1, 0)));
    _initial_selection_done = true;
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
// vim:filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99:

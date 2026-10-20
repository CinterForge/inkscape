// SPDX-License-Identifier: GPL-2.0-or-later

#include "font-list.h"

#include <iomanip>
#include <ranges>
#include <utility>
#include <giomm/menu.h>
#include <giomm/simpleactiongroup.h>
#include <glibmm/markup.h>
#include <glibmm/miscutils.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/filterlistmodel.h>
#include <gtkmm/gridlayoutchild.h>
#include <gtkmm/layoutmanager.h>
#include <gtkmm/menubutton.h>
#include <gtkmm/picture.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/searchentry2.h>
#include <gtkmm/separator.h>
#include <gtkmm/snapshot.h>
#include <gtkmm/stringlist.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/treeexpander.h>
#include <gtkmm/treelistmodel.h>

#include "desktop.h"
#include "inkscape.h"
#include "io/resource.h"
#include "io/sys.h"
#include "preferences.h"
#include "ui/builder-utils.h"
#include "ui/dialog/xml-tree.h"
#include "ui/icon-loader.h"
#include "ui/icon-names.h"
#include "ui/widget/drop-down-list.h"
#include "ui/widget/generic/popover-menu.h"
#include "util/font-collections.h"

using Inkscape::FontInfo;
using Inkscape::UI::create_builder;

namespace {

// construct font name from Pango face and family;
// return font name as it is recorded in the font itself, as far as Pango allows it
Glib::ustring get_full_name(FontInfo const &font_info)
{
    return Inkscape::get_full_font_name(font_info.ff, font_info.face);
}

Glib::ustring get_alt_name(Glib::ustring const &fontspec)
{
    static Glib::ustring sans = "sans-serif";
    if (fontspec.find(sans) != Glib::ustring::npos) {
        auto end = fontspec[sans.size()];
        if (end == 0 || end == ' ' || end == ',') {
            return _("Sans Serif") + fontspec.substr(sans.size());
        }
    }
    return fontspec; // use font spec verbatim
}

Glib::ustring get_font_icon(FontInfo const &font, bool missing_font = false)
{
    if (missing_font) {
        return "missing-element-symbolic";
    } else if (font.variable_font) {
        return ""; // add icon for variable fonts?
    } else if (font.synthetic) {
        return "generic-font-symbolic";
    }
    return {};
}

double get_default_font_size(int unit)
{
    double size = 10;
    if (auto css = SP_ACTIVE_DESKTOP->getCurrentOrToolStyle("/tools/text", true)) {
        SPStyle query;
        query.mergeCSS(css);
        size = query.font_size.computed;
        sp_repr_css_attr_unref(css);
    }
    size = sp_style_css_size_px_to_units(size, unit);
    size = sp_style_css_size_round_for_user_display(size);
    return size;
}

// Gio models require Glib object-based elements,
// we use this class to keep track of all fonts.
class FontElement : public Glib::Object
{
    enum class Type
    {
        Font,   // all fonts when not sorting by family
        Family, // when sorting by family, this is a font that represents a family (a node in a tree)
        Style   // when sorting by family, this is one of the "style" fonts in a family (a leaf in a tree)
    } _type;

public:
    static Glib::RefPtr<FontElement> create_font(FontInfo const &font)
    {
        return Glib::make_refptr_for_instance<FontElement>(new FontElement({}, font, {}, Type::Font));
    }

    static Glib::RefPtr<FontElement> create_style(FontInfo const &font)
    {
        return Glib::make_refptr_for_instance<FontElement>(new FontElement({}, font, {}, Type::Style));
    }

    static Glib::RefPtr<FontElement> create_family(FontInfo const &font, std::vector<FontInfo> family)
    {
        return Glib::make_refptr_for_instance<FontElement>(new FontElement(std::move(family), font, {}, Type::Family));
    }

    static Glib::RefPtr<FontElement> create_injected_font(FontInfo const &font, Glib::ustring alt_spec, bool is_missing)
    {
        auto element =
            Glib::make_refptr_for_instance<FontElement>(new FontElement({}, font, std::move(alt_spec), Type::Font));
        element->_missing_font = is_missing;
        element->_injected = true;
        return element;
    }

    static Glib::RefPtr<FontElement> create_placeholder()
    {
        auto element = Glib::make_refptr_for_instance<FontElement>(new FontElement({}, {}, {}, Type::Font));
        element->_placeholder = true;
        return element;
    }

    Glib::ustring icon_name() const { return get_font_icon(_font, _missing_font); }
    Glib::ustring icon_tooltip() const
    {
        if (_missing_font) {
            // this font is not installed / not available
            return _("This font is missing");
        } else if (_font.synthetic) {
            // this is an alias for some fallback font (ex: 'Serif', 'Sans') and/or faux style
            return _("This is an alias or synthetic font");
        }
        return {};
    }
    FontInfo const &font() const { return _font; }
    Glib::ustring const &get_alt_spec() const { return _alt_fontspec; }
    bool is_present() const
    {
        // true if this font is installed
        return _font.ff != nullptr;
    }
    bool is_family() const { return _type == Type::Family; }
    bool is_injected() const { return _injected; }
    void clear_injected()
    {
        _injected = false;
        _placeholder = true;
    }
    bool is_placeholder() const { return _placeholder; }
    std::vector<FontInfo> const &family() const { return _family; }
    // get markup for a full font name
    Glib::ustring get_full_name_markup() const
    {
        auto name = get_font_name(Type::Font);
        return Glib::ustring::compose("<small>%1</small>", name);
    }
    // get markup for a font name
    Glib::ustring get_name_markup() const
    {
        auto name = get_font_name(_type);
        return Glib::ustring::compose("<small>%1</small>", name);
    }
    Glib::ustring get_name_tooltip() const { return get_font_name(_type); }
    // get markup for a font badge - number of styles in a family
    Glib::ustring get_badge_markup() const
    {
        if (has_badge_markup_content()) {
            // count
            return Glib::ustring::compose("<small>  %1  </small>", _family.size());
        }
        // Return a similarly shaped string but without the number, to keep general label sizes the
        // same (GtkListViews need all items to be same height, and if this label is entirely
        // hidden, it has an effect on the name/badge row height)
        return "<small> </small>";
    }
    bool has_badge_markup_content() const { return _family.size() > 1; }
    // get markup to render font preview
    Glib::ustring get_sample_markup(int font_size_percent, Glib::ustring sample_text)
    {
        // if no sample text given, then render font name
        auto text = Glib::Markup::escape_text(
            sample_text.empty() ? get_font_name(_type == Type::Family ? _type : Type::Font) : sample_text);

        auto &alt = _alt_fontspec;
        auto font_desc =
            Glib::Markup::escape_text(is_present() ? Inkscape::get_font_description(_font.ff, _font.face).to_string()
                                                   : (alt.empty() ? "sans-serif" : alt));
        auto alpha = _missing_font ? "60%" : "100%";
        return Glib::ustring::format("<span allow_breaks='false' alpha='", alpha, "' size='", font_size_percent,
                                     "%' font='", font_desc, "'>", text, "</span>");
    }

private:
    FontElement(std::vector<FontInfo> family, FontInfo font, Glib::ustring alt, Type type)
        : _font(std::move(font))
        , _family(std::move(family))
        , _type(type)
        , _alt_fontspec(std::move(alt))
    {}

    Glib::ustring get_font_name(Type type) const
    {
        auto present = is_present();
        Glib::ustring name;
        Glib::RefPtr<Pango::FontFace> empty;

        switch (type) {
            case Type::Font:
                // full font name: family + style
                name = Glib::Markup::escape_text(present ? Inkscape::get_full_font_name(_font.ff, _font.face)
                                                         : get_alt_name(_alt_fontspec));
                break;
            case Type::Family:
                // font family name only
                name = Glib::Markup::escape_text(present ? Inkscape::get_full_font_name(_font.ff, empty)
                                                         : get_alt_name(_alt_fontspec));
                break;
            case Type::Style:
                // font style only
                name = Glib::Markup::escape_text(_font.face->get_name());
                break;
            default:
                assert(false);
                break;
        }
        return name;
    }

    FontInfo _font;
    std::vector<FontInfo> _family;
    // empty element to keep space in a store
    bool _placeholder = false;
    // if font is not present in a system, then show "missing font" icon
    bool _missing_font = false;
    // injected element - always show it first, regardless of filters or sorting order
    bool _injected = false;
    // this is a fontspec of the missing font
    Glib::ustring _alt_fontspec;
};

class FixedHeightLabel : public Gtk::Label
{
public:
    void set_fixed_height(int height)
    {
        _height = height;
        queue_resize();
    }

private:
    int _height;
    mutable double _ratio;

    void measure_vfunc(Gtk::Orientation const orientation, int const for_size, int &min, int &nat, int &min_baseline,
                       int &nat_baseline) const override
    {
        Gtk::Label::measure_vfunc(orientation, for_size, min, nat, min_baseline, nat_baseline);

        if (orientation == Gtk::Orientation::VERTICAL) {
            _ratio = _height / (double)nat;
            nat = _height;
            nat_baseline *= _ratio;
            min = nat;
            min_baseline = nat_baseline;
        }
    }

    void snapshot_vfunc(Glib::RefPtr<Gtk::Snapshot> const &snapshot) override
    {
        if (_ratio < 1) {
            // scale down as needed to fit (no need to scale up, plus we want to stay inside our
            // allocation)
            snapshot->scale(_ratio, _ratio);
        }
        Gtk::Label::snapshot_vfunc(snapshot);
    }
};

class CachedLabel : public Gtk::Box
{
public:
    CachedLabel()
        : _image{nullptr}
        , _label{nullptr}
    {}

    void set_markup(Glib::ustring const &markup, int sample_font_size)
    {
        _markup = markup;
        _saved_texture.reset();

        auto height = 18 * sample_font_size / 100;

        // Use an image if we already have a texture on disk. Else show a real label.
        auto filepath = get_cache_path();
        auto exists = Inkscape::IO::file_test(filepath.c_str(), G_FILE_TEST_EXISTS);
        if (exists) {
            if (_label) {
                _label->unparent();
                _label = nullptr;
            }
            if (!_image) {
                _image = Gtk::make_managed<Gtk::Picture>();
                _image->set_content_fit(Gtk::ContentFit::SCALE_DOWN);
                append(*_image);
            }
            _image->set_size_request(-1, height);
            _image->set_filename(filepath);
        } else {
            if (_image) {
                _image->unparent();
                _image = nullptr;
            }
            if (!_label) {
                _label = Gtk::make_managed<FixedHeightLabel>();
                _label->set_halign(Gtk::Align::CENTER);
                append(*_label);
            }
            _label->set_fixed_height(height);
            _label->set_markup(markup);
        }
    }

private:
    Gtk::Picture *_image;
    FixedHeightLabel *_label;
    Glib::ustring _markup;
    Glib::RefPtr<Gdk::Texture> _saved_texture;

    Glib::ustring get_cache_folder()
    {
        return Inkscape::IO::Resource::get_path_string(Inkscape::IO::Resource::Domain::CACHE,
                                                       Inkscape::IO::Resource::Type::NONE, "font-previews");
    }

    Glib::ustring get_cache_path()
    {
        // Escape the markup into a more filesystem-friendly version.
        // Caution: if font names only differ by these characters, we'll have a cache collision
        std::string basename = _markup;
        for (char &c : basename) {
            if (std::string(". !$*?'\"#-\\/<>()=\n").find(c) != std::string::npos) {
                c = '_';
            }
        }
        // version cache name, so we can clean up easily if format changes
        basename = "v1_" + basename + ".png";

        return Glib::build_filename(get_cache_folder(), basename, nullptr);
    }

    void snapshot_vfunc(Glib::RefPtr<Gtk::Snapshot> const &snapshot_in) override
    {
        // If we're rendering an image, just early exit - no special work needed
        if (_image) {
            Gtk::Box::snapshot_vfunc(snapshot_in);
            return;
        }

        if (!_saved_texture) {
            // Take snapshot of label
            auto snapshot = Gtk::Snapshot::create();
            snapshot_child(*_label, snapshot);

            // Turn that snapshot into a texture
            auto surface = get_native()->get_surface();
            auto node_gobj = gtk_snapshot_to_node(snapshot->gobj());
            if (!node_gobj) {
                return; // not ready to render yet
            }
            auto renderer_gobj = gsk_renderer_new_for_surface(surface->gobj());
            if (!renderer_gobj) {
                gsk_render_node_unref(node_gobj);
                return; // not ready to render yet
            }
            auto texture = Glib::wrap(gsk_renderer_render_texture(renderer_gobj, node_gobj, nullptr));
            gsk_renderer_unrealize(renderer_gobj);
            g_object_unref(renderer_gobj);
            gsk_render_node_unref(node_gobj);

            // Save that texture to disk when idle
            auto folder = get_cache_folder();
            auto path = get_cache_path();
            Glib::signal_idle().connect(
                [texture, folder, path] {
                    g_mkdir_with_parents(folder.c_str(), 0700);
                    texture->save_to_png(path);
                    return false;
                },
                Glib::PRIORITY_LOW);

            _saved_texture = texture;
        }

        // Use the saved texture as the requested snapshot
        auto bbox = Gdk::Graphene::Rect(0, 0, _saved_texture->get_width(), _saved_texture->get_height());
        snapshot_in->append_texture(_saved_texture, bbox);
    }
};

// This function constructs a widget to show font info in a list view.
// List view is capable of being transformed into a tree-like display too.
void on_set_up_listitem(Glib::RefPtr<Gtk::ListItem> const &list_item)
{
    // Each ListItem contains a TreeExpander, which contains a box.
    auto expander = Gtk::make_managed<Gtk::TreeExpander>();
    auto vbox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 1);
    auto upper = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 3);
    auto lower = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 4);
    vbox->set_margin_top(2);
    vbox->set_overflow(Gtk::Overflow::HIDDEN);
    auto sample = Gtk::make_managed<CachedLabel>();
    sample->set_halign(Gtk::Align::START);
    sample->set_margin_start(2); // extra space for fonts extend past the bbox
    auto name = Gtk::make_managed<Gtk::Label>();
    name->set_ellipsize(Pango::EllipsizeMode::END);
    name->set_halign(Gtk::Align::START);
    name->set_margin_start(4); // indent more than label due to optical alignment
    lower->append(*name);
    auto badge = Gtk::make_managed<Gtk::Label>();
    badge->set_halign(Gtk::Align::CENTER);
    lower->append(*badge);
    vbox->append(*upper);
    vbox->append(*lower);
    auto icon = Gtk::make_managed<Gtk::Image>();
    icon->set_pixel_size(16);
    icon->set_valign(Gtk::Align::CENTER);
    upper->append(*icon);
    upper->append(*sample);
    expander->set_child(*vbox);
    list_item->set_child(*expander);
    list_item->set_activatable();
    expander->set_indent_for_icon();
    expander->set_indent_for_depth();
    // expander can be collapsed with an action, but it cannot be executed in setup function:
    // expander->activate_action("listitem.collapse");
}

void on_bind_listitem(int sample_font_size, bool show_name, Glib::ustring const &sample_text,
                      Glib::RefPtr<Gtk::ListItem> const &list_item)
{
    auto row = std::dynamic_pointer_cast<Gtk::TreeListRow>(list_item->get_item());
    if (!row)
        return;
    // if only leaves in the tree can be selected:
    // list_item->set_selectable(!row->is_expandable());
    auto element = std::dynamic_pointer_cast<FontElement>(row->get_item());
    if (!element)
        return;
    auto expander = dynamic_cast<Gtk::TreeExpander *>(list_item->get_child());
    if (!expander)
        return;

    expander->set_list_row(row);

    auto vbox = dynamic_cast<Gtk::Box *>(expander->get_child());
    auto &upper = dynamic_cast<Gtk::Box &>(*vbox->get_first_child());
    auto &lower = dynamic_cast<Gtk::Box &>(*upper.get_next_sibling());
    auto &icon = dynamic_cast<Gtk::Image &>(*upper.get_first_child());
    auto &sample = dynamic_cast<CachedLabel &>(*icon.get_next_sibling());
    auto &name = dynamic_cast<Gtk::Label &>(*lower.get_first_child());
    auto &badge = dynamic_cast<Gtk::Label &>(*name.get_next_sibling());

    sample.set_markup(element->get_sample_markup(sample_font_size, sample_text), sample_font_size);

    if (show_name) {
        name.set_markup(element->get_name_markup());
        badge.set_markup(element->get_badge_markup());
        if (element->has_badge_markup_content()) {
            badge.add_css_class("tag-box");
        } else {
            badge.remove_css_class("tag-box");
        }
    }
    name.set_visible(show_name);
    badge.set_visible(show_name);
    icon.set_from_icon_name(element->icon_name());
    icon.set_tooltip_text(element->icon_tooltip());
    icon.set_visible(!element->icon_name().empty());
}

// helper function that constructs tree model
Glib::RefPtr<Gio::ListStore<FontElement>> create_element_model(Glib::RefPtr<Glib::ObjectBase> const &item = {})
{
    auto element = std::dynamic_pointer_cast<FontElement>(item);
    if (!element || element && element->family().size() < 2) {
        // An item without children, i.e. a leaf in the tree.
        return {};
    }

    auto result = Gio::ListStore<FontElement>::create();
    for (auto &f : element->family()) {
        result->append(FontElement::create_style(f));
    }

    // no visible children?
    if (result->get_n_items() == 0) {
        return {};
    }

    return result;
}

// Grid view items have a simple shape: just two labels,
// top one for a font preview and bottom one for a font name.
// Then we style a box hosting them to make items distinct.
void on_set_up_griditem(Glib::RefPtr<Gtk::ListItem> const &list_item)
{
    auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 1);
    auto sample = Gtk::make_managed<CachedLabel>();
    auto name = Gtk::make_managed<Gtk::Label>();
    sample->set_halign(Gtk::Align::CENTER);
    sample->set_valign(Gtk::Align::CENTER);
    sample->set_expand();
    name->set_halign(Gtk::Align::CENTER);
    name->set_hexpand();
    name->set_margin_start(1);
    name->set_margin_end(1);
    name->set_ellipsize(Pango::EllipsizeMode::END);
    box->add_css_class("item-box");
    box->add_css_class("round-rect-shade");
    box->append(*sample);
    box->append(*name);
    list_item->set_child(*box);
}

void on_bind_griditem(int sample_font_size, bool show_name, Glib::ustring const &sample_text,
                      Glib::RefPtr<Gtk::ListItem> const &list_item)
{
    auto element = std::dynamic_pointer_cast<FontElement>(list_item->get_item());
    if (!element)
        return;
    auto box = dynamic_cast<Gtk::Box *>(list_item->get_child());
    if (!box)
        return;
    auto label = dynamic_cast<CachedLabel *>(box->get_first_child());
    auto name = dynamic_cast<Gtk::Label *>(label->get_next_sibling());

    auto shown_text = sample_text.empty() ? "Aa" : sample_text;
    label->set_markup(element->get_sample_markup(sample_font_size, shown_text), sample_font_size);
    if (show_name) {
        name->set_markup(element->get_full_name_markup());
    }
    name->set_visible(show_name);
    box->set_tooltip_text(element->get_name_tooltip());
}

void refilter(Glib::RefPtr<Gtk::BoolFilter> &filter)
{
    Gtk::Filter *f = filter.get();
    gtk_filter_changed(f->gobj(), GtkFilterChange::GTK_FILTER_CHANGE_DIFFERENT);
}

} // namespace

namespace Inkscape::UI::Widget {

std::unique_ptr<FontSelectorInterface> FontList::create_font_list(Glib::ustring path)
{
    return std::make_unique<FontList>(path);
}

char const *get_sort_icon(FontOrder order)
{
    char const *icon = nullptr;

    switch (order) {
        case FontOrder::ByFamily:
            icon = "sort-by-family-symbolic";
            break;
        case FontOrder::ByName:
            icon = "sort-alphabetically-symbolic";
            break;
        case FontOrder::ByWeight:
            icon = "sort-by-weight-symbolic";
            break;
        case FontOrder::ByWidth:
            icon = "sort-by-width-symbolic";
            break;
        default:
            g_warning("Missing case in get_sort_icon");
            break;
    }

    return icon;
}

FontList::FontList(Glib::ustring preferences_path)
    : _prefs(std::move(preferences_path))
    , _builder(create_builder("font-list.glade"))
    , _main_grid(get_widget<Gtk::Grid>(_builder, "main-grid"))
    , _tag_list(get_widget<Gtk::ListBox>(_builder, "categories"))
    , _font_list(get_widget<Gtk::ListView>(_builder, "font-list"))
    , _font_grid(get_widget<Gtk::GridView>(_builder, "font-grid"))
    , _font_size(get_widget<FontSizeSelector>(_builder, "font-size"))
    , _preview_size_scale(get_widget<Gtk::Scale>(_builder, "preview-font-size"))
    , _preview_size_spin(get_widget<Gtk::SpinButton>(_builder, "preview-font-size-value"))
    , _grid_sample_entry(get_widget<Gtk::Entry>(_builder, "grid-sample"))
    , _list_sample_entry(get_widget<Gtk::Entry>(_builder, "sample-text"))
    , _tag_box(get_widget<Gtk::FlowBox>(_builder, "tag-box"))
    , _info_box(get_widget<Gtk::Box>(_builder, "info-box"))
    , _progress_box(get_widget<Gtk::Box>(_builder, "progress-box"))
    , _search(get_widget<Gtk::SearchEntry2>(_builder, "font-search"))
    , _var_axes(get_widget<Gtk::ScrolledWindow>(_builder, "var-axes"))
    , _font_tags(FontTags::get())
{
    _font_store = Gio::ListStore<FontElement>::create();

    // common filtering action for placeholder and injected font:
    // - placeholders: always hidden
    // - injected fonts: always visible
#define HANDLE_SPECIAL_FONT                                   \
    auto font = std::dynamic_pointer_cast<FontElement>(item); \
    if (!font || font->is_placeholder())                      \
        return false;                                         \
    if (font->is_injected())                                  \
        return true;

    // filter for:
    // - grouping fonts by family (only used in list view, grid view shows all family members)
    _family_filter = Gtk::BoolFilter::create(Gtk::ClosureExpression<bool>::create([this](auto &item) {
        HANDLE_SPECIAL_FONT

        // if grouping by family is on filter out individual font styles leaving only "Regular" font
        if (_order == FontOrder::ByFamily && !font->is_family())
            return false;

        return true;
    }));

    // filter for:
    // - font collections
    // - font categories
    _font_filter = Gtk::BoolFilter::create(Gtk::ClosureExpression<bool>::create([this](auto &item) {
        HANDLE_SPECIAL_FONT

        // apply categories, if any
        auto &active_categories = _font_tags.get_selected_tags();
        if (!active_categories.empty()) {
            bool filter_in = false;
            auto &&set = _font_tags.get_font_tags(font->font().face);
            for (auto &&ftag : active_categories) {
                if (set.contains(ftag.tag)) {
                    filter_in = true;
                    break;
                }
            }
            if (!filter_in)
                return false;
        }

        // check for selected font collections, if any
        auto fc = FontCollections::get();
        auto &font_collections = fc->get_selected_collections();
        if (!font_collections.empty()) {
            bool filter_in = false;
            for (auto &col : font_collections) {
                if (fc->is_font_in_collection(col, font->font().ff->get_name())) {
                    filter_in = true;
                    break;
                }
            }
            if (!filter_in)
                return false;
        }

        return true; // filter in
    }));

    // filter for matching search text
    _text_filter = Gtk::BoolFilter::create(Gtk::ClosureExpression<bool>::create([this](auto &item) {
        HANDLE_SPECIAL_FONT

        if (_search_term.empty())
            return true;

        // TODO: vary name based on font '_type'?
        auto text = get_full_name(font->font()).lowercase();
        return text.find(_search_term) != Glib::ustring::npos;
    }));

#undef HANDLE_SPECIAL_FONT

    // set up tree view
    {
        // cascade filters; currently family filter needs to be first to find the "Regular" font
        auto filtered1 = Gtk::FilterListModel::create(_font_store, _family_filter);
        auto filtered2 = Gtk::FilterListModel::create(filtered1, _font_filter);
        auto filtered3 = Gtk::FilterListModel::create(filtered2, _text_filter);
        auto tree_model =
            Gtk::TreeListModel::create(filtered3, [](auto &item) { return create_element_model(item); }, false, false);
        _list_selection = Gtk::SingleSelection::create(tree_model);
        // we need autoselect off, so collapsing nodes does not move selection
        _list_selection->set_autoselect(false);
        // manually unselecting current font doesn't seem to serve any purpose, so it is disabled
        _list_selection->set_can_unselect(false);
        auto factory = Gtk::SignalListItemFactory::create();
        factory->signal_setup().connect([](auto &item) { on_set_up_listitem(item); });
        factory->signal_bind().connect(
            [this](auto &item) { on_bind_listitem(_sample_font_size, _show_font_names, _sample_text, item); });
        _font_list.set_show_separators();
        _font_list.set_model(_list_selection);
        _font_list.set_factory(factory);
    }
    // set up grid view
    {
        // cascade filters; no family filter for a grid, there's no collapsing there
        auto filtered1 = Gtk::FilterListModel::create(_font_store, _font_filter);
        auto filtered2 = Gtk::FilterListModel::create(filtered1, _text_filter);
        _grid_selection = Gtk::SingleSelection::create(filtered2);
        _grid_selection->set_can_unselect(false);
        _font_grid.set_model(_grid_selection);
        auto factory = Gtk::SignalListItemFactory::create();
        factory->signal_setup().connect([](auto &item) { on_set_up_griditem(item); });
        factory->signal_bind().connect(
            [this](auto &item) { on_bind_griditem(_sample_font_size, _show_font_names, _grid_sample_text, item); });
        _font_grid.set_factory(factory);
    }

    _var_axes.set_visible(false);
    _var_axes.set_child(_font_variations);
    _font_variations.get_size_group(0)->add_widget(get_widget<Gtk::Label>(_builder, "font-size-label"));
    _font_variations.get_size_group(1)->add_widget(_font_size);
    _font_variations.connectChanged([this] {
        if (_update.pending())
            return;
        _signal_fontspec_changed.emit();
    });

    set_hexpand();
    set_vexpand();
    append(_main_grid);
    set_margin_start(0);
    set_margin_end(0);
    set_margin_top(5);
    set_margin_bottom(0);

    auto prefs = Preferences::get();

    std::pair<char const *, FontOrder> sorting[4] = {{N_("Group by family"), FontOrder::ByFamily},
                                                     {N_("Sort alphabetically"), FontOrder::ByName},
                                                     {N_("Light to heavy"), FontOrder::ByWeight},
                                                     {N_("Condensed to expanded"), FontOrder::ByWidth}};
    auto sort_menu = Gtk::make_managed<PopoverMenu>(Gtk::PositionType::BOTTOM);
    for (auto &[label, order] : sorting) {
        auto item = Gtk::make_managed<PopoverMenuItem>();
        if (order == FontOrder::ByFamily) {
            _sort_by_family = item;
        }
        auto hbox = Gtk::make_managed<Gtk::Box>();
        hbox->append(*Gtk::manage(sp_get_icon_image(get_sort_icon(order), Gtk::IconSize::NORMAL)));
        hbox->append(*Gtk::make_managed<Gtk::Label>(_(label)));
        hbox->set_spacing(4);
        item->set_child(*hbox);
        item->signal_activate().connect([=, this] {
            _order = order;
            set_sort_icon();
            sort_fonts();
            prefs->setInt(_prefs + "/font-order", static_cast<int>(_order));
        });
        sort_menu->append(*item);
    }
    get_widget<Gtk::MenuButton>(_builder, "btn-sort").set_popover(*sort_menu);
    get_widget<Gtk::Button>(_builder, "id-reset-filter").signal_clicked().connect([this] {
        bool modified = false;
        if (_font_tags.deselect_all()) {
            modified = true;
        }
        auto fc = FontCollections::get();
        if (!fc->get_selected_collections().empty()) {
            fc->clear_selected_collections();
            modified = true;
        }
        if (modified) {
            filters_updated();
        }
    });

    _charmap_popover.set_child(_charmap);
    get_widget<Gtk::MenuButton>(_builder, "btn-charmap").set_popover(_charmap_popover);
    _charmap_popover.signal_show().connect([this] {
        try {
            auto spec = get_fontspec();
            _current_font_instance = FontFactory::get().FaceFromPangoString(spec.c_str());

            Glib::ustring name;
            if (auto element = std::dynamic_pointer_cast<FontElement>(get_selected_font())) {
                auto const &font = element->font();
                name = get_full_name(font);
            }
            _charmap.set_font(_current_font_instance.get(), name);
            return;
        } catch (std::exception &ex) {
            // TODO: show notification
            std::cerr << ex.what() << std::endl;
        }
        _current_font_instance = {};
        _charmap.set_font(nullptr, {});
    });
    _charmap_popover.signal_closed().connect([this] {
        // clear old content
        _charmap.set_font(nullptr, {});
    });
    _charmap.signal_insert_text().connect([this](auto &text) {
        // insert glyph selected in a char viewer
        _signal_insert_text.emit(text);
    });

    _search.signal_changed().connect([this] { apply_filters_keep_selection(true); });

    _sample_font_size = prefs->getIntLimited(_prefs + "/preview-size", _sample_font_size, 100, 800);
    _preview_size_scale.set_format_value_func(
        [](double val) { return Glib::ustring::format(std::fixed, std::setprecision(0), val) + "%"; });
    _preview_size_scale.set_value(_sample_font_size);
    _preview_size_spin.set_value(_sample_font_size);
    _preview_size_spin.signal_value_changed().connect([=, this] {
        // We avoid rebuilding the UI while the user is dragging bar for performance reasons, so we wait until we're
        // idle and see if we're still dragging at that point.
        _preview_size_idle = Glib::signal_idle().connect([=, this] {
            if (_preview_size_scale.has_css_class("dragging")) {
                return true; // keep running this idle callback until the user releases the drag
            }
            // OK we're done with a drag (or were never in one) and can rebuild the UI
            _sample_font_size = _preview_size_spin.get_value();
            prefs->setInt(_prefs + "/preview-size", _sample_font_size);
            rebuild_ui();
            return false;
        });
    });
    auto to_top = prefs->getBool(_prefs + "/font-size-top", false);
    set_font_size_layout(to_top);
    auto size_top = &get_widget<Gtk::CheckButton>(_builder, "font-size-top");
    size_top->set_active(to_top);
    get_widget<Gtk::CheckButton>(_builder, "font-size-bottom").set_active(!to_top);
    size_top->signal_toggled().connect([=, this] {
        bool top = size_top->get_active();
        set_font_size_layout(top);
        prefs->setBool(_prefs + "/font-size-top", top);
    });

    auto show_names = &get_widget<Gtk::CheckButton>(_builder, "show-font-name");
    auto set_show_names = [=, this](bool show) {
        _show_font_names = show;
        prefs->setBool(_prefs + "/show-font-names", show);
        rebuild_ui();
    };
    auto show = prefs->getBool(_prefs + "/show-font-names", true);
    set_show_names(show);
    show_names->set_active(show);
    show_names->signal_toggled().connect([=] {
        bool show = show_names->get_active();
        set_show_names(show);
    });

    // sample text to show for each font; empty to show font name
    _sample_text = prefs->getString(_prefs + "/sample-text");
    _list_sample_entry.set_text(_sample_text);
    _list_sample_entry.signal_changed().connect([=, this] {
        _sample_text = _list_sample_entry.get_text();
        prefs->setString(_prefs + "/sample-text", _sample_text);
        rebuild_ui();
    });

    // sample text for grid
    _grid_sample_text = prefs->getString(_prefs + "/grid-text", "Aa");
    _grid_sample_entry.set_text(_grid_sample_text);
    _grid_sample_entry.signal_changed().connect([=, this] {
        _grid_sample_text = _grid_sample_entry.get_text();
        prefs->setString(_prefs + "/grid-text", _grid_sample_text);
        rebuild_ui();
    });

    // Populate samples submenu from stringlist
    auto samples_submenu = get_object<Gio::Menu>(_builder, "samples-submenu");
    auto samples_stringlist = get_object<Gtk::StringList>(_builder, "samples-stringlist");

    auto truncate = [](Glib::ustring const &text) {
        constexpr int N = 30; // limit number of characters in label
        if (text.length() <= N) {
            return text;
        }

        auto substr = text.substr(0, N);
        auto pos = substr.rfind(' ');
        // do we have a space somewhere close to the limit of N chars?
        if (pos != Glib::ustring::npos && pos > N - N / 4) {
            // if so, truncate at space
            substr = substr.substr(0, pos);
        }
        substr += "\u2026"; // add ellipsis to truncated content
        return substr;
    };

    for (int i = 0, n_items = samples_stringlist->get_n_items(); i < n_items; i++) {
        auto text = samples_stringlist->get_string(i);
        auto menu_item = Gio::MenuItem::create(truncate(text), "");
        menu_item->set_action_and_target("win.set-sample", Glib::Variant<Glib::ustring>::create(text));
        samples_submenu->append_item(menu_item);
    }

    // Hook up action used by samples submenu
    auto action_group = Gio::SimpleActionGroup::create();
    action_group->add_action_with_parameter(
        "set-sample", Glib::Variant<Glib::ustring>::variant_type(), [=, this](Glib::VariantBase const &param) {
            auto param_str = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(param).get();
            _list_sample_entry.set_text(param_str);
        });
    insert_action_group("win", action_group);

    auto font_selected = [this](FontInfo const &font) {
        if (_update.pending())
            return;

        auto scoped = _update.block();
        auto vars = font.variations;
        if (vars.empty() && font.variable_font) {
            vars = get_inkscape_fontspec(font.ff, font.face, font.variations);
        }
        _font_variations.update(vars);
        _var_axes.set_visible(_font_variations.variations_present());
        _signal_fontspec_changed.emit();
    };

    _list_selection->signal_selection_changed().connect([font_selected, this](auto index, auto n) {
        if (auto element = std::dynamic_pointer_cast<FontElement>(get_selected_font())) {
            font_selected(element->font());
        }
    });

    _grid_selection->signal_selection_changed().connect([font_selected, this](auto index, auto n) {
        if (auto element = std::dynamic_pointer_cast<FontElement>(get_selected_font())) {
            font_selected(element->font());
        }
    });

    // set up list/tree view vs grid view mode switching
    auto list_mode = prefs->getBool(_prefs + "/list-view-mode", true);
    switch_view_mode(list_mode);
    auto show_grid = &get_widget<Gtk::ToggleButton>(_builder, "view-grid");
    auto show_list = &get_widget<Gtk::ToggleButton>(_builder, "view-list");
    if (list_mode)
        show_list->set_active();
    else
        show_grid->set_active();
    show_list->signal_toggled().connect([show_list, this] { switch_view_mode(show_list->get_active()); });

    _info_box.set_visible(false);
    _progress_box.show();

    auto prepare_tags = [this] {
        // prepare dynamic tags
        for (auto &&f : _fonts) {
            auto kind = f.family_kind >> 8;
            if (kind == 10) {
                _font_tags.tag_font(f.face, "script");
            } else if (kind >= 1 && kind <= 5) {
                _font_tags.tag_font(f.face, "serif");
            } else if (kind == 8) {
                _font_tags.tag_font(f.face, "sans");
            } else if (kind == 12) {
                _font_tags.tag_font(f.face, "symbols");
            }

            if (f.monospaced) {
                _font_tags.tag_font(f.face, "monospace");
            }
            if (f.variable_font) {
                _font_tags.tag_font(f.face, "variable");
            }
            if (f.oblique) {
                _font_tags.tag_font(f.face, "oblique");
            }
        }
    };

    // fonts to exclude from UI:
    auto hidden_font = [](FontInfo const &font) {
#if __APPLE__
        // on macOS hide fonts with names starting with a dot; those are internal system fonts
        return font.ff->get_name().raw()[0] == '.';
#else
        // other systems; no restrictions so far
        return false;
#endif
    };

    _font_stream = FontDiscovery::get().connect_to_fonts([=, this](FontDiscovery::MessageType const &msg) {
        if (auto r = Async::Msg::get_result(msg)) {
            _font_families.clear();
            auto view = **r | std::views::filter([=](std::vector<FontInfo> const &ff) {
                return !ff.empty() && !hidden_font(ff.front());
            });
            std::ranges::copy(view, std::back_inserter(_font_families));
            _fonts.clear();
            for (auto &&family : _font_families) {
                _fonts.insert(_fonts.end(), family.begin(), family.end());
            }
            prepare_tags();
        } else if (auto p = Async::Msg::get_progress(msg)) {
            // show progress
            _info_box.set_visible(false);
            _progress_box.set_visible();
            auto &progress = get_widget<Gtk::ProgressBar>(_builder, "init-progress");
            progress.set_fraction(std::get<double>(*p));
            progress.set_text(std::get<Glib::ustring>(*p));
            auto &&family = std::get<std::vector<FontInfo>>(*p);
            if (!family.empty() && !hidden_font(family.front())) {
                _fonts.insert(_fonts.end(), family.begin(), family.end());
                _font_families.push_back(std::move(family));
            }
        } else if (Async::Msg::is_finished(msg)) {
            // hide progress
            _progress_box.set_visible(false);
            _info_box.set_visible();
            // Only create the font list once finished - we've seen crashes when trying to render
            // the list while fonts were being loaded in the background task.
            sort_fonts();
        }
    });

    _font_size.signal_size_changed().connect([this](auto size, auto unit) {
        if (_update.pending())
            return;

        auto scoped = _update.block();
        if (size > 0) {
            _signal_fontsize_changed.emit();
        }
    });
    _font_size.setSize(get_default_font_size(_font_size.getUnit()));

    // restore sorting
    _order = static_cast<FontOrder>(prefs->getIntLimited(_prefs + "/font-order", static_cast<int>(_order),
                                                         static_cast<int>(FontOrder::_First),
                                                         static_cast<int>(FontOrder::_Last)));
    set_sort_icon();
    sort_fonts();

    _font_tags.get_signal_tag_changed().connect(
        [this](FontTag const *ftag, bool selected) { sync_font_tag(ftag, selected); });

    auto &filter_popover = get_widget<Gtk::Popover>(_builder, "filter-popover");
    filter_popover.signal_show().connect(
        [this] {
            // update tag checkboxes
            add_categories();
            update_filterbar();
        },
        false);

    _font_collections_update = FontCollections::get()->connect_update([this] { filters_updated(); });
    _font_collections_selection = FontCollections::get()->connect_selection_update([this] { filters_updated(); });
}

void FontList::set_sort_icon()
{
    auto order = _order;
    if (order == FontOrder::ByFamily && !_list_visible) {
        order = FontOrder::ByName;
    }
    // this option only applies to a list/tree view
    _sort_by_family->set_visible(_list_visible);

    if (char const *icon = get_sort_icon(order)) {
        auto &button = get_widget<Gtk::MenuButton>(_builder, "btn-sort");
        button.set_icon_name(icon);
    }
}

void FontList::sort_fonts()
{
    Inkscape::sort_fonts(_fonts, _order, true);

    sort_font_families(_font_families, true);

    rebuild_store();
}

int FontList::find_font(Glib::ustring const &fontspec, int from, int count) const
{
    auto &selection = _list_visible ? _list_selection : _grid_selection;
    auto total = selection->get_n_items();
    auto n = count > 0 ? std::min(count, static_cast<int>(total)) : total;
    for (int i = from; i < n; ++i) {
        auto element = std::dynamic_pointer_cast<FontElement>(get_nth_font(i));
        if (!element)
            continue;

        if (element->is_present()) {
            auto &font = element->font();
            auto spec = get_inkscape_fontspec(font.ff, font.face, font.variations);
            if (spec == fontspec) {
                return i;
            }
        } else {
            auto &spec = element->get_alt_spec();
            if (spec == fontspec) {
                return i;
            }
        }
    }
    return -1;
}

void FontList::switch_view_mode(bool show_list)
{
    // get current selection to sync selection between font widgets
    auto fontspec = get_fontspec();
    _list_visible = show_list;
    auto &list = get_widget<Gtk::ScrolledWindow>(_builder, "list");
    auto &grid = get_widget<Gtk::ScrolledWindow>(_builder, "grid");
    if (show_list) {
        grid.set_visible(false);
        _font_grid.set_model({});
        _font_list.set_model(_list_selection);
        list.set_visible();
    } else {
        list.set_visible(false);
        _font_list.set_model({});
        _font_grid.set_model(_grid_selection);
        grid.set_visible();
    }
    // update sort icon: grid view does not support grouping by family
    set_sort_icon();
    // update widgets in an option popup
    get_widget<Gtk::MenuButton>(_builder, "sample-menu-btn").set_visible(show_list);
    _list_sample_entry.set_visible(show_list);
    _grid_sample_entry.set_visible(!show_list);
    Preferences::get()->setBool(_prefs + "/list-view-mode", show_list);
    // try to reselect the same font in a new view
    select_font(fontspec);
}

bool FontList::select_font(Glib::ustring const &fontspec)
{
    auto scoped = _update.block();

    bool found = false;

    auto pos = find_font(fontspec);
    if (pos >= 0) {
        found = true;
        scroll_to_row(pos);
    }

    if (!found && _list_visible && _order == FontOrder::ByFamily) {
        // if some fonts in a tree are collapsed, their leaves are not visible to the selection model;
        // we need to check all styles too:

        int pos = -1;
        for (auto &&fam : _font_families) {
            for (auto &font : fam) {
                auto spec = get_inkscape_fontspec(font.ff, font.face, font.variations);
                if (spec == fontspec) {
                    // find "family" node in the tree
                    auto &regular = get_family_font(fam);
                    spec = get_inkscape_fontspec(regular.ff, regular.face, regular.variations);
                    pos = find_font(spec);
                    break;
                }
            }
            if (pos >= 0) {
                if (auto row = std::dynamic_pointer_cast<Gtk::TreeListRow>(_list_selection->get_object(pos))) {
                    // expand family row to populate styles
                    row->set_expanded();
                    pos = find_font(fontspec, pos, pos + 1 + fam.size());
                    if (pos >= 0) {
                        found = true;
                        scroll_to_row(pos);
                    }
                }
                break;
            }
        }
    }

    return found;
}

void FontList::rebuild_store()
{
    // recreate content of the font store

    auto scoped = _update.block();

    // save selection
    auto fontspec = get_fontspec();

    populate_font_store();

    if (!_current_fspec.empty()) {
        add_font(_current_fspec, false);
    }

    apply_filters();
    rebuild_ui();

    // reselect if that font is still available
    select_font(fontspec);
}

void FontList::apply_filters_keep_selection(bool text_only)
{
    // save selection
    auto fontspec = get_fontspec();

    if (auto placeholder = std::dynamic_pointer_cast<FontElement>(_font_store->get_object(0))) {
        if (placeholder->is_injected()) {
            // now is the time to remove injected fonts; they will be reinserted as needed
            placeholder->clear_injected();
            //
            // text_only = false;
        }
    }

    apply_filters(!text_only);

    // restore
    select_font(fontspec);
}

void FontList::apply_filters(bool all_filters)
{
    auto scoped = _update.block();

    if (all_filters) {
        refilter(_font_filter);
    }

    _search_term = _search.get_text().lowercase();
    refilter(_text_filter);

    update_font_count();
}

// Respond to a change in filters by refreshing the filterbar and font list.
void FontList::filters_updated()
{
    add_categories();
    update_filterbar();
    apply_filters_keep_selection();
}

void FontList::rebuild_ui()
{
    // force the UI to be recreated when all items are impacted

    _font_list.set_model(nullptr);
    _font_grid.set_model(nullptr);

    if (_list_visible) {
        _font_list.set_model(_list_selection);
    } else {
        _font_grid.set_model(_grid_selection);
    }
}

// add fonts to the font store replacing its content
void FontList::populate_font_store()
{
    _font_store->freeze_notify();
    _font_store->remove_all();

    // reserve first spot; it will be used for "missing" or "injected" fonts, as needed
    _font_store->append(FontElement::create_placeholder());

    _font_store->thaw_notify();

    // Now start loading fonts in chunks (to avoid blocking the UI when we have lots of fonts).
    // Resets an existing idle handler (disconnecting any existing in-progress population).
    _store_rebuild_on_idle = Glib::signal_idle().connect(sigc::mem_fun(*this, &FontList::populate_font_store_chunk));
}

// Returns true if we have more work to do
bool FontList::populate_font_store_chunk()
{
    _font_store->freeze_notify();

    static guint CHUNK_SIZE = 20;
    guint skip = _font_store->get_n_items();
    guint stop = skip + CHUNK_SIZE;
    guint count = 1; // start at one because we always have the placeholder

    if (_order == FontOrder::ByFamily) {
        for (auto &&fam : _font_families) {
            auto &regular = get_family_font(fam);
            for (auto &font : fam) {
                if (count++ >= skip) {
                    if (font == regular) {
                        // In list view, children (family styles) will get created in
                        // create_element_model().
                        _font_store->append(FontElement::create_family(regular, fam));
                    } else {
                        // In list view, this will be filtered out in preference of child items
                        // under the regular font. But in grid view, this will be visible.
                        _font_store->append(FontElement::create_font(font));
                    }
                }
            }
            if (count >= stop) {
                break;
            }
        }
    } else {
        for (auto &&font : _fonts) {
            if (count++ >= skip) {
                _font_store->append(FontElement::create_font(font));
            }
            if (count >= stop) {
                break;
            }
        }
    }

    _font_store->thaw_notify();

    // return true (keep going) if we early-exited ourselves by stopping at a chunk limit
    return count >= stop;
}

void FontList::update_font_count()
{
    auto &font_count = get_widget<Gtk::Label>(_builder, "font-count");
    // total number of fonts; subtract one, there's a placeholder or "missing" font entry always present
    auto total = _font_store->get_n_items() - 1;
    // use grid selection model, as it always reflects current number of fonts (list view can be partially collapsed)
    auto count = _grid_selection->get_n_items();
    // count could be larger than total if we insert "missing" font(s)
    auto label = count >= total ? C_("N-of-fonts", "All fonts")
                                : Glib::ustring::format(count, ' ', C_("N-of-fonts", "of"), ' ', total, ' ',
                                                        C_("N-of-fonts", "fonts"));
    font_count.set_text(label);
}

double FontList::get_fontsize() const
{
    auto size = _font_size.getSize();
    return size > 0 ? size : _current_fsize;
}

Glib::RefPtr<Glib::ObjectBase> FontList::get_nth_font(int index) const
{
    if (_list_visible) {
        if (auto row = std::dynamic_pointer_cast<Gtk::TreeListRow>(_list_selection->get_object(index))) {
            return row->get_item();
        }
    } else {
        return _grid_selection->get_object(index);
    }

    return {};
}

Glib::RefPtr<Glib::ObjectBase> FontList::get_selected_font() const
{
    if (_list_visible) {
        if (auto row = std::dynamic_pointer_cast<Gtk::TreeListRow>(_list_selection->get_selected_item())) {
            return row->get_item();
        }
    } else {
        return _grid_selection->get_selected_item();
    }

    return {};
}

Glib::ustring FontList::get_fontspec() const
{
    if (auto element = std::dynamic_pointer_cast<FontElement>(get_selected_font())) {
        auto &font = element->font();
        if (font.ff) {
            auto variations = _font_variations.get_pango_string(true);
            return Inkscape::get_inkscape_fontspec(font.ff, font.face, variations);
        } else {
            // missing fonts don't have known variation that we could tweak,
            // so ignore _font_variations UI and simply return alt_fontspec
            return element->get_alt_spec();
        }
    }
    return "sans-serif"; // no selection
}

void FontList::set_current_font(Glib::ustring const &family, Glib::ustring const &face)
{
    if (_update.pending())
        return;

    auto scoped = _update.block();

    auto fontspec = Inkscape::get_fontspec(family, face);

    if (fontspec == _current_fspec) {
        auto fspec = get_fontspec_without_variants(fontspec);
        select_font(fspec);
        return;
    }
    _current_fspec = fontspec;

    if (!fontspec.empty() && fontspec != get_fontspec()) {
        _font_variations.update(fontspec);
        _var_axes.set_visible(_font_variations.variations_present());
        add_font(fontspec, true);
    }
}

void FontList::set_current_size(double size)
{
    _current_fsize = size;
    if (_update.pending())
        return;

    auto scoped = _update.block();
    _font_size.setSize(size);
}

void FontList::add_font(Glib::ustring const &fontspec, bool select)
{
    auto scoped = _update.block();

    bool found = select_font(fontspec); // found in the tree view?
    if (found)
        return;

    auto it = std::ranges::find_if(
        _fonts, [&](FontInfo const &f) { return get_inkscape_fontspec(f.ff, f.face, f.variations) == fontspec; });

    // fonts with variations will not be found, we need to remove " @ axis=value" part
    auto fspec = get_fontspec_without_variants(fontspec);
    // auto at = fontspec.rfind('@');
    if (it == end(_fonts) && fspec != fontspec) {
        // try to match existing font
        it = std::find_if(begin(_fonts), end(_fonts), [&](FontInfo const &f) {
            return get_inkscape_fontspec(f.ff, f.face, f.variations) == fspec;
        });
        if (it != end(_fonts)) {
            bool found = select_font(fspec); // found in the tree view?
            if (found)
                return;
        }
    }

    Glib::RefPtr<FontElement> insert;

    if (it != end(_fonts)) {
        // font found in the "all fonts" vector, but
        // this font is filtered out; boost it temporarily to let it pass filtering
        insert = FontElement::create_injected_font(*it, {}, false);
    } else {
        bool missing_font = true;
        FontInfo subst;

        auto desc = Pango::FontDescription(fontspec);
        auto vars = desc.get_variations();
        if (!vars.empty()) {
            // font with variations; check if we have matching family
            subst.variations = vars;

            auto family = desc.get_family();
            it = std::ranges::find_if(_fonts, [&](FontInfo const &f) { return f.ff->get_name() == family; });
            if (it != end(_fonts)) {
                missing_font = false;
                subst.ff = it->ff;
            }
        }

        // set "missing" font entry
        insert = FontElement::create_injected_font(subst, fontspec.raw(), missing_font);
    }

    // change placeholder font entry; it's a first one in a store
    std::vector<Glib::RefPtr<Glib::ObjectBase>> v(1, insert);
    _font_store->splice(0, 1, v);
    apply_filters();
    scroll_to_row(0);
}

Gtk::Box *FontList::create_pill_box(Glib::ustring const &display_name, Glib::ustring const &tag, bool tags)
{
    auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    auto text = Gtk::make_managed<Gtk::Label>(display_name);
    text->set_ellipsize(Pango::EllipsizeMode::END);
    text->set_halign(Gtk::Align::START);
    text->set_hexpand(true);
    text->set_max_width_chars(20);
    text->set_tooltip_text(display_name);
    auto close = Gtk::make_managed<Gtk::Button>();
    close->set_has_frame(false);
    close->set_icon_name(INKSCAPE_ICON("window-close"));
    close->set_valign(Gtk::Align::CENTER);
    if (tags) {
        close->signal_clicked().connect([=, this] {
            // remove category from current filter
            update_categories(tag, false);
        });
    } else {
        close->signal_clicked().connect([=] {
            // remove collection from current filter
            FontCollections::get()->update_selected_collections(tag);
        });
    }
    box->get_style_context()->add_class("tag-box");
    box->append(*text);
    box->append(*close);
    box->set_valign(Gtk::Align::CENTER);
    return box;
}

// show selected font categories in the filter bar
void FontList::update_filterbar()
{
    // brute force approach at first
    for (auto &&btn : _tag_box.get_children()) {
        _tag_box.remove(*btn);
    }

    for (auto &&ftag : _font_tags.get_selected_tags()) {
        auto pill = create_pill_box(ftag.display_name, ftag.tag, true);
        _tag_box.append(*pill);
    }

    for (auto &&collection : FontCollections::get()->get_selected_collections()) {
        auto pill = create_pill_box(collection, collection, false);
        _tag_box.append(*pill);
    }
}

void FontList::update_categories(std::string const &tag, bool select)
{
    if (_update.pending())
        return;

    auto scoped = _update.block();

    if (!_font_tags.select_tag(tag, select))
        return;

    // update UI
    update_filterbar();

    // apply filter
    apply_filters();
}

void FontList::add_categories()
{
    for (auto row : _tag_list.get_children()) {
        if (row)
            _tag_list.remove(*row);
    }

    auto add_row = [this](Gtk::Widget *w) {
        auto row = Gtk::make_managed<Gtk::ListBoxRow>();
        row->set_can_focus(false);
        row->set_child(*w);
        row->set_sensitive(w->get_sensitive());
        _tag_list.append(*row);
    };

    for (auto &tag : _font_tags.get_tags()) {
        auto btn = Gtk::make_managed<Gtk::CheckButton>("");
        // automatic collections in italic
        auto &label = *Gtk::make_managed<Gtk::Label>();
        label.set_markup("<i>" + tag.display_name + "</i>");
        btn->set_child(label);
        btn->set_active(_font_tags.is_tag_selected(tag.tag));
        btn->signal_toggled().connect([=, this] {
            // toggle font category
            update_categories(tag.tag, btn->get_active());
        });
        add_row(btn);
    }

    // insert user collections
    auto fc = Inkscape::FontCollections::get();
    auto font_collections = fc->get_collections();
    if (!font_collections.empty()) {
        auto sep = Gtk::make_managed<Gtk::Separator>();
        sep->set_margin_bottom(3);
        sep->set_margin_top(3);
        sep->set_sensitive(false);
        add_row(sep);
    }
    for (auto &col : font_collections) {
        auto btn = Gtk::make_managed<Gtk::CheckButton>();
        auto &label = *Gtk::make_managed<Gtk::Label>();
        label.set_text(col);
        btn->set_child(label);
        btn->set_active(fc->is_collection_selected(col));
        btn->signal_toggled().connect([=] {
            // toggle font system collection
            fc->update_selected_collections(col);
        });
        add_row(btn);
    }
}

void FontList::sync_font_tag(FontTag const *ftag, bool selected)
{
    if (!ftag) {
        // many/all tags changed
        add_categories();
        update_filterbar();
    }
    // todo as needed
}

void FontList::scroll_to_row(int index)
{
    auto flags = Gtk::ListScrollFlags::SELECT;

    if (_list_visible) {
        _font_list.scroll_to(index, flags);
    } else {
        _font_grid.scroll_to(index, flags);
    }
}

// place "Font size" box at the top (true) or at the bottom (false) of the dialog
void FontList::set_font_size_layout(bool top)
{
    auto &size = get_widget<Gtk::Box>(_builder, "size-box");
    auto layout1 =
        std::dynamic_pointer_cast<Gtk::GridLayoutChild>(_main_grid.get_layout_manager()->get_layout_child(size));
    auto &variants = get_widget<Gtk::Box>(_builder, "variants");
    auto layout2 =
        std::dynamic_pointer_cast<Gtk::GridLayoutChild>(_main_grid.get_layout_manager()->get_layout_child(variants));
    auto &separator = get_widget<Gtk::Separator>(_builder, "btm-separator");
    if (top) {
        layout1->set_row(3);
        layout2->set_row(4);
        separator.set_visible(false);
    } else {
        layout1->set_row(10);
        layout2->set_row(11);
        separator.set_visible();
    }
    // pop up menu where there is space
    _font_size.setPopupPosition(top ? Gtk::PositionType::BOTTOM : Gtk::PositionType::TOP);
}

void FontList::on_map()
{
    Box::on_map();

    // grow scrollwindow to accommodate up to 4 and a half axes; beyond that - scroll
    static auto four = _font_variations.measure_height(4);
    static auto five = _font_variations.measure_height(5);
    // fractional size to expose fifth axis and let users know there's more content there
    _var_axes.set_max_content_height((four + five) / 2);
}

} // namespace Inkscape::UI::Widget

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Cursor utilities
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include "cursor-utils.h"

#include <boost/compute/detail/lru_cache.hpp>
#include <glibmm/miscutils.h>
#include <gtkmm/icontheme.h>
#include <gtkmm/settings.h>
#include <gtkmm/version.h>

#include "display/cairo-utils.h"
#include "document-update.h"
#include "document.h"
#include "helper/pixbuf-ops.h"
#include "io/file.h"
#include "io/resource.h"
#include "libnrtype/font-factory.h"
#include "object/sp-root.h"
#include "preferences.h"
#include "ui/util.h"
#include "util/units.h"

using Inkscape::IO::Resource::SYSTEM;
using Inkscape::IO::Resource::ICONS;

namespace Inkscape {
namespace {

// SVG cursor unique ID/key
using Key = std::tuple<std::string, std::string, std::string, std::uint32_t, std::uint32_t, bool, double>;

struct CursorDocCache : public Util::EnableSingleton<CursorDocCache, Util::Depends<FontFactory>> {
    std::unordered_map<std::string, std::unique_ptr<SPDocument>> map;
};

struct CursorInputParams
{
    Glib::RefPtr<Gtk::IconTheme> icon_theme;
    std::string file_name;
    Colors::Color fill;
    Colors::Color stroke;
};

struct CursorRenderResult
{
    Glib::RefPtr<Gdk::Texture> texture;
    Geom::IntPoint size;
    Geom::IntPoint hotspot;
};

/**
 * Loads an SVG cursor from the specified file name.
 *
 * Returns pointer to cursor (or null cursor if we could not load a cursor).
 */
CursorRenderResult render_svg_cursor(double scale, CursorInputParams const &in)
{
    // GTK puts cursors in a "cursors" subdirectory of icon themes. We'll do the same... but
    // note that we cannot use the normal GTK method for loading cursors as GTK knows nothing
    // about scalable SVG cursors. We must locate and load the files ourselves. (Even if
    // GTK could handle scalable cursors, we would need to load the files ourselves inorder
    // to modify CSS 'fill' and 'stroke' properties.)
    auto fill = in.fill;
    auto stroke = in.stroke;

    // Make list of icon themes, highest priority first.
    std::vector<std::string> theme_names;

    // Set in preferences
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Glib::ustring theme_name = prefs->getString("/theme/iconTheme", prefs->getString("/theme/defaultIconTheme", ""));
    if (!theme_name.empty()) {
        theme_names.push_back(std::move(theme_name));
    }

    // System
    theme_name = Gtk::Settings::get_default()->property_gtk_icon_theme_name();
    theme_names.push_back(std::move(theme_name));

    // Our default
    theme_names.emplace_back("hicolor");

    // quantize opacity to limit number of cursor variations we generate
    fill.setOpacity(std::floor(std::clamp(fill.getOpacity(), 0.0, 1.0) * 100) / 100);
    stroke.setOpacity(std::floor(std::clamp(stroke.getOpacity(), 0.0, 1.0) * 100) / 100);

    const auto enable_drop_shadow = prefs->getBool("/options/cursor-drop-shadow", true);

    // Find the rendered size of the icon.
    // cursor scaling? note: true by default - this has to be in sync with inkscape-preferences where it is true
    bool cursor_scaling = prefs->getBool("/options/cursorscaling", true); // Fractional scaling is broken but we can't detect it.
    if (!cursor_scaling) {
        scale = 1;
    }

    constexpr int max_cached_cursors = 100;
    static boost::compute::detail::lru_cache<Key, CursorRenderResult> cursor_cache(max_cached_cursors);

    // construct a key
    Key cursor_key = {theme_names[0], theme_names[1], in.file_name,
                            fill.toRGBA(), stroke.toRGBA(),
                            enable_drop_shadow, scale};
    if (auto const it = cursor_cache.get(cursor_key)) {
        return *it;
    }

    // Find theme paths.
    auto theme_paths = in.icon_theme->get_search_path();

    // cache cursor SVG documents too, so we can regenerate cursors (with different colors) quickly
    auto &cursor_docs = CursorDocCache::get().map;
    SPRoot* root = nullptr;

    if (auto it = cursor_docs.find(in.file_name); it != end(cursor_docs)) {
        root = it->second->getRoot();
    }

    if (!root) {
        // Loop over theme names and paths, looking for file.
        Glib::RefPtr<Gio::File> file;
        std::string full_file_path;
        bool file_exists = false;
        for (auto const &theme_name : theme_names) {
            for (auto const &theme_path : theme_paths) {
                full_file_path = Glib::build_filename(theme_path, theme_name, "cursors", in.file_name);
                // std::cout << "Checking: " << full_file_path << std::endl;
                file = Gio::File::create_for_path(full_file_path);
                file_exists = file->query_exists();
                if (file_exists) break;
            }
            if (file_exists) break;
        }

        if (!file->query_exists()) {
            std::cerr << "load_svg_cursor: Cannot locate cursor file: " << in.file_name << std::endl;
            return {};
        }

        auto document = ink_file_open(file).first;

        if (!document) {
            std::cerr << "load_svg_cursor: Could not open document: " << full_file_path << std::endl;
            return {};
        }

        root = document->getRoot();
        if (!root) {
            std::cerr << "load_svg_cursor: Could not find SVG element: " << full_file_path << std::endl;
            return {};
        }

        cursor_docs[in.file_name] = std::move(document);
    }

    if (!root) {
        return {};
    }

    // Set the CSS 'fill' and 'stroke' properties on the SVG element (for cascading).
    SPCSSAttr *css = sp_repr_css_attr(root->getRepr(), "style");
    sp_repr_css_set_property_string(css, "fill", fill.toString(false));
    sp_repr_css_set_property_string(css, "stroke", stroke.toString(false));
    sp_repr_css_set_property_double(css, "fill-opacity",   fill.getOpacity());
    sp_repr_css_set_property_double(css, "stroke-opacity", stroke.getOpacity());
    root->changeCSS(css, "style");
    sp_repr_css_attr_unref(css);

    if (!enable_drop_shadow) {
        // turn off drop shadow, if any
        Glib::ustring shadow("drop-shadow");
        auto objects = root->document->getObjectsByClass(shadow);
        for (auto&& el : objects) {
            if (auto val = el->getAttribute("class")) {
                Glib::ustring cls = val;
                auto pos = cls.find(shadow);
                if (pos != Glib::ustring::npos) {
                    cls.erase(pos, shadow.length());
                }
                el->setAttribute("class", cls);
            }
        }
    }

    // Some cursors are un-versioned, so always attempt to adjust legacy files.
    sp_file_fix_hotspot(root);

    int w = root->document->getWidth().value("px");
    int h = root->document->getHeight().value("px");

    Geom::Rect area(0, 0, w, h);
    int dpi = Inkscape::Util::Quantity::convert(scale, "in", "px");

    // render document into internal bitmap; returns null on failure
    auto ink_pixbuf = std::unique_ptr<Inkscape::Pixbuf>(sp_generate_internal_bitmap(root->document, area, dpi));
    if (!ink_pixbuf) {
        std::cerr << "load_svg_cursor: failed to create pixbuf for: " << in.file_name << std::endl;
        return {};
    }

    // Calculate the hotspot
    auto const root_pos = Geom::Point(-root->root_x.computed, -root->root_y.computed);
    auto const hotspot = (area.clamp(root_pos) * scale).round();

    auto cursor = CursorRenderResult{
        .texture = to_texture(ink_pixbuf->getSurface()),
        .size = {w, h},
        .hotspot = hotspot
    };

    cursor_cache.insert(std::move(cursor_key), cursor);

    return cursor;
}

} // namespace

Glib::RefPtr<Gdk::Cursor>
load_svg_cursor(Gtk::Widget &widget,
                std::string const &file_name,
                std::optional<Colors::Color> maybe_fill,
                std::optional<Colors::Color> maybe_stroke)
{
    auto params = CursorInputParams{
        .icon_theme = Gtk::IconTheme::get_for_display(widget.get_display()),
        .file_name = file_name,
        .fill = maybe_fill.value_or(Colors::Color(0xffffffff)),
        .stroke = maybe_stroke.value_or(Colors::Color(0x000000ff))
    };

#if GTKMM_CHECK_VERSION(4, 16, 0) // Use new scaling cursor API if possible.
    return Gdk::Cursor::create_from_slot([params] (int, double scale, int &width, int &height, int &hotspot_x, int &hotspot_y) {
        auto res = render_svg_cursor(scale, params);
        width = res.size.x();
        height = res.size.y();
        hotspot_x = res.hotspot.x();
        hotspot_y = res.hotspot.y();
        return std::move(res.texture);
    });
#else
    auto res = render_svg_cursor(widget.get_scale_factor(), params);

    if (!res.texture) {
        return {};
    }

    return Gdk::Cursor::create(std::move(res.texture), res.hotspot.x(), res.hotspot.y());
#endif
}

/**
 * Loads an SVG cursor from the specified file name, and sets it as the cursor
 * of the given widget.
 */
void
set_svg_cursor(Gtk::Widget &widget,
                std::string const &file_name,
                std::optional<Colors::Color> fill,
                std::optional<Colors::Color> stroke)
{
    auto cursor = load_svg_cursor(widget, file_name, fill, stroke);
    widget.set_cursor(std::move(cursor));
}

} // namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

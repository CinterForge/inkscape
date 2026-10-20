// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Utility functions for UI
 *
 * Authors:
 *   Tavmjong Bah
 *   John Smith
 *
 * Copyright (C) 2004, 2013, 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "util.h"

#include <cstdint>
#include <stdexcept>
#include <glibmm/i18n.h>
#include <glibmm/regex.h>
#include <glibmm/spawn.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/image.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/popover.h>
#include <gtkmm/revealer.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/textbuffer.h>
#include <gtkmm/tooltip.h>
#include <2geom/bezier.h>

#ifdef GDK_WINDOWING_X11
#include <gdk/x11/gdkx.h>
#endif

#include "defocus-target.h"
#include "desktop.h"
#include "inkscape.h"
#include "inkscape-window.h"
#include "colors/utils.h" // color to hex string
#include "ui/dialog-run.h"
#include "util/numeric/converters.h"
#include "widget/generic/spin-button.h"

// NOTE: Include windows stuff last, as it #defines ERROR leading to compilation errors
#if (defined (_WIN32) || defined (_WIN64))
#undef NOGDI
#include <gdk/win32/gdkwin32.h>
#include <dwmapi.h>
/* For Windows 10 version 1809, 1903, 1909. */
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE_OLD
#define DWMWA_USE_IMMERSIVE_DARK_MODE_OLD 19
#endif
/* For Windows 10 version 2004 and higher, and Windows 11. */
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#endif

/*
 * Ellipse text if longer than maxlen, "50% start text + ... + ~50% end text"
 * Text should be > length 8 or just return the original text
 */
Glib::ustring ink_ellipsize_text(Glib::ustring const &src, std::size_t maxlen)
{
    if (src.length() > maxlen && maxlen > 8) {
        using std::size_t;
        size_t p1 = (size_t) maxlen / 2;
        size_t p2 = (size_t) src.length() - (maxlen - p1 - 1);
        return src.substr(0, p1) + "…" + src.substr(p2);
    }
    return src;
}

/**
 * Show widget, if the widget has a Gtk::Reveal parent, reveal instead.
 *
 * @param widget - The child widget to show.
 */
void reveal_widget(Gtk::Widget *widget, bool show)
{
    auto revealer = dynamic_cast<Gtk::Revealer *>(widget->get_parent());
    if (revealer) {
        revealer->set_reveal_child(show);
    }
    if (show) {
        widget->set_visible(true);
    } else if (!revealer) {
        widget->set_visible(false);
    }
}


bool is_widget_effectively_visible(Gtk::Widget const *widget) {
    if (!widget) return false;

    // TODO: what's the right way to determine if widget is visible on the screen?
    return widget->get_child_visible();
}

namespace Inkscape::UI {

/**
 * Recursively set all the icon sizes inside this parent widget. Any GtkImage will be changed
 * so only call this on widget stacks where all children have the same expected sizes.
 *
 * @param parent - The parent widget to traverse
 * @param pixel_size - The new pixel size of the images it contains
 */
void set_icon_sizes(Gtk::Widget *parent, int pixel_size)
{
    if (!parent) return;

    for_each_descendant(*parent, [=](Gtk::Widget &widget) {
        if (dynamic_cast<Gtk::SpinButton*>(&widget) || dynamic_cast<Widget::InkSpinButton*>(&widget)) {
            // do not descend into spinbuttons; it will impact +/- icons too
            return ForEachResult::_skip;
        }
        if (auto const ico = dynamic_cast<Gtk::Image *>(&widget)) {
            ico->set_from_icon_name(ico->get_icon_name());
            ico->set_pixel_size(pixel_size);
        }
        return ForEachResult::_continue;
    });
}

void set_icon_sizes(GtkWidget* parent, int pixel_size)
{
    set_icon_sizes(Glib::wrap(parent), pixel_size);
}

void gui_warning(const std::string &msg, Gtk::Window *parent_window) {
    g_warning("%s", msg.c_str());
    if (INKSCAPE.active_desktop()) {
        Gtk::MessageDialog warning(_(msg.c_str()), false, Gtk::MessageType::WARNING, Gtk::ButtonsType::OK, true);
        warning.set_transient_for(parent_window ? *parent_window : *INKSCAPE.active_desktop()->getInkscapeWindow());
        dialog_run(warning);
    }
}

void system_open(const Glib::ustring &path)
{
#ifdef _WIN32
    ShellExecute(nullptr, "open", path.c_str(), nullptr, nullptr, SW_SHOWDEFAULT);
#elif defined(__APPLE__)
    Glib::spawn_async("", { "open", path.raw() }, Glib::SpawnFlags::SEARCH_PATH);
#else
    char * const uripath = g_filename_to_uri(path.c_str(), nullptr, nullptr);
    Glib::spawn_async("", { "xdg-open", uripath }, Glib::SpawnFlags::SEARCH_PATH);
    g_free(uripath);
#endif
}

Gtk::Widget *get_nth_child(Gtk::Widget &widget, std::size_t index)
{
    auto child = widget.get_first_child();
    for (std::size_t i = 0; i != index && child; ++i) {
        child = child->get_next_sibling();
    }
    return child;
}

std::size_t get_n_children(Gtk::Widget const &widget)
{
    return std::ranges::distance(children(widget));
}

/**
 * Returns a named descendent of parent, which has the given name, or nullptr if there's none.
 *
 * \param[in] parent The widget to search
 * \param[in] name   The name of the desired child widget
 *
 * \return The specified child widget, or nullptr if it cannot be found
 */
Gtk::Widget *find_widget_by_name(Gtk::Widget &parent, Glib::ustring const &name, bool visible_only)
{
    return for_each_descendant(parent, [&](auto const &widget) {
        if (visible_only && !widget.get_visible()) return ForEachResult::_skip;

        return widget.get_name().raw() == name.raw() ? ForEachResult::_break : ForEachResult::_continue;
    });
}

/**
 * This function traverses a tree of widgets searching for first focusable widget.
 *
 * \param[in] parent The widget to start traversal from - top of the tree
 *
 * \return The first focusable widget or nullptr if none are focusable.
 */
Gtk::Widget *find_focusable_widget(Gtk::Widget &parent)
{
    return for_each_descendant(parent, [](auto const &widget)
          { return widget.get_focusable() ? ForEachResult::_break : ForEachResult::_continue; });
}

/// Returns if widget is a descendant of given ancestor, i.e.: itself, a child, or a childʼs child.
/// @param descendant The widget of interest
/// @param ancestor   The potential ancestor
/// @return Whether the widget of interest is a descendant of the given ancestor.
bool is_descendant_of(Gtk::Widget const &descendant, Gtk::Widget const &ancestor)
{
    for (auto &parent : parent_chain(descendant)) {
        if (&parent == &ancestor) {
            return true;
        }
    }
    return false;
}

/// Returns if widget or one of its descendants has focus.
/// @param widget The widget of interest.
/// @return If the widget of interest or a descendant has focus.
bool contains_focus(Gtk::Widget &widget)
{
    if (widget.has_focus()) {
        return true;
    }

    Gtk::Root const *root = widget.get_root();
    if (!root) {
        return false;
    }

    Gtk::Widget const *focused = root->get_focus();
    if (!focused) {
        return false;
    }

    return focused->is_ancestor(widget);
}

/// Get the relative font size as determined by a widgetʼs style/Pango contexts.
int get_font_size(Gtk::Widget &widget)
{
    auto pango_context = widget.get_pango_context();
    auto font_description = pango_context->get_font_description();
    double font_size = font_description.get_size();
    font_size /= Pango::SCALE;
    if (font_description.get_size_is_absolute()) {
        font_size *= 0.75;
    }
    return font_size;
}

void ellipsize(Gtk::Label &label, int const max_width_chars, Pango::EllipsizeMode const mode)
{
    if (max_width_chars <= 0) return;

    label.set_max_width_chars(max_width_chars);
    label.set_ellipsize(mode);
    label.set_has_tooltip(true);
    label.signal_query_tooltip().connect([&](int, int, bool,
                                             Glib::RefPtr<Gtk::Tooltip> const &tooltip)
    {
        if (!label.get_layout()->is_ellipsized()) return false;

        tooltip->set_text(label.get_text());
        return true;
    }, true);
}

void set_defocus_target(Gtk::Widget* panel, DefocusTarget* target) {
    if (!panel) return;

    for_each_descendant(*panel, [target](auto& widget) {
        if (auto sb = dynamic_cast<Widget::InkSpinButton*>(&widget)) {
            sb->setDefocusTarget(target);
        }
        return ForEachResult::_continue;
    });
}

void close_parent_popover(Gtk::Widget &widget)
{
    for (auto &parent : parent_chain(widget) | std::views::drop(1)) {
        if (auto popover = dynamic_cast<Gtk::Popover *>(&parent)) {
            popover->popdown();
            break;
        }
    }
}

} // namespace Inkscape::UI

Gdk::RGBA mix_colors(const Gdk::RGBA& a, const Gdk::RGBA& b, float ratio) {
    auto lerp = [](double v0, double v1, double t){ return (1.0 - t) * v0 + t * v1; };
    Gdk::RGBA result;
    result.set_rgba(
        lerp(a.get_red(),   b.get_red(),   ratio),
        lerp(a.get_green(), b.get_green(), ratio),
        lerp(a.get_blue(),  b.get_blue(),  ratio),
        lerp(a.get_alpha(), b.get_alpha(), ratio)
    );
    return result;
}

double get_luminance(Gdk::RGBA const &rgba)
{
    // This formula is recommended at https://www.w3.org/TR/AERT/#color-contrast
    return 0.299 * rgba.get_red  ()
         + 0.587 * rgba.get_green()
         + 0.114 * rgba.get_blue ();
}

GdkRGBA get_color(Gtk::Widget const &widget)
{
    GdkRGBA result;
    gtk_widget_get_color(const_cast<GtkWidget *>(widget.gobj()), &result);
    return result;
}

Gdk::RGBA get_color_with_class(Gtk::Widget &widget,
                               Glib::ustring const &css_class)
{
    if (!css_class.empty()) widget.add_css_class(css_class);
    auto result = widget.get_color();
    if (!css_class.empty()) widget.remove_css_class(css_class);
    return result;
}

guint32 to_guint32(Gdk::RGBA const &rgba)
{
        return static_cast<guint32>(0xFF * rgba.get_red  () + 0.5) << 24 |
               static_cast<guint32>(0xFF * rgba.get_green() + 0.5) << 16 |
               static_cast<guint32>(0xFF * rgba.get_blue () + 0.5) <<  8 |
               static_cast<guint32>(0xFF * rgba.get_alpha() + 0.5);
}

Gdk::RGBA color_to_rgba(Inkscape::Colors::Color const &color)
{
    return to_rgba(color.toRGBA());
}

Gdk::RGBA to_rgba(guint32 const u32)
{
    auto rgba = Gdk::RGBA{};
    rgba.set_red  (((u32 & 0xFF000000) >> 24) / 255.0);
    rgba.set_green(((u32 & 0x00FF0000) >> 16) / 255.0);
    rgba.set_blue (((u32 & 0x0000FF00) >>  8) / 255.0);
    rgba.set_alpha(((u32 & 0x000000FF)      ) / 255.0);
    return rgba;
}

/**
 * These GUI related color conversions allow us to convert from
 * SVG xml attributes to Gdk colors, without needing the entire CMS
 * framework, which would be excessive for widget painting.
 */
Glib::ustring gdk_to_css_color(const Gdk::RGBA& color) {
    return Inkscape::Colors::rgba_to_hex(to_guint32(color), true);
}
Gdk::RGBA css_color_to_gdk(const char *value) {
    try {
        return to_rgba(value ? Inkscape::Colors::hex_to_rgba(value) : 0x0);
    } catch (Inkscape::Colors::ColorError &e) {
        return {};
    }
}

// 2Geom <-> Cairo

Cairo::RectangleInt geom_to_cairo(const Geom::IntRect &rect)
{
    return Cairo::RectangleInt{rect.left(), rect.top(), rect.width(), rect.height()};
}

Geom::IntRect cairo_to_geom(const Cairo::RectangleInt &rect)
{
    return Geom::IntRect::from_xywh(rect.x, rect.y, rect.width, rect.height);
}

Cairo::Matrix geom_to_cairo(const Geom::Affine &affine)
{
    return Cairo::Matrix(affine[0], affine[1], affine[2], affine[3], affine[4], affine[5]);
}

Geom::IntPoint dimensions(const Cairo::RefPtr<Cairo::ImageSurface> &surface)
{
    return Geom::IntPoint(surface->get_width(), surface->get_height());
}

Geom::IntPoint dimensions(const Gdk::Rectangle &allocation)
{
    return Geom::IntPoint(allocation.get_width(), allocation.get_height());
}

Geom::Affine gtk_to_2geom(graphene_matrix_t const &mat)
{
    Geom::Affine aff;
    if (!graphene_matrix_to_2d(&mat, &aff[0], &aff[1], &aff[2], &aff[3], &aff[4], &aff[5])) {
        std::cerr << "graphene_matrix_to_2d: not convertible" << std::endl;
    }
    return aff;
}

std::vector<GskColorStop> create_cubic_gradient(
    const Gdk::RGBA& from,
    const Gdk::RGBA& to,
    Geom::Point ctrl1,
    Geom::Point ctrl2,
    Geom::Point p0,
    Geom::Point p1,
    int steps
) {
    // validate input points
    for (auto&& pt : {p0, ctrl1, ctrl2, p1}) {
        if (pt.x() < 0 || pt.x() > 1 ||
            pt.y() < 0 || pt.y() > 1) {
            throw std::invalid_argument("Invalid points for cubic gradient; 0..1 coordinates expected.");
        }
    }
    if (steps < 2 || steps > 999) {
        throw std::invalid_argument("Invalid number of steps for cubic gradient; 2 to 999 steps expected.");
    }

    std::vector<GskColorStop> result;

    --steps;
    for (int step = 0; step <= steps; ++step) {
        auto t = static_cast<double>(step) / steps;
        auto p = Geom::bernstein_value_at(t, std::begin({p0, ctrl1, ctrl2, p1}), 3);

        float offset = p.x();
        float ratio = p.y();

        result.push_back(GskColorStop{
            .offset = offset,
            .color = *mix_colors(from, to, ratio).gobj()
        });
    }

    return result;
}

Gdk::RGBA change_alpha(const Gdk::RGBA& color, double new_alpha) {
    auto copy(color);
    copy.set_alpha(new_alpha);
    return copy;
}

uint32_t conv_gdk_color_to_rgba(const Gdk::RGBA& color, double replace_alpha) {
    auto alpha = replace_alpha >= 0 ? replace_alpha : color.get_alpha();
    auto rgba =
            uint32_t(0xff * color.get_red()) << 24 |
            uint32_t(0xff * color.get_green()) << 16 |
            uint32_t(0xff * color.get_blue()) << 8 |
            uint32_t(0xff * alpha);
    return rgba;
}

void set_dark_titlebar(Glib::RefPtr<Gdk::Surface> const &surface, bool is_dark)
{
#if (defined (_WIN32) || defined (_WIN64))
    if (surface->gobj()) {
        BOOL w32_darkmode = is_dark;
        HWND hwnd = (HWND)gdk_win32_surface_get_handle((GdkSurface*)surface->gobj());
        if (DwmSetWindowAttribute) {
            DWORD attr = DWMWA_USE_IMMERSIVE_DARK_MODE;
            if (FAILED(DwmSetWindowAttribute(hwnd, attr, &w32_darkmode, sizeof(w32_darkmode)))) {
                attr = DWMWA_USE_IMMERSIVE_DARK_MODE_OLD;
                DwmSetWindowAttribute(hwnd, attr, &w32_darkmode, sizeof(w32_darkmode));
            }
        }
    }
#endif
}

// round_numbers helper callback
static int fmt_number(const _GMatchInfo* match, _GString* ret, void* prec) {
    auto number = g_match_info_fetch(match, 1);

    char* end = nullptr;
    double val = g_ascii_strtod(number, &end);
    if (*number && (end == nullptr || end > number)) {
        auto precision = *static_cast<int*>(prec);
        auto fmt = Inkscape::Util::format_number(val, precision);
        g_string_append(ret, fmt.c_str());
    } else {
        g_string_append(ret, number);
    }

    auto text = g_match_info_fetch(match, 2);
    g_string_append(ret, text);

    g_free(number);
    g_free(text);

    return false;
}

Glib::ustring round_numbers(const Glib::ustring& text, int precision) {
    // match floating point number followed by something else (not a number); repeat
    static const auto numbers = Glib::Regex::create("([-+]?(?:(?:\\d+\\.?\\d*)|(?:\\.\\d+))(?:[eE][-+]?\\d*)?)([^+\\-0-9]*)", Glib::Regex::CompileFlags::MULTILINE);

    return numbers->replace_eval(text, text.size(), 0, Glib::Regex::MatchFlags::NOTEMPTY, &fmt_number, &precision);
}

// Round the selected floating point numbers in the attribute edit popover.
void truncate_digits(const Glib::RefPtr<Gtk::TextBuffer>& buffer, int precision) {
    if (!buffer) return;

    auto start = buffer->begin();
    auto end = buffer->end();

    bool had_selection = buffer->get_has_selection();
    int start_idx = 0, end_idx = 0;
    if (had_selection) {
        buffer->get_selection_bounds(start, end);
        start_idx = start.get_offset();
        end_idx = end.get_offset();
    }

    auto text = buffer->get_text(start, end);
    auto ret = round_numbers(text, precision);
    buffer->erase(start, end);
    buffer->insert_at_cursor(ret);

    if (had_selection) {
        // Restore selection but note that its length may have decreased.
        end_idx -= text.size() - ret.size();
        if (end_idx < start_idx) {
            end_idx = start_idx;
        }
        buffer->select_range(buffer->get_iter_at_offset(start_idx), buffer->get_iter_at_offset(end_idx));
    }
}

Glib::RefPtr<Gdk::Texture> to_texture(Cairo::RefPtr<Cairo::Surface> const &surface)
{
    if (!surface) {
        return {};
    }

    assert(surface->get_type() == Cairo::Surface::Type::IMAGE);

    auto img = Cairo::ImageSurface(surface->cobj());
    assert(img.get_format() == Cairo::ImageSurface::Format::ARGB32);

    auto bytes = g_bytes_new_with_free_func(img.get_data(),
                                            img.get_stride() * img.get_height(),
                                            (GDestroyNotify)cairo_surface_destroy,
                                            cairo_surface_reference(surface->cobj()));

    auto texture = gdk_memory_texture_new(img.get_width(),
                                          img.get_height(),
                                          GDK_MEMORY_DEFAULT,
                                          bytes,
                                          img.get_stride());

    g_bytes_unref(bytes);

    return Glib::wrap(texture);
}

void restrict_minsize_to_square(Gtk::Widget& widget, int min_size_px) {
    auto name = widget.get_name();
    assert(!name.empty());
    auto css = Gtk::CssProvider::create();
    std::ostringstream ost;
    ost << "#" << name << " {min-width:" << min_size_px << "px; min-height:" << min_size_px << "px;}";
    css->load_from_string(ost.str());
    auto style_context = widget.get_style_context();
    // load with a priority higher than that of the "style.css"
    style_context->add_provider(css, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 2);
}

void set_degree_suffix(Inkscape::UI::Widget::InkSpinButton& button) {
    button.set_suffix("\u00b0", false); // degree symbol
}

void set_percent_suffix(Inkscape::UI::Widget::InkSpinButton& button) {
    button.set_suffix(_("%"), false);
}

char const *get_text(Gtk::Editable const &editable)
{
    return gtk_editable_get_text(const_cast<GtkEditable *>(editable.gobj())); // C API is const-incorrect
}

Gtk::Button* create_button(const char* label_text, const char* icon_name) {
    if (!label_text || !icon_name) return nullptr;

    auto button = Gtk::make_managed<Gtk::Button>();
    auto box = Gtk::make_managed<Gtk::Box>();
    box->set_spacing(4);
    auto icon = Gtk::make_managed<Gtk::Image>();
    icon->set_from_icon_name(icon_name);
    auto label = Gtk::make_managed<Gtk::Label>(label_text);
    box->append(*icon);
    box->append(*label);
    button->set_child(*box);
    box->set_halign(Gtk::Align::CENTER);
    return button;
}

Glib::ustring get_synthetic_object_name(SPObject const* object) {
    if (!object) return {};

    auto id = object->getId();
    if (auto item = cast<SPItem>(object)) {
        if (id) {
            return Glib::ustring::compose("%1 %2", item->displayName(), id);
        }
        return Glib::ustring::compose("%1", item->displayName());
    }
    else if (id) {
        return Glib::ustring::compose("#%1", id);
    }
    else if (auto repr = object->getRepr()) {
        return Glib::ustring::compose("<%1>", repr->name());
    }
    return "object";
}

Geom::Point get_surface_transform(Gtk::Native const &native)
{
    double x, y;
    const_cast<Gtk::Native &>(native).get_surface_transform(x, y);
    return {x, y};
}

Geom::Affine compute_transform(Gtk::Widget const &widget, Gtk::Widget const &target)
{
    graphene_matrix_t mat;
    if (!gtk_widget_compute_transform(const_cast<GtkWidget *>(widget.gobj()), const_cast<GtkWidget *>(target.gobj()), &mat)) {
        std::cerr << "compute_transform(): not convertible" << std::endl;
    }
    return gtk_to_2geom(mat);
}

Geom::Affine get_event_transform(Glib::RefPtr<Gdk::Surface const> const &event_surface, Gtk::Widget const &target)
{
    auto native = Gtk::Native::get_for_surface(event_surface);
    auto &event_widget = dynamic_cast<Gtk::Widget const &>(*native);
    return Geom::Translate{-get_surface_transform(*native)} * compute_transform(event_widget, target);
}

bool is_x11_display()
{
#ifdef GDK_WINDOWING_X11
    auto display = gdk_display_get_default();
    return GDK_IS_X11_DISPLAY(display);
#else
    return false;
#endif
}

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

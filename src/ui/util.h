// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Utility functions for UI
 *
 * Authors:
 *   Tavmjong Bah
 *   John Smith
 *
 * Copyright (C) 2013, 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef UI_UTIL_SEEN
#define UI_UTIL_SEEN

#include <ranges>
#include <boost/iterator/iterator_facade.hpp>
#include <gtkmm/notebook.h>
#include <gtkmm/button.h>
#include <gtkmm/gestureclick.h>
#include <2geom/rect.h>

class SPObject;

namespace Gtk {
class Button;
}

namespace Inkscape::UI::Widget {
class InkSpinButton;
}

/*
 * Use these errors when building from glade files for graceful
 * fallbacks and prevent crashes from corrupt ui files.
 */
class UIBuilderError : public std::exception {};
class UIFileUnavailable : public UIBuilderError {};
class WidgetUnavailable : public UIBuilderError {};

namespace Cairo {
class Matrix;
class ImageSurface;
} // namespace Cairo

namespace Glib {
class ustring;
} // namespace Glib

namespace Gtk {
class Editable;
class Label;
class TextBuffer;
class Widget;
} // namespace Gtk

namespace Inkscape::Colors {
class Color;
} // namespace Inkscape::Colors

Glib::ustring ink_ellipsize_text(Glib::ustring const &src, std::size_t maxlen);

void reveal_widget(Gtk::Widget *widget, bool show);

// check if widget in a container is actually visible
bool is_widget_effectively_visible(Gtk::Widget const *widget);

namespace Inkscape::UI {
class DefocusTarget;

void set_icon_sizes(Gtk::Widget *parent, int pixel_size);
void set_icon_sizes(GtkWidget *parent, int pixel_size);

void gui_warning(const std::string &msg, Gtk::Window * parent_window = nullptr);

struct NullWidgetSentinel {};

template <typename T>
class SiblingIterator : public boost::iterator_facade<SiblingIterator<T>, T, boost::bidirectional_traversal_tag>
{
public:
    SiblingIterator() = default;
    explicit SiblingIterator(T *widget) : _widget(widget) {}

    bool operator==(NullWidgetSentinel) const { return !_widget; };

private:
    T *_widget = nullptr;

    void increment() { _widget = _widget->get_next_sibling(); }
    void decrement() { _widget = _widget->get_prev_sibling(); }
    T &dereference() const { return *_widget; }

    friend class boost::iterator_core_access;
};

template <typename T>
class ParentIterator : public boost::iterator_facade<ParentIterator<T>, T, boost::forward_traversal_tag>
{
public:
    ParentIterator() = default;
    explicit ParentIterator(T *widget) : _widget(widget) {}

    bool operator==(NullWidgetSentinel) const { return !_widget; };

private:
    T *_widget = nullptr;

    void increment() { _widget = _widget->get_parent(); }
    T &dereference() const { return *_widget; }

    friend class boost::iterator_core_access;
};

/**
 * Returns a widget's direct children starting from get_first_widget() and calling get_next_sibling() each time.
 * @return A bidirectional range of Gtk::Widget [const] &, where the const is deduced.
 */
inline auto children(Gtk::Widget       &widget) { return std::ranges::subrange{SiblingIterator{widget.get_first_child()}, NullWidgetSentinel{}}; }
inline auto children(Gtk::Widget const &widget) { return std::ranges::subrange{SiblingIterator{widget.get_first_child()}, NullWidgetSentinel{}}; }
inline auto children(Gtk::Widget      &&widget) = delete;

/**
 * Returns a widget's parent chain starting from the widget itself and calling get_parent() each time.
 * @return A forward range of Gtk::Widget [const] &, where the const is deduced.
 */
inline auto parent_chain(Gtk::Widget       &widget) { return std::ranges::subrange{ParentIterator{&widget}, NullWidgetSentinel{}}; }
inline auto parent_chain(Gtk::Widget const &widget) { return std::ranges::subrange{ParentIterator{&widget}, NullWidgetSentinel{}}; }
inline auto parent_chain(Gtk::Widget      &&widget) = delete;

/// Whether for_each_descendant() will continue or stop after calling Func per child.
enum class ForEachResult {
    _continue, // go on to the next widget
    _break,    // stop here, return current widget
    _skip      // do not recurse into current widget, go to the next one
};

/// Opens the given path with platform-specific tools.
void system_open(const Glib::ustring &path);
/// Get the widgetʼs child at the given position. Return null if the index is invalid.
Gtk::Widget *get_nth_child(Gtk::Widget &widget, std::size_t index);
/// Get the number of children of a widget.
std::size_t get_n_children(Gtk::Widget const &widget);
/// For each child in get_children(widget), call widget.remove(*child). May not necessarily delete child if there are other references.
template <typename Widget> void remove_all_children(Widget &widget)
{
    for (auto child = widget.get_first_child(); child; ) {
        auto next = child->get_next_sibling();
        widget.remove(*child);
        child = next;
    }
}

/// Call Func with a reference to each descendant of @p parent, until it returns _break.
/// @param widget    The initial widget at the top of the hierarchy, to start at
/// @param func      The widget-testing predicate, returning whether to continue
/// @return The first widget for which @a func returns _break or nullptr if none
template <typename Func>
Gtk::Widget *for_each_descendant(Gtk::Widget &widget, Func &&func)
{
    static_assert(std::is_invocable_r_v<ForEachResult, Func, Gtk::Widget &>);

    auto ret = func(widget);
    if (ret == ForEachResult::_break) return &widget;

    // skip this widget?
    if (ret == ForEachResult::_skip) return nullptr;

    for (auto &child : children(widget)) {
        auto const descendant = for_each_descendant(child, func);
        if (descendant) return descendant;
    }

    return nullptr;
}

/**
 * Returns the pages of a Gtk::Notebook.
 * @return A random-access range of Gtk::Widget &.
 */
inline auto notebook_pages(Gtk::Notebook &notebook)
{
    return std::ranges::iota_view(0, notebook.get_n_pages()) |
           std::views::transform([&notebook] (int n) -> auto & { return *notebook.get_nth_page(n); });
}

[[nodiscard]] Gtk::Widget *find_widget_by_name(Gtk::Widget &parent, Glib::ustring const &name, bool visible_only);
[[nodiscard]] Gtk::Widget *find_focusable_widget(Gtk::Widget &parent);
[[nodiscard]] bool is_descendant_of(Gtk::Widget const &descendant, Gtk::Widget const &ancestor);
[[nodiscard]] bool contains_focus(Gtk::Widget &widget);

// set defocus target on all spinbuttons in a container/dialog/panel
void set_defocus_target(Gtk::Widget* panel, DefocusTarget* target);

[[nodiscard]] int get_font_size(Gtk::Widget &widget);

// If max_width_chars is > 0, then the created Label has :max-width-chars set to
// that limit, the :ellipsize mode is set to the passed-in @a mode, & a ::query-
// tooltip handler is connected to show the label as the tooltip when ellipsized
void ellipsize(Gtk::Label &label, int max_width_chars, Pango::EllipsizeMode mode);

/// Close the parent popover of @a widget, if one exists.
void close_parent_popover(Gtk::Widget &widget);

} // namespace Inkscape::UI

// Mix two RGBA colors using simple linear interpolation:
//  0 -> only a, 1 -> only b, x in 0..1 -> (1 - x)*a + x*b
Gdk::RGBA mix_colors(const Gdk::RGBA& a, const Gdk::RGBA& b, float ratio);

// Create the same color, but with a different opacity (alpha)
Gdk::RGBA change_alpha(const Gdk::RGBA& color, double new_alpha);

/// Calculate luminance of an RGBA color from its RGB in range 0 to 1 inclusive.
/// This uses the perceived brightness formula given at: https://www.w3.org/TR/AERT/#color-contrast
double get_luminance(const Gdk::RGBA &color);

GdkRGBA get_color(Gtk::Widget const &widget);

// Get CSS color for a Widget, based on its current state & a given CSS class.
// N.B.!! Big GTK devs donʼt think changing classes should work ‘within a frame’
// …but it does… & GTK3 GtkCalendar does that – so keep doing it, till we canʼt!
Gdk::RGBA get_color_with_class(Gtk::Widget &widget,
                               Glib::ustring const &css_class);

// Convert a Gdk color to a hex code for css injection.
Glib::ustring gdk_to_css_color(const Gdk::RGBA& color);
Gdk::RGBA css_color_to_gdk(const char *value);

guint32 to_guint32(Gdk::RGBA const &rgba);
Gdk::RGBA color_to_rgba(Inkscape::Colors::Color const &color);
Gdk::RGBA to_rgba(guint32 const u32);

// convert Gdk::RGBA into 32-bit rrggbbaa color, optionally replacing alpha, if specified
uint32_t conv_gdk_color_to_rgba(const Gdk::RGBA& color, double replace_alpha = -1);

Geom::IntRect cairo_to_geom(const Cairo::RectangleInt &rect);
Cairo::RectangleInt geom_to_cairo(const Geom::IntRect &rect);
Cairo::Matrix geom_to_cairo(const Geom::Affine &affine);
Geom::IntPoint dimensions(const Cairo::RefPtr<Cairo::ImageSurface> &surface);
Geom::IntPoint dimensions(const Gdk::Rectangle &allocation);

Geom::Affine gtk_to_2geom(graphene_matrix_t const &mat);

// create a gradient with multiple steps to approximate profile described by given cubic spline
std::vector<GskColorStop> create_cubic_gradient(
    const Gdk::RGBA& from,
    const Gdk::RGBA& to,
    Geom::Point ctrl1,
    Geom::Point ctrl2,
    Geom::Point p0 = Geom::Point(0, 0),
    Geom::Point p1 = Geom::Point(1, 1),
    int steps = 8
);

// If on Windows, get the native window & set it to DWMA_USE_IMMERSIVE_DARK_MODE
void set_dark_titlebar(Glib::RefPtr<Gdk::Surface> const &surface, bool is_dark);

// Parse string that can contain floating point numbers and round them to given precision;
// Used on path data ("d" attribute).
Glib::ustring round_numbers(const Glib::ustring& text, int precision);

// As above, but operating in-place on a TextBuffer
void truncate_digits(const Glib::RefPtr<Gtk::TextBuffer>& buffer, int precision);

/**
 * Convert an image surface in ARGB32 format to a texture.
 * The texture shares data with the surface, so the surface shouldn't modified afterwards.
 */
Glib::RefPtr<Gdk::Texture> to_texture(Cairo::RefPtr<Cairo::Surface> const &surface);

// Restrict widget's min size (min-width & min-height) to specified minimum to keep it square (when it's centered).
// Widget has to have a name given with set_name.
void restrict_minsize_to_square(Gtk::Widget& widget, int min_size_px);

// Add degree symbol suffix to the spin button
void set_degree_suffix(Inkscape::UI::Widget::InkSpinButton& button);
// Add percent symbol suffix to the spin button
void set_percent_suffix(Inkscape::UI::Widget::InkSpinButton& button);

/// Get the text from a GtkEditable without the temporary copy imposed by gtkmm.
char const *get_text(Gtk::Editable const &editable);

// Create managed button with label and icon
Gtk::Button* create_button(const char* label, const char* icon);

// Get a display name for the given object using its type and ID.
// This name can be used if the object's label is not set.
Glib::ustring get_synthetic_object_name(SPObject const* object);

/// Simply wraps Gtk::Native::get_surface_transform().
Geom::Point get_surface_transform(Gtk::Native const &native);

/// Simply wraps Gtk::Widget::compute_transform(). (Missing in GTKmm.)
Geom::Affine compute_transform(Gtk::Widget const &widget, Gtk::Widget const &target);

/**
 * Given an event received by a widget, return the coordinate transformation that brings the event's
 * coordinates into the widget's coordinate system. This is not necessary when using event controllers,
 * but is necessary when accessing Gdk::Event::get_position() or Gdk::Event::get_history() directly.
 */
Geom::Affine get_event_transform(Glib::RefPtr<Gdk::Surface const> const &event_surface, Gtk::Widget const &target);

// Returns true if running under X11.
bool is_x11_display();

namespace Inkscape::UI {

/**
 * Connect a button click handler that receives keyboard modifier state.
 *
 * This function creates a GestureClick controller for the given button and connects
 * it to a callback that receives the modifier state when the button is clicked.
 *
 * @param button The button to connect the controller to
 * @param callback A callable that accepts Gdk::ModifierType parameter
 *
 * Example usage:
 * @code
 * connect_click_with_state(button, [](Gdk::ModifierType state) {
 *     if (Controller::has_flag(state, Gdk::ModifierType::SHIFT_MASK)) {
 *         // Handle Shift+Click
 *     } else {
 *         // Handle normal click
 *     }
 * });
 * @endcode
 */
template<typename Callback>
void connect_click_with_state(Gtk::Button& button, Callback&& callback) {
    auto click = Gtk::GestureClick::create();
    click->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    click->signal_released().connect([callback = std::forward<Callback>(callback), click = click.get()](int, double, double) {
        auto state = click->get_current_event_state();
        callback(state);
    });
    button.add_controller(click);
}

} // namespace Inkscape::UI

#endif // UI_UTIL_SEEN

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

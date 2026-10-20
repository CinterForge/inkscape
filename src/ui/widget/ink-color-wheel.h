// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * @file
 * HSLuv color wheel widget, based on the web implementation at
 * https://www.hsluv.org
*//*
 * Authors:
 *   Tavmjong Bah
 *   Massinissa Derriche <massinissa.derriche@gmail.com>
 *   Daniel Boles <dboles.src+inkscape@gmail.com>
 *
 * Copyright (C) 2018, 2021, 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INK_COLORWHEEL_H
#define INK_COLORWHEEL_H

#include <2geom/line.h>
#include <gtkmm/aspectframe.h>
#include <gtkmm/gesture.h> // Gtk::EventSequenceState

#include "color-wheel.h"
#include "colors/color.h"
#include "ui/widget-vfuncs-class-init.h" // for focus

namespace Gtk {
class Builder;
class DrawingArea;
class EventControllerMotion;
class GestureClick;
} // namespace Gtk

namespace Inkscape::Colors {
class Color;
} // namespace Inkscape::Colors

namespace Inkscape::UI::Widget {

class Bin;

struct ColorPoint final
{
    ColorPoint();
    ColorPoint(double x, double y, Colors::Color color);
    ColorPoint(double x, double y, guint color);

    std::pair<double const &, double const &> get_xy() const { return {x, y}; }

    // eurgh!
    double x;
    double y;
    Colors::Color color;
};

/**
 * @class ColorWheelBase
 */
// AspectFrame because we are circular & enforcing 1:1 eases drawing without overallocating buffers
class ColorWheelBase : public Gtk::AspectFrame, public ColorWheel
{
public:
    ColorWheelBase(Colors::Space::Type type, std::vector<double> initial_color);
    ColorWheelBase(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder, Colors::Space::Type type, std::vector<double> initial_color);

    /// Set the RGB of the wheel. If @a emit is true & hue changes, we call color_changed() for you
    /// @param overrideHue whether to set hue to 0 if min==max(r,g,b) – only used by ColorwheelHSL…
    /// @param emit false if you want to manually call color_changed() e.g. to avoid multiple emits
    /// @return whether or not the value actually changed, to enable avoiding redraw if it does not
    virtual bool setColor(Colors::Color const &color,
                        bool overrideHue = true, bool emit = true) = 0;
    virtual Colors::Color getColor() const { return _values; }

    bool isAdjusting() const { return _adjusting; }

    /// Connect a slot to be called after the color has changed.
    sigc::connection connect_color_changed(sigc::slot<void ()>);

    // debug facility - performance testing only
    void redraw(const Cairo::RefPtr<Cairo::Context>& ctx) override { on_drawing_area_draw(ctx, 1024, 1024); }
protected:
    Colors::Color _values;
    bool _adjusting = false;

    /// Call when color has changed! Emits signal_color_changed & calls _drawing_area->queue_draw()
    void color_changed();
    void queue_drawing_area_draw();

    [[nodiscard]] Gtk::Allocation get_drawing_area_allocation() const;
    [[nodiscard]] bool drawing_area_has_focus() const;
    void focus_drawing_area();

private:
    void set_color(const Colors::Color& color) override { setColor(color, false, false); }
    // Colors::Color get_color() const override { return getColor(); }
    sigc::connection connect_color_changed(sigc::slot<void(const Colors::Color&)> callback) override {
        return _signal_color_changed.connect([this, callback](){ callback(getColor()); });
    }
    Gtk::Widget& get_widget() override { return *this; }

    void construct();
    sigc::signal<void ()> _signal_color_changed;

    UI::Widget::Bin *_bin;
    Gtk::DrawingArea *_drawing_area;
    virtual void on_drawing_area_size(int width, int height, int baseline) {}
    virtual void on_drawing_area_draw(Cairo::RefPtr<Cairo::Context> const &cr, int, int) = 0;

    /// All event controllers are connected to the DrawingArea.
    virtual Gtk::EventSequenceState on_click_pressed (Gtk::GestureClick const& controller, 
                                                      int n_press, double x, double y) = 0;
    virtual Gtk::EventSequenceState on_click_released(int n_press, double x, double y) = 0;

    virtual void on_motion(Gtk::EventControllerMotion const &motion, double x, double y) = 0;
    void _on_motion(Gtk::EventControllerMotion const &motion, double x, double y);

    virtual bool on_key_pressed(unsigned keyval, unsigned keycode, Gdk::ModifierType state)
    {
        return false;
    }

    void on_key_released(unsigned keyval, unsigned keycode, Gdk::ModifierType state);
};

/**
 * @class ColorWheelHSL
 */
class ColorWheelHSL
    : public WidgetVfuncsClassInit // As Gtkmm4 doesn't wrap focus_vfunc
    , public ColorWheelBase
{
public:
    ColorWheelHSL();
    ColorWheelHSL(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
    bool setColor(Colors::Color const &color,
                bool overrideHue = true, bool emit = true) override;

private:
    void on_drawing_area_size(int width, int height, int baseline) override;
    void on_drawing_area_draw(Cairo::RefPtr<Cairo::Context> const &cr, int, int) override;
    std::optional<bool> focus(Gtk::DirectionType direction) override;

    bool _set_from_xy(double x, double y);
    bool set_from_xy_delta(double dx, double dy);
    bool _is_in_ring(double x, double y);
    bool _is_in_triangle(double x, double y);
    void _update_ring_color(double x, double y);

    enum class DragMode {
        NONE,
        HUE,
        SATURATION_VALUE
    };

    DragMode _mode = DragMode::NONE;
    bool _focus_on_ring = true;

    Gtk::EventSequenceState on_click_pressed (Gtk::GestureClick const& controller,
                                              int n_press, double x, double y) final;
    Gtk::EventSequenceState on_click_released(int n_press, double x, double y) final;
    void on_motion(Gtk::EventControllerMotion const &motion, double x, double y) final;
    bool on_key_pressed(unsigned keyval, unsigned keycode, Gdk::ModifierType state) final;

    // caches to speed up drawing
    using TriangleCorners = std::array<ColorPoint, 3>;
    using MinMax          = std::array<double    , 2>;
    std::optional<Geom::IntPoint> _cache_size;
    std::optional<MinMax             > _radii;
    std::optional<TriangleCorners    > _triangle_corners;
    std::optional<Geom::Point        > _marker_point;
    std::vector  <guint32            > _buffer_ring, _buffer_triangle;
    Cairo::RefPtr<Cairo::ImageSurface> _source_ring, _source_triangle;
    [[nodiscard]] MinMax          const &get_radii           ();
    [[nodiscard]] TriangleCorners const &get_triangle_corners();
    [[nodiscard]] Geom::Point     const &get_marker_point    ();
                  void                   update_ring_source    ();
    [[nodiscard]] TriangleCorners        update_triangle_source();
};

/**
 * Used to represent the in RGB gamut colors polygon of the HSLuv color wheel.
 */
struct PickerGeometry {
    std::vector<Geom::Point> vertices; ///< Vertices, in counter-clockwise order.
    double outer_circle_radius; ///< Smallest circle with center at origin such that polygon fits inside.
    double inner_circle_radius; ///< Largest circle with center at origin such that it fits inside polygon.
};


/**
 * @class ColorWheelHSLuv
 */
class ColorWheelHSLuv : public ColorWheelBase
{
public:
    ColorWheelHSLuv();
    ColorWheelHSLuv(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);

    /// See base doc & N.B. that overrideHue is unused by this class
    bool setColor(Colors::Color const &color,
                bool overrideHue = true, bool emit = true) override;

    void updateGeometry();

private:
    void on_drawing_area_draw(Cairo::RefPtr<Cairo::Context> const &cr, int, int) final;

    bool _set_from_xy(double const x, double const y);
    void _setFromPoint(Geom::Point const &pt) { _set_from_xy(pt[Geom::X], pt[Geom::Y]); }
    void _updatePolygon();
    bool _vertex() const;

    Gtk::EventSequenceState on_click_pressed (Gtk::GestureClick const& controller,
                                              int n_press, double x, double y) final;
    Gtk::EventSequenceState on_click_released(int n_press, double x, double y) final;
    void on_motion(Gtk::EventControllerMotion const &motion, double x, double y) final;
    bool on_key_pressed(unsigned keyval, unsigned keycode, Gdk::ModifierType state) final;

    double _scale = 1.0;
    std::unique_ptr<PickerGeometry> _picker_geometry;
    std::vector<guint32> _buffer_polygon;
    Cairo::RefPtr<::Cairo::ImageSurface> _surface_polygon;
    Geom::IntPoint _cache_size;
    int _square_size = 1;
};

/**
 * @class MultiMarkerWheel
 */
class MultiMarkerWheel
    : public WidgetVfuncsClassInit
    , public ColorWheelBase
{
public:
    MultiMarkerWheel();
    MultiMarkerWheel(BaseObjectType *cobject, Glib::RefPtr<Gtk::Builder> const &builder);
    bool setColor(Colors::Color const &color, bool overrideHue = true, bool emit = true) override;
    void setColors(std::vector<Colors::Color> colors);
    Colors::Color getColor() const override
    {
        if (!_values_vector.empty() && _active_index >= 0 && _active_index < _values_vector.size())
            return _values_vector[_active_index];
        else
          return Colors::Color(0x00000000);
    }
    bool setActiveIndex(int index)
    {
        if (!_values_vector.empty() && index >= 0 && index < _values_vector.size()) {
            _active_index = index;
            return true;
        } else
            return false;
    }
    int getActiveIndex()
    {
        if (!_values_vector.empty() && _active_index >= 0 && _active_index < _values_vector.size())
            return _active_index;
        else
            return -1;
    }
    int getHoverIndex()
    {
        if (!_values_vector.empty() && _hover_index >= 0 && _hover_index < _values_vector.size())
            return _hover_index;
        else
            return -1;
    }
    bool changeColor(int index, Colors::Color const &color);
    sigc::connection connect_color_hovered(sigc::slot<void ()> slot) { return _signal_color_hovered.connect(std::move(slot)); }
    void toggleHueLock(bool locked){_hue_lock = locked ;}
    bool getHueLock(){return _hue_lock;}
    std::vector<Colors::Color> const &getColors() const { return _values_vector; }
    void setLightness(double value);
    void setSaturation(double value);
    void redrawOnHueLocked(){queue_drawing_area_draw();}

private:
    void on_drawing_area_size(int width, int height, int baseline) override;
    void on_drawing_area_draw(Cairo::RefPtr<Cairo::Context> const &cr, int, int) override;
    std::optional<bool> focus(Gtk::DirectionType direction) override;
    bool _is_in_wheel(double x, double y);
    void _update_wheel_color(double x, double y, int index);
    void _draw_line_to_marker(Cairo::RefPtr<Cairo::Context> const &cr, double mx, double my, double cx, double cy, Colors::Color const &value, int index);
    void _draw_marker(Cairo::RefPtr<Cairo::Context> const &cr, Colors::Color const &value, int index);
    int _get_marker_index(Geom::Point const &p);
    void _update_hue_lock_positions();

    enum class DragMode
    {
        NONE,
        HUE,
        SATURATION_VALUE
    };

    static constexpr double _wheel_width = 1.0;
    DragMode _mode = DragMode::NONE;
    bool _focus_on_wheel = true;

    Gtk::EventSequenceState on_click_pressed(Gtk::GestureClick const &controller, int n_press, double x,
                                             double y) final;
    Gtk::EventSequenceState on_click_released(int n_press, double x, double y) final;
    void on_motion(Gtk::EventControllerMotion const &motion, double x, double y) final;
    bool on_key_pressed(unsigned keyval, unsigned keycode, Gdk::ModifierType state) final;

    // caches to speed up drawing
    using MinMax = std::array<double, 2>;
    std::vector<Colors::Color> _values_vector;
    std::optional<Geom::IntPoint> _cache_size;
    std::optional<MinMax> _radii;
    std::optional<Geom::Point> _marker_point;
    std::vector<std::optional<Geom::Point>> _markers_points;
    std::vector<guint32> _buffer_wheel;
    Cairo::RefPtr<Cairo::ImageSurface> _source_wheel;
    MinMax const &get_radii();
    Geom::Point get_marker_point(int index);
    int _active_index = 0;
    int _hover_index = -1;
    bool _hue_lock = false;
    std::vector<double>_relative_hue_angles;
    static constexpr double marker_click_tolerance = 5.0;
    sigc::signal<void ()> _signal_color_hovered;
    double lightness = 1.0;
    double saturation = 1.0;
    void update_wheel_source();
};

} // namespace Inkscape::UI::Widget

#endif // INK_COLORWHEEL_HSLUV_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim:filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8: textwidth=99:

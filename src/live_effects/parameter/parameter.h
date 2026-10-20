// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::LivePathEffectParameters
 *
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_LIVEPATHEFFECT_PARAMETER_H
#define INKSCAPE_LIVEPATHEFFECT_PARAMETER_H

#include <vector>
#include <2geom/forward.h>
#include <2geom/pathvector.h>
#include <glibmm/ustring.h>

#include <sigc++/scoped_connection.h>
#include "live_effects/lpeobject.h"
#include "ui/widget/registered-widget.h"

// In gtk2, this wasn't an issue; we could toss around
// G_MAXDOUBLE and not worry about size allocations. But
// in gtk3, it is an issue: it allocates widget size for the maxmium
// value you pass to it, leading to some insane lengths.
// If you need this to be more, please be conservative about it.
inline constexpr double SCALARPARAM_G_MAXDOUBLE =
    10000000000.0; // TODO fixme: using an arbitrary large number as a magic value seems fragile.

class KnotHolder;
class SPLPEItem;
class SPDesktop;
class SPItem;

namespace Gtk {
class Widget;
} // namespace Gtk

namespace Inkscape {

namespace Display {
class TemporaryItem;
} // namespace Display

namespace NodePath {
class Path;
} // namespace NodePath

namespace UI::Widget {
class Registry;
} // namespace UI::Widget

namespace LivePathEffect {

class Effect;

class Parameter {
public:
    Parameter(Glib::ustring label, Glib::ustring tip, Glib::ustring key, Inkscape::UI::Widget::Registry *wr,
              Effect *effect);
    virtual ~Parameter();

    Parameter(const Parameter &) = delete;
    Parameter &operator=(const Parameter &) = delete;

    virtual bool param_readSVGValue(char const *strvalue) = 0; // returns true if new value is valid / accepted.
    virtual Glib::ustring param_getSVGValue() const = 0;
    virtual Glib::ustring param_getDefaultSVGValue() const = 0;
    virtual void param_widget_is_visible(bool is_visible) { widget_is_visible = is_visible; }
    virtual void param_widget_is_enabled(bool is_enabled) { widget_is_enabled = is_enabled; }
    void write_to_SVG();
    void read_from_SVG();
    void setUpdating(bool updating) { _updating = updating; }
    bool getUpdating() const { return _updating; }
    virtual void param_set_default() = 0;
    virtual void param_update_default(char const *default_value) = 0;
    // This creates a new, Gtk::manage()d widget
    virtual Gtk::Widget *param_newWidget() = 0;

    // FIXME: Should return a const reference, then callers need not check for nullptr.
    Glib::ustring const *param_getTooltip() const { return &param_tooltip; };

    // overload these for your particular parameter to make it provide knotholder handles or canvas helperpaths
    virtual bool providesKnotHolderEntities() const { return false; }
    virtual void addKnotHolderEntities(KnotHolder * /*knotholder*/, SPItem * /*item*/){};
    virtual void addCanvasIndicators(SPLPEItem const * /*lpeitem*/, std::vector<Geom::PathVector> & /*hp_vec*/){};

    virtual void param_editOncanvas(SPItem * /*item*/, SPDesktop * /*dt*/){};
    virtual void param_setup_nodepath(Inkscape::NodePath::Path * /*np*/){};

    virtual void param_transform_multiply(Geom::Affine const & /*postmul*/, bool set){};
    virtual std::vector<SPObject *> param_get_satellites();
    void param_higlight(bool highlight);
    sigc::scoped_connection selection_changed_connection;
    void change_selection(Inkscape::Selection *selection);
    void update_satellites();
    Glib::ustring param_key;
    Glib::ustring param_tooltip;
    Inkscape::UI::Widget::Registry *param_wr;
    Glib::ustring param_label;
    EffectType effectType() const;
    // force all LPE params has overrided method
    virtual ParamType paramType() const = 0;
    bool oncanvas_editable;
    bool widget_is_visible;
    bool widget_is_enabled;
    void connect_selection_changed();

protected:
    bool _updating = false;
    Inkscape::Display::TemporaryItem *ownerlocator = nullptr;
    Effect *param_effect;
    void param_write_to_repr(const char *svgd);
};


class ScalarParam : public Parameter {
  public:
    ScalarParam(const Glib::ustring &label, const Glib::ustring &tip, const Glib::ustring &key,
                Inkscape::UI::Widget::Registry *wr, Effect *effect, double default_value = 1.0);

    ScalarParam(const ScalarParam &) = delete;
    ScalarParam &operator=(const ScalarParam &) = delete;

    bool param_readSVGValue(char const *strvalue) override;
    Glib::ustring param_getSVGValue() const override;
    Glib::ustring param_getDefaultSVGValue() const override;
    void param_transform_multiply(Geom::Affine const &postmul, bool set) override;

    void param_set_default() override;
    void param_update_default(double default_value);
    void param_update_default(char const *default_value) override;
    void param_set_value(double val);
    void param_make_integer(bool yes = true);
    void param_set_range(double min, double max);
    void param_set_digits(unsigned digits);
    void param_set_increments(double step, double page);
    void param_set_no_leading_zeros();
    void param_set_width_chars(int width_chars);
    void addSlider(bool add_slider_widget) { add_slider = add_slider_widget; };
    double param_get_max() { return max; };
    double param_get_min() { return min; };
    void param_set_undo(bool set_undo);
    Gtk::Widget *param_newWidget() override;
    ParamType paramType() const override { return ParamType::SCALAR; };
    inline operator double() const { return value; };

protected:
    double value;
    double min;
    double max;
    bool integer;
    double defvalue;
    unsigned digits;
    double inc_step;
    double inc_page;
    bool add_slider;
    bool _set_undo;
    bool _no_leading_zeros;
    int _width_chars;
};

} // namespace LivePathEffect

} // namespace Inkscape

#endif // INKSCAPE_LIVEPATHEFFECT_PARAMETER_H

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

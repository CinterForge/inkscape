// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::LivePathEffectParameters
 *
 * Copyright (C) Johan Engelen 2008 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_LIVEPATHEFFECT_PARAMETER_SCALARARRAY_H
#define INKSCAPE_LIVEPATHEFFECT_PARAMETER_SCALARARRAY_H

#include "live_effects/lpeobject.h"
#include "live_effects/effect.h"
#include "live_effects/parameter/array.h"
#include "live_effects/parameter/parameter.h"

namespace Inkscape::LivePathEffect {

class ScalarArrayParam : public ArrayParam<double> {
public:
    ScalarArrayParam(Glib::ustring const &label,
                     Glib::ustring const &tip,
                     Glib::ustring const &key,
                     Inkscape::UI::Widget::Registry *wr,
                     Effect *effect,
                     double default_value = 0.0,
                     bool visible = true,
                     size_t n = 0);

    ScalarArrayParam(const ScalarArrayParam &) = delete;
    ScalarArrayParam &operator=(const ScalarArrayParam &) = delete;

    Gtk::Widget *param_newWidget() override;

    void param_setActive(size_t index) {
        _active_index = index;
        param_effect->refresh_widgets = true;
    }

    Glib::ustring param_getDefaultSVGValue() const override;
    void param_set_default() override;
    void param_update_default(double default_value);
    void param_update_default(char const *default_value) override;
    void param_make_integer(bool yes = true);
    void param_set_range(double min, double max);
    void param_set_digits(unsigned digits);
    void param_set_increments(double step, double page);

    void addSlider(bool add_slider_widget) { add_slider = add_slider_widget; };
    void on_value_changed(Inkscape::UI::Widget::RegisteredScalar *rsu);

    double param_get_min() const { return min; };
    double param_get_max() const { return max; };
    void param_set_undo(bool set_undo);

    ParamType paramType() const override { return ParamType::SCALAR_ARRAY; };

private:
    friend class LPETaperStroke;

    size_t _active_index = 0;
    double min;
    double max;
    bool integer;
    unsigned digits;
    double inc_step;
    double inc_page;
    bool add_slider;
    bool _set_undo;
    double defvalue;
};

} // namespace Inkscape::LivePathEffect

#endif // INKSCAPE_LIVEPATHEFFECT_PARAMETER_SCALARARRAY_H

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

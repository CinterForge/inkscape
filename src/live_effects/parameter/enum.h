// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::LivePathEffectParameters
 *
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_LIVEPATHEFFECT_PARAMETER_ENUM_H
#define INKSCAPE_LIVEPATHEFFECT_PARAMETER_ENUM_H

#include <glibmm/ustring.h>

#include "live_effects/effect.h"
#include "live_effects/parameter/parameter.h"
#include "ui/icon-names.h"
#include "ui/widget/registered-enums.h"

namespace Inkscape::LivePathEffect {

template<typename E> class EnumParam : public Parameter {
public:
    EnumParam(  const Glib::ustring& label,
                const Glib::ustring& tip,
                const Glib::ustring& key,
                const Util::EnumDataConverter<E>& c,
                Inkscape::UI::Widget::Registry* wr,
                Effect* effect,
                E default_value,
                bool sort = true)
        : Parameter(label, tip, key, wr, effect)
    {
        enumdataconv = &c;
        defvalue = default_value;
        value = defvalue;
        sorted = sort;
    };

    EnumParam(const EnumParam&) = delete;
    EnumParam& operator=(const EnumParam&) = delete;

    Gtk::Widget * param_newWidget() override {
        auto const regenum = Gtk::make_managed<Inkscape::UI::Widget::RegisteredEnum<E>>(param_label, param_tooltip,
                       param_key, *enumdataconv, *param_wr, param_effect->getRepr(), param_effect->getSPDoc(), sorted);
        regenum->set_active_by_id(value);
        regenum->combobox()->setProgrammatically = false;
        regenum->combobox()->signal_changed().connect(sigc::mem_fun (*this, &EnumParam::_on_change_combo));
        regenum->set_undo_parameters(RC_("Undo", "Change enumeration parameter"), INKSCAPE_ICON("dialog-path-effects"));
        return regenum;
    };

    void _on_change_combo() { param_effect->refresh_widgets = true; }

    bool param_readSVGValue(const gchar * strvalue) override {
        if (!strvalue) {
            param_set_default();
            return true;
        }

        param_set_value( enumdataconv->get_id_from_key(Glib::ustring(strvalue)) );

        return true;
    };
    Glib::ustring param_getSVGValue() const override {
        return enumdataconv->get_key(value);
    };
    
    Glib::ustring param_getDefaultSVGValue() const override {
        return enumdataconv->get_key(defvalue).c_str();
    };
    
    E get_value() const {
        return value;
    }

    inline operator E() const {
        return value;
    };

    void param_set_default() override {
        param_set_value(defvalue);
    }
    
    void param_update_default(E default_value) {
        defvalue = default_value;
    }
    
    void param_update_default(const gchar * default_value) override {
        param_update_default(enumdataconv->get_id_from_key(Glib::ustring(default_value)));
    }
    
    void param_set_value(E val) {
        value = val;
    }
    ParamType paramType() const override { return ParamType::ENUM; };
private:
    E value;
    E defvalue;
    bool sorted;

    const Util::EnumDataConverter<E> * enumdataconv;
};

}; // namespace Inkscape::LivePathEffect

#endif // INKSCAPE_LIVEPATHEFFECT_PARAMETER_ENUM_H

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

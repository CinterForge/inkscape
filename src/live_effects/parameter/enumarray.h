// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::LivePathEffectParameters
 *
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_LIVEPATHEFFECT_PARAMETER_ENUMARRAY_H
#define INKSCAPE_LIVEPATHEFFECT_PARAMETER_ENUMARRAY_H

#include <glib.h>
#include "live_effects/lpeobject.h"
#include "live_effects/effect.h"
#include "live_effects/parameter/array.h"
#include "live_effects/parameter/parameter.h"

namespace Inkscape {

namespace LivePathEffect {
typedef unsigned E;
class EnumArrayParam : public ArrayParam<Glib::ustring> {
public:
    EnumArrayParam( const Glib::ustring& label,
                const Glib::ustring& tip,
                const Glib::ustring& key,
                const Util::EnumDataConverter<E>& c,
                Inkscape::UI::Widget::Registry* wr,
                Effect* effect,
                E default_value,
                bool visible = true,
                size_t n = 0,
                bool sort = true)
    : ArrayParam<Glib::ustring>(label, tip, key, wr, effect, n)
    , defvalue(default_value)
    {
        enumdataconv = &c;

        sorted = sort;
        widget_is_visible = visible;
    }

    ~EnumArrayParam() override = default;

    Gtk::Widget *param_newWidget() override {
        if (widget_is_visible && valid_index(_active_index)) {
            auto const regenum = Gtk::make_managed<UI::Widget::RegisteredEnum<E>>(param_label, param_tooltip,
                        param_key, *enumdataconv, *param_wr, param_effect->getRepr(), param_effect->getSPDoc(), sorted);
            regenum->combobox()->setProgrammatically = true;
            regenum->set_active_by_id(enumdataconv->get_id_from_key(_vector[_active_index]));
            regenum->combobox()->setProgrammatically = true;
            regenum->combobox()->signal_changed().connect(sigc::bind(sigc::mem_fun (*this, &EnumArrayParam::_on_change_combo),regenum));
            regenum->set_undo_parameters(RC_("Undo", "Change enumeration parameter"), INKSCAPE_ICON("dialog-path-effects"));
            regenum->combobox()->setProgrammatically = true;
            return regenum;
        } else {
            return nullptr;
        }
    };

    void _on_change_combo(Inkscape::UI::Widget::RegisteredEnum<E> *regenum) { 
        auto key = regenum->combobox()->get_as_attribute();
        if (key.empty()) return;

        regenum->combobox()->setProgrammatically = true;
        _vector[_active_index] = key;
        param_set_and_write_new_value(_vector);
    }

    void param_setActive(size_t index) {
        _active_index = index;
        param_effect->refresh_widgets = true;
    }

    Glib::ustring param_getDefaultSVGValue() const override {
        return enumdataconv->get_key(defvalue).c_str();
    };

    void param_set_default() override {
        for (auto &vec : _vector) {
            vec = enumdataconv->get_key(defvalue).c_str();
        }
    };

    void param_update_default(E default_value) { 
        defvalue = default_value; 
    };

    void param_update_default(const gchar *default_value) override {
        param_update_default(enumdataconv->get_id_from_key(Glib::ustring(default_value)));
    }

    ParamType paramType() const override { return ParamType::ENUM_ARRAY; };

protected:
    friend class LPETaperStroke;

private:
    size_t _active_index = 0;
    E defvalue;
    bool sorted;
    const Util::EnumDataConverter<E> * enumdataconv;
    EnumArrayParam(const EnumArrayParam &) = delete;
    EnumArrayParam &operator=(const EnumArrayParam &) = delete;
};


} // namespace LivePathEffect

} // namespace Inkscape

#endif // INKSCAPE_LIVEPATHEFFECT_PARAMETER_ENUMARRAY_H

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

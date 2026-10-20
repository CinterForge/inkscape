// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::LivePathEffectParameters
 *
 * Copyright (C) Maximilian Albert 2008 <maximilian.albert@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_LIVEPATHEFFECT_PARAMETER_UNIT_H
#define INKSCAPE_LIVEPATHEFFECT_PARAMETER_UNIT_H

#include "live_effects/parameter/parameter.h"

namespace Inkscape {

namespace Util {
class Unit;
} // namespace Util

namespace LivePathEffect {

class UnitParam : public Parameter {
public:
    UnitParam(const Glib::ustring& label,
              const Glib::ustring& tip,
              const Glib::ustring& key, 
              Inkscape::UI::Widget::Registry* wr,
              Effect* effect,
              Glib::ustring default_unit = "px");
    ~UnitParam() override;

    UnitParam(const UnitParam&) = delete;
    UnitParam& operator=(const UnitParam&) = delete;

    bool param_readSVGValue(const gchar * strvalue) override;
    Glib::ustring param_getSVGValue() const override;
    Glib::ustring param_getDefaultSVGValue() const override;
    void param_set_default() override;
    void param_set_value(const gchar * unit);
    void param_update_default(const gchar * default_unit) override;
    const gchar *get_abbreviation() const;
    Gtk::Widget * param_newWidget() override;
    
    ParamType paramType() const override { return ParamType::UNIT; };

private:
    Glib::ustring defunit;
    std::unique_ptr<Inkscape::Util::Unit const> unit;
};

} // namespace LivePathEffect
} // namespace Inkscape

#endif // INKSCAPE_LIVEPATHEFFECT_PARAMETER_UNIT_H

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

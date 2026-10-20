// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Maximilian Albert 2008 <maximilian.albert@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "unit.h"

#include <glibmm/i18n.h>

#include "live_effects/effect.h"
#include "ui/icon-names.h"
#include "ui/widget/registered-widget.h"
#include "util/units.h"

namespace Inkscape::LivePathEffect {

UnitParam::UnitParam(const Glib::ustring& label, const Glib::ustring& tip,
                     const Glib::ustring& key, Inkscape::UI::Widget::Registry* wr,
                     Effect* effect, Glib::ustring default_unit)
    : Parameter(label, tip, key, wr, effect)
    , defunit{default_unit}
{
    unit = std::make_unique<Inkscape::Util::Unit const>(*Util::UnitTable::get().getUnit(default_unit));
}

UnitParam::~UnitParam() = default;

bool
UnitParam::param_readSVGValue(const gchar * strvalue)
{
    if (strvalue) {
        param_set_value(strvalue);
        return true;
    }
    return false;
}

Glib::ustring
UnitParam::param_getSVGValue() const
{
    return unit.get()->abbr;
}

Glib::ustring
UnitParam::param_getDefaultSVGValue() const
{
    return defunit;
}

void
UnitParam::param_set_default()
{
    param_set_value(defunit.c_str());
}

void 
UnitParam::param_update_default(const gchar * default_unit)
{
    defunit = "px"; // fallback to px
    if (default_unit) {
        defunit = default_unit;
    }
}

void
UnitParam::param_set_value(const gchar * strvalue)
{
    if (strvalue) {
        param_effect->refresh_widgets = true;
        unit = std::make_unique<Inkscape::Util::Unit const>(*Util::UnitTable::get().getUnit(strvalue));
    }
}

const gchar *
UnitParam::get_abbreviation() const
{
    return unit.get()->abbr.c_str();
}

Gtk::Widget *
UnitParam::param_newWidget()
{
    auto const unit_menu = Gtk::make_managed<UI::Widget::RegisteredUnitMenu>( param_label,
                                                                              param_key,
                                                                             *param_wr,
                                                                              param_effect->getRepr(),
                                                                              param_effect->getSPDoc() );

    unit_menu->setUnit(unit.get()->abbr);
    unit_menu->set_undo_parameters(RC_("Undo", "Change unit parameter"), INKSCAPE_ICON("dialog-path-effects"));
    return unit_menu;
}


} // Inkscape::LivePathEffect

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

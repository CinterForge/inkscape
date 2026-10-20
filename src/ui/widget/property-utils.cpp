// SPDX-License-Identifier: GPL-2.0-or-later

#include "property-utils.h"

#include <glib/gi18n.h>
#include <gtkmm/adjustment.h>

#include "generic/spin-button.h"
#include "ui/util.h"

namespace Inkscape::UI::Utils {

void init_property_button(Gtk::Button& button, PropertyButton type, const Glib::ustring& tooltip) {
    const char* tip = "";
    const char* icon = "";

    switch (type) {
    case Reset:
        icon = _("reset-settings");
        tip = _("Clear property");
        break;
    case Edit:
        icon = _("edit");
        tip = _("Edit property");
        break;
    case Add:
        icon = _("plus");
        tip = _("Define property");
        break;
    case Remove:
        icon = _("minus");
        tip = _("Remove property");
        break;
    }

    button.set_icon_name(icon);
    button.set_tooltip_text(tooltip.empty() ? tip : tooltip);
    button.set_has_frame(false);
    button.set_halign(Gtk::Align::START);
    button.set_valign(Gtk::Align::CENTER);
}

void init_spin_button(const SpinPropertyDef& def) {
    auto& button = *def.button;
    auto& adj = def.adjustment;
    button.set_adjustment(Gtk::Adjustment::create(0, adj.min, adj.max, adj.inc, adj.page_inc));
    button.set_digits(adj.digits);
    button.set_scaling_factor(adj.scale);
    if (def.label) button.set_label(def.label);
    if (def.tooltip) button.set_tooltip_text(def.tooltip);
    switch (def.unit) {
        case Degree:
            set_degree_suffix(button);
            break;
        case Percent:
            set_percent_suffix(button);
            break;
        default:
            break;
    }

    if (def.reset) {
        init_property_button(*def.reset, Reset);
    }
}

} // namespace

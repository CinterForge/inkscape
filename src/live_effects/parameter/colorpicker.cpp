// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "colorpicker.h"

#include <glibmm/i18n.h>
#include <gtkmm/box.h>

#include "document-undo.h"                       // for DocumentUndo
#include "live_effects/effect.h"                 // for Effect
#include "live_effects/parameter/colorpicker.h"  // for ColorPickerParam
#include "ui/icon-names.h"                       // for INKSCAPE_ICON
#include "ui/pack.h"                             // for pack_start
#include "ui/widget/registered-widget.h"         // for RegisteredColorPicker
#include "util/safe-printf.h"                    // for safeprintf

class SPDocument;

namespace Inkscape {
namespace LivePathEffect {

ColorPickerParam::ColorPickerParam( const Glib::ustring& label, const Glib::ustring& tip,
                      const Glib::ustring& key, Inkscape::UI::Widget::Registry* wr,
                      Effect* effect, std::optional<Colors::Color> default_color)
    : Parameter(label, tip, key, wr, effect),
      value(default_color),
      defvalue(default_color)
{
}

void
ColorPickerParam::param_set_default()
{
    param_setValue(defvalue);
}

void 
ColorPickerParam::param_update_default(const gchar * default_value)
{
    defvalue->set(default_value ? default_value : "");
}

bool
ColorPickerParam::param_readSVGValue(const gchar *val)
{
    param_setValue(Colors::Color::parse(val));
    return true;
}

Glib::ustring
ColorPickerParam::param_getSVGValue() const
{
    return value->toString();
}

Glib::ustring
ColorPickerParam::param_getDefaultSVGValue() const
{
    return defvalue->toString();
}

Gtk::Widget *
ColorPickerParam::param_newWidget()
{
    auto const hbox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 2);
    hbox->set_margin(5);

    auto const colorpickerwdg = Gtk::make_managed<UI::Widget::RegisteredColorPicker>( param_label,
                                                                                      param_label,
                                                                                      param_tooltip,
                                                                                      param_key,
                                                                                      param_key + "_opacity_LPE",
                                                                                     *param_wr,
                                                                                      param_effect->getRepr(),
                                                                                      param_effect->getSPDoc() );

    {
        SPDocument *document = param_effect->getSPDoc();
        DocumentUndo::ScopedInsensitive _no_undo(document);
        colorpickerwdg->setColor(*value);
    }

    colorpickerwdg->set_undo_parameters(RC_("Undo", "Change color button parameter"), INKSCAPE_ICON("dialog-path-effects"));

    UI::pack_start(*hbox, *colorpickerwdg, true, true);
    return hbox;
}

void
ColorPickerParam::param_setValue(std::optional<Colors::Color> newvalue)
{
    value = newvalue;
}

} /* namespace LivePathEffect */
} /* namespace Inkscape */

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

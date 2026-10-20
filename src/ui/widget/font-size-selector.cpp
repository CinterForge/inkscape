// SPDX-License-Identifier: GPL-2.0-or-later

#include <glibmm/i18n.h>

#include "font-size-selector.h"
#include "style.h"
#include "svg/css-ostringstream.h"

namespace Inkscape::UI::Widget {

#define INIT_PROPERTIES \
    , _unit("/options/font/unitType", SP_CSS_UNIT_PT) \
    , _max_size("/dialogs/textandfont/maxFontSize", 10000)

FontSizeSelector::FontSizeSelector()
    : Glib::ObjectBase("FontSizeSelector")
    INIT_PROPERTIES
{
    _construct();
}

FontSizeSelector::FontSizeSelector(GtkBox *cobject, const Glib::RefPtr<Gtk::Builder>& builder)
    : Glib::ObjectBase("FontSizeSelector")
    , BuildableWidget(cobject, builder)
    INIT_PROPERTIES
{
    _construct();
}

void FontSizeSelector::_construct()
{
    set_orientation(Gtk::Orientation::HORIZONTAL);
    set_spacing(4);

    // Set up size combo
    _size_combo = Gtk::make_managed<UI::Widget::NumberComboBox>();
    append(*_size_combo);

    // Ranges for size
    auto& entry = _size_combo->get_entry();
    entry.set_min_size("9999");
    entry.set_digits(3);
    entry.set_range(0.001, _max_size);

    // Set up unit combo
    _tracker = std::make_unique<UnitTracker>(Util::UNIT_TYPE_LINEAR);
    _unit_combo = _tracker->create_unit_dropdown();
    append(*_unit_combo);

    // Now do initial values and track future changes
    _updateUnit();
    _unit.action = [this] { _updateUnit(); };
    _max_size.action = [this] { _size_combo->get_entry().set_range(0.001, _max_size); };
    _size_combo->signal_value_changed().connect(sigc::mem_fun(*this, &FontSizeSelector::_sizeChanged));
    _unit_combo->signal_changed().connect(sigc::mem_fun(*this, &FontSizeSelector::_unitChanged));
}

double FontSizeSelector::getSize() const
{
    return _size_combo->get_entry().get_value();
}

void FontSizeSelector::setSize(double size)
{
    _size_combo->get_entry().set_value(size);
}

int FontSizeSelector::getUnit() const
{
    return _unit;
}

void FontSizeSelector::setPopupPosition(Gtk::PositionType pos)
{
    _size_combo->set_popup_position(pos);
}

void FontSizeSelector::_sizeChanged(double size)
{
    _signal_size_changed.emit(size, _unit);
}

void FontSizeSelector::_unitChanged()
{
    auto menu_unit = _tracker->getActiveUnit();

    // This nonsense is to get SP_CSS_UNIT_xx value corresponding to unit.
    SPILength temp_size;
    CSSOStringStream temp_size_stream;
    temp_size_stream << 1 << menu_unit->abbr;
    temp_size.read(temp_size_stream.str().c_str());
    Preferences::get()->setInt("/options/font/unitType", temp_size.unit);
}

void FontSizeSelector::_updateUnit()
{
    if (_previous_unit && _unit == *_previous_unit) {
        return;
    }

    auto unit_str = sp_style_get_css_unit_string(_unit);
    _tracker->setActiveUnitByAbbr(unit_str);

    auto tooltip = Glib::ustring::format(_("Font size"), " (", unit_str, ")");
    _size_combo->set_tooltip_text(tooltip);
    _size_combo->set_menu_options(sp_style_get_default_font_size_list(_unit));

    // Convert size value
    if (_previous_unit) {
        auto px_size = sp_style_css_size_units_to_px(getSize(), *_previous_unit);
        auto new_size = sp_style_css_size_px_to_units(px_size, _unit);
        new_size = sp_style_css_size_round_for_user_display(new_size);
        setSize(new_size);
    }

    _previous_unit = _unit;
}

} // namespace Inkscape::UI::Widget

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :

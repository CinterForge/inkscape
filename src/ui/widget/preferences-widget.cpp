// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Widgets for Inkscape Preferences dialog.
 */
/*
 * Authors:
 *   Marco Scholten
 *   Bruno Dilly <bruno.dilly@gmail.com>
 *
 * Copyright (C) 2004, 2006, 2007 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "ui/widget/preferences-widget.h"

#include <glibmm/i18n.h>

#include "desktop.h"
#include "include/gtkmm_version.h"
#include "inkscape-window.h"
#include "inkscape.h"
#include "preferences.h"
#include "ui/dialog/choose-file-utils.h"
#include "ui/dialog/choose-file.h"
#include "ui/icon-loader.h"
#include "ui/pack.h"
#include "ui/util.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace Inkscape::UI::Widget {

DialogPage::DialogPage()
{
    set_margin(12);

    set_orientation(Gtk::Orientation::VERTICAL);
    set_column_spacing(12);
    set_row_spacing(6);
}

/**
 * Add a widget to the bottom row of the dialog page
 *
 * \param[in] indent         Whether the widget should be indented by one column
 * \param[in] label          The label text for the widget
 * \param[in] widget         The widget to add to the page
 * \param[in] suffix         Text for an optional label at the right of the widget
 * \param[in] tip            Tooltip text for the widget
 * \param[in] expand_widget  Whether to expand the widget horizontally
 * \param[in] other_widget   An optional additional widget to display at the right of the first one
 */
int DialogPage::add_line(bool                 indent,
                         Glib::ustring const &label,
                         Gtk::Widget         &widget,
                         Glib::ustring const &suffix,
                         const Glib::ustring &tip,
                         bool                 expand_widget,
                         Gtk::Widget         *other_widget)
{
    if (!tip.empty())
        widget.set_tooltip_text(tip);
    
    auto const hb = Gtk::make_managed<Gtk::Box>();
    hb->set_spacing(12);
    hb->set_hexpand(true);
    UI::pack_start(*hb, widget, expand_widget, expand_widget);
    hb->set_valign(Gtk::Align::CENTER);
    
    // Add a label in the first column if provided
    if (!label.empty()) {
        auto const label_widget = Gtk::make_managed<Gtk::Label>(label, Gtk::Align::START,
                                                                Gtk::Align::CENTER, true);
        label_widget->set_mnemonic_widget(widget);
        label_widget->set_markup(label_widget->get_text());
        
        if (indent) {
            label_widget->set_margin_start(12);
        }

        label_widget->set_valign(Gtk::Align::CENTER);
        attach_next_to(*label_widget, Gtk::PositionType::BOTTOM);

        attach_next_to(*hb, *label_widget, Gtk::PositionType::RIGHT, 1, 1);
    } else {
        if (indent) {
            hb->set_margin_start(12);
        }

        attach_next_to(*hb, Gtk::PositionType::BOTTOM, 2, 1);
    }

    // Add a label on the right of the widget if desired
    if (!suffix.empty()) {
        auto const suffix_widget = Gtk::make_managed<Gtk::Label>(suffix, Gtk::Align::START, Gtk::Align::CENTER, true);
        suffix_widget->set_markup(suffix_widget->get_text());
        UI::pack_start(*hb, *suffix_widget,false,false);
    }

    // Pack an additional widget into a box with the widget if desired
    if (other_widget)
        UI::pack_start(*hb, *other_widget, expand_widget, expand_widget);

    // Query what row we are at, so callers can reference this label & widget again later
    int col, row, width, height;
    query_child(*hb, col, row, width, height);
    return row;
}

void DialogPage::add_group_header(Glib::ustring name, int columns)
{
    if (name.empty()) return;

    auto const label_widget = Gtk::make_managed<Gtk::Label>(Glib::ustring("<b>").append(name).append("</b>"),
                                                            Gtk::Align::START, Gtk::Align::CENTER, true);
    
    label_widget->set_use_markup(true);
    label_widget->set_valign(Gtk::Align::CENTER);
    attach_next_to(*label_widget, Gtk::PositionType::BOTTOM, columns, 1);
}

void DialogPage::add_group_note(Glib::ustring name)
{
    if (name.empty()) return;

    auto const label_widget = Gtk::make_managed<Gtk::Label>(Glib::ustring("<i>").append(name).append("</i>"),
                                                            Gtk::Align::START , Gtk::Align::CENTER, true);
    label_widget->set_use_markup(true);
    label_widget->set_valign(Gtk::Align::CENTER);
    label_widget->set_wrap(true);
    label_widget->set_wrap_mode(Pango::WrapMode::WORD);
    attach_next_to(*label_widget, Gtk::PositionType::BOTTOM, 2, 1);
}

void DialogPage::set_tip(Gtk::Widget& widget, Glib::ustring const &tip)
{
    widget.set_tooltip_text (tip);
}

void PrefCheckButton::init(Glib::ustring const &label, Glib::ustring const &prefs_path,
    bool default_value, bool invert_ui)
{
    _prefs_path = prefs_path;
    _invert_ui = invert_ui;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (!label.empty())
        this->set_label(label);
    auto value = prefs->getBool(_prefs_path, default_value) ^ _invert_ui;
    this->set_active(value);
}

void PrefCheckButton::on_toggled()
{
    if (this->get_visible()) //only take action if the user toggled it
    {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setBool(_prefs_path, this->get_active() ^ _invert_ui);
    }
    this->changed_signal.emit(this->get_active());
}

void PrefRadioButton::init(Glib::ustring const &label, Glib::ustring const &prefs_path,
    Glib::ustring const &string_value, bool default_value, PrefRadioButton* group_member)
{
    _prefs_path = prefs_path;
    _value_type = VAL_STRING;
    _string_value = string_value;
    (void)default_value;
    this->set_label(label);

    if (group_member)
    {
        this->set_group(*group_member);
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Glib::ustring val = prefs->getString(_prefs_path);
    if ( !val.empty() )
        this->set_active(val == _string_value);
    else
        this->set_active( false );
}

void PrefRadioButton::init(Glib::ustring const &label, Glib::ustring const &prefs_path,
    int int_value, bool default_value, PrefRadioButton* group_member)
{
    _prefs_path = prefs_path;
    _value_type = VAL_INT;
    _int_value = int_value;
    this->set_label(label);

    if (group_member)
    {
        this->set_group(*group_member);
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (default_value)
        this->set_active( prefs->getInt(_prefs_path, int_value) == _int_value );
    else
        this->set_active( prefs->getInt(_prefs_path, int_value + 1) == _int_value );
}

void PrefRadioButton::on_toggled()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    if (this->get_visible() && this->get_active() ) //only take action if toggled by user (to active)
    {
        if ( _value_type == VAL_STRING )
            prefs->setString(_prefs_path, _string_value);
        else if ( _value_type == VAL_INT )
            prefs->setInt(_prefs_path, _int_value);
    }
    this->changed_signal.emit(this->get_active());
}

PrefRadioButtons::PrefRadioButtons(const std::vector<PrefItem>& buttons, const Glib::ustring& prefs_path) {
    set_spacing(2);

    PrefRadioButton* group = nullptr;
    for (auto&& item : buttons) {
        auto* btn = Gtk::make_managed<PrefRadioButton>();
        btn->init(item.label, prefs_path, item.int_value, item.is_default, group);
        btn->set_tooltip_text(item.tooltip);
        append(*btn);
        if (!group) group = btn;
    }
}

void PrefSpinButton::init(Glib::ustring const &prefs_path,
              double lower, double upper, double step_increment, double /*page_increment*/,
              double default_value, bool is_int, bool is_percent)
{
    _prefs_path = prefs_path;
    _is_int = is_int;
    _is_percent = is_percent;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    double value;
    if (is_int) {
        if (is_percent) {
            value = 100 * prefs->getDoubleLimited(prefs_path, default_value, lower/100.0, upper/100.0);
        } else {
            value = (double) prefs->getIntLimited(prefs_path, (int) default_value, (int) lower, (int) upper);
        }
    } else {
        value = prefs->getDoubleLimited(prefs_path, default_value, lower, upper);
    }

    this->set_range (lower, upper);
    this->set_increments (step_increment, 0);
    this->set_value (value);
    this->set_width_chars(6);
    if (is_int)
        this->set_digits(0);
    else if (step_increment < 0.1)
        this->set_digits(4);
    else
        this->set_digits(2);

    signal_value_changed().connect(sigc::mem_fun(*this, &PrefSpinButton::on_value_changed));
}

void PrefSpinButton::on_value_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (this->get_visible()) //only take action if user changed value
    {
        if (_is_int) {
            if (_is_percent) {
                prefs->setDouble(_prefs_path, this->get_value()/100.0);
            } else {
                prefs->setInt(_prefs_path, (int) this->get_value());
            }
        } else {
            prefs->setDouble(_prefs_path, this->get_value());
        }
    }
    this->changed_signal.emit(this->get_value());
}

void PrefSpinUnit::init(Glib::ustring const &prefs_path,
              double lower, double upper, double step_increment,
              double default_value, UnitType unit_type, Glib::ustring const &default_unit)
{
    _prefs_path = prefs_path;
    _is_percent = (unit_type == UNIT_TYPE_DIMENSIONLESS);

    resetUnitType(unit_type);
    setUnit(default_unit);
    setRange (lower, upper); /// @fixme  this disregards changes of units
    setIncrements (step_increment, 0);
    if (step_increment < 0.1) {
        setDigits(4);
    } else {
        setDigits(2);
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    double value = prefs->getDoubleLimited(prefs_path, default_value, lower, upper);
    Glib::ustring unitstr = prefs->getUnit(prefs_path);
    if (unitstr.length() == 0) {
        unitstr = default_unit;
        // write the assumed unit to preferences:
        prefs->setDoubleUnit(_prefs_path, value, unitstr);
    }
    setValue(value, unitstr);

    signal_value_changed().connect(sigc::mem_fun(*this, &PrefSpinUnit::on_my_value_changed));
}

void PrefSpinUnit::on_my_value_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (getWidget()->get_visible()) //only take action if user changed value
    {
        prefs->setDoubleUnit(_prefs_path, getValue(getUnit()->abbr), getUnit()->abbr);
    }
}

const double ZoomCorrRuler::textsize = 7;
const double ZoomCorrRuler::textpadding = 5;

ZoomCorrRuler::ZoomCorrRuler(int width, int height) :
    _unitconv(1.0),
    _border(5)
{
    set_size(width, height);

    set_draw_func(sigc::mem_fun(*this, &ZoomCorrRuler::on_draw));
}

void ZoomCorrRuler::set_size(int x, int y)
{
    _min_width = x;
    _height = y;
    set_size_request(x + _border*2, y + _border*2);
}

// The following two functions are borrowed from 2geom's toy-framework-2; if they are useful in
// other locations, we should perhaps make them (or adapted versions of them) publicly available
static void
draw_text(cairo_t *cr, Geom::Point loc, const char* txt, bool bottom = false,
          double fontsize = ZoomCorrRuler::textsize, std::string fontdesc = "Sans") {
    PangoLayout* layout = pango_cairo_create_layout (cr);
    pango_layout_set_text(layout, txt, -1);

    // set font and size
    std::ostringstream sizestr;
    sizestr << fontsize;
    fontdesc = fontdesc + " " + sizestr.str();
    PangoFontDescription *font_desc = pango_font_description_from_string(fontdesc.c_str());
    pango_layout_set_font_description(layout, font_desc);
    pango_font_description_free (font_desc);

    PangoRectangle logical_extent;
    pango_layout_get_pixel_extents(layout, nullptr, &logical_extent);
    cairo_move_to(cr, loc[Geom::X], loc[Geom::Y] - (bottom ? logical_extent.height : 0));
    pango_cairo_show_layout(cr, layout);
}

static void
draw_number(cairo_t *cr, Geom::Point pos, double num) {
    std::ostringstream number;
    number << num;
    draw_text(cr, pos, number.str().c_str(), true);
}

/*
 * \arg dist The distance between consecutive minor marks
 * \arg major_interval Number of marks after which to draw a major mark
 */
void
ZoomCorrRuler::draw_marks(Cairo::RefPtr<Cairo::Context> const &cr,
                          double const dist, int const major_interval)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    const double zoomcorr = prefs->getDouble("/options/zoomcorrection/value", 1.0);
    double mark = 0;
    int i = 0;
    double step = dist * zoomcorr / _unitconv;
    bool draw_minor = true;
    if (step <= 0) {
        return;
    }
    else if (step < 2) {
        // marks too dense
        draw_minor = false;
    }
    int last_pos = -1;
    while (mark <= _drawing_width) {
        cr->move_to(mark, _height);
        if ((i % major_interval) == 0) {
            // don't overcrowd the marks
            if (static_cast<int>(mark) > last_pos) {
                // major mark
                cr->line_to(mark, 0);
                Geom::Point textpos(mark + 3, ZoomCorrRuler::textsize + ZoomCorrRuler::textpadding);
                draw_number(cr->cobj(), textpos, dist * i);

                last_pos = static_cast<int>(mark) + 1;
            }
        } else if (draw_minor) {
            // minor mark
            cr->line_to(mark, ZoomCorrRuler::textsize + 2 * ZoomCorrRuler::textpadding);
        }
        mark += step;
        ++i;
    }
}

void
ZoomCorrRuler::on_draw(Cairo::RefPtr<Cairo::Context> const &cr, int const width, int const height)
{
    auto const &w = width;
    _drawing_width = w - _border * 2;

    auto const fg = get_color();
    cr->set_line_width(1);
    cr->set_source_rgb(fg.get_red(), fg.get_green(), fg.get_blue());

    cr->translate(_border, _border); // so that we have a small white border around the ruler
    cr->move_to (0, _height);
    cr->line_to (_drawing_width, _height);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Glib::ustring abbr = prefs->getString("/options/zoomcorrection/unit");
    if (abbr == "cm") {
        draw_marks(cr, 0.1, 10);
    } else if (abbr == "in") {
        draw_marks(cr, 0.25, 4);
    } else if (abbr == "mm") {
        draw_marks(cr, 10, 10);
    } else if (abbr == "pc") {
        draw_marks(cr, 1, 10);
    } else if (abbr == "pt") {
        draw_marks(cr, 10, 10);
    } else if (abbr == "px") {
        draw_marks(cr, 10, 10);
    } else {
        draw_marks(cr, 1, 1);
    }
    cr->stroke();
}

void
ZoomCorrRulerSlider::on_slider_value_changed()
{
    if (this->get_visible() || freeze) //only take action if user changed value
    {
        freeze = true;
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setDouble("/options/zoomcorrection/value", _slider->get_value() / 100.0);
        _sb->set_value(_slider->get_value());
        _ruler.queue_draw();
        freeze = false;
    }
}

void
ZoomCorrRulerSlider::on_spinbutton_value_changed()
{
    if (this->get_visible() || freeze) //only take action if user changed value
    {
        freeze = true;
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setDouble("/options/zoomcorrection/value", _sb->get_value() / 100.0);
        _slider->set_value(_sb->get_value());
        _ruler.queue_draw();
        freeze = false;
    }
}

void
ZoomCorrRulerSlider::on_unit_changed() {
    if (!_unit.get_sensitive()) {
        // when the unit menu is initialized, the unit is set to the default but
        // it needs to be reset later so we don't perform the change in this case
        return;
    }
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setString("/options/zoomcorrection/unit", _unit.getUnitAbbr());
    double conv = _unit.getConversion(_unit.getUnitAbbr(), "px");
    _ruler.set_unit_conversion(conv);
    if (_ruler.get_visible()) {
        _ruler.queue_draw();
    }
}

bool ZoomCorrRulerSlider::on_mnemonic_activate ( bool group_cycling )
{
    return _sb->mnemonic_activate ( group_cycling );
}

void
ZoomCorrRulerSlider::init(int ruler_width, int ruler_height, double lower, double upper,
                      double step_increment, double page_increment, double default_value)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    double value = prefs->getDoubleLimited("/options/zoomcorrection/value", default_value, lower, upper) * 100.0;

    freeze = false;

    _ruler.set_size(ruler_width, ruler_height);

    _slider = Gtk::make_managed<Gtk::Scale>(Gtk::Orientation::HORIZONTAL);

    _slider->set_size_request(_ruler.width(), -1);
    _slider->set_range (lower, upper);
    _slider->set_increments (step_increment, page_increment);
    _slider->set_value (value);
    _slider->set_digits(2);

    _slider->signal_value_changed().connect(sigc::mem_fun(*this, &ZoomCorrRulerSlider::on_slider_value_changed));
    _sb = Gtk::make_managed<Inkscape::UI::Widget::SpinButton>();
    _sb->signal_value_changed().connect(sigc::mem_fun(*this, &ZoomCorrRulerSlider::on_spinbutton_value_changed));
    _unit.signal_changed().connect(sigc::mem_fun(*this, &ZoomCorrRulerSlider::on_unit_changed));

    _sb->set_range (lower, upper);
    _sb->set_increments (step_increment, 0);
    _sb->set_value (value);
    _sb->set_digits(2);
    _sb->set_max_width_chars(5);    // to fit "100.00"
    _sb->set_halign(Gtk::Align::CENTER);
    _sb->set_valign(Gtk::Align::END);

    _unit.set_sensitive(false);
    _unit.setUnitType(UNIT_TYPE_LINEAR);
    _unit.set_sensitive(true);
    _unit.setUnit(prefs->getString("/options/zoomcorrection/unit"));
    _unit.set_halign(Gtk::Align::CENTER);
    _unit.set_valign(Gtk::Align::END);

    _slider->set_hexpand(true);
    _ruler.set_hexpand(true);
    auto const table = Gtk::make_managed<Gtk::Grid>();
    table->attach(*_slider, 0, 0, 1, 1);
    table->attach(*_sb,      1, 0, 1, 1);
    table->attach(_ruler,   0, 1, 1, 1);
    table->attach(_unit,    1, 1, 1, 1);

    UI::pack_start(*this, *table, UI::PackOptions::shrink);
}

void
PrefSlider::on_slider_value_changed()
{
    if (this->get_visible() || freeze) //only take action if user changed value
    {
        freeze = true;
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setDouble(_prefs_path, _slider->get_value());
        if (_sb) _sb->set_value(_slider->get_value());
        freeze = false;
    }
}

void
PrefSlider::on_spinbutton_value_changed()
{
    if (this->get_visible() || freeze) //only take action if user changed value
    {
        freeze = true;
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        if (_sb) {
            prefs->setDouble(_prefs_path, _sb->get_value());
            _slider->set_value(_sb->get_value());
        }
        freeze = false;
    }
}

bool PrefSlider::on_mnemonic_activate ( bool group_cycling )
{
    return _sb ? _sb->mnemonic_activate ( group_cycling ) : false;
}

void PrefSlider::init(Glib::ustring const &prefs_path,
                 double lower, double upper, double step_increment, double page_increment, double default_value, int digits)
{
    _prefs_path = prefs_path;

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    double value = prefs->getDoubleLimited(prefs_path, default_value, lower, upper);

    freeze = false;

    _slider = Gtk::make_managed<Gtk::Scale>(Gtk::Orientation::HORIZONTAL);

    _slider->set_range (lower, upper);
    _slider->set_increments (step_increment, page_increment);
    _slider->set_value (value);
    _slider->set_digits(digits);
    _slider->signal_value_changed().connect(sigc::mem_fun(*this, &PrefSlider::on_slider_value_changed));
    if (_spin) {
        _sb = Gtk::make_managed<Inkscape::UI::Widget::SpinButton>();
        _sb->signal_value_changed().connect(sigc::mem_fun(*this, &PrefSlider::on_spinbutton_value_changed));
        _sb->set_range (lower, upper);
        _sb->set_increments (step_increment, 0);
        _sb->set_value (value);
        _sb->set_digits(digits);
        _sb->set_halign(Gtk::Align::CENTER);
        _sb->set_valign(Gtk::Align::CENTER);
    }

    auto const table = Gtk::make_managed<Gtk::Grid>();
    _slider->set_hexpand();
    table->attach(*_slider, 0, 0, 1, 1);
    if (_sb) table->attach(*_sb, 1, 0, 1, 1);

    UI::pack_start(*this, *table, UI::PackOptions::expand_widget);
}

void PrefCombo::init(Glib::ustring const &prefs_path,
                     std::span<Glib::ustring const> labels,
                     std::span<int const> values,
                     int const default_value)
{
    int const labels_size = labels.size();
    int const values_size = values.size();
    if (values_size != labels_size) {
        std::cerr << "PrefCombo::"
                  << "Different number of values/labels in " << prefs_path.raw() << std::endl;
        return;
    }

    _prefs_path = prefs_path;
    auto prefs = Inkscape::Preferences::get();
    int row = 0;
    int value = prefs->getInt(_prefs_path, default_value);

    for (int i = 0; i < labels_size; ++i) {
        append(labels[i]);
        _values.push_back(values[i]);
        if (value == values[i]) {
            row = i;
        }
    }
    set_selected(row);
    property_selected().signal_changed().connect([this](){ on_changed(); });
}

void PrefCombo::init(Glib::ustring const &prefs_path,
                     std::span<Glib::ustring const> labels,
                     std::span<Glib::ustring const> values,
                     Glib::ustring const &default_value)
{
    int const labels_size = labels.size();
    int const values_size = values.size();
    if (values_size != labels_size) {
        std::cerr << "PrefCombo::"
                  << "Different number of values/labels in " << prefs_path.raw() << std::endl;
        return;
    }

    _prefs_path = prefs_path;
    auto prefs = Inkscape::Preferences::get();
    int row = 0;
    Glib::ustring value = prefs->getString(_prefs_path);
    if (value.empty()) {
        value = default_value;
    }

    for (int i = 0; i < labels_size; ++i) {
        append(labels[i]);
        _ustr_values.push_back(values[i]);
        if (value == values[i]) {
            row = i;
        }
    }
    set_selected(row);
    property_selected().signal_changed().connect([this](){ on_changed(); });
}

void PrefCombo::on_changed()
{
    if (!get_visible()) return; //only take action if user changed value

    auto prefs = Inkscape::Preferences::get();
    auto row = get_selected();
    if (!_values.empty()) {
        prefs->setInt(_prefs_path, _values.at(row));
    }
    else {
        prefs->setString(_prefs_path, _ustr_values.at(row));
    }
}

void PrefEntryButtonHBox::init(Glib::ustring const &prefs_path,
            bool visibility, Glib::ustring const &default_string)
{
    _prefs_path = prefs_path;
    _default_string = default_string;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    relatedEntry = Gtk::make_managed<Gtk::Entry>();
    relatedButton = Gtk::make_managed<Gtk::Button>(_("Reset"));
    relatedEntry->set_invisible_char('*');
    relatedEntry->set_visibility(visibility);
    relatedEntry->set_text(prefs->getString(_prefs_path));
    UI::pack_start(*this, *relatedEntry);
    UI::pack_start(*this, *relatedButton);
    relatedButton->signal_clicked().connect(
            sigc::mem_fun(*this, &PrefEntryButtonHBox::onRelatedButtonClickedCallback));
    relatedEntry->signal_changed().connect(
            sigc::mem_fun(*this, &PrefEntryButtonHBox::onRelatedEntryChangedCallback));
}

void PrefEntryButtonHBox::onRelatedEntryChangedCallback()
{
    if (this->get_visible()) //only take action if user changed value
    {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setString(_prefs_path, relatedEntry->get_text());
    }
}

void PrefEntryButtonHBox::onRelatedButtonClickedCallback()
{
    if (this->get_visible()) //only take action if user changed value
    {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setString(_prefs_path, _default_string);
        relatedEntry->set_text(_default_string);
    }
}

bool PrefEntryButtonHBox::on_mnemonic_activate ( bool group_cycling )
{
    return relatedEntry->mnemonic_activate ( group_cycling );
}

void PrefEntryFileButtonHBox::init(Glib::ustring const &prefs_path,
            bool visibility)
{
    _prefs_path = prefs_path;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    
    relatedEntry = Gtk::make_managed<Gtk::Entry>();
    relatedEntry->set_invisible_char('*');
    relatedEntry->set_visibility(visibility);
    relatedEntry->set_text(prefs->getString(_prefs_path));
    
    relatedButton = Gtk::make_managed<Gtk::Button>();
    auto const pixlabel = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 3);
    auto const im = sp_get_icon_image("applications-graphics", Gtk::IconSize::NORMAL);
    UI::pack_start(*pixlabel, *im);
    auto const l = Gtk::make_managed<Gtk::Label>();
    l->set_markup_with_mnemonic(_("_Browse..."));
    UI::pack_start(*pixlabel, *l);
    relatedButton->set_child(*pixlabel);

    UI::pack_end(*this, *relatedButton, false, false, 4);
    UI::pack_start(*this, *relatedEntry, true, true);

    relatedButton->signal_clicked().connect(
            sigc::mem_fun(*this, &PrefEntryFileButtonHBox::onRelatedButtonClickedCallback));
    relatedEntry->signal_changed().connect(
            sigc::mem_fun(*this, &PrefEntryFileButtonHBox::onRelatedEntryChangedCallback));
}

void PrefEntryFileButtonHBox::onRelatedEntryChangedCallback()
{
    if (this->get_visible()) { // Only take action if user changed value.
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setString(_prefs_path, Glib::filename_from_utf8(relatedEntry->get_text()));
    }
}

void PrefEntryFileButtonHBox::onRelatedButtonClickedCallback()
{
    if (this->get_visible()) { // Only take action if user changed value.

        // Get the current directory for finding files.
        static std::string current_folder;
        Inkscape::UI::Dialog::get_start_directory(current_folder, _prefs_path, true);

        auto filters = Gio::ListStore<Gtk::FileFilter>::create();

        // Create a filter to limit options to executables.
        // (Only used to select Bitmap and SVG editors.)
        auto filter_app = Gtk::FileFilter::create();
        filter_app->set_name(_("Applications"));
        filter_app->add_mime_type("application/x-executable"); // Linux (xdg-mime query filetype)
        filter_app->add_mime_type("application/x-pie-executable"); // Linux (filetype --mime-type)
        filter_app->add_mime_type("application/x-mach-binary"); // MacOS
        filter_app->add_mime_type("application/vnd.microsoft.portable-executable"); // Windows
        filter_app->add_suffix("exe"); // Windows
        filters->append(filter_app);

        // Just in case...
        auto filter_all = Gtk::FileFilter::create();
        filter_all->set_name(_("All Files"));
        filter_all->add_pattern("*");
        filters->append(filter_all);

        // Create a dialog.
        SPDesktop *desktop = SP_ACTIVE_DESKTOP;
        auto window = desktop->getInkscapeWindow();
        auto file = choose_file_open(_("Select an editor"), window, filters, current_folder, _("Select"));

        if (!file) {
            return; // Cancel
        }

        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setString(_prefs_path, file->get_path());
        relatedEntry->set_text(file->get_parse_name());
    }
}

bool PrefEntryFileButtonHBox::on_mnemonic_activate ( bool group_cycling )
{
    return relatedEntry->mnemonic_activate ( group_cycling );
}

void PrefOpenFolder::init(Glib::ustring const &entry_string, Glib::ustring const &tooltip)
{
    relatedEntry = Gtk::make_managed<Gtk::Entry>();
    relatedButton = Gtk::make_managed<Gtk::Button>();
    auto const pixlabel = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 3);
    auto const im = sp_get_icon_image("document-open", Gtk::IconSize::NORMAL);
    UI::pack_start(*pixlabel, *im);
    auto const l = Gtk::make_managed<Gtk::Label>();
    l->set_markup_with_mnemonic(_("Open"));
    UI::pack_start(*pixlabel, *l);
    relatedButton->set_child(*pixlabel);
    relatedButton->set_tooltip_text(tooltip);
    relatedEntry->set_text(entry_string);
    relatedEntry->set_sensitive(false);
    UI::pack_end(*this, *relatedButton, false, false, 4);
    UI::pack_start(*this, *relatedEntry, true, true);
    relatedButton->signal_clicked().connect(sigc::mem_fun(*this, &PrefOpenFolder::onRelatedButtonClickedCallback));
}

void PrefOpenFolder::onRelatedButtonClickedCallback()
{
    g_mkdir_with_parents(relatedEntry->get_text().c_str(), 0700);
    // helper function in ui/util.h to open folder path
    system_open(relatedEntry->get_text());
}

void PrefEditFolder::init(Glib::ustring const &entry_string, Glib::ustring const &prefs_path, Glib::ustring const &reset_string)
{
    _prefs_path = prefs_path;
    _reset_string = reset_string;

    // warning popup
    warningPopup = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 3);
    warningPopupLabel = Gtk::make_managed<Gtk::Label>();
    warningPopupButton = Gtk::make_managed<Gtk::Button>();
    UI::pack_start(*warningPopup, *warningPopupLabel);
    warningPopupButton->set_label(_("Create"));
    warningPopupButton->set_visible(true);
    UI::pack_end(*warningPopup, *warningPopupButton, false, false, 4);
    popover = Gtk::make_managed<Gtk::Popover>();
    popover->set_child(*warningPopup);
    popover->set_parent(*this);
    warningPopupButton->signal_clicked().connect(sigc::mem_fun(*this, &PrefEditFolder::onCreateButtonClickedCallback));

    // reset button
    resetButton = Gtk::make_managed<Gtk::Button>();
    auto const resetim = sp_get_icon_image("reset-settings", Gtk::IconSize::NORMAL);
    resetButton->set_child(*resetim);
    resetButton->set_tooltip_text(_("Reset to default directory"));
    resetButton->set_margin_start(4);
    UI::pack_end(*this, *resetButton, false, false, 0);
    resetButton->signal_clicked().connect(
            sigc::mem_fun(*this, &PrefEditFolder::onResetButtonClickedCallback));

    // open button
    openButton = Gtk::make_managed<Gtk::Button>();
    auto const openim = sp_get_icon_image("document-open", Gtk::IconSize::NORMAL);
    openButton->set_child(*openim);
    openButton->set_tooltip_text(_("Open directory"));
    openButton->set_margin_start(4);
    UI::pack_end(*this, *openButton, false, false, 0);
    openButton->signal_clicked().connect(
            sigc::mem_fun(*this, &PrefEditFolder::onOpenButtonClickedCallback));

    // linked entry/select box
    relatedPathBox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 0);
    relatedPathBox->set_css_classes({"linked"});

    // select button
    selectButton = Gtk::make_managed<Gtk::Button>();
    auto const selectl = Gtk::make_managed<Gtk::Label>();
    selectl->set_markup_with_mnemonic(_("..."));
    selectButton->set_child(*selectl);
    selectButton->set_tooltip_text(_("Select a new directory"));
    UI::pack_end(*relatedPathBox, *selectButton, false, false, 0);
    selectButton->signal_clicked().connect(
            sigc::mem_fun(*this, &PrefEditFolder::onChangeButtonClickedCallback));

    // entry string
    relatedEntry = Gtk::make_managed<Gtk::Entry>();
    relatedEntry->set_text(entry_string);
    relatedEntry->set_width_chars(12);
    relatedEntry->set_sensitive(true);
    UI::pack_start(*relatedPathBox, *relatedEntry, true, true);
    
    // when warning icon clicked
    relatedEntry->signal_icon_press().connect([&](Gtk::Entry::IconPosition) {
            UI::popup_at(*popover, *relatedEntry, relatedEntry->get_icon_area(Gtk::Entry::IconPosition::SECONDARY));
    });
    relatedEntry->signal_changed().connect(
            sigc::mem_fun(*this, &PrefEditFolder::onRelatedEntryChangedCallback));

    UI::pack_start(*this, *relatedPathBox, true, true);

    // check path at init
    checkPathValidity();
}

void PrefEditFolder::onChangeButtonClickedCallback()
{
    // Get the current directory for finding files.
    static std::string current_folder;
    Inkscape::UI::Dialog::get_start_directory(current_folder, _prefs_path, true);

    // Create a dialog.
    auto dialog = Gtk::FileDialog::create();
    dialog->set_initial_folder(Gio::File::create_for_path(current_folder));
    dialog->select_folder(dynamic_cast<Gtk::Window &>(*get_root()), sigc::track_object([&dialog = *dialog, this] (auto &result) {
        try {
            if (auto folder = dialog.select_folder_finish(result)) {
                // write folder path into prefs & update entry
                setFolderPath(folder);
                return;
            }
        } catch (Gtk::DialogError const& e) {
            if (e.code() == Gtk::DialogError::Code::FAILED) {
                std::cerr << "PrefEditFolder::onChangeButtonClickedCallback: "
                          << "Gtk::FileDialog returned " << e.what() << std::endl;
            }
        }
    }, *this), {});
}

void PrefEditFolder::setFolderPath(Glib::RefPtr<Gio::File const> folder)
{
    Glib::ustring folder_path = folder->get_parse_name();
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setString(_prefs_path, folder_path);
    relatedEntry->set_text(folder_path);
}

void PrefEditFolder::onOpenButtonClickedCallback()
{
    // helper function in ui/util.h to open folder path
    system_open(relatedEntry->get_text());
}

void PrefEditFolder::onResetButtonClickedCallback()
{
    relatedEntry->set_text(_reset_string);
}

void PrefEditFolder::onRelatedEntryChangedCallback()
{
    checkPathValidity();
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setString(_prefs_path, Glib::filename_to_utf8(relatedEntry->get_text()));
}

void PrefEditFolder::checkPathValidity()
{
    _fileInfo = std::make_unique<QueryFileInfo>(relatedEntry->get_text().raw(), [this](auto info) {
        checkPathValidityResults(info);
    });
}

void PrefEditFolder::checkPathValidityResults(Glib::RefPtr<Gio::FileInfo> fileInfo)
{
    PrefEditFolder::Fileis fileis = NONEXISTENT;
    if (fileInfo) { // failsafe, is possible it returns nullptr
        if (fileInfo->get_file_type() == Gio::FileType::DIRECTORY) {
            fileis = PrefEditFolder::Fileis::DIRECTORY;
        } else {
            fileis = PrefEditFolder::Fileis::OTHER;
        }
    }

    // test path validity
    switch (fileis)
    {
        case PrefEditFolder::Fileis::DIRECTORY:
            relatedEntry->unset_icon(Gtk::Entry::IconPosition::SECONDARY);
            // helper class in the stylesheet to remove icons (hack)
            relatedEntry->add_css_class("no-icon");
            relatedEntry->set_icon_sensitive(Gtk::Entry::IconPosition::SECONDARY, false);
            // invalidate the icon tooltip, making it inherit the Gtk::Entry one
            relatedEntry->set_has_tooltip(false);
            openButton->set_sensitive(true);
            break;
        case PrefEditFolder::Fileis::OTHER:
            relatedEntry->set_icon_from_icon_name("dialog-warning", Gtk::Entry::IconPosition::SECONDARY);
            relatedEntry->remove_css_class("no-icon");
            relatedEntry->set_icon_tooltip_markup(_("This is a file. Please select a directory."), Gtk::Entry::IconPosition::SECONDARY);
            relatedEntry->set_icon_sensitive(Gtk::Entry::IconPosition::SECONDARY, true);
            warningPopupLabel->set_markup(relatedEntry->get_icon_tooltip_markup(Gtk::Entry::IconPosition::SECONDARY));
            warningPopupButton->set_visible(false);
            openButton->set_sensitive(false);
            break;
        case PrefEditFolder::Fileis::NONEXISTENT:
            relatedEntry->set_icon_from_icon_name("dialog-warning", Gtk::Entry::IconPosition::SECONDARY);
            relatedEntry->remove_css_class("no-icon");
            relatedEntry->set_icon_sensitive(Gtk::Entry::IconPosition::SECONDARY, true);
            relatedEntry->set_icon_tooltip_markup(_("This directory does not exist."), Gtk::Entry::IconPosition::SECONDARY);
            warningPopupLabel->set_markup(relatedEntry->get_icon_tooltip_markup(Gtk::Entry::IconPosition::SECONDARY));
            warningPopupButton->set_visible(true);
            openButton->set_sensitive(false);
            break;
        default:
            std::cerr << "PrefEditFolder::checkPathValidityResults: "
                      << "Invalid fileis value!" << std::endl;
            break;
    }

    // disable reset button if path is same as default path
    (relatedEntry->get_text() == _reset_string)
        ? resetButton->set_sensitive(false)
        : resetButton->set_sensitive(true);
}

void PrefEditFolder::onCreateButtonClickedCallback()
{
    g_mkdir_with_parents(relatedEntry->get_text().c_str(), 0700);
    popover->popdown();
    checkPathValidity();
}


void PrefEntry::init(Glib::ustring const &prefs_path, bool visibility)
{
    _prefs_path = prefs_path;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    this->set_invisible_char('*');
    this->set_visibility(visibility);
    this->set_text(prefs->getString(_prefs_path));
}

void PrefEntry::on_changed()
{
    if (this->get_visible()) //only take action if user changed value
    {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setString(_prefs_path, this->get_text());
    }
}

void PrefEntryFile::on_changed()
{
    if (this->get_visible()) //only take action if user changed value
    {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setString(_prefs_path, Glib::filename_to_utf8(this->get_text()));
    }
}

void PrefMultiEntry::init(Glib::ustring const &prefs_path, int height)
{
    // TODO: Figure out if there's a way to specify height in lines instead of px
    //       and how to obtain a reasonable default width if 'expand_widget' is not used
    set_size_request(100, height);
    set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
    set_has_frame(true);

    set_child(_text);

    _prefs_path = prefs_path;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Glib::ustring value = prefs->getString(_prefs_path);
    value = Glib::Regex::create("\\|")->replace_literal(value, 0, "\n", (Glib::Regex::MatchFlags)0);
    _text.get_buffer()->set_text(value);
    _text.get_buffer()->signal_changed().connect(sigc::mem_fun(*this, &PrefMultiEntry::on_changed));
}

void PrefMultiEntry::on_changed()
{
    if (get_visible()) //only take action if user changed value
    {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        Glib::ustring value = _text.get_buffer()->get_text();
        value = Glib::Regex::create("\\n")->replace_literal(value, 0, "|", (Glib::Regex::MatchFlags)0);
        prefs->setString(_prefs_path, value);
    } 
}

void PrefColorPicker::init(Glib::ustring const &label, Glib::ustring const &prefs_path,
                           std::string const &default_color)
{
    _prefs_path = prefs_path;
    setTitle(label);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    setColor(prefs->getColor(_prefs_path, default_color));
}

void PrefColorPicker::on_changed(Inkscape::Colors::Color const &color)
{
    if (this->get_visible()) { //only take action if the user toggled it
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setColor(_prefs_path, color);
    }
}

void PrefUnit::init(Glib::ustring const &prefs_path)
{
    _prefs_path = prefs_path;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    setUnitType(UNIT_TYPE_LINEAR);
    setUnit(prefs->getString(_prefs_path));
}

void PrefUnit::on_changed()
{
    if (this->get_visible()) //only take action if user changed value
    {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setString(_prefs_path, getUnitAbbr());
    }
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

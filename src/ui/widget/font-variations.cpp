// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *   Tavmjong Bah <tavmjong@free.fr>
 *   Michael Kowalski <michal_kowalski@hotmail.com>
 *
 * Copyright (C) 2018 Felipe Corrêa da Silva Sanches, Tavmong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <cmath>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/enums.h>
#include <gtkmm/object.h>
#include <gtkmm/sizegroup.h>
#include <gtkmm/spinbutton.h>
#include <iostream>
#include <iomanip>
#include <map>

#include <gtkmm.h>
#include <glibmm/i18n.h>

#include <libnrtype/font-instance.h>
#include <string>
#include <utility>
#include "libnrtype/font-factory.h"

#include "font-variations.h"

// For updating from selection
#include "svg/css-ostringstream.h"

#include "ui/util.h"

namespace Inkscape::UI::Widget {

Glib::ustring get_axis_tooltip(const std::string& tag) {

    static std::map<std::string, Glib::ustring> map = {

        // ----- Axes defined in font by font designer (upper case ASCII) -----
        // Note: these are NOT standarized and the following tooltips could be incorrect.

        // TRANSLATORS: “Grade” (GRAD in CSS) is an axis that can be used to alter stroke thicknesses (or other forms)
        // without affecting the type's overall width, inter-letter spacing, or kerning — unlike altering weight.
        {"GRAD", _("Alter stroke thicknesses (or other forms) without affecting the type’s overall width")},
        // TRANSLATORS: “Parametric Thick Stroke”, XOPQ, is a reference to its logical name, “X Opaque”,
        // which describes how it alters the opaque stroke forms of glyphs typically in the X dimension
        {"XOPQ", _("Alter the opaque stroke forms of glyphs in the X dimension")},
        // TRANSLATORS: “Parametric Thin Stroke”, YOPQ, is a reference to its logical name, “Y Opaque”,
        // which describes how it alters the opaque stroke forms of glyphs typically in the Y dimension
        {"YOPQ", _("Alter the opaque stroke forms of glyphs in the Y dimension")},
        // TRANSLATORS: “Parametric Counter Width”, XTRA, is a reference to its logical name, “X-Transparent,”
        // which describes how it alters a font’s transparent spaces (also known as negative shapes)
        // inside and around all glyphs along the X dimension
        {"XTRA", _("Alter the transparent spaces inside and around all glyphs along the X dimension")},
        {"YTRA", _("Alter the transparent spaces inside and around all glyphs along the Y dimension")},
        // TRANSLATORS: Width/height of Chinese glyphs
        {"XTCH", _("Alter the width of Chinese glyphs")},
        {"YTCH", _("Alter the height of Chinese glyphs")},
        // TRANSLATORS: “Parametric Lowercase Height”
        {"YTLC", _("Vary the height of counters and other spaces between the baseline and x-height")},
        // TRANSLATORS: “Parametric Uppercase Counter Height”
        {"YTUC", _("Vary the height of uppercase letterforms")},
        // TRANSLATORS: “Parametric Ascender Height”
        {"YTAS", _("Vary the height of lowercase ascenders")},
        // TRANSLATORS: “Parametric Descender Depth”
        {"YTDE", _("Vary the depth of lowercase descenders")},
        // TRANSLATORS: “Parametric Figure Height”
        {"YTFI", _("Vary the height of figures")},
        // TRANSLATORS: "Serif rise" - found in the wild (https://github.com/googlefonts/amstelvar)
        {"YTSE", _("Vary the shape of the serifs")},
        // TRANSLATORS: Flare - flaring of the stems
        {"FLAR", _("Controls the flaring of the stems")},
        // TRANSLATORS: Volume - The volume axis works only in combination with the Flare axis. It transforms the serifs
        // and adds a little more edge to details.
        {"VOLM", _("Volume works in combination with flare to transform serifs")},
        // Softness
        {"SOFT", _("Softness makes letterforms more soft and rounded")},
        // Casual
        {"CASL", _("Adjust the letterforms from a more serious style to a more casual style")},
        // Cursive
        {"CRSV", _("Control the substitution of cursive forms")},
        // Fill
        {"FILL", _("Fill can turn transparent forms opaque")},
        // Monospace
        {"MONO", _("Adjust the glyphs from a proportional width to a fixed width")},
        // Wonky
        {"WONK", _("Binary switch used to control substitution of “wonky” forms")},
        // Element shape
        {"ESHP", _("Selection of the base element glyphs are composed of")},
        // Element shape
        {"ELSH", _("Controls element shape characteristics")},
        // Element grid
        {"ELGR", _("Controls how many elements are used per one grid unit")},
        // Element grid
        {"EGRD", _("Controls how many elements are used per one grid unit")},
        // Proposed axis "height"
        {"HGHT", _("Controls the font file’s height parameter")},
        // Non-standard Y-axis stem thickness
        {"YAXS", _("Controls stem thickness in vertical direction")},
        // Vertical Element Alignment
        {"YELA", _("Controls vertical element alignment")},
        // Corner roundness
        {"ROND", _("Controls corner roundness")},
        // Bleed
        {"BLED", _("Controls ink bleed effect")},
        // Scanlines
        {"SCAN", _("Controls scanline effect")},
        // Morph
        {"MORF", _("Controls morphing characteristics")},
        // Extrusion
        {"EDPT", _("Controls depth of extrusion")},
        // Edge highlight
        {"EHLT", _("Controls edge highlighting")},
        // Hyper expansion
        {"HEXP", _("Controls hyper expansion characteristics")},
        // Bounce
        {"BNCE", _("Controls bounce/spring effect")},
        // Informal
        {"INFM", _("Controls informality characteristics")},
        // Spacing
        {"SPAC", _("Controls character spacing")},
        // Negative space
        {"NEGA", _("Controls negative spacing")},
        // X-rotation
        {"XROT", _("Controls character 3D horizontal rotation")},
        // Y-rotation
        {"YROT", _("Controls character 3D vertical rotation")},
        // Sharpness
        {"SHRP", _("Controls sharpness characteristics")},


        // ----- Axes defined in OpenType specification (lower case ASCII) -----

        // TRANSLATORS: “Optical Size”
        // Optical sizes in a variable font are different versions of a typeface optimized for use at singular specific sizes,
        // such as 14 pt or 144 pt. Small (or body) optical sizes tend to have less stroke contrast, more open and wider spacing,
        // and a taller x-height than those of their large (or display) counterparts.
        {"opsz", _("Optimize the typeface for use at specific size")},
        // TRANSLATORS: Slant controls the font file’s slant parameter for oblique styles.
        {"slnt", _("Controls the font file’s slant parameter for oblique styles")},
        // Italic
        {"ital", _("Turns on the font’s italic forms")},
        // TRANSLATORS: Weight controls the font file’s weight parameter.
        {"wght", _("Controls the font file’s weight parameter")},
        // TRANSLATORS: Width controls the font file’s width parameter.
        {"wdth", _("Controls the font file’s width parameter")},


        // ----- Experimental values, see https://variationsguide.typenetwork.com -----
        // Note: These are NOT part of the OpenType specification despite being lower case ASCII.

        {"xtab", _("Controls the tabular width")},
        {"udln", _("Controls the weight of an underline")},
        {"shdw", _("Controls the depth of a shadow")},
        {"refl", _("Controls the Y reflection")},
        {"otln", _("Controls the weight of a font’s outline")},
        {"engr", _("Controls the width of an engraving")},
        {"embo", _("Controls the depth of an emboss")},
        {"rxad", _("Controls the relative X advance - horizontal motion of the glyph")},
        {"ryad", _("Controls the relative Y advance - vertical motion of the glyph")},
        {"rsec", _("Controls the relative second value - as in one second of animation time")},
        {"vrot", _("Controls the rotation of the glyph in degrees")},
        {"vuid", _("Controls the glyph’s unicode ID")},
        {"votf", _("Controls the glyph’s feature variation")},
    };

    auto it = map.find(tag);
    if (it != end(map)) {
        return it->second;
    }
    else {
        return Glib::ustring(tag);
    }
}

FontVariationAxis::FontVariationAxis(OTVarAxis const &axis, Glib::ustring tooltip)
    : Gtk::Box(Gtk::Orientation::HORIZONTAL)
    , tag(axis.tag)
    , name(axis.name)
    , def(axis.def) // Default value
{
    // std::cout << "FontVariationAxis::FontVariationAxis:: "
    //           << "  tag: "  << std::setw(4)  << axis.tag
    //           << "  name: " << std::setw(20) << axis.name
    //           << "  min: "  << std::setw(4)  << axis.minimum
    //           << "  def: "  << std::setw(4)  << axis.def
    //           << "  max: "  << std::setw(4)  << axis.maximum
    //           << "  val: "  << std::setw(4)  << axis.set_val << std::endl;

    set_spacing(4);

    label = Gtk::make_managed<Gtk::Label>(axis.name);
    label->set_tooltip_text(tooltip);
    label->set_xalign(0.0f); // left-align
    append(*label);

    edit = Gtk::make_managed<SpinButton>();
    edit->set_max_width_chars(5);
    edit->set_valign(Gtk::Align::CENTER);
    edit->set_margin_top(2);
    edit->set_margin_bottom(2);
    edit->set_tooltip_text(tooltip);
    append(*edit);

    auto magnitude = static_cast<int>(log10(axis.maximum - axis.minimum));
    precision = 2 - magnitude;
    if (precision < 0) precision = 0;

    auto adj = Gtk::Adjustment::create(axis.set_val, axis.minimum, axis.maximum);
    auto step = pow(10.0, -precision);
    adj->set_step_increment(step);
    adj->set_page_increment(step * 10.0);
    edit->set_adjustment(adj);
    edit->set_digits(precision);

    auto adj_scale = Gtk::Adjustment::create(axis.set_val, axis.minimum, axis.maximum);
    adj_scale->set_step_increment(step);
    adj_scale->set_page_increment(step * 10.0);
    scale = Gtk::make_managed<Gtk::Scale>();
    scale->set_digits (precision);
    scale->set_hexpand(true);
    scale->set_adjustment(adj_scale);
    scale->get_style_context()->add_class("small-slider");
    scale->set_draw_value(false);
    append(*scale);

    // sync slider with spin button
    g_object_bind_property(adj->gobj(), "value", adj_scale->gobj(), "value", GBindingFlags(G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL));
}

void FontVariationAxis::set_value(double value) {
    if (get_value() != value) {
        scale->get_adjustment()->set_value(value);
    }
}

// ------------------------------------------------------------- //

FontVariations::FontVariations()
    : Gtk::Box(Gtk::Orientation::VERTICAL)
{
    // std::cout << "FontVariations::FontVariations" << std::endl;
    set_name("FontVariations");

    _size_group = Gtk::SizeGroup::create(Gtk::SizeGroup::Mode::HORIZONTAL);
    _size_group_edit = Gtk::SizeGroup::create(Gtk::SizeGroup::Mode::HORIZONTAL);
}

// Update GUI based on query.
void FontVariations::update(Glib::ustring const &font_spec)
{
    // Get visible axes from FontInstance
    auto font_instance = FontFactory::get().FaceFromFontSpecification(font_spec.c_str());
    const auto& ot_axes = font_instance ? font_instance->get_opentype_varaxes() : std::vector<OTVarAxis>();

    // Do we need to recreate widgets?
    bool rebuild = false;
    if (_ot_axes.size() != ot_axes.size()) {
        rebuild = true;
    } else if (std::equal(begin(ot_axes), end(ot_axes), begin(_ot_axes))) {
        // Identical (including set values), nothing to do.
        return;
    } else {
        bool same_def = std::equal(begin(ot_axes), end(ot_axes), begin(_ot_axes), [=](const auto& a, const auto& b){
            return a.same_definition(b);
        });

        // different axes definitions?
        if (!same_def) rebuild = true;
    }

    auto scoped(_update.block());

    if (rebuild) {
        // rebuild UI if variable axes definitions have changed
        build_ui(ot_axes);
    }
    else {
        // update UI in-place, some values are different
        for (auto i = 0; i < _axes.size(); ++i) {
            if (_axes[i]->get_name() == _ot_axes[i].name) {
                const auto eps = 0.00001;
                if (abs(_axes[i]->get_value() - ot_axes[i].set_val) > eps) {
                    _axes[i]->set_value(ot_axes[i].set_val);
                }
            } else {
                g_message("axis definition mismatch '%s'", _axes[i]->get_name().c_str());
            }
        }
    }

    _ot_axes = ot_axes;
}


void FontVariations::build_ui(const std::vector<OTVarAxis>& ot_axes) {
    // remove existing widgets, if any
    auto children = get_children();
    for (auto child : children) {
        if (auto group = dynamic_cast<FontVariationAxis*>(child)) {
            _size_group->remove_widget(*group->get_label());
            _size_group_edit->remove_widget(*group->get_editbox());
        }
        remove(*child);
    }

    _axes.clear();
    // create new widgets
    for (const auto& a : ot_axes) {
        auto label_tooltip = get_axis_tooltip(a.tag);
        auto axis = Gtk::make_managed<FontVariationAxis>(a, label_tooltip);
        _axes.push_back(axis);
        append(*axis);
        _size_group->add_widget(*(axis->get_label())); // Keep labels the same width
        _size_group_edit->add_widget(*axis->get_editbox());
        axis->get_editbox()->get_adjustment()->signal_value_changed().connect(
            [this](){ if (!_update.pending()) {_signal_changed.emit();} }
        );
    }
}

#if false
void
FontVariations::fill_css( SPCSSAttr *css ) {

    // Eventually will want to favor using 'font-weight', etc. but at the moment these
    // can't handle "fractional" values. See CSS Fonts Module Level 4.
    sp_repr_css_set_property(css, "font-variation-settings", get_css_string().c_str());
}

Glib::ustring
FontVariations::get_css_string() {

    Glib::ustring css_string;

    for (auto axis: axes) {
        Glib::ustring tag = axis->get_tag();
        std::stringstream value;
        value << std::fixed << std::setprecision(axis->get_precision()) << axis->get_value();
        css_string += "'" + tag + "' " + value.str() + "', ";
    }

    return css_string;
}
#endif

Glib::ustring
FontVariations::get_pango_string(bool include_defaults) const {

    Glib::ustring pango_string;

    if (!_axes.empty()) {

        pango_string += "@";

        for (const auto& axis: _axes) {
            if (!include_defaults && axis->get_value() == axis->get_def()) continue;
            Glib::ustring tag = axis->get_tag();
            CSSOStringStream str;
            str << std::fixed << std::setprecision(axis->get_precision()) << axis->get_value();
            pango_string += tag + "=" + str.str() + ",";
        }

        pango_string.erase (pango_string.size() - 1); // Erase last ',' or '@'
    }

    return pango_string;
}

bool FontVariations::variations_present() const {
    return !_axes.empty();
}

Glib::RefPtr<Gtk::SizeGroup> FontVariations::get_size_group(int index) {
    switch (index) {
    case 0: return _size_group;
    case 1: return _size_group_edit;
    default: return Glib::RefPtr<Gtk::SizeGroup>();
    }
}

int FontVariations::measure_height(int axis_count) {
    std::vector<OTVarAxis> axes(axis_count);
    build_ui(axes);
    int min=0,nat=0,b1,b2;
    measure(Gtk::Orientation::VERTICAL, 9999, min, nat, b1, b2);
    build_ui({});
    return nat;
}

}

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

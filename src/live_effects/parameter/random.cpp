// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::LivePathEffectParameters
 *
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "random.h"

#include <glibmm/i18n.h>
#include <glibmm/ustring.h>

#include "live_effects/effect.h"
#include "svg/stringstream.h"
#include "svg/svg.h"
#include "ui/icon-names.h"

#define noLPERANDOMPARAM_DEBUG

/* RNG stolen from /display/nr-filter-turbulence.cpp */
static constexpr int RAND_m = 2147483647; // 2**31 - 1
static constexpr int RAND_a =      16807; // 7**5; primitive root of m
static constexpr int RAND_q =     127773; // m / a
static constexpr int RAND_r =       2836; // m % a
static constexpr int BSize  =      0x100;

namespace Inkscape::LivePathEffect {

RandomParam::RandomParam(const Glib::ustring& label, const Glib::ustring& tip,
                         const Glib::ustring& key, Inkscape::UI::Widget::Registry* wr,
                         Effect* effect, gdouble default_value, long default_seed, bool randomsign)
    : Parameter(label, tip, key, wr, effect)
{
    defvalue = default_value;
    value = defvalue;
    min = -SCALARPARAM_G_MAXDOUBLE;
    max = SCALARPARAM_G_MAXDOUBLE;
    integer = false;

    defseed = default_seed;
    startseed = defseed;
    seed = startseed;
    _randomsign = randomsign;
}

bool
RandomParam::param_readSVGValue(const gchar * strvalue)
{
    double newval, newstartseed;
    gchar** stringarray = g_strsplit (strvalue, ";", 2);
    unsigned int success = sp_svg_number_read_d(stringarray[0], &newval);
    if (success == 1) {
        success += sp_svg_number_read_d(stringarray[1], &newstartseed);
        if (success == 2) {
            param_set_value(newval, static_cast<long>(newstartseed));
        } else {
            param_set_value(newval, defseed);
        }
        g_strfreev(stringarray);
        return true;
    }
    g_strfreev(stringarray);
    return false;
}

Glib::ustring
RandomParam::param_getSVGValue() const
{
    Inkscape::SVGOStringStream os;
    os << value << ';' << startseed;
    return os.str();
}

Glib::ustring
RandomParam::param_getDefaultSVGValue() const
{
    Inkscape::SVGOStringStream os;
    os << defvalue << ';' << defseed;
    return os.str();
}

void
RandomParam::param_set_default()
{
    param_set_value(defvalue, defseed);
}

void
RandomParam::param_update_default(gdouble default_value){
    defvalue = default_value;
}

void
RandomParam::param_update_default(const gchar * default_value){
    double newval;
    unsigned int success = sp_svg_number_read_d(default_value, &newval);
    if (success == 1) {
        param_update_default(newval);
    }
}

void
RandomParam::param_set_value(gdouble val, long newseed)
{
    value = val;
    if (integer)
        value = round(value);
    if (value > max)
        value = max;
    if (value < min)
        value = min;

    startseed = setup_seed(newseed);
    // we reach maximum value so randomize over to fix duple in next cycle
    Glib::ustring version = param_effect->lpeversion.param_getSVGValue();
    if (startseed == RAND_m - 1 && ((
        effectType() != ROUGH_HATCHES &&
        effectType() != ROUGHEN) || 
        version >= "1.2")) 
    {
        startseed = rand() * startseed;
    }
    seed = startseed;
}

void
RandomParam::param_set_range(gdouble min, gdouble max)
{
    // if you look at client code, you'll see that many effects
    // has a tendency to set an upper range of Geom::infinity().
    // Once again, in gtk2, this is not a problem. But in gtk3,
    // widgets get allocated the amount of size they ask for,
    // leading to excessively long widgets.
    if (min >= -SCALARPARAM_G_MAXDOUBLE) {
        this->min = min;
    } else {
        this->min = -SCALARPARAM_G_MAXDOUBLE;
    }
    if (max <= SCALARPARAM_G_MAXDOUBLE) {
        this->max = max;
    } else {
        this->max = SCALARPARAM_G_MAXDOUBLE;
    }
}

void
RandomParam::param_make_integer(bool yes)
{
    integer = yes;
}

void
RandomParam::resetRandomizer()
{
    seed = startseed;
}

Gtk::Widget *
RandomParam::param_newWidget()
{
    auto const regrandom = Gtk::make_managed<UI::Widget::RegisteredRandom>( param_label,
                                                                            param_tooltip,
                                                                            param_key,
                                                                           *param_wr,
                                                                            param_effect->getRepr(),
                                                                            param_effect->getSPDoc() );
    regrandom->setValue(value, startseed);
    if (integer) {
        regrandom->setDigits(0);
        regrandom->setIncrements(1, 10);
    }
    regrandom->setRange(min, max);
    regrandom->setProgrammatically = false;
    regrandom->signal_value_changed().connect(sigc::mem_fun(*this, &RandomParam::on_value_changed));
    regrandom->set_undo_parameters(RC_("Undo", "Change random parameter"), INKSCAPE_ICON("dialog-path-effects"));
    return regrandom;
}

void RandomParam::on_value_changed()
{
    param_effect->refresh_widgets = true;
}

RandomParam::operator gdouble()
{
    if (_randomsign) {
        return (rand() * value) - (rand() * value);
    } else {
        return rand() * value;
    }
}

long
RandomParam::setup_seed(long lSeed)
{
  if (lSeed <= 0) lSeed = -(lSeed % (RAND_m - 1)) + 1;
  if (lSeed > RAND_m - 1) lSeed = RAND_m - 1;
  return lSeed;
}

// generates random number between 0 and 1
gdouble
RandomParam::rand()
{
  long result;
  result = RAND_a * (seed % RAND_q) - RAND_r * (seed / RAND_q);
  if (result <= 0) result += RAND_m;
  seed = result;

  gdouble dresult = (gdouble)(result % BSize) / BSize;
  return dresult;
}

} // namespace Inkscape::LivePathEffect

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

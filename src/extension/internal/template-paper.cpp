// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Contain internal sizes of paper which can be used in various
 * functions to make and size pages.
 *
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2022 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "template-paper.h"

#include "clear-n_.h"
#include "extension/system.h"
#include "extension/template.h"

namespace Inkscape::Extension::Internal {

/**
 * Return the width and height of the new page with the orientation.
 */
Geom::Point TemplatePaper::get_template_size(Inkscape::Extension::Template *tmod) const
{
    std::string orient = tmod->get_param_optiongroup("orientation", "port");
    double min = tmod->get_param_float("min");
    double max = tmod->get_param_float("max");
    if (orient == "port") {
        return Geom::Point(min, max);
    } else if (orient == "land") {
        return Geom::Point(max, min);
    }
    g_warning("Unknown orientation for paper! '%s'", orient.c_str());
    return Geom::Point(100, 100);
}

void TemplatePaper::init()
{
    // clang-format off
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">"
            "<id>org.inkscape.template.paper</id>"
            "<name>" N_("Paper Sizes") "</name>"
            "<description>" N_("Standard paper document formats") "</description>"
            "<category>" NC_("TemplateCategory", "Print") "</category>"

            "<param name='unit' gui-text='" N_("Unit") "' type='string'>mm</param>"
            "<param name='min' gui-text='" N_("Shortest Side") "' type='float' min='1.0' max='100000.0'>210.0</param>"
            "<param name='max' gui-text='" N_("Longest Side") "' type='float' min='1.0' max='100000.0'>297.0</param>"
            "<param name='orientation' gui-text='" N_("Orientation") "' type='optiongroup' appearance='radio'>"
              "<option value='port'>" N_("Portrait") "</option>"
              "<option value='land'>" N_("Landscape") "</option>"
            "</param>"

            "<template unit='mm' icon='print_portrait' priority='-100' visibility='search'>"
"<preset name='" N_("A4 (Portrait)") "' label='210 × 297 mm' min='210' max='297' orientation='port' priority='-110' visibility='icon'/>"
"<preset name='" N_("A4 (Landscape)") "' label='297 × 210 mm' min='210' max='297' orientation='land' icon='print_landscape' priority='-109' visibility='icon'/>"
"<preset name='" N_("US Letter (Portrait)") "' label='8.5 × 11 in' min='8.5' max='11' unit='in' orientation='port' icon='print_US_portrait' priority='-108' visibility='icon'/>"
"<preset name='" N_("US Letter (Landscape)") "' label='11 × 8.5 in' min='8.5' max='11' unit='in' orientation='land' icon='print_US_landscape' priority='-107' visibility='icon'/>"
"<preset name='" N_("A0") "' label='841 × 1189 mm' min='841' max='1189' visibility='all'/>"
"<preset name='" N_("A1") "' label='594 × 841 mm' min='594' max='841' visibility='all'/>"
"<preset name='" N_("A2") "' label='420 × 594 mm' min='420' max='594' visibility='all'/>"
"<preset name='" N_("A3") "' label='297 × 420 mm' min='297' max='420' visibility='all'/>"
"<preset name='" N_("A4") "' label='210 × 297 mm' min='210' max='297' visibility='list,search'/>"
"<preset name='" N_("A5") "' label='148 × 210 mm' min='148' max='210' visibility='all'/>"
"<preset name='" N_("A6") "' label='105 × 148 mm' min='105' max='148' />"
"<preset name='" N_("A7") "' label='74 × 105 mm' min='74' max='105' />"
"<preset name='" N_("A8") "' label='52 × 74 mm' min='52' max='74' />"
"<preset name='" N_("A9") "' label='37 × 52 mm' min='37' max='52' />"
"<preset name='" N_("A10") "' label='26 × 37 mm' min='26' max='37' />"
"<preset name='" N_("B0") "' label='1000 × 1414 mm' min='1000' max='1414' />"
"<preset name='" N_("B1") "' label='707 × 1000 mm' min='707' max='1000' />"
"<preset name='" N_("B2") "' label='500 × 707 mm' min='500' max='707' />"
"<preset name='" N_("B3") "' label='353 × 500 mm' min='353' max='500' />"
"<preset name='" N_("B4") "' label='250 × 353 mm' min='250' max='353' />"
"<preset name='" N_("B5") "' label='176 × 250 mm' min='176' max='250' />"
"<preset name='" N_("B6") "' label='125 × 176 mm' min='125' max='176' />"
"<preset name='" N_("B7") "' label='88 × 125 mm' min='88' max='125' />"
"<preset name='" N_("B8") "' label='62 × 88 mm' min='62' max='88' />"
"<preset name='" N_("B9") "' label='44 × 62 mm' min='44' max='62' />"
"<preset name='" N_("B10") "' label='31 × 44 mm' min='31' max='44' />"
"<preset name='" N_("C0") "' label='917 × 1297 mm' min='917' max='1297' />"
"<preset name='" N_("C1") "' label='648 × 917 mm' min='648' max='917' />"
"<preset name='" N_("C2") "' label='458 × 648 mm' min='458' max='648' />"
"<preset name='" N_("C3") "' label='324 × 458 mm' min='324' max='458' />"
"<preset name='" N_("C4") "' label='229 × 324 mm' min='229' max='324' />"
"<preset name='" N_("C5") "' label='162 × 229 mm' min='162' max='229' />"
"<preset name='" N_("C6") "' label='114 × 162 mm' min='114' max='162' />"
"<preset name='" N_("C7") "' label='81 × 114 mm' min='81' max='114' />"
"<preset name='" N_("C8") "' label='57 × 81 mm' min='57' max='81' />"
"<preset name='" N_("C9") "' label='40 × 57 mm' min='40' max='57' />"
"<preset name='" N_("C10") "' label='28 × 40 mm' min='28' max='40' />"
"<preset name='" N_("D1") "' label='545 × 771 mm' min='545' max='771' />"
"<preset name='" N_("D2") "' label='385 × 545 mm' min='385' max='545' />"
"<preset name='" N_("D3") "' label='272 × 385 mm' min='272' max='385' />"
"<preset name='" N_("D4") "' label='192 × 272 mm' min='192' max='272' />"
"<preset name='" N_("D5") "' label='136 × 192 mm' min='136' max='192' />"
"<preset name='" N_("D6") "' label='96 × 136 mm' min='96' max='136' />"
"<preset name='" N_("D7") "' label='68 × 96 mm' min='68' max='96' />"
"<preset name='" N_("E3") "' label='400 × 560 mm' min='400' max='560' />"
"<preset name='" N_("E4") "' label='280 × 400 mm' min='280' max='400' />"
"<preset name='" N_("E5") "' label='200 × 280 mm' min='200' max='280' />"
"<preset name='" N_("E6") "' label='140 × 200 mm' min='140' max='200' />"
"<preset name='" N_("Ledger/Tabloid") "' label='11 × 17 in' min='11' max='17' unit='in' visibility='all'/>"
"<preset name='" N_("US Executive") "' label='7.25 × 10.5 in' min='7.25' max='10.5' unit='in' icon='print_US_portrait' visibility='all'/>"
"<preset name='" N_("US Legal") "' label='8.5 × 14 in' min='8.5' max='14' unit='in' icon='print_US_portrait' visibility='all'/>"
"<preset name='" N_("US Letter") "' label='8.5 × 11 in' min='8.5' max='11' unit='in' visibility='list,search'/>"
"<preset name='" N_("DL Envelope") "' label='220 × 110 mm' min='110' max='220' orientation='land' icon='envelope_landscape' visibility='all'/>"
"<preset name='" N_("US #10 Envelope") "' label='9.5 × 4.125 in' min='4.125' max='9.5' unit='in' orientation='land' icon='envelope_landscape' visibility='all'/>"
"<preset name='" N_("Arch A") "' label='9 × 12 in' min='9' max='12' unit='in' />"
"<preset name='" N_("Arch B") "' label='12 × 18 in' min='12' max='18' unit='in' />"
"<preset name='" N_("Arch C") "' label='18 × 24 in' min='18' max='24' unit='in' />"
"<preset name='" N_("Arch D") "' label='24 × 36 in' min='24' max='36' unit='in' />"
"<preset name='" N_("Arch E") "' label='36 × 48 in' min='36' max='48' unit='in' />"
"<preset name='" N_("Arch E1") "' label='30 × 42 in' min='30' max='42' unit='in' />"
            "</template>"
        "</inkscape-extension>",
        std::make_unique<TemplatePaper>());
    // clang-format on
}

} // namespace Inkscape::Extension::Internal

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

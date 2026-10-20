// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_INKSCAPE_EXTENSION_INTERNAL_FILTER_SHADOWS_H__
#define SEEN_INKSCAPE_EXTENSION_INTERNAL_FILTER_SHADOWS_H__
/* Change the 'SHADOWS' above to be your file name */

/*
 * Copyright (C) 2013 Authors:
 *   Ivan Louette (filters)
 *   Nicolas Dufour (UI) <nicoduf@yahoo.fr>
 *
 * Shadow filters
 *   Drop shadow
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
/* ^^^ Change the copyright to be you and your e-mail address ^^^ */

#include "extension/extension.h"
#include "extension/internal/clear-n_.h"
#include "extension/system.h"
#include "filter.h"

namespace Inkscape {
namespace Extension {
namespace Internal {
namespace Filter {

/**
    \brief    Custom predefined Drop shadow filter.

    Colorizable Drop shadow.

    Filter's parameters:
    * Blur radius (0.->200., default 3) -> blur (stdDeviation)
    * Horizontal offset (-50.->50., default 6.0) -> offset (dx)
    * Vertical offset (-50.->50., default 6.0) -> offset (dy)
    * Blur type (enum, default outer) ->
        outer = composite1 (operator="in"), composite2 (operator="over", in1="SourceGraphic", in2="offset")
        inner = composite1 (operator="out"), composite2 (operator="atop", in1="offset", in2="SourceGraphic")
        innercut = composite1 (operator="in"), composite2 (operator="out", in1="offset", in2="SourceGraphic")
        outercut = composite1 (operator="out"), composite2 (operator="in", in1="SourceGraphic", in2="offset")
        shadow = composite1 (operator="out"), composite2 (operator="atop", in1="offset", in2="offset")
    * Color (guint, default 0,0,0,127) -> flood (flood-opacity, flood-color)
    * Use object's color (boolean, default false) -> composite1 (in1, in2)
*/
class ColorizableDropShadow : public Inkscape::Extension::Internal::Filter::Filter
{
protected:
    gchar const *get_filter_text(Inkscape::Extension::Extension *ext) override;

public:
    ColorizableDropShadow()
        : Filter(){};
    ~ColorizableDropShadow() override
    {
        if (_filter != nullptr)
            g_free((void *)_filter);
        return;
    }

    static void init()
    {
        // clang-format off
        Inkscape::Extension::build_from_mem(
            "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
              "<name>" N_("Drop Shadow") "</name>\n"
              "<id>org.inkscape.effect.filter.ColorDropShadow</id>\n"
              "<param name=\"tab\" type=\"notebook\">\n"
                "<page name=\"optionstab\" gui-text=\"" N_("Options") "\">\n"
                  "<param name=\"blur\" gui-text=\"" N_("Blur radius (px)") "\" type=\"float\" appearance=\"full\" min=\"0.0\" max=\"200.0\">3.0</param>\n"
                  "<param name=\"xoffset\" gui-text=\"" N_("Horizontal offset (px)") "\" type=\"float\" appearance=\"full\" min=\"-50.0\" max=\"50.0\">6.0</param>\n"
                  "<param name=\"yoffset\" gui-text=\"" N_("Vertical offset (px)") "\" type=\"float\" appearance=\"full\" min=\"-50.0\" max=\"50.0\">6.0</param>\n"
                  "<param name=\"type\" gui-text=\"" N_("Shadow type:") "\" type=\"optiongroup\" appearance=\"combo\" >\n"
                    "<option value=\"outer\">" N_("Outer") "</option>\n"
                    "<option value=\"inner\">" N_("Inner") "</option>\n"
                    "<option value=\"outercut\">" N_("Outer cutout") "</option>\n"
                    "<option value=\"innercut\">" N_("Inner cutout") "</option>\n"
                    "<option value=\"shadow\">" N_("Shadow only") "</option>\n"
                  "</param>\n"
                "</page>\n"
                "<page name=\"coltab\" gui-text=\"" N_("Blur color") "\">\n"
                  "<param name=\"color\" gui-text=\"" N_("Color") "\" type=\"color\">127</param>\n"
                  "<param name=\"objcolor\" gui-text=\"" N_("Use object's color") "\" type=\"bool\" >false</param>\n"
                "</page>\n"
              "</param>\n"
              "<effect>\n"
                "<object-type>all</object-type>\n"
                "<effects-menu>\n"
                  "<submenu name=\"" N_("Filters") "\">\n"
                     "<submenu name=\"" N_("Shadows and Glows") "\"/>\n"
                  "</submenu>\n"
                "</effects-menu>\n"
              "<menu-tip>" N_("Colorizable Drop shadow") "</menu-tip>\n"
              "</effect>\n"
            "</inkscape-extension>\n", std::make_unique<ColorizableDropShadow>());
        // clang-format on
    };
};

gchar const *ColorizableDropShadow::get_filter_text(Inkscape::Extension::Extension *ext)
{
    if (_filter != nullptr)
        g_free((void *)_filter);

    // Style parameters

    auto color = ext->get_param_color("color");
    float blur_std = ext->get_param_float("blur");
    float offset_x = ext->get_param_float("xoffset");
    float offset_y = ext->get_param_float("yoffset");

    // Handle mode parameter

    const char *comp1op;
    const char *comp1in1;
    const char *comp1in2;
    const char *comp2in1;
    const char *comp2in2;
    const char *comp2op;

    bool objcolor = ext->get_param_bool("objcolor");
    const gchar *mode = ext->get_param_optiongroup("type");

    comp1in1 = "flood";
    comp1in2 = "offset";
    if (g_ascii_strcasecmp("outer", mode) == 0) {
        comp1op = "in";
        comp2op = "over";
        comp2in1 = "SourceGraphic";
        comp2in2 = "comp1";
    } else if (g_ascii_strcasecmp("inner", mode) == 0) {
        comp1op = "out";
        comp2op = "atop";
        comp2in1 = "comp1";
        comp2in2 = "SourceGraphic";
    } else if (g_ascii_strcasecmp("outercut", mode) == 0) {
        comp1op = "in";
        comp2op = "out";
        comp2in1 = "comp1";
        comp2in2 = "SourceGraphic";
    } else if (g_ascii_strcasecmp("innercut", mode) == 0) {
        comp1op = "out";
        comp2op = "in";
        comp2in1 = "comp1";
        comp2in2 = "SourceGraphic";
        if (objcolor) {
            std::swap(comp2in1, comp2in2);
            objcolor = false; // don't swap comp1 inputs later
        }
    } else { // shadow only
        comp1op = "in";
        comp2op = "atop";
        comp2in1 = "comp1";
        comp2in2 = "comp1";
    }

    if (objcolor) {
        std::swap(comp1in1, comp1in2);
    }

    // clang-format off
    auto old = std::locale::global(std::locale::classic());
    _filter = g_strdup_printf(
        "<filter xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\" style=\"color-interpolation-filters:sRGB;\" inkscape:label=\"Drop Shadow\">\n"
            "<feFlood result=\"flood\" in=\"SourceGraphic\" flood-opacity=\"%f\" flood-color=\"%s\"/>\n"
            "<feGaussianBlur result=\"blur\" in=\"SourceGraphic\" stdDeviation=\"%f\"/>\n"
            "<feOffset result=\"offset\" in=\"blur\" dx=\"%f\" dy=\"%f\"/>\n"
            "<feComposite result=\"comp1\" operator=\"%s\" in=\"%s\" in2=\"%s\"/>\n"
            "<feComposite result=\"comp2\" operator=\"%s\" in=\"%s\" in2=\"%s\"/>\n"
        "</filter>\n",

        color.getOpacity(), color.toString(false).c_str(),
        blur_std,
        offset_x, offset_y,

        comp1op, comp1in1, comp1in2,
        comp2op, comp2in1, comp2in2
    );
    std::locale::global(old);
    // clang-format on

    return _filter;
}; /* Drop shadow filter */

}; /* namespace Filter */
}; /* namespace Internal */
}; /* namespace Extension */
}; /* namespace Inkscape */

/* Change the 'SHADOWS' below to be your file name */
#endif /* SEEN_INKSCAPE_EXTENSION_INTERNAL_FILTER_SHADOWS_H__ */

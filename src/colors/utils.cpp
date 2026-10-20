// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 AUTHORS
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <algorithm>
#include <cmath>
#include <glibmm/regex.h>
#include <glibmm/stringutils.h>
#include <iomanip>
#include <sstream>

#include "colors/color.h"
#include "colors/spaces/base.h"
#include "colors/spaces/named.h"
#include "spaces/enum.h"

namespace Inkscape::Colors {

/**
 * Parse a color directly without any CSS or CMS support. This function is ONLY
 * intended to parse values stored in inkscape specific screen-attributes and
 * preferences.
 *
 * DO NOT use this as a common color parser, it does not support any other format
 * other than RRGGBBAA and anything else will cause an error.
 *
 * @arg value - Must be in format #RRGGBBAA only or an empty string.
 */
uint32_t hex_to_rgba(std::string const &value)
{
    if (value.empty())
        return 0x0;

    std::istringstream ss(value);
    if (value.size() != 9 || ss.get() != '#') {
        throw ColorError("Baddly formatted color, it must be in #RRGGBBAA format");
    }
    unsigned int hex;
    ss >> std::hex >> hex;
    return hex;
}

/**
 * Convert a 32bit unsigned int into a set of 3 or 4 double values for rgba.
 *
 * @arg rgba - The integer in the format 0xRRGGBBAA
 * @arg opacity - Include the opacity channel, if false throws opacity away.
 *
 * @returns the values of the rgba channels between 0.0 and 1.0
 */
std::vector<double> rgba_to_values(uint32_t rgba, bool opacity)
{
    std::vector<double> values(3 + opacity);
    values[0] = SP_RGBA32_R_F(rgba);
    values[1] = SP_RGBA32_G_F(rgba);
    values[2] = SP_RGBA32_B_F(rgba);
    if (opacity) {
        values[3] = SP_RGBA32_A_F(rgba);
    }
    return values;
}

/**
 * Output the RGBA value as a #RRGGBB hex color, if alpha is true
 * then the output will be #RRGGBBAA instead.
 */
std::string rgba_to_hex(uint32_t value, bool alpha, bool uppercase)
{
    std::ostringstream oo;
    oo.imbue(std::locale("C"));
    oo << "#" << std::setfill('0') << std::setw(alpha ? 8 : 6)
       << std::hex << (uppercase ? std::uppercase : std::nouppercase)
       << (alpha ? value : value >> 8);
    return oo.str();
}

/**
 * Create a somewhat unique id for the given color used for palette identification.
 */
std::string color_to_id(std::optional<Color> const &color)
{
    if (!color)
        return "none";

    auto name = color->getName();
    if (!name.empty() && name[0] != '#')
        return desc_to_id(name);

    std::ostringstream oo;

    // Special case cssname
    if (auto cns = std::dynamic_pointer_cast<Space::NamedColor>(color->getSpace())) {
        auto name = cns->getNameFor(color->toRGBA());
        if (!name.empty()) {
            oo << "css-" << color->toString();
            return oo.str();
        }
    }

    oo << color->getSpace()->getName() << "-" << std::hex << std::setfill('0');
    for (double const &value : color->getValues()) {
        unsigned int diget = value * 0xff;
        oo << std::setw(2) << diget;
    }

    auto ret = oo.str();
    std::transform(ret.begin(), ret.end(), ret.begin(), ::tolower);
    return ret;
}

/**
 * Transform a color name or description into an id used for palette identification.
 */
std::string desc_to_id(std::string const &desc)
{
    auto name = Glib::ustring(desc);
    // Convert description to ascii, strip out symbols, remove duplicate dashes and prefixes
    static auto const reg1 = Glib::Regex::create("[^[:alnum:]]");
    name = reg1->replace(name, 0, "-", static_cast<Glib::Regex::MatchFlags>(0));
    static auto const reg2 = Glib::Regex::create("-{2,}");
    name = reg2->replace(name, 0, "-", static_cast<Glib::Regex::MatchFlags>(0));
    static auto const reg3 = Glib::Regex::create("(^-|-$)");
    name = reg3->replace(name, 0, "", static_cast<Glib::Regex::MatchFlags>(0));
    // Move important numbers from the start where they are invalid xml, to the end.
    static auto const reg4 = Glib::Regex::create("^(\\d+)(-?)([^\\d]*)");
    name = reg4->replace(name, 0, "\\3\\2\\1", static_cast<Glib::Regex::MatchFlags>(0));
    return name.lowercase();
}

/**
 * Make a darker or lighter version of the color, useful for making checkerboards.
 */
Color make_contrasted_color(Color const &orig, double amount)
{
    if (auto color = orig.converted(Space::Type::HSL)) {
        auto lightness = (*color)[2];
        color->set(2, lightness + ((lightness < 0.08 ? 0.08 : -0.08) * amount));
        color->convert(orig.getSpace());
        return *color;
    }
    return orig;
}

/**
 * Make a themed dark or light color based on a previous shade, returns RGB color.
 */
Color make_theme_color(Color const &orig, bool dark)
{
    // color of the image strip to HSL, so we can manipulate its lightness
    auto color = *orig.converted(Colors::Space::Type::HSLUV);

    if (dark) {
        // limit saturation to improve contrast with some artwork
        color.set(1, std::min(color[1], 0.8));
        // make a darker shade and limit to remove extremes
        color.set(2, std::min(color[2] * 0.7, 0.3));
    } else {
        // make a lighter shade and limit to remove extremes
        color.set(2, std::max(color[2] + (1.0 - color[2]) * 0.5, 0.8));
    }

    return *color.converted(Colors::Space::Type::RGB);
}

/**
 * Make a disabled color, a desaturated version of the given color.
 */
Color make_disabled_color(Color const &orig, bool dark)
{
    auto hsl = *orig.converted(Colors::Space::Type::HSLUV);
    // reduce saturation and lightness/darkness (on dark/light theme)
    static double lf = 0.35; // lightness factor - 35% of lightness
    static double sf = 0.30; // saturation factor - 30% of saturation
    // for both light and dark themes the idea it to compress full range of color lightness (0..1)
    // to a narrower range to convey subdued look of disabled widget (that's the lf * l part);
    // then we move the lightness floor to 0.70 for light theme and 0.20 for dark theme:
    auto saturation = hsl.get(1) * sf;
    auto lightness = lf * hsl.get(2) + (dark ? 0.20 : 0.70); // new lightness in 0..1 range
    hsl.set(1, saturation);
    hsl.set(2, lightness);
    return *hsl.converted(Colors::Space::Type::RGB);
}

double perceptual_lightness(double l)
{
    return l <= 0.885645168 ? l * 0.09032962963 : std::cbrt(l) * 0.249914424 - 0.16;
}

/**
 * Return a value for how the light the color appears to be using HSLUV
 */
double get_perceptual_lightness(Color const &color)
{
    return perceptual_lightness((*color.converted(Space::Type::HSLUV))[2] * 100);
}

std::pair<double, double> get_contrasting_color(double l)
{
    double constexpr l_threshold = 0.85;
    if (l > l_threshold) { // Draw dark over light.
        auto t = (l - l_threshold) / (1.0 - l_threshold);
        return {0.0, 0.4 - 0.1 * t};
    } else { // Draw light over dark.
        auto t = (l_threshold - l) / l_threshold;
        return {1.0, 0.6 + 0.1 * t};
    }
}

}; // namespace Inkscape::Colors

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

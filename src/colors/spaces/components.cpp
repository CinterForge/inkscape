// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Meta data about color channels and how they are presented to users.
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2023 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "components.h"

#include <algorithm>
#include <cmath>
#include <libintl.h> // avoid glib include
#include <map>
// #include <utility>
#include <glib/gi18n.h>
#include <glibmm/value.h>

#include "enum.h"

namespace Inkscape::Colors::Space {

static const std::vector<Components> get_color_spaces() {

static const std::vector<Components> color_spaces = {
    {
        Type::RGB, Type::RGB, Traits::Picker,
        {
            { "r", _("_R"), _("Red"), Unit::EightBit },
            { "g", _("_G"), _("Green"), Unit::EightBit },
            { "b", _("_B"), _("Blue"), Unit::EightBit }
        }
    },
    {
        Type::linearRGB, Type::NONE, Traits::Internal,
        {
            { "r", _("<sub>l</sub>_R"), _("Linear Red"), Unit::EightBit },
            { "g", _("<sub>l</sub>_G"), _("Linear Green"), Unit::EightBit },
            { "b", _("<sub>l</sub>_B"), _("Linear Blue"), Unit::EightBit }
        }
    },
    {
        Type::HSL, Type::HSL, Traits::Picker,
        {
            { "h", _("_H"), _("Hue"), Unit::Degree },
            { "s", _("_S"), _("Saturation"), Unit::Percent },
            { "l", _("_L"), _("Lightness"), Unit::Percent }
        }
    },
    {
        Type::HSV, Type::HSV, Traits::Picker,
        {
            { "h", _("_H"), _("Hue"), Unit::Degree },
            { "s", _("_S"), _("Saturation"), Unit::Percent },
            { "v", _("_V"), _("Value"), Unit::Percent }
        }
    },
    {
        Type::CMYK, Type::NONE, Traits::Picker,
        {
            { "c", _("_C"), C_("CMYK", "Cyan"), Unit::Percent },
            { "m", _("_M"), C_("CMYK", "Magenta"), Unit::Percent },
            { "y", _("_Y"), C_("CMYK", "Yellow"), Unit::Percent },
            { "k", _("_K"), C_("CMYK", "Black"), Unit::Percent }
        }
    },
    {
        Type::CMY, Type::NONE, Traits::Picker,
        {
            { "c", _("_C"), C_("CMYK", "Cyan"), Unit::Percent },
            { "m", _("_M"), C_("CMYK", "Magenta"), Unit::Percent },
            { "y", _("_Y"), C_("CMYK", "Yellow"), Unit::Percent },
        }
    },
    {
        Type::HSLUV, Type::HSLUV, Traits::Picker,
        {
            { "h", _("_H*"), _("Hue"), Unit::Degree },
            { "s", _("_S*"), _("Saturation"), Unit::Percent },
            { "l", _("_L*"), _("Lightness"), Unit::Percent }
        }
    },
    {
        Type::OKHSL, Type::OKHSL, Traits::Picker,
        {
            { "h", _("_H<sub>ok</sub>"), _("Hue"), Unit::Degree },
            { "s", _("_S<sub>ok</sub>"), _("Saturation"), Unit::Percent },
            { "l", _("_L<sub>ok</sub>"), _("Lightness"), Unit::Percent }
        }
    },
    {
        Type::OKHSV, Type::OKHSV, Traits::Internal,
        {
            { "h", _("_H<sub>ok</sub>"), _("Hue"), Unit::Degree },
            { "s", _("_S<sub>ok</sub>"), _("Saturation"), Unit::Percent },
            { "v", _("_V<sub>ok</sub>"), _("Value"), Unit::Percent }
        }
    },
    {
        Type::LCH, Type::NONE, Traits::Internal,
        {
            { "l", _("_L"), _("Luminance"), Unit::EightBit },
            { "c", _("_C"), _("Chroma"), Unit::EightBit },
            { "h", _("_H"), _("Hue"), Unit::Degree },
        }
    },
    {
        Type::LUV, Type::NONE, Traits::Internal,
        {
            { "l", _("_L"), _("Luminance"), Unit::Percent },
            { "u", _("_U"), _("Chroma U"), Unit::Percent },
            { "v", _("_V"), _("Chroma V"), Unit::Percent },
        }
    },
    {
        Type::OKLAB, Type::NONE, Traits::Internal,
        {
            { "l", _("_L<sub>ok</sub>"), _("Lightness"), Unit::Percent },
            { "a", _("_A<sub>ok</sub>"), _("Component A"), Unit::Percent },
            { "b", _("_B<sub>ok</sub>"), _("Component B"), Unit::Percent }
        }
    },
    {
        Type::OKLCH, Type::OKHSL, Traits::Picker,
        {
            { "l", _("_L<sub>ok</sub>"), _("Lightness"), Unit::Percent },
            { "c", _("_C<sub>ok</sub>"), _("Chroma"), Unit::Chroma40 }, // 100% is 0.4
            { "h", _("_H<sub>ok</sub>"), _("Hue"), Unit::Degree }
        }
    },
    {
        Type::LAB, Type::NONE, Traits::Internal,
        {
            { "l", _("_L"), _("Lightness"), Unit::Percent },
            { "a", _("_A"), _("Component A"), Unit::EightBit },
            { "b", _("_B"), _("Component B"), Unit::EightBit }
        }
    },
    {
        Type::YCbCr, Type::NONE, Traits::CMS,
        {
            { "y", _("_Y"), _("Y"), Unit::EightBit },
            { "cb", _("C_r"), _("Cb"), Unit::EightBit },
            { "cr", _("C_b"), _("Cr"), Unit::EightBit }
        }
    },
    {
        Type::XYZ, Type::NONE, Traits::Internal,
        {
            { "x", "_X", "X", Unit::EightBit },
            { "y", "_Y", "Y", Unit::EightBit },
            { "z", "_Z", "Z", Unit::EightBit }
        }
    },
    {
        Type::XYZ50, Type::NONE, Traits::Internal,
        {
            { "x", "_X", "X", Unit::EightBit },
            { "y", "_Y", "Y", Unit::EightBit },
            { "z", "_Z", "Z", Unit::EightBit }
        }
    },
    {
        Type::YXY, Type::NONE, Traits::Internal,
        {
            { "y1", "_Y", "Y", Unit::EightBit },
            { "x", "_x", "x", Unit::EightBit },
            { "y2", "y", "y", Unit::EightBit }
        }
    },
    {
        Type::Gray, Type::NONE, Traits::Internal,
        {
            { "gray", _("G"), _("Gray"), Unit::Linear1024 }
        }
    },
    {
        Type::Alpha, Type::NONE, Traits::Internal,
        {
        }
    }
};
    return color_spaces;
}


Component::Component(Type type, unsigned int index, std::string id, std::string name, std::string tip, Unit unit)
    : type(type)
    , index(index)
    , id(std::move(id))
    , name(std::move(name))
    , tip(std::move(tip))
    , unit(unit) {

    switch (unit) {
    case Unit::EightBit:
        scale = 255;
        break;
    case Unit::Degree:
        scale = 360;
        break;
    case Unit::Percent:
        scale = 100;
        break;
    case Unit::Linear1024:
        scale = 1024;
        break;
    case Unit::Chroma40:
        scale = 40;
        break;
    default:
        throw std::logic_error("Missing case statement in Component ctor.");
    }
}

Component::Component(std::string id, std::string name, std::string tip, Unit unit)
    : Component(Type::NONE, -1, id, name, tip, unit)
{}

/**
 * Clamp the value to between 0.0 and 1.0, except for hue which is wrapped around.
 */
double Component::normalize(double value) const
{
    if (unit == Unit::Degree && (value < 0.0 || value > 1.0)) {
        return value - std::floor(value);
    }
    return std::clamp(value, 0.0, 1.0);
}

void Components::add(std::string id, std::string name, std::string tip, Unit unit)
{
    _components.emplace_back(_type, _components.size(), std::move(id), std::move(name), std::move(tip), unit);
}

std::map<Type, Components> _build(bool alpha)
{
    std::map<Type, Components> sets;
    for (auto& components : get_color_spaces()) {
        unsigned int index = 0;
        for (auto& component : components.getAll()) {
            // patch components
            auto& rw = const_cast<Component&>(component);
            rw.type = components.getType();
            rw.index = index++;
        }
        sets[components.getType()] = components;
    }

    if (alpha) {
        for (auto &[key, val] : sets) {
            // alpha component with unique ID, so it doesn't clash with "a" in Lab
            val.add("alpha", C_("Transparency (alpha)", "_A"), _("Alpha"), Unit::Percent);
        }
    }
    return sets;
}

Components const &Components::get(Type space, bool alpha)
{
    static std::map<Type, Components> sets_no_alpha = _build(false);
    static std::map<Type, Components> sets_with_alpha = _build(true);

    auto &lookup_set = alpha ? sets_with_alpha : sets_no_alpha;
    if (auto search = lookup_set.find(space); search != lookup_set.end()) {
        return search->second;
    }
    return lookup_set[Type::NONE];
}

Type Components::get_wheel_type() const {
    return _wheel_type;
}

} // namespace Inkscape::Colors::Space

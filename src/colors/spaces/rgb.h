// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLORS_SPACES_RGB_H
#define SEEN_COLORS_SPACES_RGB_H

#include "base.h"

namespace Inkscape::Colors::Space {

/**
 * Return the RGB color profile, this is static for all RGB sub-types
 */
static std::shared_ptr<Colors::CMS::Profile> const getRGBProfile()
{
    static std::shared_ptr<Colors::CMS::Profile> srgb_profile;
    if (!srgb_profile) {
        srgb_profile = Colors::CMS::Profile::create_srgb();
    }
    return srgb_profile;
}

template <typename T>
class RGBBase : public ConvertableSpace<T>
{
public:
    RGBBase(Type type, std::string name, std::string shortName, std::string icon, bool spaceIsUnbounded = false)
        : ConvertableSpace<T>(type, std::move(name), std::move(shortName), std::move(icon), spaceIsUnbounded)
    {}

    std::shared_ptr<Colors::CMS::Profile> const getProfile() const override { return getRGBProfile(); }
};

class RGB : public ProfileSpace<true>
{
public:
    constexpr static int ProfileChannels = 3;
    constexpr static int OutputChannels = 3;

    RGB(): ProfileSpace(Type::RGB, 3, "RGB", "RGB", "color-selector-rgb") {
        _svgNames.emplace_back("sRGB");
    }
    // Unique constructor for CSSNAME which is just sRGB with strings
    RGB(Type type, std::string name, std::string shortName, std::string icon)
        : ProfileSpace(type, 3, std::move(name), std::move(shortName), std::move(icon))
    {}
    ~RGB() override = default;

    std::shared_ptr<Colors::CMS::Profile> const getProfile() const override { return getRGBProfile(); }
protected:

    friend class Inkscape::Colors::Color;

    std::string toString(std::vector<double> const &values, bool opacity = true) const override;

public:
    class Parser : public LegacyParser
    {
    public:
        Parser(bool alpha)
            : LegacyParser("rgb", Type::RGB, alpha)
        {}
        bool parse(std::istringstream &input, std::vector<double> &output) const override;
    };
};

} // namespace Inkscape::Colors::Space

#endif // SEEN_COLORS_SPACES_RGB_H

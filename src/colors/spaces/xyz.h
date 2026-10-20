// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLORS_SPACES_XYZ_H
#define SEEN_COLORS_SPACES_XYZ_H

#include "base.h"

namespace Inkscape::Colors::Space {

/**
 * Return the XYZ D65 color profile
 */
static std::shared_ptr<Colors::CMS::Profile> const getXYZ65Profile()
{
    static std::shared_ptr<Colors::CMS::Profile> xyz_profile = Colors::CMS::Profile::create_xyz65();
    return xyz_profile;
}

template <typename T>
class XYZBase : public ConvertableSpace<T>
{
public:
    XYZBase(Type type, std::string name, std::string shortName, std::string icon, bool spaceIsUnbounded = false)
        : ConvertableSpace<T>(type, std::move(name), std::move(shortName), std::move(icon), spaceIsUnbounded)
    {}

    std::shared_ptr<Inkscape::Colors::CMS::Profile> const getProfile() const override { return getXYZ65Profile(); }
};

class XYZ : public ProfileSpace<true>
{
public:
    XYZ(): ProfileSpace(Type::XYZ, 3, "XYZ", "XYZ", "color-selector-xyz", true) {
        _svgNames.emplace_back("xyz-d65");
        _svgNames.emplace_back("xyz");
        _intent = RenderingIntent::RELATIVE_COLORIMETRIC_NOBPC;
        _intent_priority = 10;
    }
    ~XYZ() override = default;

    std::shared_ptr<Inkscape::Colors::CMS::Profile> const getProfile() const override { return getXYZ65Profile(); }

    std::string toString(std::vector<double> const &values, bool opacity = true) const override;
};

class XYZ50 : public ProfileSpace<true>
{
public:
    XYZ50(): ProfileSpace(Type::XYZ50, 3, "XYZ D50", "XYZ D50", "color-selector-xyz", true) {
        _svgNames.emplace_back("xyz-d50");
    }
    ~XYZ50() override = default;

    std::shared_ptr<Inkscape::Colors::CMS::Profile> const getProfile() const override;

    std::string toString(std::vector<double> const &values, bool opacity = true) const override;
};

} // namespace Inkscape::Colors::Space

#endif // SEEN_COLORS_SPACES_XYZ_H

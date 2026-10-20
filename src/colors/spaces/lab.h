// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLORS_SPACES_LAB_H
#define SEEN_COLORS_SPACES_LAB_H

#include "base.h"

namespace Inkscape::Colors::Space {

/**
 * Return the RGB color profile, this is static for all RGB sub-types
 */
static std::shared_ptr<Inkscape::Colors::CMS::Profile> const getLabProfile()
{
    static std::shared_ptr<Colors::CMS::Profile> lab_profile = Colors::CMS::Profile::create_lab();
    return lab_profile;
}

template <typename T>
class LabBase : public ConvertableSpace<T>
{
public:
    LabBase(Type type, std::string name, std::string shortName, std::string icon, bool spaceIsUnbounded = false)
        : ConvertableSpace<T>(type, std::move(name), std::move(shortName), std::move(icon), spaceIsUnbounded)
    {}

    std::shared_ptr<Inkscape::Colors::CMS::Profile> const getProfile() const override { return getLabProfile(); }
};


class Lab : public ProfileSpace<true>
{
public:
    static constexpr double LUMA_SCALE = 100;
    // CSS Actual values are scaled -128 -> 127
    static constexpr double MIN_SCALE = -128;
    static constexpr double MAX_SCALE = 127;

    Lab(): ProfileSpace(Type::LAB, 3, "Lab", "Lab", "color-selector-lab", true) {
        _svgNames.emplace_back("lab");
        _intent = RenderingIntent::ABSOLUTE_COLORIMETRIC;
        _intent_priority = 10;
    }
    ~Lab() override = default;

    std::shared_ptr<Inkscape::Colors::CMS::Profile> const getProfile() const override { return getLabProfile(); }

    std::string toString(std::vector<double> const &values, bool opacity) const override;

    class Parser : public Colors::Parser
    {
    public:
        Parser()
            : Colors::Parser("lab", Type::LAB)
        {}
        bool parse(std::istringstream &input, std::vector<double> &output) const override;
    };

    static void scaleDown(std::vector<double> &in_out);
    static void scaleUp(std::vector<double> &in_out);
};

} // namespace Inkscape::Colors::Space

#endif // SEEN_COLORS_SPACES_LAB_H

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

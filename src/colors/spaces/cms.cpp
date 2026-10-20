// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2023 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "cms.h"

#include <iomanip>

#include "colors/cms/profile.h"
#include "colors/cms/transform.h"
#include "colors/color.h"
#include "colors/manager.h"
#include "colors/printer.h"

namespace Inkscape::Colors::Space {

// When we support a color space that lcms2 does not, record here
static cmsUInt32Number customSigOKLabData = 0x4f4b4c42; // 'OKLB';

static std::map<cmsUInt32Number, Space::Type> _lcmssig_to_space = {
    {cmsSigRgbData, Space::Type::RGB},        {cmsSigHlsData, Space::Type::HSL},
    {cmsSigCmykData, Space::Type::CMYK},      {cmsSigCmyData, Space::Type::CMY},
    {cmsSigHsvData, Space::Type::HSV},        {cmsSigLuvData, Space::Type::HSLUV},
    {customSigOKLabData, Space::Type::OKLAB}, {cmsSigXYZData, Space::Type::XYZ},
    {cmsSigXYZData, Space::Type::YXY},        {cmsSigLabData, Space::Type::LAB},
    {cmsSigYCbCrData, Space::Type::YCbCr},    {cmsSigGrayData, Space::Type::Gray},
};

CMS::CMS(std::shared_ptr<Inkscape::Colors::CMS::Profile> profile, std::string name)
    : ProfileSpace(Type::CMS,
            profile ? profile->getSize() : 3,
            name.empty() ? profile->getName(true) : name,
            name.empty() ? profile->getName(true) : name,
            "color-selector-cms")
    , _profile_size(profile->getSize())
    , _profile_type(_lcmssig_to_space[profile->getColorSpace()])
    , _profile(profile)
{
    _svgNames.emplace_back(getName());
    _intent_priority = 100;
}

/**
 * Naked CMS space for testing and data retention where the profile is unavailable.
 */
CMS::CMS(std::string profile_name, unsigned profile_size, Space::Type profile_type)
    : ProfileSpace(Type::CMS, profile_size + 3, profile_name, profile_name, {profile_name}, "color-selector-cms")
    , _profile_size(profile_size)
    , _profile_type(profile_type)
    , _profile(nullptr)
{
    _intent_priority = 100;
}

/**
 * Return the profile for this cms space. If this is anonymous, it returns srgb
 * so the transformation on the fallback color is transparent.
 */
std::shared_ptr<Colors::CMS::Profile> const CMS::getProfile() const
{
    static auto srgb_profile = Colors::CMS::Profile::create_srgb();
    if (!hasValidCmsProfile()) {
        return srgb_profile;
    }
    return _profile;
}

/**
 * If this space lacks a profile, it's really the sRGB fallback values,
 * so we strip out any other values from the io, otherwise we strip the
 * fallback rgb instead.
 *
 * Not needed for surface conversions, just SVG ones. So this isn't a
 * static version of spaceToProfile you will see in other modules.
 */
bool CMS::spaceToProfile(std::vector<double> &io) const
{
    bool has_rgb = io.size() >= _profile_size + 3;
    bool has_alpha = io.size() == _profile_size + (has_rgb * 3) + 1;

    if (!_profile) {
        // Remove the CMS values leaving just the backup RGB
        io.erase(io.begin() + 3, io.end() - has_alpha);
    } else if (has_rgb) {
        // Remove RGB backup leaving just the CMS values
        io.erase(io.begin(), io.begin() + 3);
    }
    return true;
}

/**
 * Parse a string stream into a vector of doubles which are always values in
 * this CMS space / icc profile.
 *
 * @args ss - String input which doesn't have to be at the start of the string.
 * @returns output - The vector to populate with the numbers found in the string.
 * @returns the name of the cms profile requested.
 */
std::string CMS::CmsParser::parseColor(std::istringstream &ss, std::vector<double> &output, bool &more) const
{
    std::string icc_name;
    ss >> icc_name;

    if (!icc_name.empty() && icc_name.back() == ',')
        icc_name.pop_back();

    bool end = false;
    while (!end && append_css_value(ss, output, end, ','))
        continue;

    if (output.size() == 0) {
        std::string named;
        ss >> named;
        if (!named.empty() && ss.get() == ')') {
            std::cerr << "Found SVG2 ICC named color '" << named.c_str() << "' for profile '" << icc_name.c_str()
                      << "', which not supported yet.\n";
        }
    }

    return icc_name;
}

/**
 * Output these values into this CMS space.
 *
 * @args values - The values for each channel in the icc profile.
 * @args opacity - Should opacity be included. This is ignored since cms
 *                 output is ALWAYS without opacity.
 *
 * @returns the string suitable for css and style use.
 */
std::string CMS::toString(std::vector<double> const &values, bool /*opacity*/) const
{
    if (values.size() < _profile_size)
        return "";

    // RGBA Hex fallback plus icc-color section
    auto oo = IccColorPrinter(_profile_size, getName());

    // When an icc color was parsed, but there is no profile
    if (!hasValidCmsProfile()) {
        // Not enough values for a fallback option (maybe corrupt?)
        if (values.size() < _profile_size + 3)
            return "";
        std::vector<double> remains(values.begin() + 3, values.end());
        oo << remains;
    } else {
        oo << values;
    }
    // opacity is never added to the printer here, always ignored
    return rgba_to_hex(toRGBA(values), false) + " " + (std::string)oo;
}

/**
 * Return true if, using a rough hueruistic, this color could be considered to be using
 * too much ink if it was printed using the ink as specified.
 *
 * NOTE: This is only useful for CMYK profiles. Anything else will return false.
 *
 * @arg input - Channel values in this space.
 */
bool CMS::overInk(std::vector<double> const &input) const
{
    if (input.empty() || _profile_type != Type::CMYK)
        return false;

    /* Some literature states that when the sum of paint values exceed 320%, it is considered to be a satured color,
       which means the paper can get too wet due to an excessive amount of ink. This may lead to several issues
       such as misalignment and poor quality of printing in general.*/
    return input[0] + input[1] + input[2] + input[3] > 3.2;
}

}; // namespace Inkscape::Colors::Space

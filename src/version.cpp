// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Versions
 *
 * Authors:
 *   MenTaLguY <mental@rydia.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Kris De Gussem <Kris.DeGussem@gmail.com>
 *   Rafał Siejakowski <rs@rs-math.net>
 *
 * Copyright (C) 2003 MenTaLguY
 * Copyright (C) 2012 Kris De Gussem
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <sstream>
#include "version.h"

namespace Inkscape {
Version::Version()
    : _string_representation{"0.0"}
{}

Version::Version(unsigned major_version, unsigned minor_version, std::string_view suffix)
    : _major{major_version}
    , _minor{minor_version}
    , _suffix{suffix}
{}

Version::Version(unsigned major_version, unsigned minor_version)
    : Version(major_version, minor_version, "")
{}

std::optional<Version> Version::from_string(char const *version_string)
{
    if (!version_string) {
        return {};
    }

    try {
        std::istringstream ss{version_string};
        ss.imbue(std::locale::classic());

        // Throw an exception if an error occurs when parsing the major and minor numbers.
        ss.exceptions(std::ios::failbit | std::ios::badbit);
        unsigned major, minor;
        std::string suffix;

        ss >> major;
        char tmp = 0;
        ss >> tmp;
        if (tmp != '.') {
            return {};
        }
        ss >> minor;

        // Don't throw exception if failbit gets set (empty string is OK).
        ss.exceptions(std::ios::goodbit);
        ss >> suffix;
        return Version{major, minor, suffix};
    } catch (...) {
        return {};
    }
}

std::string const &Version::str() const
{
    if (_string_representation.empty()) {
        _string_representation = std::to_string(_major) + '.' + std::to_string(_minor) + _suffix;
    }
    return _string_representation;
}
} // namespace Inkscape

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

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   MenTaLguY <mental@rydia.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Rafał Siejakowski <rs@rs-math.net>
 *
 * Copyright (C) 2003 MenTaLguY
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_VERSION_H
#define SEEN_INKSCAPE_VERSION_H

#define SVG_VERSION "1.1"

#include <compare>
#include <optional>
#include <string>
#include <string_view>

namespace Inkscape {

class Version {
public:
    Version();

    // Note: somebody pollutes our namespace with major() and minor()
    Version(unsigned major_version, unsigned minor_version);
    Version(unsigned major_version, unsigned minor_version, std::string_view suffix);

    /// Build a Version form a string, returning an empty optional in case of error.
    static std::optional<Version> from_string(const char *version_string);

    std::partial_ordering operator<=>(Version const &other) const
    {
        return _major == other._major ? _minor <=> other._minor : _major <=> other._major;
    }
    bool operator==(Version const &other) const { return _major == other._major && _minor == other._minor; }

    // Run Inclusive check on version [min_version, max_version]
    bool isInsideRangeInclusive(Version const &min_version, Version const &max_version) const
    {
        return min_version <= *this && *this <= max_version;
    }

    // Run Exclusive check on version (min_version, max_version)
    bool isInsideRangeExclusive(Version const &min_version, Version const &max_version) const
    {
        return min_version < *this && *this < max_version;
    }
    std::string const &str() const;

private:
    unsigned int _major = 0;
    unsigned int _minor = 0;
    std::string _suffix; ///< For example, for development version
    std::string mutable _string_representation;
};
} // namespace Inkscape

#endif // SEEN_INKSCAPE_VERSION_H
/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:75
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

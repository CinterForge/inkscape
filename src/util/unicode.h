// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 8/11/25.
//

#ifndef UNICODE_H
#define UNICODE_H

#include <glib.h>
#include <cstdint>
#include <vector>
#include <glibmm/ustring.h>

namespace Inkscape::Util {

struct UnicodeRange {
    // Unicode range, inclusive
    gunichar from;
    gunichar to;
    // Translated name of the range
    Glib::ustring name;
};

// List of predefined Unicode ranges with names
const std::vector<UnicodeRange>& get_unicode_ranges();

// Retrieve Unicode character name (read from official NamesList.txt), English names only ATM.
std::string get_unicode_name(std::uint32_t unicode);

}

#endif //UNICODE_H

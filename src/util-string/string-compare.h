// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 12/4/25.
//

#ifndef INKSCAPE_STRING_COMPARE_H
#define INKSCAPE_STRING_COMPARE_H

#include <string>

namespace Inkscape {

// Comparison function for natural sort:
//
// The idea is to have "name100" follow "name2", even though it comes first alphabetically
//
bool natural_compare(const std::string& s1, const std::string& s2) {
    std::size_t i = 0, j = 0;
    while (i < s1.length() && j < s2.length()) {
        if (std::isdigit(s1[i]) && std::isdigit(s2[j])) {
            // Extract and compare numeric parts
            long num1 = 0;
            while (i < s1.length() && std::isdigit(s1[i])) {
                num1 = num1 * 10 + (s1[i] - '0');
                i++;
            }

            long num2 = 0;
            while (j < s2.length() && std::isdigit(s2[j])) {
                num2 = num2 * 10 + (s2[j] - '0');
                j++;
            }

            if (num1 != num2) {
                return num1 < num2;
            }
        } else {
            // Compare non-numeric parts
            if (s1[i] != s2[j]) {
                return s1[i] < s2[j];
            }
            i++;
            j++;
        }
    }
    // Handle cases where one string is a prefix of the other
    return s1.length() < s2.length();
}

}

#endif //INKSCAPE_STRING_COMPARE_H
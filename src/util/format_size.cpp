// SPDX-License-Identifier: GPL-2.0-or-later

#include "format_size.h"
#include <glibmm/ustring.h>
#include <sstream>

namespace Inkscape {
namespace Util {

Glib::ustring format_size(std::size_t value) {
    if (!value) {
        return Glib::ustring("0");
    }

    typedef std::vector<char> Digits;
    typedef std::vector<Digits *> Groups;

    Groups groups;

    Digits *digits;

    while (value) {
        unsigned places=3;
        digits = new Digits();
        digits->reserve(places);

        while ( value && places ) {
            digits->push_back('0' + (char)( value % 10 ));
            value /= 10;
            --places;
        }

        groups.push_back(digits);
    }

    Glib::ustring temp;

    while (true) {
        digits = groups.back();
        while (!digits->empty()) {
            temp.append(1, digits->back());
            digits->pop_back();
        }
        delete digits;

        groups.pop_back();
        if (groups.empty()) {
            break;
        }

        temp.append(",");
    }

    return temp;
}

Glib::ustring format_file_size(std::size_t value) {
    std::ostringstream ost;
    if (value < 1024) {
        ost << value << " B";
    }
    else {
        double size = value;
        int index = 0;
        do {
            size /= 1024;
            ++index;
        } while (size > 1024);

        static const char* unit[] = {"", "k", "M", "G", "T", "P", "E", "Z", "Y"};
        ost.precision(1);
        ost << std::fixed << size << ' ' << unit[index] << 'B';
    }
    return ost.str();
}

}} // namespace

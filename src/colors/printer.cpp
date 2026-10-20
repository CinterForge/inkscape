// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 AUTHORS
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "printer.h"

namespace Inkscape::Colors {

CssPrinter &CssPrinter::operator<<(double value)
{
    if (!_done) {
        auto base_printer = (std::ostringstream*)this;
        if (_count == _channels && _slash_opacity) {
            // We print opacity as a percentage
            *base_printer << " / " << (int)(value * 100) << "%";
        } else if (_count < _channels) {
            std::ostringstream oo;
            oo.imbue(getloc());
            oo.precision(precision());
            oo << std::fixed << value;

            // To bypass the lack of significant-figures option we strip trailing zeros in a sub-process.
            auto number = oo.str();
            while (number.find(".") != std::string::npos &&
                   (number.substr(number.length() - 1, 1) == "0" || number.substr(number.length() - 1, 1) == "."))
                number.pop_back();

            if (number == "-0")
                number = "0";

            *base_printer << (_count ? _sep : "") << number;
        }
        _count++;
    }
    return *this;
}

CssPrinter &CssPrinter::operator<<(int value)
{
    if (!_done && _count < _channels) {
        auto base_printer = (std::ostringstream*)this;
        *base_printer << (_count ? _sep : "") << value;
        _count++;
    }
    return *this;
}

CssPrinter &CssPrinter::operator<<(std::vector<double> const &values)
{
    for (auto &value : values) {
        if (_count < _channels)
            *this << value;
    }
    return *this;
}

}; // namespace Inkscape::Colors

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

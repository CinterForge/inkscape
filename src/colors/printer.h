// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 AUTHORS
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_COLORS_PRINTER_H
#define SEEN_COLORS_PRINTER_H

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace Inkscape::Colors {

class CssPrinter : public std::ostringstream
{
public:
    CssPrinter(unsigned channels, std::string prefix, std::string ident = "", std::string sep = " ")
        : _channels(channels)
        , _sep(std::move(sep))
    {
        imbue(std::locale("C"));
        precision(3);
        *this << prefix << "(";
        if (!ident.empty()) {
            *this << ident;
            _count = 1;
            _channels += 1;
        }
    }

    CssPrinter &operator<<(double);
    CssPrinter &operator<<(int);
    CssPrinter &operator<<(std::vector<double> const &);
    CssPrinter &operator<<(const char *value) {*(std::ostringstream*)this << value; return *this;}

    operator std::string()
    {
        if (!_done) {
            _done = true;
            *this << ")";
        }
        if (_count < _channels) {
            std::cerr << " Expected " << _channels << " channels but got " << _count << "\n";
            return "";
        }
        return str();
    }

protected:
    bool _slash_opacity = false;
    bool _done = false;
    unsigned _count = 0;
    unsigned _channels = 0;
    std::string _sep;
};

class IccColorPrinter : public CssPrinter
{
public:
    IccColorPrinter(unsigned channels, std::string ident)
        : CssPrinter(channels, "icc-color", std::move(ident), ", ")
    {}
};

class CssLegacyPrinter : public CssPrinter
{
public:
    CssLegacyPrinter(unsigned channels, std::string prefix, bool opacity)
        : CssPrinter(channels + (int)opacity, prefix + (opacity ? "a" : ""), "", ", ")
    {}
};

class CssFuncPrinter : public CssPrinter
{
public:
    CssFuncPrinter(unsigned channels, std::string prefix)
        : CssPrinter(channels, std::move(prefix))
    {
        _slash_opacity = true;
    }
};

class CssColorPrinter : public CssPrinter
{
public:
    CssColorPrinter(unsigned channels, std::string ident)
        : CssPrinter(channels, "color", std::move(ident))
    {
        _slash_opacity = true;
    }
};

} // namespace Inkscape::Colors

#endif // SEEN_COLORS_PRINTER_H

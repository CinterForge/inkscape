// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_NUMBER_OPT_NUMBER_H
#define SEEN_NUMBER_OPT_NUMBER_H

/** \file
 * <number-opt-number> implementation.
 */
/*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glib.h>
#include <glib/gprintf.h>
#include <cstdlib>

#include "svg/stringstream.h"

class NumberOptNumber
{
public:
    NumberOptNumber()
    {
        _num = 0.0;
        _set = false;
        _optnum = 0.0;
        _optset = false;
    }

    NumberOptNumber(float num)
    {
        _num = num;
        _set = true;
        _optnum = 0.0;
        _optset = false;
    }

    NumberOptNumber(float num, float optnum)
    {
        _num = num;
        _set = true;
        _optnum = optnum;
        _optset = true;
    }

    float getNumber() const
    {
        return _set ? _num : -1;
    }

    float getOptNumber(bool or_num = false) const
    {
        return _optset ? _optnum : (or_num ? _num : -1);
    }

    void setNumber(float num)
    {
        _set = true;
        _num = num;
    }

    void setOptNumber(float optnum)
    {
        _optset = optnum != -1;
        _optnum = optnum;
    }

    bool numIsSet() const
    {
        return _set;
    }

    bool optNumIsSet() const
    {
        return _optset;
    }
    
    std::string getValueString() const
    {
        Inkscape::SVGOStringStream os;

        if (_set) {
            os << _num;
            if (_optset) {
                os << " " << _optnum;
            }
        }

        return os.str();
    }

    void set(char const *str)
    {
        if (!str) {
            return;
        }

        _set = false;
        _optset = false;

        char **values = g_strsplit(str, " ", 2);

        if (values[0]) {
            _num = g_ascii_strtod(values[0], nullptr);
            _set = true;

            if (values[1]) {
                _optnum = g_ascii_strtod(values[1], nullptr);
                _optset = true;
            }
        }

        g_strfreev(values);
    }

private:
    float _num;
    float _optnum;
    bool _set : 1;
    bool _optset : 1;
};

#endif // SEEN_NUMBER_OPT_NUMBER_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

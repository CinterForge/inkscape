// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * PathString - builder for SVG path strings
 *//*
 * Authors: see git history
 * 
 * Copyright 2008 Jasper van de Gronde <th.v.d.gronde@hccnet.nl>
 * Copyright 2013 Tavmjong Bah <tavmjong@free.fr>
 * Copyright (C) 2018 Authors
 * 
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "svg/path-string.h"
#include "svg/svg.h"
#include "util/numeric/precision.h"
#include "preferences.h"

// 1<=numericprecision<=16, doubles are only accurate upto (slightly less than) 16 digits (and less than one digit doesn't make sense)
// Please note that these constants are used to allocate sufficient space to hold serialized numbers
static int const minprec = 1;
static int const maxprec = 16;

namespace Inkscape {
namespace SVG {

/**
 * Construct a path string using Inkscape's default preferences
 */
PathString::PathString()
{
    // Load the pathstring configuration from a standard set of preferences
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    _format = (PATHSTRING_FORMAT)prefs->getIntLimited("/options/svgoutput/pathstring_format", 1, 0, PATHSTRING_FORMAT_SIZE - 1 );
    _force_repeat_commands = !prefs->getBool("/options/svgoutput/disable_optimizations" ) && prefs->getBool("/options/svgoutput/forcerepeatcommands");
    int precision = std::max<int>(minprec,std::min<int>(maxprec, Util::get_default_numeric_precision()));
    int minexp = prefs->getInt("/options/svgoutput/minimumexponent", -8);
    _abs_state = State(precision, minexp);
    _rel_state = State(precision, minexp);
}

/**
 * Construct a path string, manually overriding Inkscape's preferences
 */
PathString::PathString(PATHSTRING_FORMAT format, int precision, int minexp, bool force_repeat)
    : _abs_state(precision, minexp)
    , _rel_state(precision, minexp)
    , _format(format)
    , _force_repeat_commands(force_repeat)
{
}

// For absolute and relative paths... the entire path is kept in the "tail".
// For optimized path, at a switch between absolute and relative, add tail to commonbase.
void PathString::_appendOp(char abs_op, char rel_op) {
    bool abs_op_repeated = _abs_state.prevop == abs_op && !_force_repeat_commands;
    bool rel_op_repeated = _rel_state.prevop == rel_op && !_force_repeat_commands;

    // For absolute and relative paths... do nothing.
    switch (_format) {
        case PATHSTRING_ABSOLUTE:
            if ( !abs_op_repeated ) _abs_state.appendOp(abs_op);
            break;
        case PATHSTRING_RELATIVE:
            if ( !rel_op_repeated ) _rel_state.appendOp(rel_op);
            break;
        case PATHSTRING_OPTIMIZE:
            {
            unsigned int const abs_added_size = abs_op_repeated ? 0 : 2;
            unsigned int const rel_added_size = rel_op_repeated ? 0 : 2;
            if ( _rel_state.str.size()+2 < _abs_state.str.size()+abs_added_size ) {

                // Store common prefix
                commonbase += _rel_state.str;
                _rel_state.str.clear();
                // Copy rel to abs
                _abs_state = _rel_state;
                _abs_state.switches++;
                abs_op_repeated = false;
                // We do not have to copy abs to rel:
                //   _rel_state.str.size()+2 < _abs_state.str.size()+abs_added_size
                //   _rel_state.str.size()+rel_added_size < _abs_state.str.size()+2
                //   _abs_state.str.size()+2 > _rel_state.str.size()+rel_added_size
            } else if ( _abs_state.str.size()+2 < _rel_state.str.size()+rel_added_size ) {

                // Store common prefix
                commonbase += _abs_state.str;
                _abs_state.str.clear();
                // Copy abs to rel
                _rel_state = _abs_state;
                _abs_state.switches++;
                rel_op_repeated = false;
            }
            if ( !abs_op_repeated ) _abs_state.appendOp(abs_op);
            if ( !rel_op_repeated ) _rel_state.appendOp(rel_op);
            }
            break;
        default:
            std::cerr << "Better not be here!" << std::endl;
    }
}

void PathString::State::append(Geom::Coord v) {
    str += ' ';
    appendNumber(v, _precision, _minexp);
}

void PathString::State::append(Geom::Point p) {
    str += ' ';
    appendNumber(p[Geom::X], _precision, _minexp);
    str += ',';
    appendNumber(p[Geom::Y], _precision, _minexp);
}

void PathString::State::append(Geom::Coord v, Geom::Coord &rv, Geom::Dim2 dim, bool set_current)
{
    str += ' ';
    appendNumber(v, rv);
    if (set_current) {
        current_pos[dim] = rv;
    }
}

void PathString::State::append(Geom::Point p, Geom::Point &rp, bool set_current)
{
    str += ' ';
    appendNumber(p[Geom::X], rp[Geom::X]);
    str += ',';
    appendNumber(p[Geom::Y], rp[Geom::Y]);
    if (set_current) {
        current_pos = rp;
    }
}

// NOTE: The following appendRelativeCoord function will not be exact if the total number of digits needed
// to represent the difference exceeds the precision of a double. This is not very likely though, and if
// it does happen the imprecise value is not likely to be chosen (because it will probably be a lot longer
// than the absolute value).

// NOTE: This assumes v and r are already rounded (this includes flushing to zero if they are < 10^minexp)
void PathString::State::appendRelativeCoord(Geom::Coord v, Geom::Dim2 dim, bool set_current)
{
    Geom::Coord r = current_pos[dim];

    int const minexp = _minexp - _precision + 1;
    int const digitsEnd = (int)floor(log10(std::min(fabs(v),fabs(r)))) - _precision; // Position just beyond the last significant digit of the smallest (in absolute sense) number
    double const roundeddiff = floor((v-r)*pow(10.,-digitsEnd-1)+.5);
    int const numDigits = (int)floor(log10(fabs(roundeddiff)))+1; // Number of digits in roundeddiff

    double diff = 0;
    if (r == 0 || v == 0) {
        appendNumber(v - r, _precision, minexp, diff);
    } else if (numDigits > 0) {
        appendNumber(v - r, numDigits, minexp, diff);
    } else {
        // This assumes the input numbers are already rounded to 'precision' digits
        str += '0';
    }
    if (set_current) {
        current_pos[dim] += diff;
    }
}

void PathString::State::appendRelative(Geom::Point p, bool set_current)
{
    str += ' ';
    appendRelativeCoord(p[Geom::X], Geom::X, set_current);
    str += ',';
    appendRelativeCoord(p[Geom::Y], Geom::Y, set_current);
}

void PathString::State::closePath()
{
    current_pos = initial_point;
}

void PathString::State::setInitialPoint()
{
    initial_point = current_pos;
}

void PathString::State::appendRelative(Geom::Coord v, Geom::Dim2 d, bool set_current)
{
    str += ' ';
    appendRelativeCoord(v, d, set_current);
}

void PathString::State::appendNumber(double v, int precision, int minexp)
{
    str.append(sp_svg_number_write_de(v, precision, minexp));
}

void PathString::State::appendNumber(double v, double &rv)
{
    appendNumber(v, _precision, _minexp, rv);
}

void PathString::State::appendNumber(double v, int precision, int minexp, double &result_out)
{
    size_t const oldsize = str.size();
    appendNumber(v, precision, minexp);
    auto begin_of_num = str.c_str() + oldsize; // Slightly evil, I know (but std::string should be storing its
                                               // data in one big block of memory, so...)
    sp_svg_number_read_d(begin_of_num, &result_out);
}
}}

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

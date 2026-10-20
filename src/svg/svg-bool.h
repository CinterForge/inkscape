// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_SVG_BOOL_H
#define SEEN_SP_SVG_BOOL_H
/*
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2021 Martin Owens
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glib.h>

class SVGBool {
public:
    SVGBool(bool default_value);

    operator bool() const { return _is_set ? _value : _default; }

    bool read(gchar const *str);
    void unset();
    void readOrUnset(gchar const *str);

private:
    bool _is_set = false;
    bool _value = false;
    bool _default = false;
};

#endif // SEEN_SP_SVG_BOOL_H

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

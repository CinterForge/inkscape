// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_SVG_BOX_H
#define SEEN_SP_SVG_BOX_H
/*
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2022 Martin Owens
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glib.h>
#include <optional>
#include "svg/svg-length.h"
#include <2geom/transforms.h>
#include <2geom/rect.h>

enum BoxSide {
    BOX_TOP,
    BOX_RIGHT,
    BOX_BOTTOM,
    BOX_LEFT
};

class SVGBox {
public:
    SVGBox();
    SVGBox(Geom::OptRect box) { set(box); }

    bool read(const std::string &value, const Geom::Scale &doc_scale);
    void unset();
    void readOrUnset(gchar const *str, const Geom::Scale &doc_scale);
    void update(double em, double ex, double width, double height);

    operator bool() const { return _is_set; }

    std::string write() const;
    std::string toString(const std::string &unit, const Geom::Scale &doc_scale, std::optional<unsigned int> precision = {}, bool add_unit = true) const;
    bool fromString(const std::string &value, const std::string &unit, const Geom::Scale &doc_scale);
    bool fromString(BoxSide side, const std::string &value, const std::string &unit, const Geom::Scale &doc_scale);
    bool isZero() const;

    void set(BoxSide side, double value, bool confine = false);
    void set(double top, double right, double bottom, double left);
    void set(double top, double horz, double bottom) { set(top, horz, bottom, horz); }
    void set(double vert, double horz)               { set(vert, horz, vert, horz);  }
    void set(double size)                            { set(size, size, size, size);  }

    void set(Geom::Rect box) { set(box.top(), box.right(), box.bottom(), box.left()); }
    void set(Geom::OptRect box) {
        unset();
        if (box) {
            set(*box);
        }
    }

    double get(BoxSide side) const { return _value[side].computed; }
    SVGLength top() const { return _value[BOX_TOP]; }
    SVGLength right() const { return _value[BOX_RIGHT] ? _value[BOX_RIGHT] : top(); }
    SVGLength bottom() const { return _value[BOX_BOTTOM] ? _value[BOX_BOTTOM] : top(); }
    SVGLength left() const { return _value[BOX_LEFT] ? _value[BOX_LEFT] : right(); }

    static Geom::Dim2 get_scale_axis(BoxSide side) { return (side & 1) ? Geom::X : Geom::Y; }
private:
    bool _is_set = false;
    
    SVGLength _value[4];
};

#endif // SEEN_SP_SVG_BOX_H

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

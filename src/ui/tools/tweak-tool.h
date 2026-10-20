// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_TOOLS_TWEAK_TOOl_H
#define INKSCAPE_UI_TOOLS_TWEAK_TOOl_H

/*
 * tweaking paths without node editing
 *
 * Authors:
 *   bulia byak
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "ui/tools/tool-base.h"
#include "display/control/canvas-item-ptr.h"

#define TC_MIN_PRESSURE      0.0
#define TC_MAX_PRESSURE      1.0
#define TC_DEFAULT_PRESSURE  0.35

namespace Inkscape { class CanvasItemBpath; }

namespace Inkscape::UI::Tools {

enum {
    TWEAK_MODE_MOVE,
    TWEAK_MODE_MOVE_IN_OUT,
    TWEAK_MODE_MOVE_JITTER,
    TWEAK_MODE_SCALE,
    TWEAK_MODE_ROTATE,
    TWEAK_MODE_MORELESS,
    TWEAK_MODE_PUSH,
    TWEAK_MODE_SHRINK_GROW,
    TWEAK_MODE_ATTRACT_REPEL,
    TWEAK_MODE_ROUGHEN,
    TWEAK_MODE_COLORPAINT,
    TWEAK_MODE_COLORJITTER,
    TWEAK_MODE_BLUR
};

class TweakTool : public ToolBase
{
public:
    TweakTool(SPDesktop *desktop);
    ~TweakTool() override;

    /* extended input data */
    double pressure;

    /* attributes */
    bool usepressure;
    bool usetilt;

    double width;
    double force;
    double fidelity;

    int mode;

    bool is_drawing;

    bool is_dilating;
    bool has_dilated;
    Geom::Point last_push;
    CanvasItemPtr<CanvasItemBpath> dilate_area;

    bool do_h;
    bool do_s;
    bool do_l;
    bool do_o;

    sigc::scoped_connection style_set_connection;

    void set(Preferences::Entry const &val) override;
    bool root_handler(CanvasEvent const &event) override;
    void update_cursor(bool with_shift);

private:
    Modifiers::Modifier *mod_tweak_invert;

    bool set_style(SPCSSAttr const *css);
};

} // namespace Inkscape::UI::Tool

#endif // INKSCAPE_UI_TOOLS_TWEAK_TOOl_H

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

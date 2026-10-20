// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_TOOLS_SPRAY_TOOl_H
#define INKSCAPE_UI_TOOLS_SPRAY_TOOl_H

/*
 * Spray Tool
 *
 * Authors:
 *   Pierre-Antoine MARC
 *   Pierre CACLIN
 *   Aurel-Aimé MARMION
 *   Julien LERAY
 *   Benoît LAVORATA
 *   Vincent MONTAGNE
 *   Pierre BARBRY-BLOT
 *   Jabiertxo ARRAIZA
 *   Adrian Boguszewski
 *
 * Copyright (C) 2009 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <2geom/pathvector.h>

#include "display/control/canvas-item-ptr.h"
#include "object/object-set.h"
#include "ui/tools/tool-base.h"

namespace Inkscape { class CanvasItemBpath; }

#define TC_MIN_PRESSURE      0.0
#define TC_MAX_PRESSURE      1.0
#define TC_DEFAULT_PRESSURE  0.35

namespace Inkscape::UI::Tools {

enum
{
    SPRAY_MODE_COPY,
    SPRAY_MODE_CLONE,
    SPRAY_MODE_SINGLE_PATH,
    SPRAY_MODE_ERASER,
    SPRAY_OPTION,
};

class SprayTool : public ToolBase
{
public:
    SprayTool(SPDesktop *desktop);
    ~SprayTool() override;

    /* extended input data */
    double pressure;

    /* attributes */
    bool usepressurewidth = false;
    bool usepressurepopulation = false;;
    bool usepressurescale = false;
    bool usetilt = false;
    bool usetext = false;

    double width = 0.2;
    double ratio = 0.0;
    double tilt = 0.0;
    double rotation_variation = 0.0;
    double population = 0.0;
    double scale_variation = 1.0;
    double scale = 1.0;
    double mean = 0.2;
    double standard_deviation = 0.2;

    int distrib = 1;

    int mode = 0;

    bool is_drawing = false;

    bool is_dilating = false;
    bool has_dilated = false;
    Geom::Point last_push;
    CanvasItemPtr<CanvasItemBpath> dilate_area;
    CanvasItemPtr<CanvasItemBpath> shapes_area;
    std::vector<SPItem*> items;
    bool no_overlap = false;
    bool picker = false;
    bool pick_center = false;
    bool pick_inverse_value = false;
    bool pick_fill = false;
    bool pick_stroke = false;
    bool pick_no_overlap = false;
    bool over_transparent = true;
    bool over_no_transparent = true;
    double offset = 0.0;
    int pick = 0;
    bool do_trace = false;
    bool pick_to_size = false;
    bool pick_to_presence = false;
    bool pick_to_color = false;
    bool pick_to_opacity = false;
    bool single_click = false;
    double single_scale = 0;
    double single_angle = 0;
    double last_pressure = 0;
    bool invert_picked = false;
    double gamma_picked = 0.0;
    double rand_picked = 0.0;
    Geom::PathVector shapes;

    sigc::scoped_connection release_connection;

    void set(Preferences::Entry const &val) override;
    virtual void setCloneTilerPrefs();
    bool root_handler(CanvasEvent const &event) override;
    void update_cursor();

    ObjectSet *objectSet() { return &object_set; }
    SPItem *single_path_output = nullptr;

private:
    ObjectSet object_set;
};

} // namespace Inkscape::UI::Tools

#endif // INKSCAPE_UI_TOOLS_SPRAY_TOOl_H

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


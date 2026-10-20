// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_TOOLS_CALLIGRAPHIC_TOOL_H
#define INKSCAPE_UI_TOOLS_CALLIGRAPHIC_TOOL_H

/*
 * Handwriting-like drawing mode
 *
 * Authors:
 *   Mitsuru Oka <oka326@parkcity.ne.jp>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * The original dynadraw code:
 *   Paul Haeberli <paul@sgi.com>
 *
 * Copyright (C) 1998 The Free Software Foundation
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <deque>
#include <2geom/pathvector.h>
#include "ui/modifiers.h"
#include "ui/tools/dynamic-base.h"

class SPItem;
class Path;

namespace Inkscape {

class CanvasItemBpath;
struct ExtendedInput;

namespace UI {
namespace Tools {

class CalligraphicTool : public DynamicBase
{
public:
    CalligraphicTool(SPDesktop *desktop);
    ~CalligraphicTool() override;

    void set(Preferences::Entry const &val) override;
    bool root_handler(CanvasEvent const &event) override;

private:
    /** newly created objects remain selected */
    bool keep_selected = true;

    double hatch_spacing = 0.0;
    double hatch_spacing_step = 0.0;
    SPItem *hatch_item = nullptr;
    Geom::PathVector hatch_path;
    std::deque<double> hatch_nearest_past;
    std::deque<double> hatch_pointer_past;
    std::deque<Geom::Point> inertia_vectors;
    Geom::Point hatch_last_nearest, hatch_last_pointer;
    std::deque<Geom::Point> hatch_vectors;
    bool hatch_escaped = false;
    CanvasItemPtr<CanvasItemBpath> hatch_area;
    bool just_started_drawing = false;
    bool trace_bg = false;

    Modifiers::Modifier *mod_calli_hatching;
    Modifiers::Modifier *mod_calli_subtract;
    Modifiers::Modifier *mod_calli_unionize;

	void clear_current();
	void set_to_accumulated(bool unionize, bool subtract);
	bool accumulate();
	void fit_and_split(bool release);
	void draw_temporary_box();
	void cancel();
	void brush();
    bool apply(Geom::Point const &p);
    void extinput(ExtendedInput const &ext);
    void reset(Geom::Point const &p);
};

} // namespace Tools
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_TOOLS_CALLIGRAPHIC_TOOL_H

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

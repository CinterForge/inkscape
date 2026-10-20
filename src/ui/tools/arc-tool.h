// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_TOOLS_ARC_TOOl_H
#define INKSCAPE_UI_TOOLS_ARC_TOOl_H

/*
 * Ellipse drawing context
 *
 * Authors:
 *   Mitsuru Oka
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2000-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2002 Mitsuru Oka
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "ui/tools/tool-base.h"
#include "object/weakptr.h"
#include "ui/modifiers.h"

class SPItem;
class SPGenericEllipse;
namespace Inkscape { class Selection; }

namespace Inkscape::UI::Tools {

class ArcTool : public ToolBase
{
public:
    ArcTool(SPDesktop *desktop);
    ~ArcTool() override;

    bool root_handler(CanvasEvent const &event) override;
    bool item_handler(SPItem *item, CanvasEvent const &event) override;

private:
    SPWeakPtr<SPGenericEllipse> arc;

    Geom::Point center;

    sigc::connection sel_changed_connection;

    Modifiers::Modifier *mod_select_add_to;
    Modifiers::Modifier *mod_trans_confine;
    Modifiers::Modifier *mod_trans_increment;
    Modifiers::Modifier *mod_trans_off_center;

    void selection_changed(Selection *selection);

    void drag(Geom::Point const &pt, unsigned state);
	void finishItem();
	void cancel();
};

} // namespace Inkscape::UI::Tools

#endif // INKSCAPE_UI_TOOLS_ARC_TOOl_H

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

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Add customizable shortcuts for tools
 *
 * Authors:
 *   Martin Owens
 *
 * Copyright (C) 2024 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "shortcuts.h"

#include <glibmm/i18n.h>

#include "inkscape-application.h"

std::vector<std::vector<Glib::ustring>> raw_data_tools_shortcuts = {
    // clang-format off
      {"tool.all.quick-preview",    N_("Quick Preview"),          "Tools", N_("Preview how the document will look while the key is pressed.")     }
    , {"tool.all.quick-zoom",       N_("Quick Zoom"),             "Tools", N_("Zoom into the selected objects while the key is pressed.")         }
    , {"tool.all.quick-pan",        N_("Quick Pan Canvas"),       "Tools", N_("Pan the canvas with the mouse while the key is pressed.")          }
    , {"tool.all.focus-first-widget", N_("Focus First Widget"),   "Tools", N_("Focus the first input widget in the active tool's toolbar.")       }

    , {"tool.sel.stkey-grab",       N_("Grab/Move Objects"),      "Tools", N_("Move the objects even if the mouse is not over the selected object.")}
    , {"tool.sel.stkey-scale",      N_("Scale Objects"),          "Tools", N_("Resize or scale selected objects without using the scaling handles.")}
    , {"tool.sel.stkey-rotate",     N_("Rotate Objects"),         "Tools", N_("Rotate the selected objects without using the rotate handles.")}

    , {"tool.pen.to-line",          N_("Pen Segment To Line"),    "Tools", N_("Convert the last pen segment to a straight line.")                 }
    , {"tool.pen.to-curve",         N_("Pen Segment To Curve"),   "Tools", N_("Convert the last pen segment to a curved line.")                   }
    , {"tool.pen.to-guides",        N_("Pen Segments To Guides"), "Tools", N_("Convert the pen shape into guides.")                               }
    // clang-format on
};

/**
 * Add a bunch of tool specific names to the action data which the tool
 * will handle manually and aren't tied to an actual action.
 */
void init_tool_shortcuts(InkscapeApplication* app)
{
    app->get_action_extra_data().add_data(raw_data_tools_shortcuts);
}


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

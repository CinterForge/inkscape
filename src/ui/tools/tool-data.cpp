// SPDX-License-Identifier: GPL-2.0-or-later
#include "tool-data.h"

#include <glibmm/i18n.h>
#include "ui/dialog/inkscape-preferences.h"
#include "ui/modifiers.h"

namespace Modifiers = Inkscape::Modifiers;

std::map<Glib::ustring, ToolData> const &get_tool_data()
{
    static std::map<Glib::ustring, ToolData> tool_data = {
        // clang-format off
        {"Select",       {TOOLS_SELECT,          PREFS_PAGE_TOOLS_SELECTOR,       "/tools/select",          }},
        {"Node",         {TOOLS_NODES,           PREFS_PAGE_TOOLS_NODE,           "/tools/nodes",           }},
        {"Booleans",     {TOOLS_BOOLEANS,        PREFS_PAGE_TOOLS,/*No Page*/     "/tools/booleans",        }},
        {"Marker",       {TOOLS_MARKER,          PREFS_PAGE_TOOLS,/*No Page*/     "/tools/marker",          }},
        {"Rect",         {TOOLS_SHAPES_RECT,     PREFS_PAGE_TOOLS_SHAPES_RECT,    "/tools/shapes/rect",     }},
        {"Arc",          {TOOLS_SHAPES_ARC,      PREFS_PAGE_TOOLS_SHAPES_ELLIPSE, "/tools/shapes/arc",      }},
        {"Star",         {TOOLS_SHAPES_STAR,     PREFS_PAGE_TOOLS_SHAPES_STAR,    "/tools/shapes/star",     }},
        {"3DBox",        {TOOLS_SHAPES_3DBOX,    PREFS_PAGE_TOOLS_SHAPES_3DBOX,   "/tools/shapes/3dbox",    }},
        {"Spiral",       {TOOLS_SHAPES_SPIRAL,   PREFS_PAGE_TOOLS_SHAPES_SPIRAL,  "/tools/shapes/spiral",   }},
        {"Pencil",       {TOOLS_FREEHAND_PENCIL, PREFS_PAGE_TOOLS_PENCIL,         "/tools/freehand/pencil", }},
        {"Pen",          {TOOLS_FREEHAND_PEN,    PREFS_PAGE_TOOLS_PEN,            "/tools/freehand/pen",    }},
        {"Calligraphic", {TOOLS_CALLIGRAPHIC,    PREFS_PAGE_TOOLS_CALLIGRAPHY,    "/tools/calligraphic",    }},
        {"Text",         {TOOLS_TEXT,            PREFS_PAGE_TOOLS_TEXT,           "/tools/text",            }},
        {"Gradient",     {TOOLS_GRADIENT,        PREFS_PAGE_TOOLS_GRADIENT,       "/tools/gradient",        }},
        {"Mesh",         {TOOLS_MESH,            PREFS_PAGE_TOOLS, /* No Page */  "/tools/mesh",            }},
        {"Zoom",         {TOOLS_ZOOM,            PREFS_PAGE_TOOLS_ZOOM,           "/tools/zoom",            }},
        {"Measure",      {TOOLS_MEASURE,         PREFS_PAGE_TOOLS_MEASURE,        "/tools/measure",         }},
        {"Dropper",      {TOOLS_DROPPER,         PREFS_PAGE_TOOLS_DROPPER,        "/tools/dropper",         }},
        {"Tweak",        {TOOLS_TWEAK,           PREFS_PAGE_TOOLS_TWEAK,          "/tools/tweak",           }},
        {"Spray",        {TOOLS_SPRAY,           PREFS_PAGE_TOOLS_SPRAY,          "/tools/spray",           }},
        {"Connector",    {TOOLS_CONNECTOR,       PREFS_PAGE_TOOLS_CONNECTOR,      "/tools/connector",       }},
        {"PaintBucket",  {TOOLS_PAINTBUCKET,     PREFS_PAGE_TOOLS_PAINTBUCKET,    "/tools/paintbucket",     }},
        {"Eraser",       {TOOLS_ERASER,          PREFS_PAGE_TOOLS_ERASER,         "/tools/eraser",          }},
        {"LPETool",      {TOOLS_LPETOOL,         PREFS_PAGE_TOOLS, /* No Page */  "/tools/lpetool",         }},
        {"Pages",        {TOOLS_PAGES,           PREFS_PAGE_TOOLS,                "/tools/pages",           }},
        {"Picker",       {TOOLS_PICKER,          PREFS_PAGE_TOOLS, /* No Page */  "/tools/picker",          }}
        // clang-format on
    };
    return tool_data;
}

std::map<Glib::ustring, Glib::ustring> const &get_tool_msg()
{
    static std::map<Glib::ustring, Glib::ustring> tool_msg = {
        // clang-format off
        {"Select",       _("<b>Click</b> to Select and Transform objects, <b>Drag</b> to select many objects.")                                                                                                                   },
        {"Node",         _("Modify selected path points (nodes) directly.")                                                                                                                                                       },
        {"Booleans",     _("Construct shapes with the interactive Boolean tool.")                                                                                                                                                 },
        {"Rect",         _("<b>Drag</b> to create a rectangle. <b>Drag controls</b> to round corners and resize. <b>Click</b> to select.")                                                                                        },
        {"Arc",          _("<b>Drag</b> to create an ellipse. <b>Drag controls</b> to make an arc or segment. <b>Click</b> to select.")                                                                                           },
        {"Star",         _("<b>Drag</b> to create a star. <b>Drag controls</b> to edit the star shape. <b>Click</b> to select.")                                                                                                  },
        {"3DBox",        _("<b>Drag</b> to create a 3D box. <b>Drag controls</b> to resize in perspective. <b>Click</b> to select (with <b>Ctrl+Alt</b> for single faces).")                                                      },
        {"Spiral",       _("<b>Drag</b> to create a spiral. <b>Drag controls</b> to edit the spiral shape. <b>Click</b> to select.")                                                                                              },
        {"Marker",       _("<b>Click</b> a shape to start editing its markers. <b>Drag controls</b> to change orientation, scale, and position.")                                                                                 },
        {"Pencil",       _("<b>Drag</b> to create a freehand line. <b>Shift</b> appends to selected path, <b>Alt</b> activates sketch mode.")                                                                                     },
        {"Pen",          _("<b>Click</b> or <b>click and drag</b> to start a path; with <b>Shift</b> to append to selected path. <b>Ctrl+click</b> to create single dots (straight line modes only).")                            },
        {"Calligraphic", Glib::ustring::compose(_("<b>Drag</b> to draw a calligraphic stroke; with <b>%1</b> to track a guide path. <b>Arrow keys</b> adjust width (left/right) and angle (up/down)."),
                                                Modifiers::Modifier::get(Modifiers::Type::CALLI_HATCHING)->get_label())},
        {"Text",         _("<b>Click</b> to select or create text, <b>drag</b> to create flowed text; then type.")                                                                                                                },
        {"Gradient",     _("<b>Drag</b> or <b>double click</b> to create a gradient on selected objects, <b>drag handles</b> to adjust gradients.")                                                                               },
        {"Mesh",         _("<b>Drag</b> or <b>double click</b> to create a mesh on selected objects, <b>drag handles</b> to adjust meshes.")                                                                                      },
        {"Zoom",         Glib::ustring::compose(_("<b>Click</b> or <b>drag around an area</b> to zoom in, <b>%1+Click</b> to zoom out."),
                                                Modifiers::Modifier::get(Modifiers::Type::CANVAS_ZOOM_INVERT)->get_label())},
        {"Measure",      _("<b>Drag</b> to measure the dimensions of objects.  Press <b>Alt+C</b> to copy the length to the clipboard.")                                                                                          },
        {"Dropper",      Glib::ustring::compose(_("<b>Click</b> to set fill, <b>%1+Click</b> to set stroke; <b>drag</b> to average color in area; with <b>%2</b> to pick inverse color; <b>Ctrl+C</b> to copy the color under mouse to clipboard"),
                                                Modifiers::Modifier::get(Modifiers::Type::DROPPER_STROKE)->get_label(),
                                                Modifiers::Modifier::get(Modifiers::Type::DROPPER_INVERT)->get_label())},
        {"Tweak",        _("To tweak a path by pushing, select it and drag over it.")                                                                                                                                             },
        {"Spray",        _("<b>Drag</b>, <b>click</b> or <b>click and scroll</b> to spray the selected objects.")                                                                                                                 },
        {"Connector",    _("<b>Click and drag</b> between shapes to create a connector.")                                                                                                                                         },
        {"PaintBucket",  Glib::ustring::compose(_("<b>Click</b> to paint a bounded area, <b>%1+Click</b> to union the new fill with the current selection, <b>%2+Click</b> to change the clicked object's fill and stroke to the current setting."),
                                                Modifiers::Modifier::get(Modifiers::Type::SELECT_ADD_TO)->get_label(),
                                                Modifiers::Modifier::get(Modifiers::Type::FLOOD_ITEM)->get_label())},
        {"Eraser",       _("<b>Drag</b> to erase.")                                                                                                                                                                               },
        {"LPETool",      _("Choose a subtool from the toolbar")                                                                                                                                                                   },
        {"Pages",        _("Create and manage pages.")},
        {"Picker",       _("Pick objects.")}
        // clang-format on
    };
    return tool_msg;
}

Glib::ustring const &pref_path_to_tool_name(Glib::UStringView pref_path)
{
    auto &data = get_tool_data();

    // Todo: Would prefer to identify tools by their enum, not by their name
    // or prefs path. Then none of this function would even be necessary.
    for (auto &[name, data] : data) {
        if (data.pref_path == pref_path) {
            return name;
        }
    }

    static Glib::ustring const empty;
    return empty;
}

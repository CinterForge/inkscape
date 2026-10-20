// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_CONNECTOR_CONTEXT_H
#define SEEN_CONNECTOR_CONTEXT_H

/*
 * Connector creation tool
 *
 * Authors:
 *   Michael Wybrow <mjwybrow@users.sourceforge.net>
 *
 * Copyright (C) 2005 Michael Wybrow
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <2geom/pathvector.h>

#include "object/weakptr.h"
#include "ui/modifiers.h"
#include "ui/tools/tool-base.h"
#include "xml/node-observer.h"

class SPItem;
class SPKnot;

namespace Avoid {
    class ConnRef;
}

namespace Inkscape {
class CanvasItemBpath;
class Selection;
namespace XML { class Node; }
struct ButtonPressEvent;
struct MotionEvent;
struct ButtonReleaseEvent;
} // namespace Inkscape

#define SP_CONNECTOR_CONTEXT(obj) (dynamic_cast<Inkscape::UI::Tools::ConnectorTool*>((Inkscape::UI::Tools::ToolBase*)obj))

enum {
    SP_CONNECTOR_CONTEXT_IDLE,
    SP_CONNECTOR_CONTEXT_DRAGGING,
    SP_CONNECTOR_CONTEXT_CLOSE,
    SP_CONNECTOR_CONTEXT_STOP,
    SP_CONNECTOR_CONTEXT_REROUTING,
    SP_CONNECTOR_CONTEXT_NEWCONNPOINT
};

using SPKnotList = std::map<SPKnot *, int>;

namespace Inkscape::UI::Tools {

class ConnectorTool;

class CCToolShapeNodeObserver : public Inkscape::XML::NodeObserver
{
    friend class ConnectorTool;
    ~CCToolShapeNodeObserver() override = default; // can only exist as a direct base of ConnectorTool

    void notifyAttributeChanged(Inkscape::XML::Node &, GQuark, Util::ptr_shared, Util::ptr_shared) final;
};

class CCToolLayerNodeObserver : public Inkscape::XML::NodeObserver
{
    friend class ConnectorTool;
    ~CCToolLayerNodeObserver() override = default; // can only exist as a direct base of ConnectorTool

    void notifyChildRemoved(Inkscape::XML::Node &, Inkscape::XML::Node &, Inkscape::XML::Node *) final;
};

class ConnectorTool
    : public ToolBase
    , private CCToolShapeNodeObserver
    , private CCToolLayerNodeObserver
{
public:
    ConnectorTool(SPDesktop *desktop);
    ~ConnectorTool() override;

    Inkscape::Selection *selection{nullptr};
    Geom::Point p[5];

    /** \invar npoints in {0, 2}. */
    gint npoints{0};
    unsigned int state : 4;

    // Red curve
    Inkscape::CanvasItemBpath *red_bpath{nullptr};
    std::optional<Geom::PathVector> red_curve;
    uint32_t red_color{0xff00007f};

    // Green curve
    std::optional<Geom::PathVector> green_curve;

    // The new connector
    SPItem *newconn{nullptr};
    Avoid::ConnRef *newConnRef{nullptr};
    gdouble curvature{0.0};
    bool isOrthogonal{false};

    // The active shape
    SPItem *active_shape{nullptr};
    Inkscape::XML::Node *active_shape_repr{nullptr};
    Inkscape::XML::Node *active_shape_layer_repr{nullptr};

    // Same as above, but for the active connector
    SPWeakPtr<SPItem> active_conn;
    Inkscape::XML::Node *active_conn_repr{nullptr};
    sigc::connection sel_changed_connection;

    // The activehandle
    SPKnot *active_handle{nullptr};

    // The selected handle, used in editing mode
    SPKnot *selected_handle{nullptr};

    SPItem *clickeditem{nullptr};
    SPKnot *clickedhandle{nullptr};

    SPKnotList knots;
    SPKnot *endpt_handle[2]{};
    sigc::connection endpt_handler_connection[2];
    gchar *shref{nullptr};
    gchar *sub_shref{nullptr};
    gchar *ehref {nullptr};
    gchar *sub_ehref{nullptr};

    void set(Preferences::Entry const &val) override;
    bool root_handler(CanvasEvent const &event) override;
    bool item_handler(SPItem* item, CanvasEvent const &event) override;

    void cc_clear_active_shape();
    void cc_set_active_conn(SPItem *item);
    void cc_clear_active_conn();

private:
    void _selectionChanged(Inkscape::Selection *selection);

    bool _handleButtonPress(ButtonPressEvent const &bevent);
    bool _handleMotionNotify(MotionEvent const &mevent);
    bool _handleButtonRelease(ButtonReleaseEvent const &revent);
    bool _handleKeyPress(guint const keyval);

    void _setInitialPoint(Geom::Point const p);
    void _setSubsequentPoint(Geom::Point const p);
    void _finishSegment(Geom::Point p);
    void _resetColors();
    void _finish();
    void _concatColorsAndFlush();
    void _flushWhite(Geom::PathVector &c);

    void _activeShapeAddKnot(SPItem* item, SPItem* subitem);
    void _setActiveShape(SPItem *item);
    bool _ptHandleTest(Geom::Point& p, gchar **href, gchar **subhref);

    void _reroutingFinish(Geom::Point *const p);

    CCToolShapeNodeObserver &shapeNodeObserver() { return *this; }
    CCToolLayerNodeObserver &layerNodeObserver() { return *this; }
    friend CCToolShapeNodeObserver;
    friend CCToolLayerNodeObserver;

    Modifiers::Modifier *mod_select_add_to;
    Modifiers::Modifier *mod_select_force_drag;
};

void cc_selection_set_avoid(SPDesktop *, bool const set_ignore);
void cc_create_connection_point(ConnectorTool* cc);
void cc_remove_connection_point(ConnectorTool* cc);
bool cc_item_is_connector(SPItem *item);

} // namespace Inkscape::UI::Tools

#endif /* !SEEN_CONNECTOR_CONTEXT_H */

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

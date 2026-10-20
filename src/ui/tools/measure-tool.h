// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Our fine measuring tool
 *
 * Authors:
 *   Felipe Correa da Silva Sanches <juca@members.fsf.org>
 *   Jabiertxo Arraiza <jabier.arraiza@marker.es>
 * Copyright (C) 2011 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_TOOLS_MEASURE_TOOL_H
#define INKSCAPE_UI_TOOLS_MEASURE_TOOL_H

#include "display/control/canvas-item-enums.h"
#include "display/control/canvas-item-ptr.h"
#include "display/control/canvas-item-bpath.h"
#include "ui/tools/tool-base.h"
#include <glibmm/i18n.h>

class SPKnot;
class SPShape;
namespace Inkscape { class CanvasItemCurve; }

namespace Inkscape::UI::Tools {

namespace MT {  // using namespace not to interfere with any other tool

// indexes for the maps lengths labels symbols measureIsSet in std::vector<ClipboardMeaClass::Measure>
// created outside the class otherwise we would have had to use te class name everytime
enum LengthIDs
{
    SEGMENT,
    LENGTH,
    LENGTH_BETWEEN,
    DX,
    DY,
    ANGLE,
    SHAPE_LENGTH,
    SHAPE_WIDTH,
    SHAPE_HEIGHT,
    SHAPE_X,
    SHAPE_Y
};

/**
 * This class stores the measurements that are copied to the clipboard.
 * There are 3 maps for the lengths, labels and symbols.
 * The maps are similar and can all be accessed with LengthIDs as indexes.
 * The symbols (some of them currently) are used to label the measurements on the desktop (long labels would clutter the 
 * screen even more...)
 * 
 */
class ClipboardMeaClass
{
public:
    ClipboardMeaClass();
    ~ClipboardMeaClass();
   
    std::map<int,Glib::ustring> symbols {  // using to show on the workspace/canvas
        {LengthIDs::SEGMENT, "S"},  
        {LengthIDs::LENGTH, "L"},
        {LengthIDs::LENGTH_BETWEEN, "lb"},
        {LengthIDs::DX, "dX"},
        {LengthIDs::DY, "dY"},
        {LengthIDs::ANGLE, "α"},
        {LengthIDs::SHAPE_LENGTH, "sl"},
        {LengthIDs::SHAPE_WIDTH, "sw"},
        {LengthIDs::SHAPE_HEIGHT, "sh"},
        {LengthIDs::SHAPE_X, "sX"},
        {LengthIDs::SHAPE_Y, "sY"},
    };
    std::map<int,Glib::ustring> labels {
        {LengthIDs::SEGMENT, _("Segment")},
        {LengthIDs::LENGTH, _("Length")},
        {LengthIDs::LENGTH_BETWEEN, _("Length between")},
        {LengthIDs::DX, _("dX")},
        {LengthIDs::DY, _("dY")},
        {LengthIDs::ANGLE, _("Angle")},
        {LengthIDs::SHAPE_LENGTH, _("Shape length")},
        {LengthIDs::SHAPE_WIDTH,  _("Shape width")},
        {LengthIDs::SHAPE_HEIGHT, _("Shape height")},
        {LengthIDs::SHAPE_X, _("Shape X")},
        {LengthIDs::SHAPE_Y, _("Shape Y")},
    };
        std::map<int,bool> measureIsSet {  // if set to true the measurements are set and can be used
        {LengthIDs::SEGMENT, false},
        {LengthIDs::LENGTH, false},
        {LengthIDs::LENGTH_BETWEEN, false},
        {LengthIDs::DX, false},
        {LengthIDs::DY, false},
        {LengthIDs::ANGLE, false},
        {LengthIDs::SHAPE_LENGTH, false},
        {LengthIDs::SHAPE_WIDTH, false},
        {LengthIDs::SHAPE_HEIGHT, false},
        {LengthIDs::SHAPE_X, false},
        {LengthIDs::SHAPE_Y, false},
    };
    std::map<int,double> lengths {
        {LengthIDs::SEGMENT, 0}, // not used - just keeping maps the same...
        {LengthIDs::LENGTH, 0},
        {LengthIDs::LENGTH_BETWEEN, 0},
        {LengthIDs::DX, 0},
        {LengthIDs::DY, 0},
        {LengthIDs::ANGLE, 0},
        {LengthIDs::SHAPE_LENGTH, 0},
        {LengthIDs::SHAPE_WIDTH, 0},
        {LengthIDs::SHAPE_HEIGHT, 0},
        {LengthIDs::SHAPE_X, 0},
        {LengthIDs::SHAPE_Y, 0},
    };
    std::map<Glib::ustring,double> segLengths; // this is dynamic

// composes the String without checking - need to check that the measurements are set (updated) before using it
    Glib::ustring composeMeaStr(int id, int precision, Glib::ustring unit, bool withLabel = true, bool withUnit = true, bool tabSeparated = false);
    void unsetShapeMeasures();
};

} // namespace MT


class PathMeasure
{
public:
    enum class Mode {
        NONE,
        SEGMENT,
        CORNER
    };

    Mode mode = Mode::NONE;
    int subpath_index = -1;
    int segment_index = -1;
    int corner_index = -1;

    PathMeasure() = default;
    PathMeasure(SPShape *const shape, Geom::Point const &near_point, double tolerance);

    operator bool() const { return _shape; }
    bool operator== (PathMeasure const &other) const
    {
        return other.mode == mode
            && other.subpath_index == subpath_index
            && other.segment_index == segment_index
            && other.corner_index == corner_index
            && other._shape == _shape;
    }

    Geom::Path subpath() const;
    Geom::Path segmentPath() const;
    Geom::Path curvePath(double tolerance) const;
    Geom::PathVector getAnglePath(double arm_size, double arc_size, Geom::Affine affine, bool inside_arc = true);
    std::optional<Geom::Angle> getCornerAngle() const;
    std::optional<std::array<Geom::Point, 3>> getCornerAnglePoints() const;

private:

    SPShape *_shape = nullptr;
    Geom::PathVector _curve;
    mutable std::optional<std::array<Geom::Point, 3>> _corner_points;

    static std::optional<std::array<Geom::Point, 3>> getAnglePoints(Geom::Curve const &c1, Geom::Curve const &c2);
    static Geom::Angle getAngle(Geom::Curve const &c1, Geom::Curve const &c2);
    static Geom::Angle getAngle(std::array<Geom::Point, 3> p);
    static Geom::Point getCurvePoint(Geom::Curve const &c, bool end);
};


class MeasureTool : public ToolBase
{
public:
    MeasureTool(SPDesktop *desktop);
    ~MeasureTool() override;

    bool root_handler(CanvasEvent const &event) override;
    void showCanvasItems(bool to_guides = false, bool to_item = false, bool to_phantom = false, Inkscape::XML::Node *measure_repr = nullptr);
    void reverseKnots();
    void toGuides();
    void toPhantom();
    void toMarkDimension();
    void toItem();
    void reset();
    void setMarkers();
    void setMarker(bool isStart);
    Geom::Point readMeasurePoint(bool is_start) const;
    void writeMeasurePoint(Geom::Point point, bool is_start) const;

    void showInfoBox(Geom::Point cursor, bool into_groups);
    void addCanvasItemText(std::vector<CanvasItemPtr<CanvasItem>> &items, Geom::Point pos, Glib::ustring const &measure_str, double fontsize, Geom::Point anchor = {});
    void setGuide(Geom::Point origin, double angle, const char *label);
    void setPoint(Geom::Point origin, Inkscape::XML::Node *measure_repr);
    void setLine(Geom::Point start_point,Geom::Point end_point, bool markers, guint32 color,
                 Inkscape::XML::Node *measure_repr = nullptr);
    void setMeasureCanvasText(bool is_angle, double precision, double amount, double fontsize,
                              Glib::ustring unit_name, Geom::Point position, guint32 background,
                              bool to_left, bool to_item, bool to_phantom,
                              Inkscape::XML::Node *measure_repr, Glib::ustring label = "");
    void setMeasureCanvasItem(Geom::Point position, bool to_item, bool to_phantom,
                              Inkscape::XML::Node *measure_repr);
    void setMeasureCanvasControlLine(Geom::Point start, Geom::Point end, bool to_item, bool to_phantom,
                                     Inkscape::CanvasItemColor color, Inkscape::XML::Node *measure_repr);
    void setLabelText(Glib::ustring const &value, Geom::Point pos, double fontsize, Geom::Coord angle,
                      guint32 background,
                      Inkscape::XML::Node *measure_repr = nullptr);

    void knotStartMovedHandler(SPKnot */*knot*/, Geom::Point const &ppointer, guint state);
    void knotEndMovedHandler(SPKnot */*knot*/, Geom::Point const &ppointer, guint state);
    void knotClickHandler(SPKnot *knot, guint state);
    void knotUngrabbedHandler(SPKnot */*knot*/,  unsigned int /*state*/);
    void setMeasureItem(Geom::PathVector pathv, bool is_curve, bool markers, guint32 color, Inkscape::XML::Node *measure_repr);
    void createAngleDisplayCurve(Geom::Point const &center, Geom::Point const &end, Geom::Point const &anchor,
                                 double angle, bool to_phantom,
                                 Inkscape::XML::Node *measure_repr = nullptr);
    void copyToClipboard();
    std::vector<Geom::Point> calcDeltaBasePoint(double dX, double dY);
    bool equalWithinRange(double value, double reference_value, double epsilon, bool positiveAllowed = true, bool negativeAllowed = true);
    MT::ClipboardMeaClass clipBMeas;

private:

    std::optional<Geom::Point> explicit_base;
    std::optional<Geom::Point> last_end;
    SPKnot *knot_start = nullptr;
    SPKnot *knot_end   = nullptr;
    int dimension_offset = 20;
    Geom::Point start_p;
    Geom::Point end_p;
    Geom::Point last_pos;

    std::vector<CanvasItemPtr<CanvasItem>> measure_tmp_items;
    std::vector<CanvasItemPtr<CanvasItem>> measure_phantom_items;
    std::vector<CanvasItemPtr<CanvasItem>> measure_item;
    std::vector<CanvasItemPtr<CanvasItem>> measure_path_items;
    std::vector<CanvasItemPtr<CanvasItem>> measure_clicked_path_items;

    double item_width;
    double item_height;
    double item_x;
    double item_y;
    double item_length;
    double path_length;
    double curve_length;
    double segment_length;
    double corner_angle;
    double _tooltip_border_size = 3.0;
    SPItem *over;
    std::unique_ptr<PathMeasure> pathmeasure;
    sigc::scoped_connection _knot_start_moved_connection;
    sigc::scoped_connection _knot_start_ungrabbed_connection;
    sigc::scoped_connection _knot_start_click_connection;
    sigc::scoped_connection _knot_end_moved_connection;
    sigc::scoped_connection _knot_end_click_connection;
    sigc::scoped_connection _knot_end_ungrabbed_connection;

    Modifiers::Modifier *mod_freehand_angle_snapping;
    Modifiers::Modifier *mod_measure_knot_dialog;
    Modifiers::Modifier *mod_measure_select_segment;
    Modifiers::Modifier *mod_move_no_snapping;
    Modifiers::Modifier *mod_select_in_groups;
};

} // namespace Inkscape::UI::Tools

#endif // INKSCAPE_UI_TOOLS_MEASURE_TOOL_H

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

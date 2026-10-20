// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape SPGrid implementation
 *
 * Authors:
 * James Ferrarelli
 * Johan Engelen <johan@shouraizou.nl>
 * Lauris Kaplinski
 * Abhishek Sharma
 * Jon A. Cruz <jon@joncruz.org>
 * Tavmong Bah <tavmjong@free.fr>
 * see git history
 *
 * Copyright (C) 2022 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_SP_GRID_H_
#define SEEN_SP_GRID_H_

#include "colors/color.h"
#include "display/control/canvas-item-ptr.h"
#include "object/sp-object.h"
#include "svg/svg-bool.h"
#include "svg/svg-length.h"
#include "svg/svg-angle.h"
#include <memory>
#include <vector>

class SPDesktop;

namespace Inkscape {
    class CanvasItemGrid;
    class Snapper;

    namespace Util {
        class Unit;
    }
} // namespace Inkscape

enum class GridType
{
    RECTANGULAR,
    AXONOMETRIC,
    MODULAR
};

class SPGrid final : public SPObject {
public:
    SPGrid();
    ~SPGrid() override;
    int tag() const override { return tag_of<decltype(*this)>; }

    static void create_new(SPDocument *doc, Inkscape::XML::Node *parent, GridType type);

    void setPrefValues();

    void show(SPDesktop *desktop);
    void hide(SPDesktop const *desktop);

    bool isEnabled() const;
    void setEnabled(bool v);

    bool isVisible() const { return isEnabled() && _visible; }
    void setVisible(bool v);

    bool isDotted() const { return _dotted; }
    void setDotted(bool v);

    bool getSnapToVisibleOnly() const { return _snap_to_visible_only; }
    void setSnapToVisibleOnly(bool v);

    Inkscape::Colors::Color const &getMajorColor() const { return _major_color; }
    void setMajorColor(Inkscape::Colors::Color const &color);

    Inkscape::Colors::Color const &getMinorColor() const { return _minor_color; }
    void setMinorColor(Inkscape::Colors::Color const &color);

    Geom::Point getOrigin() const;
    void setOrigin(Geom::Point const &new_origin);

    Geom::Point getSpacing() const;
    void setSpacing(Geom::Point const &spacing);

    guint32 getMajorLineInterval() const { return _major_line_interval; }
    void setMajorLineInterval(guint32 interval);

    double getAngleX() const { return _angle_x.computed; }
    void setAngleX(double deg);

    double getAngleZ() const { return _angle_z.computed; }
    void setAngleZ(double deg);

    bool isAngleYVertical() const { return _angle_y_vertical; }
    void setAngleYVertical(bool vertical);

    Geom::Point get_gap() const { return Geom::Point(_gap_x.computed, _gap_y.computed); }
    Geom::Point get_margin() const { return Geom::Point(_margin_x.computed, _margin_y.computed); }

    const char *typeName() const;
    const char *displayName() const;

    GridType getType() const { return _grid_type; }
    const char *getSVGType() const;
    void setSVGType(const char *svgtype);

    void setUnit(const Glib::ustring &units);
    const Inkscape::Util::Unit *getUnit() const;

    bool isPixel() const { return _pixel; }
    bool isLegacy() const { return _legacy; }

    void scale(const Geom::Scale &scale);
    Inkscape::CanvasItemGrid *getAssociatedView(SPDesktop const *desktop);

    Inkscape::Snapper *snapper();

    std::pair<Geom::Point, Geom::Point> getEffectiveOriginAndSpacing(int index = -1) const;

    std::vector<CanvasItemPtr<Inkscape::CanvasItemGrid>> views;

    void setType(GridType type);
protected:
    void build(SPDocument *doc, Inkscape::XML::Node *repr) override;
    void set(SPAttr key, const char *value) override;
    void release() override;
    void modified(unsigned int flags) override;
    void update(SPCtx *ctx, unsigned int flags) override;

private:
    void _checkOldGrid(SPDocument *doc, Inkscape::XML::Node *repr);
    void _recreateViews();

    SVGBool _visible;
    SVGBool _enabled;
    SVGBool _snap_to_visible_only;
    SVGBool _dotted;
    SVGLength _origin_x;
    SVGLength _origin_y;
    SVGLength _spacing_x;
    SVGLength _spacing_y;
    SVGAngle _angle_x; // only for axonomgrid, stored in degrees
    SVGAngle _angle_z; // only for axonomgrid, stored in degrees
    SVGBool _angle_y_vertical; // only for axonomgrid, whether y axis is vertical (isometric) or derived from x and z angles
    SVGLength _gap_x; // only for modular grid
    SVGLength _gap_y;
    SVGLength _margin_x; // only for modular grid
    SVGLength _margin_y;

    guint32 _major_line_interval = 0;

    Inkscape::Colors::Color _major_color;
    Inkscape::Colors::Color _minor_color;

    bool _pixel;        // is in user units
    bool _legacy;       // a grid from versions prior to inkscape 0.98

    GridType _grid_type;

    std::unique_ptr<Inkscape::Snapper> _snapper;

    Inkscape::Util::Unit const *_display_unit = nullptr;

    sigc::connection _page_selected_connection;
    sigc::connection _page_modified_connection;
};

#endif // SEEN_SP_GRID_H_

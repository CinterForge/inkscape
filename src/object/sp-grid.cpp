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

#include "sp-grid.h"
#include "sp-namedview.h"

#include "colors/manager.h"
#include "display/control/canvas-item-grid.h"
#include "display/control/canvas-item-ptr.h"

#include "attributes.h"
#include "desktop.h"
#include "document.h"
#include "grid-snapper.h"
#include "page-manager.h"
#include "snapper.h"
#include "svg/svg-length.h"
#include "util/units.h"

#include <glibmm/i18n.h>
#include <string>
#include <optional>

using Inkscape::Util::UnitTable;

// default colors
static auto const GRID_DEFAULT_MAJOR_COLOR = Inkscape::Colors::Color{0x0099e54d};
static auto const GRID_DEFAULT_MINOR_COLOR = Inkscape::Colors::Color{0x0099e526};

SPGrid::SPGrid()
    : _visible(true)
    , _enabled(true)
    , _dotted(false)
    , _snap_to_visible_only(true)
    , _legacy(false)
    , _angle_y_vertical(true)
    , _major_color{GRID_DEFAULT_MAJOR_COLOR}
    , _minor_color{GRID_DEFAULT_MINOR_COLOR}
    , _pixel(true)
    , _grid_type(GridType::RECTANGULAR)
{ }

void SPGrid::create_new(SPDocument *document, Inkscape::XML::Node *parent, GridType type)
{
    auto new_node = document->getReprDoc()->createElement("inkscape:grid");
    if (type == GridType::AXONOMETRIC) {
        new_node->setAttribute("type", "axonomgrid");
    }
    else if (type == GridType::MODULAR) {
        new_node->setAttribute("type", "modular");
    }
    else {
        new_node->setAttribute("type", "xygrid");
    }

    parent->appendChild(new_node);

    auto new_grid = dynamic_cast<SPGrid *>(document->getObjectByRepr(new_node));
    if (new_grid)
        new_grid->setPrefValues();

    new_grid->setEnabled(true);
    new_grid->setVisible(true);
    Inkscape::GC::release(new_node);
}

SPGrid::~SPGrid() = default;

void SPGrid::build(SPDocument *doc, Inkscape::XML::Node *repr)
{
    SPObject::build(doc, repr);

    readAttr(SPAttr::TYPE);
    readAttr(SPAttr::UNITS);
    readAttr(SPAttr::ORIGINX);
    readAttr(SPAttr::ORIGINY);
    readAttr(SPAttr::SPACINGX);
    readAttr(SPAttr::SPACINGY);
    readAttr(SPAttr::ANGLE_X);
    readAttr(SPAttr::ANGLE_Z);
    readAttr(SPAttr::ANGLE_Y_VERTICAL);
    readAttr(SPAttr::GAP_X);
    readAttr(SPAttr::GAP_Y);
    readAttr(SPAttr::MARGIN_X);
    readAttr(SPAttr::MARGIN_Y);
    readAttr(SPAttr::COLOR);
    readAttr(SPAttr::EMPCOLOR);
    readAttr(SPAttr::EMPOPACITY);
    readAttr(SPAttr::VISIBLE);
    readAttr(SPAttr::ENABLED);
    readAttr(SPAttr::MAJOR_LINE_INTERVAL);
    readAttr(SPAttr::DOTTED);
    readAttr(SPAttr::SNAP_TO_VISIBLE_ONLY);

    _checkOldGrid(doc, repr);

    _page_selected_connection = document->getPageManager().connectPageSelected([this] (void *) { update(nullptr, 0); });
    _page_modified_connection = document->getPageManager().connectPageModified([this] (void *) { update(nullptr, 0); });

    doc->addResource("grid", this);
}

void SPGrid::release()
{
    if (document) {
        document->removeResource("grid", this);
    }

    assert(views.empty());

    _page_selected_connection.disconnect();
    _page_modified_connection.disconnect();

    SPObject::release();
}

static std::optional<GridType> readGridType(char const *value)
{
    if (!value) {
        return {};
    } else if (!std::strcmp(value, "xygrid")) {
        return GridType::RECTANGULAR;
    } else if (!std::strcmp(value, "axonomgrid")) {
        return GridType::AXONOMETRIC;
    } else if (!std::strcmp(value, "modular")) {
        return GridType::MODULAR;
    } else {
        return {};
    }
}

void SPGrid::set(SPAttr key, const gchar* value)
{
    switch (key) {
        case SPAttr::TYPE: {
            auto const grid_type = readGridType(value).value_or(GridType::RECTANGULAR); // default
            if (grid_type != _grid_type) {
                _grid_type = grid_type;
                _recreateViews();
                requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        case SPAttr::UNITS:
        {
            auto unit = UnitTable::get().getUnit(value);
            if (_display_unit != unit) {
                _display_unit = unit;
                requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        case SPAttr::ORIGINX:
            _origin_x.read(value);
            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::ORIGINY:
            _origin_y.read(value);
            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::SPACINGX:
            _spacing_x.read(value);
            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::SPACINGY:
            _spacing_y.read(value);
            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::ANGLE_X: // only meaningful for axonomgrid
            _angle_x.read(value);
            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::ANGLE_Z: // only meaningful for axonomgrid
            _angle_z.read(value);
            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::ANGLE_Y_VERTICAL: // only meaningful for axonomgrid
            _angle_y_vertical.read(value);
            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::GAP_X: // only meaningful for modular
            _gap_x.read(value);
            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::GAP_Y: // only meaningful for modular
            _gap_y.read(value);
            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::MARGIN_X: // only meaningful for modular
            _margin_x.read(value);
            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::MARGIN_Y: // only meaningful for modular
            _margin_y.read(value);
            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::COLOR: {
            auto const old_opacity = _minor_color.getOpacity();
            _minor_color = Inkscape::Colors::Color::parse(value).value_or(GRID_DEFAULT_MINOR_COLOR);
            _minor_color.setOpacity(old_opacity);
            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        }
        case SPAttr::EMPCOLOR: {
            auto const old_opacity = _major_color.getOpacity();
            _major_color = Inkscape::Colors::Color::parse(value).value_or(GRID_DEFAULT_MAJOR_COLOR);
            _major_color.setOpacity(old_opacity);
            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        }
        case SPAttr::VISIBLE:
            _visible.read(value);
            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::ENABLED:
            _enabled.read(value);
            if (_snapper) _snapper->setEnabled(_enabled);
            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::OPACITY:
            _minor_color.setOpacity(value ? g_ascii_strtod(value, nullptr) : 1.0);
            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::EMPOPACITY:
            _major_color.setOpacity(value ? g_ascii_strtod(value, nullptr) : 1.0);
            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::MAJOR_LINE_INTERVAL:
            _major_line_interval = value ? std::max(std::stoi(value), 1) : 5;
            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::DOTTED:    // only meaningful for rectangular grid
            _dotted.read(value);
            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::SNAP_TO_VISIBLE_ONLY:
            _snap_to_visible_only.read(value);
            if (_snapper) _snapper->setSnapVisibleOnly(_snap_to_visible_only);
            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        default:
            SPObject::set(key, value);
            break;
    }
}

/**
 * checks for old grid attriubte keys from version 0.46 to
 * determine if there needs to be legacy attribute conversion
 */
void SPGrid::_checkOldGrid(SPDocument *doc, Inkscape::XML::Node *repr)
{
    // the old defaults
    std::unordered_map<const char *, const char *> legacyattrs = {
        { "gridoriginx", "0px" },
        { "gridoriginy", "0px" },
        { "gridspacingx", "1px" },
        { "gridspacingy", "1px" },
        { "gridcolor", "#3f3fff" },
        { "gridempcolor", "#3f3fff" },
        { "gridopacity", "0.15" },
        { "gridempopacity", "0.38" },
        { "gridempspacing", "5" },
    };

    for (auto &iter : legacyattrs) {
        if (auto attr = repr->attribute(iter.first)) {
            _legacy = true;
            iter.second = attr;
        }
    }

    if (_legacy) {
        // generate new xy grid with the correct settings
        // first create the child xml node, then hook it to repr. This order is important, to not set off listeners to repr before the new node is complete.
        Inkscape::XML::Document *xml_doc = document->getReprDoc();
        Inkscape::XML::Node *newnode = xml_doc->createElement("inkscape:grid");
        newnode->setAttribute("id", "GridFromPre046Settings");
        newnode->setAttribute("type", getSVGType());
        newnode->setAttribute("originx", legacyattrs["gridoriginx"]);
        newnode->setAttribute("originy", legacyattrs["gridoriginy"]);
        newnode->setAttribute("spacingx", legacyattrs["gridspacingx"]);
        newnode->setAttribute("spacingy", legacyattrs["gridspacingy"]);
        newnode->setAttribute("color", legacyattrs["gridcolor"]);
        newnode->setAttribute("empcolor", legacyattrs["gridempcolor"]);
        newnode->setAttribute("opacity", legacyattrs["gridopacity"]);
        newnode->setAttribute("empopacity", legacyattrs["gridempopacity"]);
        newnode->setAttribute("empspacing", legacyattrs["gridempspacing"]);

        repr->appendChild(newnode);
        Inkscape::GC::release(newnode);

        // remove all old settings
        repr->removeAttribute("gridoriginx");
        repr->removeAttribute("gridoriginy");
        repr->removeAttribute("gridspacingx");
        repr->removeAttribute("gridspacingy");
        repr->removeAttribute("gridcolor");
        repr->removeAttribute("gridempcolor");
        repr->removeAttribute("gridopacity");
        repr->removeAttribute("gridempopacity");
        repr->removeAttribute("gridempspacing");
    }
    else if (repr->attribute("id")) {
        // TODO(james): These need to come from preferences
        // fix v1.2 grids without spacing, units, origin defined
        auto fix = [&] (SPAttr attr, char const *value) {
            auto key = sp_attribute_name(attr);
            if (!repr->attribute(key)) {
                repr->setAttribute(key, value);
                set(attr, value);
            }
        };
        auto fix_double = [&] (SPAttr attr, double value) {
            auto key = sp_attribute_name(attr);
            if (!repr->attribute(key)) {
                auto str = std::to_string(value);
                repr->setAttribute(key, str.c_str());
                set(attr, str.c_str());
            }
        };

        Geom::Scale scale = document->getDocumentScale().inverse();
        Geom::Point default_origin = Geom::Point(0, 0) * scale;
        Geom::Point default_spacing = Geom::Point(1, 1) * scale;

        fix_double(SPAttr::ORIGINX, default_origin[Geom::X]);
        fix_double(SPAttr::ORIGINY, default_origin[Geom::Y]);
        fix_double(SPAttr::SPACINGY, default_spacing[Geom::Y]);

        GridType type = readGridType(repr->attribute("type")).value_or(GridType::RECTANGULAR);
        switch (type) {
            case GridType::RECTANGULAR:
                fix_double(SPAttr::SPACINGX, default_spacing[Geom::X]);
                break;
            case GridType::AXONOMETRIC:
                fix(SPAttr::ANGLE_X, "30");
                fix(SPAttr::ANGLE_Z, "30");
                break;
            case GridType::MODULAR:
                break;
            default:
                break;
        }

        auto prefs = Inkscape::Preferences::get();
        std::string prefpath = "/options/grids/" + std::string(getSVGType()) + "/units";
        Glib::ustring unit = prefs->getString(prefpath);
        if (unit.empty()) {
            setUnit("px");
            fix(SPAttr::UNITS, "px");
        } else {
            setUnit(unit);
            fix(SPAttr::UNITS, unit.c_str());
        }
    }
}

/*
 * The grid needs to be initialized based on user preferences.
 * When a grid is created by either DocumentProperties or SPNamedView,
 * update the attributes to the corresponding grid type.
 */
void SPGrid::setPrefValues()
{
    auto prefs = Inkscape::Preferences::get();

    std::string prefix;
    switch (getType()) {
        case GridType::RECTANGULAR: prefix = "/options/grids/xy"; break;
        case GridType::AXONOMETRIC: prefix = "/options/grids/axonom"; break;
        case GridType::MODULAR: prefix = "/options/grids/modular"; break;
        default: g_assert_not_reached(); break;
    }

    const auto modular = _grid_type == GridType::MODULAR;

    auto display_unit = document->getDisplayUnit();
    auto unit_pref = prefs->getString(prefix + "/units", display_unit->abbr);
    setUnit(unit_pref);

    _display_unit = UnitTable::get().getUnit(unit_pref);

    // Origin and Spacing are the only two properties that vary depending on selected units
    // SPGrid should only store values in document units, convert whatever preferences are set to "px"
    // and then scale "px" to the document unit.
    using Inkscape::Util::Quantity;
    auto scale = document->getDocumentScale().inverse();
    setOrigin(Geom::Point(
                Quantity::convert(prefs->getDouble(prefix + "/origin_x"), _display_unit, "px"),
                Quantity::convert(prefs->getDouble(prefix + "/origin_y"), _display_unit, "px")) * scale);

    auto default_spacing = modular ? 100.0 : 1.0;
    setSpacing(Geom::Point(
                Quantity::convert(prefs->getDouble(prefix + "/spacing_x", default_spacing), _display_unit, "px"),
                Quantity::convert(prefs->getDouble(prefix + "/spacing_y", default_spacing), _display_unit, "px")) * scale);

    setMajorColor(prefs->getColor(prefix + "/empcolor", modular ? "#0047cb4d" : "#0099e54d"));
    setMinorColor(prefs->getColor(prefix + "/color", modular ? "#0047cb26" : "#0099e526"));
    setMajorLineInterval(prefs->getInt(prefix + "/empspacing"));

    // these prefs are bound specifically to one type of grid
    if (_grid_type == GridType::AXONOMETRIC) {
        setDotted(prefs->getBool("/options/grids/xy/dotted"));
        setAngleX(prefs->getDouble("/options/grids/axonom/angle_x"));
        setAngleZ(prefs->getDouble("/options/grids/axonom/angle_z"));
        setAngleYVertical(prefs->getBool("/options/grids/axonom/angle_y_vertical"));
    }

    // modular grid properties
    if (_grid_type == GridType::MODULAR) {
        auto m = prefix + "/";

        auto margin = Geom::Point(
            Quantity::convert(prefs->getDouble(m + "marginx", 0), _display_unit, "px"),
            Quantity::convert(prefs->getDouble(m + "marginy", 0), _display_unit, "px")
        ) * scale;
        auto gap = Geom::Point(
            Quantity::convert(prefs->getDouble(m + "gapx", 20), _display_unit, "px"),
            Quantity::convert(prefs->getDouble(m + "gapy", 20), _display_unit, "px")
        ) * scale;

        getRepr()->setAttributeSvgDouble("marginx", margin.x());
        getRepr()->setAttributeSvgDouble("marginy", margin.y());
        getRepr()->setAttributeSvgDouble("gapx", gap.x());
        getRepr()->setAttributeSvgDouble("gapy", gap.y());

        requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
    }
}

static CanvasItemPtr<Inkscape::CanvasItemGrid> create_view(GridType grid_type, Inkscape::CanvasItemGroup *canvasgrids)
{
    switch (grid_type) {
        case GridType::RECTANGULAR: return make_canvasitem<Inkscape::CanvasItemGridXY>    (canvasgrids); break;
        case GridType::AXONOMETRIC: return make_canvasitem<Inkscape::CanvasItemGridAxonom>(canvasgrids); break;
        case GridType::MODULAR:     return make_canvasitem<Inkscape::CanvasItemGridTiles> (canvasgrids); break;
        default: g_assert_not_reached(); return {};
    }
}

void SPGrid::_recreateViews()
{
    // handle change in grid type requiring all views to be recreated as a different type
    for (auto &view : views) {
        view = create_view(_grid_type, view->get_parent());
    }
}

// update internal state on XML change
void SPGrid::modified(unsigned int flags)
{
    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        updateRepr();
    }
}

// tell canvas to redraw grid
void SPGrid::update(SPCtx *ctx, unsigned int flags)
{
    auto [origin, spacing] = getEffectiveOriginAndSpacing();

    for (auto &view : views) {
        view->set_visible(_visible && _enabled);
        if (_enabled) {
            view->set_origin(origin);
            view->set_spacing(spacing);
            view->set_major_color(getMajorColor().toRGBA());
            view->set_minor_color(getMinorColor().toRGBA());
            view->set_dotted(_dotted);
            view->set_major_line_interval(_major_line_interval);

            if (auto axonom = dynamic_cast<Inkscape::CanvasItemGridAxonom *>(view.get())) {
                axonom->set_angle_x(_angle_x.computed);
                axonom->set_angle_z(_angle_z.computed);
                axonom->set_angle_y_vertical(_angle_y_vertical);
            }

            if (auto modular = dynamic_cast<Inkscape::CanvasItemGridTiles*>(view.get())) {
                const auto scale = document->getDocumentScale();
                // "set_spacing" above sets block size; add gaps:
                auto gap = Geom::Point(_gap_x.computed,_gap_y.computed) * scale;
                auto margin = Geom::Point(_margin_x.computed, _margin_y.computed) * scale;
                modular->set_gap_size(gap);
                modular->set_margin_size(margin);
            }
        }
    }
}

/**
 * creates a new grid canvasitem for the SPDesktop given as parameter. Keeps a link to this canvasitem in the views list.
 */
void SPGrid::show(SPDesktop *desktop)
{
    if (!desktop) return;

    // check if there is already a canvasitem on this desktop linking to this grid
    for (auto &view : views) {
        if (desktop->getCanvasGrids() == view->get_parent()) {
            return;
        }
    }

    // create designated canvasitem for this grid
    views.emplace_back(create_view(_grid_type, desktop->getCanvasGrids()));

    requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void SPGrid::hide(SPDesktop const *desktop)
{
    if (!desktop) return;

    for (auto it = views.begin(); it != views.end(); ++it) {
        auto view = it->get();
        if (view->get_parent() == desktop->getCanvasGrids()) {
            views.erase(it);
            break;
        }
    }
}

void SPGrid::scale(const Geom::Scale &scale)
{
    setOrigin( getOrigin() * scale );
    setSpacing( getSpacing() * scale );
}

Inkscape::Snapper *SPGrid::snapper()
{
    if (!_snapper) {
        // lazily create
        _snapper = std::make_unique<Inkscape::GridSnapper>(this, &document->getNamedView()->snap_manager, 0);
        _snapper->setEnabled(_enabled);
        _snapper->setSnapVisibleOnly(_snap_to_visible_only);
    }
    return _snapper.get();
}

std::pair<Geom::Point, Geom::Point> SPGrid::getEffectiveOriginAndSpacing(int index) const
{
    auto origin = getOrigin();
    auto spacing = getSpacing();
    auto pitch = Geom::Point(_spacing_x.computed + _gap_x.computed, _spacing_y.computed + _gap_y.computed);
    if (index >= 0) {
        spacing = pitch;
    }

    // modular grid snapping can be supported by making it look like a series of rectangular grids (up to 4)
    switch (index) {
        case -1: // rectangular grid case
            break;
        case 0: // modular: left/top edge
            origin += Geom::Point(_gap_x.computed, _gap_y.computed) / 2;
            break;
        case 1: // modular: right/bottom edge
            origin += Geom::Point(_gap_x.computed, _gap_y.computed) / 2 + Geom::Point(_spacing_x.computed, _spacing_y.computed);
            break;
        case 2: // modular: left/top with margin
            if (_margin_x.computed || _margin_y.computed) {
                origin += Geom::Point(_gap_x.computed, _gap_y.computed) / 2 - Geom::Point(_margin_x.computed, _margin_y.computed);
            }
            else {
                spacing = Geom::Point();
            }
            break;
        case 3: // modular: right/bottom with margin
            if (_margin_x.computed || _margin_y.computed) {
                origin += Geom::Point(_gap_x.computed, _gap_y.computed) / 2 + Geom::Point(_spacing_x.computed, _spacing_y.computed) + Geom::Point(_margin_x.computed, _margin_y.computed);
            }
            else {
                spacing = Geom::Point();
            }
            break;
        default: // end of sequence
            spacing = Geom::Point();
            break;
    }

    constexpr auto MIN_VAL = 0.00001;
    if (spacing.x() < MIN_VAL || spacing.y() < MIN_VAL) {
        // too small a spacing can choke snapping; skip
        spacing = Geom::Point();
    }
    else {
        auto const scale = document->getDocumentScale();
        origin *= scale;
        spacing *= scale;
    }

    origin *= document->getPageManager().getSelectedPageOriginAffine();

    return { origin, spacing };
}

const char *SPGrid::displayName() const
{
    switch (_grid_type) {
        case GridType::RECTANGULAR: return _("Rectangular Grid");
        case GridType::AXONOMETRIC: return _("Axonometric Grid");
        case GridType::MODULAR:     return _("Modular Grid");
        default: g_assert_not_reached();
    }
}

const char *SPGrid::getSVGType() const
{
    switch (_grid_type) {
        case GridType::RECTANGULAR: return "xygrid";
        case GridType::AXONOMETRIC: return "axonomgrid";
        case GridType::MODULAR:     return "modular";
        default: g_assert_not_reached();
    }
}

void SPGrid::setSVGType(char const *svgtype)
{
    auto target_type = readGridType(svgtype);
    if (!target_type || *target_type == _grid_type) {
        return;
    }

    getRepr()->setAttribute("type", svgtype);

    requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

// finds the canvasitem active in the current active view
Inkscape::CanvasItemGrid *SPGrid::getAssociatedView(SPDesktop const *desktop)
{
    for (auto &view : views) {
        if (desktop->getCanvasGrids() == view->get_parent()) {
            return view.get();
        }
    }
    return nullptr;
}

void SPGrid::setVisible(bool v)
{
    getRepr()->setAttributeBoolean("visible", v);

    requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

bool SPGrid::isEnabled() const
{
    return _enabled;
}

void SPGrid::setEnabled(bool v)
{
    getRepr()->setAttributeBoolean("enabled", v);

    if (_snapper) _snapper->setEnabled(v);

    requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

// returns values in "px"
Geom::Point SPGrid::getOrigin() const
{
    return Geom::Point(_origin_x.computed, _origin_y.computed);
}

void SPGrid::setOrigin(Geom::Point const &new_origin)
{
    Inkscape::XML::Node *repr = getRepr();
    repr->setAttributeSvgDouble("originx", new_origin[Geom::X]);
    repr->setAttributeSvgDouble("originy", new_origin[Geom::Y]);

    requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void SPGrid::setMajorColor(Inkscape::Colors::Color const &color)
{
    getRepr()->setAttribute("empcolor", color.toString(false));
    getRepr()->setAttributeSvgDouble("empopacity", color.getOpacity());
    requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void SPGrid::setMinorColor(Inkscape::Colors::Color const &color)
{
    getRepr()->setAttribute("color", color.toString(false));
    getRepr()->setAttributeSvgDouble("opacity", color.getOpacity());
    requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

// returns values in "px"
Geom::Point SPGrid::getSpacing() const
{
    return Geom::Point(_spacing_x.computed, _spacing_y.computed);
}

void SPGrid::setSpacing(const Geom::Point &spacing)
{
    Inkscape::XML::Node *repr = getRepr();
    repr->setAttributeSvgDouble("spacingx", spacing[Geom::X]);
    repr->setAttributeSvgDouble("spacingy", spacing[Geom::Y]);

    requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void SPGrid::setMajorLineInterval(const guint32 interval)
{
    getRepr()->setAttributeInt("empspacing", interval);

    requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void SPGrid::setDotted(bool v)
{
    getRepr()->setAttributeBoolean("dotted", v);

    requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void SPGrid::setSnapToVisibleOnly(bool v)
{
    getRepr()->setAttributeBoolean("snapvisiblegridlinesonly", v);
    if (_snapper) _snapper->setSnapVisibleOnly(v);

    requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void SPGrid::setAngleX(double deg)
{
    getRepr()->setAttributeSvgDouble("gridanglex", deg);

    requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void SPGrid::setAngleZ(double deg)
{
    getRepr()->setAttributeSvgDouble("gridanglez", deg);

    requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void SPGrid::setAngleYVertical(bool vertical)
{
    getRepr()->setAttributeBoolean("angleyvertical", vertical);

    requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

const char *SPGrid::typeName() const
{
    switch (_grid_type) {
        case GridType::RECTANGULAR: return "grid-rectangular";
        case GridType::AXONOMETRIC: return "grid-axonometric";
        case GridType::MODULAR:     return "grid-modular";
        default: g_assert_not_reached(); return "grid";
    }
}

const Inkscape::Util::Unit *SPGrid::getUnit() const
{
    return _display_unit;
}

void SPGrid::setUnit(const Glib::ustring &units)
{
    if (units.empty()) return;

    if (auto new_unit = UnitTable::get().getUnit(units)) {
        getRepr()->setAttribute("units", units.c_str());
        _display_unit = new_unit;
        requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
    }
}

void SPGrid::setType(GridType type) {
    if (_grid_type == type) return;

    auto node = getRepr();
    if (type == GridType::AXONOMETRIC) {
        node->setAttribute("type", "axonomgrid");
    }
    else if (type == GridType::MODULAR) {
        node->setAttribute("type", "modular");
    }
    else {
        node->setAttribute("type", "xygrid");
    }

    setPrefValues();
    requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

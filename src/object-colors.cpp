// SPDX-License-Identifier: GPL-2.0-or-later

#include "object-colors.h"

#include "desktop-style.h"
#include "document.h"
#include "gradient-chemistry.h"
#include "object/sp-gradient.h"
#include "object/sp-linear-gradient.h"
#include "object/sp-marker.h"
#include "object/sp-mask.h"
#include "object/sp-use.h"
#include "object/sp-mesh-gradient.h"
#include "object/sp-pattern.h"
#include "object/sp-radial-gradient.h"
#include "object/sp-stop.h"
#include "object/sp-tspan.h"
#include "object/sp-text.h"
#include "style.h"

namespace Inkscape {

/*
 * reset selected object colors to their original colors  all at once
 * used when LP checkbox is unchecked
 */
void ObjectColorSet::revertToOriginalColors(bool is_reset_clicked)
{
    for (auto &[key, items] : _selected_colors) {
        if (is_reset_clicked) {
            items.second->new_color = items.second->old_color;
        }
        applyNewColorToSelection(key, items.second->old_color);
    }
}

/*
 * convert selected object colors to the new choosen colors all at once
 * used when LP checkbox is unchecked then checked again
 */
void ObjectColorSet::convertToRecoloredColors()
{
    for (auto const &[key, items] : _selected_colors) {
        if (items.second) {
            Color new_color = items.second->new_color;
            applyNewColorToSelection(key, new_color);
        }
    }
}

/*
* loop over selection and lowers opacity for items with color
* that doesn't match the parameter color for when user hovers on
* any colorpreview in the list to highlight the hovered on colored objects
*/
void ObjectColorSet::changeOpacity(bool change_opacity,uint32_t color ,bool is_preview)
{
    for (auto const &[key, value] : _selected_colors) {
        Color new_color = is_preview ? value.second->new_color : value.second->old_color;
        if (change_opacity && key != color) {
            new_color.setOpacity(0.05);
        }
        applyNewColorToSelection(key, new_color);
    }
}

/*
 * get stops vector from the _gradient_stops map and loop over it to
 * set them to the new color
 */
void ObjectColorSet::recolorStops(uint32_t old_color, Color new_color)
{
    auto stops_vector = _gradient_stops.find(old_color);
    if (stops_vector != _gradient_stops.end()) {
        for (auto stop : stops_vector->second) {
            stop->setColor(new_color);
        }
    }
}

/*
 * loop over stops list and populate the _gradient_stops
 * it has a different type of access than the _selected_colors map
 * so it has a independent map as it stores just a part of the item not the whole item
 * like _selected_colors map
 */
void ObjectColorSet::populateStopsMap(SPStop *stop)
{
    while (stop) {
        uint32_t color = stop->getColor().toRGBA();
        _gradient_stops[color].push_back(stop);
        stop = stop->getNextStop();
    }
}

/*
* populate _selected_colors map with the color as a string key with vector of objects that
* have the same color and a pair of colors that has the old and new colors of type color
* to ensure easy access on both colors
*
*/
void ObjectColorSet::populateMap(Color color, SPObject *item, ObjectStyleType type, std::string const &kind)
{
    color.enableOpacity(true);
    ColorRef ref {item, kind , type};
    ColorPair pair {color, color};
    uint32_t color_rgba = color.toRGBA();
    auto _selected = _selected_colors.find(color_rgba);
    // search if key exist and just push the object to the objects vector
    if (_selected != _selected_colors.end()) {
        _selected->second.first.push_back(ref);
    } else { // create key and push the object and their color ref
        colors.push_back(color);
        color_wheel_colors_map[color_rgba] = colors.size() - 1;
        _selected_colors.emplace(color_rgba, std::make_pair(std::vector<ColorRef>{ref}, pair));
    }
}

void ObjectColorSet::changeObjectColor(ColorRef const &item, Color const &color)
{
    std::string c = color.toString(true);
    if (item.kind == "stop") {
        return;
    }

    SPCSSAttr *css = sp_repr_css_attr_new();
    sp_repr_css_set_property_string(css, item.kind.c_str(), c);
    if (!item.item->getId()) { // for handling text content that inheirts its style from parent
        auto repr = item.item->parent->getRepr();
        sp_repr_css_change(repr, css, "style");
    } else {
        sp_desktop_apply_css_recursive(item.item, css, true);
    }
    sp_repr_css_attr_unref(css);
}

void ObjectColorSet::clearData()
{
    colors.clear();
    _gradient_stops.clear();
    _selected_colors.clear();
    color_wheel_colors_map.clear();
}

bool ObjectColorSet::setSelectedNewColor(std::vector<Colors::Color> const &new_colors)
{
    if (new_colors.empty() || new_colors.size() != colors.size()) {
        return false;
    }
    for (auto &[key, value] : _selected_colors) {
        int index = color_wheel_colors_map[key];
        value.second->new_color = new_colors[index];
    }
    return true;
}

std::vector<ColorRef> &ObjectColorSet::getSelectedItems(uint32_t key_color)
{
    static std::vector<ColorRef> empty;
    if (auto it = _selected_colors.find(key_color); it != _selected_colors.end()) {
        return it->second.first;
    }
    return empty;
}

int ObjectColorSet::getColorIndex(uint32_t key_color) const
{
    if (auto it = color_wheel_colors_map.find(key_color); it != color_wheel_colors_map.end()) {
        return it->second;
    }
    return -1;
}

std::optional<Color> ObjectColorSet::getColor(int index) const
{
    if (index < 0 || index >= colors.size()) {
        return {};
    }

    return colors[index];
}

bool ObjectColorSet::applyNewColorToSelection(uint32_t key_color, Color const &new_color)
{
    std::vector<ColorRef> &items = getSelectedItems(key_color);
    if (items.empty()) {
        return false;
    }

    for (auto const &item : items) {
        changeObjectColor(item, new_color);
    }
    recolorStops(key_color, new_color);
    return true;
}

void ObjectColorSet::setSelectedNewColor(uint32_t key_color, Color const &new_color)
{
    if (auto it = _selected_colors.find(key_color); it != _selected_colors.end()) {
        it->second.second->new_color = new_color;
    }
}

std::optional<Color> ObjectColorSet::getSelectedNewColor(uint32_t key_color) const
{
    auto _selected = _selected_colors.find(key_color);
    if (_selected != _selected_colors.end()) {
        return _selected->second.second->new_color;
    }
    return {};
}

namespace {

class ColorsExtractor
{
public:
    explicit ColorsExtractor(ObjectColorSet &m)
        : manager{m}
    {}

    void collectColors(std::vector<SPObject *> objects, ObjectStyleType type = ObjectStyleType::None);

private:
    ObjectColorSet &manager;

    void extractGradientStops(SPObject *object, bool isFill);
    void extractMeshStops(std::vector<std::vector<SPMeshNode *>> &mesh_nodes, SPObject *object, ObjectStyleType type);
    void extractObjectColors(SPObject *object, ObjectStyleType type = ObjectStyleType::None);
    void extractObjectStyle(SPObject *object, ObjectStyleType type = ObjectStyleType::None , SPUse *use = nullptr);
    void extractPatternColors(SPPattern *pattern);
    void extractMarkerColors(Glib::ustring const &marker, SPObject *object);
};

/*
 * loops over the vector of objects , firstly try to dynamically cast the spobject to spitem
 * if it is casted check if it is a mask or not if mask extract the spobjects
 * from it push into vector of spobjects then call collectColors recursivley with this vector
 * after this it calls extractObjectColors on the object it self
 */
void ColorsExtractor::collectColors(std::vector<SPObject *> objects, ObjectStyleType type)
{
    for (auto object : objects) {
        if (auto item = cast<SPItem>(object)) {
            if (auto mask = cast<SPMask>(item->getMaskObject())) {
                auto children = mask->childList(false);
                collectColors(children, ObjectStyleType::Mask);
            }

            // Text objects may contain many tspans, each of which can be styled
            // differently.
            if (auto text = cast<SPText>(item)) {
                auto children = text->childList(false);
                collectColors(children, type);

                // The text object itself isn't expected to have a color.
                continue;
            }

            // A tspan can contain recursively contain additional tspans.
            if (auto tspan = cast<SPTSpan>(item)) {
                auto children = tspan->childList(false);
                collectColors(children, type);
            }
        }

        extractObjectColors(object, type);
    }
}

/*
 * checks if object is an spgroup if it is loop over group's children
 * call extractObjectColors recursivley on group's children
 * if it is not group call extractObjectStyle
 */
void ColorsExtractor::extractObjectColors(SPObject *object, ObjectStyleType type)
{
    if (auto group = cast<SPGroup>(object)) {
        for (auto &child : group->children) {
            extractObjectColors(&child, type);
        }
    } else if (auto use = cast<SPUse>(object)) {
        extractObjectStyle(use->child, type, use);
    } else if (object) {
        extractObjectStyle(object, type);
    }
}

/*
 * firstly extract the objects markers value which has 3 markers per object (optional)
 * check for fill types (flat fill, pattern fill, gradient fill) to populate the _selected_colors map
 * do same for stroke types
 */
void ColorsExtractor::extractObjectStyle(SPObject *object, ObjectStyleType type, SPUse *use)
{
    // check object style
    if (!object || !object->style) {
        return;
    }
    SPStyle *style = object->style;
    extractMarkerColors(style->marker_start.get_value(), object);
    extractMarkerColors(style->marker_mid.get_value(), object);
    extractMarkerColors(style->marker_end.get_value(), object);

    // get flat fills
    if (style->fill.isColor()) {
        auto color = style->fill.getColor();
        ObjectStyleType fill_type = type == ObjectStyleType::None ? ObjectStyleType::Fill : type;
        manager.populateMap(color, use ? use : object, fill_type, "fill");

    } else if (style->fill.isPaintserver()) {
        // paint server can be pattern or gradient
        // get gradient stops strokes
        auto ps = style->getFillPaintServer();
        if (auto pattern = cast<SPPattern>(ps)) {
            extractPatternColors(pattern);
        }
        extractGradientStops(object, true);
    }

    if (style->stroke.isColor()) {
        auto color = style->stroke.getColor();
        ObjectStyleType stroke_type = type == ObjectStyleType::None ? ObjectStyleType::Stroke : type;
        manager.populateMap(color, use ? use : object, stroke_type, "stroke");
    } else if (style->stroke.isPaintserver()) {
        // get gradient stops strokes
        auto ps = style->getStrokePaintServer();
        if (auto pattern = cast<SPPattern>(ps)) {
            extractPatternColors(pattern);
        }
        extractGradientStops(object, false);
    }
}

/*
 * check if paint server is spgradient then check if it has patches for extracting mesh gradient
 * if it is mesh get its node array and pass it to extractMeshStops
 * if not mesh we firstly fork the gradient so we unlink its shared stops with other similar gradients
 * so change in selected one doesn't affect the unselected similar one (has same stops colors)
 * then call populateStopsMap to save stops refrencess
 * then call populateMap to save the spgradient object as a whole to have gradients colors in
 * the color list
 */
void ColorsExtractor::extractGradientStops(SPObject *object, bool isFill)
{
    auto paint_server = isFill ? object->style->getFillPaintServer() : object->style->getStrokePaintServer();
    if (paint_server && cast<SPGradient>(paint_server)) {
        auto gradient = cast<SPGradient>(paint_server);
        if (!gradient) {
            return;
        }
        if (auto vectorGradient = gradient->getVector()) {
            if (vectorGradient->hasPatches()) {
                vectorGradient->ensureArray();
                std::unique_ptr<SPMeshNodeArray> nodeArray;
                if (auto mesh = cast<SPMeshGradient>(gradient)) {
                    nodeArray = std::make_unique<SPMeshNodeArray>(mesh);
                    extractMeshStops(nodeArray->nodes, object,ObjectStyleType::Mesh);
                }

            } else {
                gradient = sp_gradient_get_forked_vector_if_necessary(gradient, true);
                if (!gradient) {
                    return;
                }
                gradient->ensureVector();
                manager.populateStopsMap(gradient->getFirstStop());
            }
        }
        bool is_swatch = gradient->getVector()->isSwatch();
        ObjectStyleType type;
        if (is_swatch) {
            type = ObjectStyleType::Swatch;
        } else if (is<SPLinearGradient>(gradient)) {
            type = ObjectStyleType::Linear;
        } else if (is<SPRadialGradient>(gradient)) {
            type = ObjectStyleType::Radial;
        }
        for (auto stop : gradient->getGradientVector()->stops) {
            if (stop.color.has_value()) {
                manager.populateMap(stop.color.value(), object, type, "stop");
            }
        }
    }
}

/*
 * mesh_nodes is a vector of vector of stops so we loop over it normally
 * call populateStopsMap and populateMap to populate both maps
 */
void ColorsExtractor::extractMeshStops(std::vector<std::vector<SPMeshNode *>> &mesh_nodes, SPObject *item, ObjectStyleType type)
{
    for (auto const &nodes : mesh_nodes) {
        for (auto const &node : nodes) {
            manager.populateStopsMap(node->stop);
            if (node->color.has_value()) {
                manager.populateMap(node->color.value(), item, type , "stop");
            }
        }
    }
}

/*
 * get root pattern then loop over its children and do the whole extraction process by calling
 * extractObjectColors to check for spgroups in pattern children
 */
void ColorsExtractor::extractPatternColors(SPPattern *pattern)
{
    auto root = pattern->rootPattern();
    for (auto &child : root->children) {
        extractObjectColors(&child, ObjectStyleType::Pattern);
    }
}

/*
* extract marker id from marker to get it by herf from the xml tree
* then try to cast the result to spmarker
* loop over the spmarker children and do the extraction process on every child
* by calling extractObjectColors
*/
void ColorsExtractor::extractMarkerColors(Glib::ustring const &marker, SPObject *object)
{
    if (marker.size() >= 5 && object->document) {
        std::string marker_id = marker.substr(4, marker.size() - 5);
        auto m = object->document->getObjectByHref(marker_id);
        if (!m) {
            return;
        }
        if (auto marker_obj = cast<SPMarker>(m)) {
            for (auto child : marker_obj->item_list()) {
                extractObjectColors(child, ObjectStyleType::Marker);
            }
        }
    }
}

} // namespace

ObjectColorSet collect_colours(std::vector<SPObject *> const &objects, ObjectStyleType type)
{
    ObjectColorSet result;
    ColorsExtractor(result).collectColors(objects, type);
    return result;
}

} // namespace Inkscape

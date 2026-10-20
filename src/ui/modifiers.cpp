// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Modifiers for inkscape
 *
 * The file provides a definition of all the ways shift/ctrl/alt modifiers
 * are used in Inkscape, and allows users to customise them in keys.xml
 *
 *//*
 * Authors:
 * 2020 Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <bitset>
#include <glibmm/i18n.h>

#include "message-context.h"
#include "modifiers.h"
#include "ui/tools/tool-base.h"
#include "ui/widget/events/canvas-event.h"

namespace Inkscape::Modifiers {

namespace {
using ModifierIdToTypeMap = std::map<std::string, Type>;
ModifierIdToTypeMap const &modifier_type_from_id()
{
    static ModifierIdToTypeMap const static_id_to_type_map {
        {"canvas-pan-drag", Type::CANVAS_PAN_DRAG},
        {"canvas-pan-x", Type::CANVAS_PAN_X},
        {"canvas-pan-y", Type::CANVAS_PAN_Y},
        {"canvas-zoom", Type::CANVAS_ZOOM},
        {"canvas-drag-zoom", Type::CANVAS_DRAG_ZOOM},
        {"canvas-rotate", Type::CANVAS_ROTATE},
        {"canvas-rotate-drag", Type::CANVAS_ROTATE_DRAG},
        {"canvas-rotate-reset", Type::CANVAS_ROTATE_RESET},
        {"canvas-rotate-snapping", Type::CANVAS_ROTATE_SNAPPING},
        {"canvas-zoom", Type::CANVAS_ZOOM},
        {"canvas-zoom-invert", Type::CANVAS_ZOOM_INVERT},
        {"canvas-zoom-rubberband", Type::CANVAS_ZOOM_RUBBERBAND},
        {"select-add-to", Type::SELECT_ADD_TO},
        {"select-in-groups", Type::SELECT_IN_GROUPS},
        {"select-touch-path", Type::SELECT_TOUCH_PATH},
        {"select-always-box", Type::SELECT_ALWAYS_BOX},
        {"select-remove-from", Type::SELECT_REMOVE_FROM},
        {"select-force-drag", Type::SELECT_FORCE_DRAG},
        {"select-cycle", Type::SELECT_CYCLE},
        {"select-duplicate", Type::SELECT_DUPLICATE},
        {"select-remove-snap", Type::SELECT_REMOVE_SNAP},
        {"arc-maintain-gap", Type::ARC_MAINTAIN_GAP},
        {"move-confine", Type::MOVE_CONFINE},
        {"move-increment", Type::MOVE_INCREMENT},
        {"move-snapping", Type::MOVE_NO_SNAPPING}, // name is move-snapping for historical reasons
        {"trans-confine", Type::TRANS_CONFINE},
        {"trans-increment", Type::TRANS_INCREMENT},
        {"trans-off-center", Type::TRANS_OFF_CENTER},
        {"trans-snapping", Type::TRANS_NO_SNAPPING}, // name is trans-snapping for historical reasons
        {"freehand-angle-snapping", Type::FREEHAND_ANGLE_SNAPPING},
        {"freehand-dot", Type::FREEHAND_DOT},
        {"freehand-dot-double", Type::FREEHAND_DOT_DOUBLE},
        {"freehand-dot-random", Type::FREEHAND_DOT_RANDOM},
        {"bool-shift", Type::BOOL_SHIFT},
        {"box3d-change-knot-direction", Type::BOX3D_CHANGE_KNOT_DIRECTION},
        {"box3d-extrude-one", Type::BOX3D_EXTRUDE_ONE},
        {"box3d-extrude-two", Type::BOX3D_EXTRUDE_TWO},
        {"calligraphic-hatching", Type::CALLI_HATCHING},
        {"calligraphic-subtract", Type::CALLI_SUBTRACT},
        {"calligraphic-unionize", Type::CALLI_UNIONIZE},
        {"dropper-dropping", Type::DROPPER_DROPPING},
        {"dropper-invert", Type::DROPPER_INVERT},
        {"dropper-stroke", Type::DROPPER_STROKE},
        {"flood-item", Type::FLOOD_ITEM},
        {"flood-touch-fill", Type::FLOOD_TOUCH_FILL},
        {"gradient-create", Type::GRADIENT_CREATE},
        {"gradient-link-handles", Type::GRADIENT_LINK_HANDLES},
        {"measure-knot-dialog", Type::MEASURE_KNOT_DIALOG},
        {"measure-select-segment", Type::MEASURE_SELECT_SEGMENT},
        {"node-bspline-handles", Type::NODE_BSPLINE_HANDLES},
        {"node-confine-handles", Type::NODE_CONFINE_HANDLES},
        {"node-confine-to-path", Type::NODE_CONFINE_TO_PATH},
        {"node-cycle-type", Type::NODE_CYCLE_TYPE},
        {"node-delete", Type::NODE_DELETE},
        {"node-delete-segment", Type::NODE_DELETE_SEGMENT},
        {"node-drag-handle", Type::NODE_DRAG_HANDLE},
        {"node-grow-linear", Type::NODE_GROW_LINEAR},
        {"node-grow-spatial", Type::NODE_GROW_SPATIAL},
        {"node-insert", Type::NODE_INSERT},
        {"node-invert", Type::NODE_INVERT},
        {"node-link-handles", Type::NODE_LINK_HANDLES},
        {"node-preserve-length", Type::NODE_PRESERVE_LENGTH},
        {"node-remove-from", Type::NODE_REMOVE_FROM},
        {"node-retract-handle", Type::NODE_RETRACT_HANDLE},
        {"node-straighten-segment", Type::NODE_STRAIGHTEN_SEGMENT},
        {"pen-cusp-node", Type::PEN_CUSP_NODE},
        {"pen-move-prev", Type::PEN_MOVE_PREV},
        {"pen-switch-axis", Type::PEN_SWITCH_AXIS},
        {"pencil-sketch", Type::PENCIL_SKETCH},
        {"tweak-invert", Type::TWEAK_INVERT},
        {"text-clear-inline-size", Type::TEXT_CLEAR_INLINE_SIZE},
    };
    return static_id_to_type_map;
}

inline std::pair<Modifiers::Type, Modifier> make_modifier(char const *id,
                                                          char const *name,
                                                          char const *desc,
                                                          KeyMask and_mask,
                                                          Trigger category,
                                                          Trigger trigger)
{
    return {modifier_type_from_id().at(id), Modifier(id, name, desc, and_mask, category, trigger)};
}

std::map<int, int> const &key_map()
{
    static const std::map<int, int> static_key_map = {
        {GDK_KEY_Alt_L, GDK_ALT_MASK},
        {GDK_KEY_Alt_R, GDK_ALT_MASK},
        {GDK_KEY_Control_L, GDK_CONTROL_MASK},
        {GDK_KEY_Control_R, GDK_CONTROL_MASK},
        {GDK_KEY_Shift_L, GDK_SHIFT_MASK},
        {GDK_KEY_Shift_R, GDK_SHIFT_MASK},
        {GDK_KEY_Meta_L, GDK_META_MASK},
        {GDK_KEY_Meta_R, GDK_META_MASK},
    };
    return static_key_map;
}
}  // end anonymous namespace

Modifier::Container &Modifier::_modifiers()
{
    // these must be in the same order as the * enum in "modifiers.h"
    static Modifier::Container static_modifiers {
    // Canvas modifiers
        make_modifier("canvas-pan-drag", _("Drag pan"), _("Pan/Scroll by right click drag"), SHIFT | CTRL, CANVAS, DRAG),
        make_modifier("canvas-pan-x", _("Horizontal pan"), _("Pan/Scroll left and right"), SHIFT, CANVAS, SCROLL),
        make_modifier("canvas-pan-y", _("Vertical pan"), _("Pan/Scroll up and down"), ALWAYS, CANVAS, SCROLL),
        make_modifier("canvas-zoom", _("Canvas zoom"), _("Zoom in and out with scroll wheel"), CTRL, CANVAS, SCROLL),
        make_modifier("canvas-drag-zoom", _("Canvas zoom"), _("Zoom in and out with Ctrl + middle mouse button"), CTRL, CANVAS, DRAG_MIDDLE_BUTTON),
        make_modifier("canvas-rotate", _("Canvas rotate"), _("Rotate the canvas with scroll wheel"), SHIFT | CTRL, CANVAS, SCROLL),
        make_modifier("canvas-rotate-drag", _("Canvas rotate"), _("Rotate the canvas with middle click drag"), CTRL, CANVAS, DRAG),
        make_modifier("canvas-rotate-reset", _("Reset canvas rotation"), _("Reset the angle while rotating canvas"), SHIFT | CTRL, CANVAS, DRAG),
        make_modifier("canvas-rotate-snapping", _("Canvas rotate snapping"), _("Snap while rotating canvas"), SHIFT, CANVAS, DRAG),
        make_modifier("canvas-zoom", _("Canvas zoom"), _("Zoom in and out with scroll wheel"), CTRL, CANVAS, SCROLL),
        make_modifier("canvas-zoom-invert", _("Invert zoom"), _("Reverse direction of middle click zoom"), SHIFT, CANVAS, CLICK),
        make_modifier("canvas-zoom-rubberband", _("Rubberband zoom"), _("Start a rubberband zoom with middle click drag"), SHIFT, CANVAS, DRAG),

    // Select tool modifiers (minus transforms)
        make_modifier("select-add-to", _("Add to selection"), _("Add items to existing selection"), SHIFT, SELECT, CLICK),
        make_modifier("select-in-groups", _("Select inside groups"), _("Ignore groups when selecting items"), CTRL, SELECT, CLICK),
        make_modifier("select-touch-path", _("Select with touch-path"), _("Draw a band around items to select them"), ALT, SELECT, DRAG),
        make_modifier("select-always-box", _("Select with box"), _("Don't drag items, select more with a box"), SHIFT, SELECT, DRAG),
        make_modifier("select-remove-from", _("Remove from selection"), _("Remove items from existing selection"), SHIFT | CTRL, SELECT, DRAG),
        make_modifier("select-force-drag", _("Forced Drag"), _("Drag objects even if the mouse isn't over them"), ALT, SELECT, DRAG),
        make_modifier("select-cycle", _("Cycle through objects"), _("Scroll through objects under the cursor"), ALT, SELECT, SCROLL),
        make_modifier("select-duplicate", _("Duplicate selection on drag"), _("Duplicate selection when starting a drag"), NEVER, SELECT, DRAG),
        make_modifier("select-remove-snap", _("Remove snap target"), _("Remove snap target during a drag"), SHIFT | ALT, SELECT, DRAG),

    // Arc tool modifier
        make_modifier("arc-maintain-gap", _("Maintain gap"), _("Move both start and end knot of the arc"), SHIFT, ARC_TOOL, DRAG),

    // Text tool modifier
        make_modifier("text-clear-inline-size", _("Clear inline size"), _("Fit the text width for auto-wrapped text"), CTRL, TEXT_TOOL, CLICK),

    // Transform handle modifiers (applies to multiple tools)
        make_modifier("move-confine", _("Move one axis only"), _("When dragging items, confine to either x or y axis"), CTRL, MOVE, DRAG),
        make_modifier("move-increment", _("Move in increments"), _("Move the objects by increments of grid pitch when dragging"), NEVER, MOVE, DRAG),
        make_modifier("move-snapping", _("No Move Snapping"), _("Disable snapping when moving objects"), SHIFT, MOVE, DRAG),
        make_modifier("trans-confine", _("Keep aspect ratio"), _("When resizing objects, confine the aspect ratio"), CTRL, TRANSFORM, DRAG),
        make_modifier("trans-increment", _("Transform in increments"), _("Scale, rotate or skew by set increments"), ALT, TRANSFORM, DRAG),
        make_modifier("trans-off-center", _("Transform around center"), _("When scaling, scale selection symmetrically around its rotation center. When rotating/skewing, transform relative to opposite corner/edge."), SHIFT, TRANSFORM, DRAG),
        make_modifier("trans-snapping", _("No Transform Snapping"), _("Disable snapping when transforming object"), SHIFT, TRANSFORM, DRAG),
    // Center handle click: seltrans.cpp:734 SHIFT
    // Align handle click: seltrans.cpp:1365 SHIFT

        make_modifier("freehand-angle-snapping", _("Snap angle"), _("Snap while rotating item"), CTRL, FREEHAND, DRAG),
        make_modifier("freehand-dot", _("Create dot"), _("Create a single dot"), CTRL, FREEHAND, CLICK),
        make_modifier("freehand-dot-double", _("Double dot size"), _("Create dots at double the size"), SHIFT, FREEHAND, CLICK),
        make_modifier("freehand-dot-random", _("Randomly adjust dot size"), _("Create dots with with a randomly adjusted size"), ALT, FREEHAND, CLICK),

        make_modifier("bool-shift", _("Switch mode"), _("Change shape builder mode temporarily by holding a modifier key"), SHIFT, BOOLEANS_TOOL, DRAG),

        make_modifier("box3d-change-knot-direction", _("Change the axis or direction of the knot"), _("Move the knot against axis Z instead of X/Y direction or vice-versa"), SHIFT, BOX3D_TOOL, DRAG),
        make_modifier("box3d-extrude-one", _("Extrude along the Z axis"), _("Extend the 3D box in one dimension only"), SHIFT, BOX3D_TOOL, DRAG),
        make_modifier("box3d-extrude-two", _("Extrude along the Y and Z axes"), _("Extend the 3D box in two dimensions"), SHIFT | CTRL, BOX3D_TOOL, DRAG),

        make_modifier("calligraphic-hatching", _("Use a guide path"), _("Use a selected path as a guide"), CTRL, CALLI_TOOL, DRAG),
        make_modifier("calligraphic-subtract", _("Subtract the drawn stroke"), _("Subtract the drawn stroke from the selection"), ALT, CALLI_TOOL, DRAG),
        make_modifier("calligraphic-unionize", _("Add in the drawn stroke"), _("Add the drawn stroke into the selection"), SHIFT, CALLI_TOOL, DRAG),

        make_modifier("dropper-dropping", _("Reverse direction of drop"), _("Drop onto the hovered item from the selected item, instead of the other way around"), CTRL, DROPPER_TOOL, CLICK),
        make_modifier("dropper-invert", _("Invert the color"), _("Invert the chosen color when applying it"), ALT, DROPPER_TOOL, CLICK),
        make_modifier("dropper-stroke", _("Apply color to stroke"), _("Apply the chosen color to the stroke, not the fill"), SHIFT, DROPPER_TOOL, CLICK),

        make_modifier("flood-item", _("Apply style to item"), _("Apply the chosen color to the stroke and fill of an item"), CTRL, FLOOD_TOOL, CLICK),
        make_modifier("flood-touch-fill", _("Replace only the first color in drag"), _("Replace only the initial color from drag throughout the drag"), ALT, FLOOD_TOOL, DRAG),

        make_modifier("gradient-create", _("Create gradient"), _("Create gradient on selected item"), CTRL, GRADIENT_TOOL, CLICK),
        make_modifier("gradient-link-handles", _("Move handles together"), _("Move both gradient handles together during a drag"), CTRL | SHIFT, GRADIENT_TOOL, DRAG),

        make_modifier("measure-knot-dialog", _("Open position dialog"), _("Open the position dialog for the selected knot"), SHIFT, MEASURE_TOOL, CLICK),
        make_modifier("measure-select-segment", _("Select just the one segment"), _("Select a segment rather than a curve"), SHIFT, MEASURE_TOOL, CLICK),

        make_modifier("node-bspline-handles", _("Move B-Spline handles"), _("When dragging a B-Spline segment, create and/or move handles instead"), SHIFT, NODE_TOOL, DRAG),
        make_modifier("node-confine-handles", _("Confine to handles"), _("When dragging, confine to the handle lines"), CTRL | ALT, NODE_TOOL, DRAG),
        make_modifier("node-confine-to-path", _("Confine to to path"), _("When dragging, confine to the path line"), ALT, NODE_TOOL, DRAG),
        make_modifier("node-cycle-type", _("Change node type"), _("Cycle through node types when clicked"), CTRL, NODE_TOOL, CLICK),
        make_modifier("node-delete", _("Delete node"), _("Delete node when clicked"), CTRL | ALT, NODE_TOOL, CLICK),
        make_modifier("node-delete-segment", _("Delete segment"), _("Delete segment when double-clicked"), CTRL | ALT, NODE_TOOL, CLICK),
        make_modifier("node-drag-handle", _("Drag handle from node"), _("Create a new handle from a node"), SHIFT, NODE_TOOL, DRAG),
        make_modifier("node-grow-linear", _("Linear node selection"), _("Select the next nodes with scroll wheel or keyboard"), CTRL | SHIFT, NODE_TOOL, SCROLL),
        make_modifier("node-grow-spatial", _("Spatial node selection"), _("Select more nodes with scroll wheel or keyboard"), ALT | SHIFT, NODE_TOOL, SCROLL),
        make_modifier("node-insert", _("Insert new node"), _("Add a new node to a curve"), CTRL | ALT, NODE_TOOL, CLICK),
        make_modifier("node-invert", _("Inverted node selection"), _("Select nodes outside the selection area"), CTRL, NODE_TOOL, DRAG),
        make_modifier("node-link-handles", _("Move handles together"), _("Move both node handles together during a drag"), SHIFT, NODE_TOOL, DRAG),
        make_modifier("node-preserve-length", _("Preserve handle length"), _("Keep the node handle length constant during a drag"), ALT, NODE_TOOL, DRAG),
        make_modifier("node-remove-from", _("Remove nodes from selection"), _("Remove selected nodes from the selection"), SHIFT | CTRL, NODE_TOOL, DRAG),
        make_modifier("node-retract-handle", _("Retract handle into node"), _("Remove handle when clicked"), ALT, NODE_TOOL, CLICK),
        make_modifier("node-straighten-segment", _("Straighten segment"), _("Straighten segment when double-clicked"), ALT, NODE_TOOL, CLICK),

        make_modifier("pen-cusp-node", _("Create cusp node"), _("Create a cusp node when making a B-Spline curve"), SHIFT, PEN_TOOL, CLICK),
        make_modifier("pen-move-prev", _("Move previous node"), _("Move previous node while placing the next"), ALT, PEN_TOOL, DRAG),
        make_modifier("pen-switch-axis", _("Switch paraxial axis"), _("Create the next paraxial node on the opposite axis"), SHIFT, PEN_TOOL, DRAG),

        make_modifier("pencil-sketch", _("Sketch mode"), _("Interpolate between sketched paths"), ALT, PENCIL_TOOL, DRAG),

        make_modifier("tweak-invert", _("Invert tweak"), _("Reverse the direction of the tweak"), SHIFT, TWEAK_TOOL, CLICK),
    };
    return static_modifiers;
}

Modifier::CategoryNames const &Modifier::_category_names()
{
    static Modifier::CategoryNames const static_category_names {
        {NO_CATEGORY, _("No Category")},
        {CANVAS, _("Canvas")},
        {SELECT, _("Selection")},
        {MOVE, _("Movement")},
        {FREEHAND, _("Freehand")},
        {TRANSFORM, _("Transformations")},
        {NODE_TOOL, _("Node Tool")},
        {BOOLEANS_TOOL, _("Shape Builder")},
        {ARC_TOOL, _("Ellipse/Arc Tool")},
        {TEXT_TOOL, _("Text Tool")},
        {BOX3D_TOOL, _("3D Box Tool")},
        {CALLI_TOOL, _("Calligraphy Tool")},
        {DROPPER_TOOL, _("Dropper Tool")},
        {FLOOD_TOOL, _("Paint Bucket Tool")},
        {TWEAK_TOOL, _("Tweak Tool")},
        {GRADIENT_TOOL, _("Gradient Tool")},
        {MEASURE_TOOL, _("Measure Tool")},
        {PEN_TOOL, _("Pen Tool")},
        {PENCIL_TOOL, _("Pencil Tool")},
    };
    return static_category_names;
}

/**
 * Given a Trigger, find which modifier is active (category lookup)
 *
 * @param  trigger - The Modifier::Trigger category in the form "CANVAS | DRAG".
 * @param  button_state - The Gdk button state from an event.
 * @return - Returns the best matching modifier id by the most number of keys.
 */
Type Modifier::which(Trigger trigger, int button_state)
{
    // Record each active modifier with it's weight
    std::map<Type, unsigned long> scales;
    for (auto const &[key, val] : _modifiers()) {
        if (val.get_trigger() == trigger && val.active(button_state)) {
            scales[key] = val.get_weight();
        }
    }
    // Sort the weightings
    using pair_type = decltype(scales)::value_type;
    auto sorted = std::max_element
    (
        std::begin(scales), std::end(scales),
        [] (const pair_type & p1, const pair_type & p2) {
            return p1.second < p2.second;
        }
    );
    return sorted->first;
}

/**
  * List all the modifiers available. Used in UI listing.
  *
  * @return a vector of Modifier objects.
  */
std::vector<Modifier const *> Modifier::getList()
{
    std::vector<Modifier const *> modifiers;
    // Go through the dynamic modifier table
    for (auto const &[_, val] : _modifiers()) {
        modifiers.push_back(&val);
    }

    return modifiers;
}

Modifier *Modifier::get(char const *id)
{
    auto const type_it = modifier_type_from_id().find(id);
    if (type_it == modifier_type_from_id().end()) {
        return nullptr;
    }
    auto const modifier_it = _modifiers().find(type_it->second);
    if (modifier_it == _modifiers().end()) {
        return nullptr;
    }
    return &(modifier_it->second);
}

/**
 * Test if this modifier is currently active.
 *
 * @param  state - The GDK button state from an event
 * @return a boolean, true if the modifiers for this action are active.
 */
bool Modifier::active(int state) const
{
    // TODO:
    //  * ALT key is sometimes MOD1, MOD2 etc, if we find other ALT keys, set the ALT bit
    //  * SUPER key could be HYPER or META, these cases need to be considered.
    auto and_mask = get_and_mask();
    auto not_mask = get_not_mask();
    auto active = Key::ALL_MODS & state;
    // Check that all keys in AND mask are pressed, and NONE of the NOT mask are.
    return and_mask != NEVER && ((active & and_mask) == and_mask) && (not_mask == NOT_SET || (active & not_mask) == 0);
}

/**
 * Test if this modifier is currently active, adding or subtracting keyval
 * during a key press or key release operation.
 *
 * @param state - The GDK button state from an event
 * @param keyval - The GDK keyval from a key press/release event
 * @param release - Boolean, if true the keyval is removed instead
 *
 * @return a boolean, true if the modifiers for this action are active.
 */
bool Modifier::active(int state, int keyval, bool release) const
{
    return active(add_keyval(state, keyval, release));
}

/**
 * Generate a label for any modifier keys based on the mask
 *
 * @param  mask - The Modifier Mask such as {SHIFT & CTRL}
 * @return a string of the keys needed for this mask to be true.
 */
std::string generate_label(KeyMask mask, std::string sep)
{
    auto ret = std::string();
    if(mask == NOT_SET) {
        return "-";
    }
    if(mask == NEVER) {
        ret.append(_("[NEVER]"));
        return ret;
    }
    if(mask & CTRL) ret.append(_("Ctrl"));
    if(mask & SHIFT) {
        if(!ret.empty()) ret.append(sep);
        ret.append(_("Shift"));
    }
    if(mask & ALT) {
        if(!ret.empty()) ret.append(sep);
        ret.append(_("Alt"));
    }
    if(mask & SUPER) {
        if(!ret.empty()) ret.append(sep);
        ret.append(_("Super"));
    }
    if(mask & HYPER) {
        if(!ret.empty()) ret.append(sep);
        ret.append(_("Hyper"));
    }
    if(mask & META) {
        if(!ret.empty()) ret.append(sep);
        ret.append(_("Meta"));
    }
    return ret;
}

/**
 * Calculate the weight of this mask based on how many bits are set.
 *
 * @param  mask - The Modifier Mask such as {SHIFT & CTRL}
 * @return count of all modifiers being pressed (or excluded)
 */
unsigned long calculate_weight(KeyMask mask)
{

    if (mask < 0)
        return 0;
    std::bitset<sizeof(mask)> bit_mask(mask);
    return bit_mask.count();
}

static void responsive_tooltip_from_list(MessageContext *message_context, KeyEvent const &event,
                                         std::vector<std::pair<Modifier *, std::string>> mods)
{
    std::string ctrl_msg;
    std::string shift_msg;
    std::string alt_msg;

    // NOTE: This will hide any keys changed to SUPER or multiple keys such as CTRL+SHIFT
    for (const auto& mod : mods) {
        switch (mod.first->get_and_mask()) {
            case CTRL:
                ctrl_msg += mod.second + ", ";
                break;
            case SHIFT:
                shift_msg += mod.second + ", ";
                break;
            case ALT:
                alt_msg += mod.second + ", ";
                break;
            default:
                g_warning("Unhandled responsive tooltip: %s", mod.second.c_str());
        }
    }
    if (!ctrl_msg.empty()) {
        ctrl_msg.erase(ctrl_msg.size() - 2);
        ctrl_msg = "<b>Ctrl</b>: " + ctrl_msg;
    }
    if (!shift_msg.empty()) {
        shift_msg.erase(shift_msg.size() - 2);
        shift_msg = "<b>Shift</b>: " + shift_msg;
    }
    if (!alt_msg.empty()) {
        alt_msg.erase(alt_msg.size() - 2);
        alt_msg = "<b>Alt</b>: " + alt_msg;
    }

    UI::Tools::sp_event_show_modifier_tip(
        message_context,
        event,
        ctrl_msg.empty() ? nullptr : ctrl_msg.c_str(),
        shift_msg.empty() ? nullptr : shift_msg.c_str(),
        alt_msg.empty() ? nullptr : alt_msg.c_str()
    );
}

/**
 * Set the responsive tooltip for this tool, given the selected types.
 *
 * @param message_context - The desktop's message context for showing tooltips
 * @param event - The current event status (which keys are pressed)
 * @param num_types - Number of Modifier::Type arguments to follow.
 * @param ... - One or more Modifier::Type arguments.
 */
void responsive_tooltip(MessageContext *message_context, KeyEvent const &event, int num_types, ...)
{
    std::vector<std::pair<Modifier *, std::string>> mods;

    va_list args;
    va_start(args, num_types);
    for(int i = 0; i < num_types; i++) {
        auto modifier = Modifier::get(va_arg(args, Type));
        mods.emplace_back(std::pair(modifier, modifier->get_name()));
    }
    va_end(args);

    responsive_tooltip_from_list(message_context, event, mods);
}

/**
 * Set the responsive tooltip for this tool, given the selected types and labels.
 *
 * @note: This is designed for a better UX (arc tool can say "makes circles or ellipses" rather
 *        than the generic "keep aspect ratio") but should not be used to force the same modifier
 *        action to be shared when it is conceptually a different action. Be willing to make new
 *        actions with their own labels and shortcut preference.
 *
 * @param message_context - The desktop's message context for showing tooltips
 * @param event - The current event status (which keys are pressed)
 * @param num_types - Number of Modifier::Type arguments to follow.
 * @param ... - One or more Modifier::Type arguments, each followed by a gchar* label.
 */
void responsive_tooltip_with_labels(MessageContext *message_context, KeyEvent const &event, int num_types, ...)
{
    std::vector<std::pair<Modifier *, std::string>> mods;

    va_list args;
    va_start(args, num_types);
    for(int i = 0; i < num_types; i++) {
        auto modifier = Modifier::get(va_arg(args, Type));
        auto label = va_arg(args, gchar const *);
        mods.emplace_back(std::pair(modifier, label));
    }
    va_end(args);

    responsive_tooltip_from_list(message_context, event, mods);
}

/**
 * Add or remove the GDK keyval to the button state if it's one of the
 * keys that define the key mask. Useful for PRESS and RELEASE events.
 *
 * @param state - The GDK button state from an event
 * @param keyval - The GDK keyval from a key press/release event
 * @param release - Boolean, if true the keyval is removed instead
 *
 * @return a new state including the requested change.
 */
int add_keyval(int state, int keyval, bool release)
{
    if (auto it = key_map().find(keyval); it != key_map().end()) {
        if (release)
            state &= ~it->second;
        else
            state |= it->second;
    }
    return state;
}

/**
 * Checks if a GDK keyval is a modifier press/release (like CTRL, ALT, SUPER, etc).
 *
 * @param keyval - The GDK keyval from a key press/release event
 *
 * @return whether the keyval is a modifier
 */
bool keyval_is_a_modifier(int keyval)
{
    return (keyval == GDK_KEY_Alt_L     ||
            keyval == GDK_KEY_Alt_R     ||
            keyval == GDK_KEY_Control_L ||
            keyval == GDK_KEY_Control_R ||
            keyval == GDK_KEY_Shift_L   ||
            keyval == GDK_KEY_Shift_R   ||
            keyval == GDK_KEY_Meta_L    ||  // Meta is when you press Shift+Alt (at least on my machine)
            keyval == GDK_KEY_Meta_R);
}

} // namespace Inkscape::Modifiers

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

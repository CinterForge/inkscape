// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_MODIFIERS_H
#define SEEN_SP_MODIFIERS_H
/*
 *  Copyright (C) 2020 Martin Owens <doctormo@gmail.com>

 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gdkmm/enums.h>
#include <map>


namespace Inkscape {
class MessageContext;

struct KeyEvent;

namespace Modifiers {

using KeyMask = int;
using Trigger = int;

enum Key : KeyMask {
    NEVER = -2, // Never happen, switch off
    NOT_SET = -1, // Not set (user or keys)
    ALWAYS = 0, // Always happens, no modifier needed
    SHIFT = GDK_SHIFT_MASK,
    CTRL = GDK_CONTROL_MASK,
    ALT = GDK_ALT_MASK,
    SUPER = GDK_SUPER_MASK,
    HYPER = GDK_HYPER_MASK,
    META = GDK_META_MASK,
    ALL_MODS = SHIFT | CTRL | ALT | SUPER | HYPER | META,
};

// Triggers used for collision warnings, two tools are using the same trigger
enum Triggers : Trigger {
    NO_CATEGORY, CANVAS, SELECT, MOVE, TRANSFORM, FREEHAND,
    NODE_TOOL, BOOLEANS_TOOL, BOX3D_TOOL, CALLI_TOOL,
    DROPPER_TOOL, FLOOD_TOOL, SPIRAL_TOOL, STAR_TOOL,
    TWEAK_TOOL, GRADIENT_TOOL, MEASURE_TOOL, PENCIL_TOOL,
    PEN_TOOL, ARC_TOOL, TEXT_TOOL,
    // Action taken to trigger this modifier, starts at
    // bit 6 so categories and triggers can be combined.
    CLICK = 32,
    DRAG = 64,
    DRAG_MIDDLE_BUTTON = 128,
    SCROLL = 256,
};

/**
 * This anonymous enum is used to provide a list of the Shifts
 */
enum class Type {
    // {TOOL_NAME}_{ACTION_NAME}

    // Canvas tools (applies to any tool selection)
    CANVAS_PAN_DRAG,      // Pan freely {CTRL+SHIFT+DRAG}
    CANVAS_PAN_X,         // Pan left and right {SHIFT+SCROLL}
    CANVAS_PAN_Y,         // Pan up and down {NOTHING+SCROLL}
    CANVAS_DRAG_ZOOM,     // Pan and zoom the canvas {CTRL+MBUTTON2}
    CANVAS_ROTATE,        // Rotate CW and CCW {CTRL+SHIFT+SCROLL}
    CANVAS_ROTATE_DRAG,   // Rotate CW and CCW {CTRL+DRAG}
    CANVAS_ROTATE_RESET,  // Reset angle while rotating {CTRL+SHIFT+DRAG}
    CANVAS_ROTATE_SNAPPING, // Snap while rotating {SHIFT+DRAG}
    CANVAS_ZOOM,          // Zoom in and out {CTRL+SCROLL}
    CANVAS_ZOOM_INVERT,   // Invert the direction of zoom {CLICK+SHIFT}
    CANVAS_ZOOM_RUBBERBAND, // Start a rubberband zoom drag {DRAG+SHIFT}

    // Select tool (minus transform; some of these are used in other tools)
    SELECT_ADD_TO,        // Add selection {SHIFT+CLICK}
    SELECT_IN_GROUPS,     // Select within groups {CTRL+CLICK}
    SELECT_TOUCH_PATH,    // Draw band to select {ALT+DRAG+Nothing selected}
    SELECT_ALWAYS_BOX,    // Draw box to select {SHIFT+DRAG}
    SELECT_REMOVE_FROM,   // Remove from selection {CTRL+DRAG}
    SELECT_FORCE_DRAG,    // Drag objects even if the mouse isn't over them {ALT+DRAG+Selected}
    SELECT_CYCLE,         // Cycle through objects under cursor {ALT+SCROLL}
    SELECT_DUPLICATE,     // Duplicate selection when starting a drag {ALT+DRAG+Selected} unassigned by default
    SELECT_REMOVE_SNAP,   // Remove snap target during a drag {SHIFT+ALT+DRAG+Selected}

    // Arc tool
    ARC_MAINTAIN_GAP,     // Move both the start and end knots {SHIFT+DRAG}

    // Text tool
    TEXT_CLEAR_INLINE_SIZE,// Have width fit the content for auto-wrapped text {CTRL+CLICK}

    // Transform handles (applies to multiple tools)
    MOVE_CONFINE,         // Limit dragging to X OR Y only {DRAG+CTRL}
    MOVE_INCREMENT,       // Move in increments of grid pitch (preserves distances w.r.t grid) {DRAG+ALT} unassigned by default
    MOVE_NO_SNAPPING,     // Disable snapping while moving {DRAG+SHIFT}
    TRANS_CONFINE,        // Confine resize aspect ratio {HANDLE+CTRL}
    TRANS_INCREMENT,      // Scale/Rotate/skew by fixed ratio angles {HANDLE+ALT}
    TRANS_OFF_CENTER,     // Scale/Rotate/skew from opposite corner {HANDLE+SHIFT}
    TRANS_NO_SNAPPING,    // Disable snapping while transforming {HANDLE+SHIFT}

    // Some common motions shared by freehand tools
    FREEHAND_ANGLE_SNAPPING, // Use angular snapping {DRAG+CTRL}
    FREEHAND_DOT,         // Create a single dot {CLICK+CTRL}
    FREEHAND_DOT_DOUBLE,  // Double the size of a created dot {CLICK+SHIFT}
    FREEHAND_DOT_RANDOM,  // Randomly adjust the size of a created dot {CLICK+ALT}

    BOOL_SHIFT,           // Shift the shape builder into its alternative mode.

    BOX3D_CHANGE_KNOT_DIRECTION, // Move the knot along Z axis instead of X/Y direction or vice-versa
    BOX3D_EXTRUDE_ONE,    // Extrudes a box along a single dimension {DRAG+SHIFT}
    BOX3D_EXTRUDE_TWO,    // Do not constrain extrusion to perspective line {DRAG+CTRL} while extruding

    CALLI_HATCHING,       // Use a guide path {DRAG+CTRL}
    CALLI_SUBTRACT,       // Subtract the drawn path {DRAG+ALT}
    CALLI_UNIONIZE,       // Add in the drawn path {DRAG+SHIFT}

    DROPPER_DROPPING,     // Reverse the direction of drop - from selected object to hover object {CLICK+CTRL}
    DROPPER_INVERT,       // Invert the color {CLICK+ALT}
    DROPPER_STROKE,       // Apply to stroke, not fill {CLICK+SHIFT}

    FLOOD_ITEM,           // Applies style to fill+stroke of target item {CLICK+CTRL}
    FLOOD_TOUCH_FILL,     // Flood only the first point of contact during drag {DRAG+ALT}

    GRADIENT_CREATE,      // Creates gradient on selected object {DOUBLECLICK+CTRL}
    GRADIENT_LINK_HANDLES, // Move both handles at once {DRAG+SHIFT+CTRL}

    MEASURE_KNOT_DIALOG,  // Show properties dialog of knot {CLICK+SHIFT}
    MEASURE_SELECT_SEGMENT, // Select just the segment under mouse {CLICK+SHIFT}

    NODE_BSPLINE_HANDLES, // Create/move bspline handles {DRAG+SHIFT}
    NODE_CONFINE_HANDLES, // When confining, use handles as limits {DRAG+CTRL+ALT}
    NODE_CONFINE_TO_PATH, // When confining, use handles as limits {DRAG+ALT}
    NODE_CYCLE_TYPE,      // Change node type {CLICK+CTRL}
    NODE_DELETE,          // Delete node {CLICK+CTRL+ALT}
    NODE_DELETE_SEGMENT,  // Delete segment {DOUBLECLICK+CTRL}
    NODE_DRAG_HANDLE,     // Drag handle out of node {CLICK+SHIFT}
    NODE_GROW_LINEAR,     // Scroll wheel selection of nodes {SCROLL+CTRL}
    NODE_GROW_SPATIAL,    // Scroll wheel selection of nodes
    NODE_INSERT,          // Insert node into curve {CLICK+CTRL+ALT}
    NODE_INVERT,          // Select nodes outside of the selection box {DRAG+CTRL}
    NODE_LINK_HANDLES,    // Move both handles at once {DRAG+SHIFT}
    NODE_PRESERVE_LENGTH, // Preserve handle length {DRAG+ALT}
    NODE_REMOVE_FROM,     // Remove selected nodes from selection {DRAG+SHIFT+CTRL}
    NODE_RETRACT_HANDLE,  // Remove handle {CLICK+ALT}
    NODE_STRAIGHTEN_SEGMENT, // Straighten segment {DOUBLECLICK+ALT}

    PEN_CUSP_NODE,        // Makes cusp node {CLICK+SHIFT}
    PEN_MOVE_PREV,        // Moves previous node {DRAG+ALT}
    PEN_SWITCH_AXIS,      // Uses other axis for line {DRAG+SHIFT}

    PENCIL_SKETCH,        // Enables sketch mode {DRAG+ALT}

    TWEAK_INVERT,         // Reverse the direction of the tweak {CLICK+SHIFT}

    // TODO: Alignment omitted because it's UX is not completed
};

// Generate a label such as Shift+Ctrl from any KeyMask
std::string   generate_label(KeyMask mask, std::string sep = "+");
unsigned long calculate_weight(KeyMask mask);

// Generate a responsive tooltip set
void responsive_tooltip(MessageContext *message_context, KeyEvent const &event, int num_types, ...);

// Generate a responsive tooltip set, but with custom labels for actions.
// NOTE: This is designed for a better UX (arc tool can say "makes circles or ellipses" rather than
//  the generic "keep aspect ratio") but should not be used to force the same modifier action to be
//  shared when it is conceptually a different action. Be willing to make new actions with their
//  own labels and shortcut preference.
void responsive_tooltip_with_labels(MessageContext *message_context, KeyEvent const &event, int num_types, ...);

int add_keyval(int state, int keyval, bool release = false);
bool keyval_is_a_modifier(int keyval);

/**
 * A class to represent ways functionality is driven by shift modifiers
 */
class Modifier {
private:
    /** An easy to use definition of the table of modifiers by Type and ID. */
    using Container = std::map<Type, Modifier>;
    using CategoryNames = std::map<Trigger, std::string>;

    /** A table of all the created modifiers and their ID lookups. */
    static Container &_modifiers();
    static CategoryNames const &_category_names();

    char const * _id;    // A unique id used by keys.xml to identify it
    char const * _name;  // A descriptive name used in preferences UI
    char const * _desc;  // A more verbose description used in preferences UI

    Trigger _category; // The category of tool, what it might conflict with
    Trigger _trigger; // The type of trigger/action

    // Default values if nothing is set in keys.xml
    KeyMask _and_mask_default; // The pressed keys must have these bits set
    unsigned long _weight_default = 0;

    // User set data, set by keys.xml (or other included file)
    KeyMask _and_mask_keys = NOT_SET;
    KeyMask _not_mask_keys = NOT_SET;
    unsigned long _weight_keys = 0;
    KeyMask _and_mask_user = NOT_SET;
    KeyMask _not_mask_user = NOT_SET;
    unsigned long _weight_user = 0;

public:
    char const * get_id() const { return _id; }
    char const * get_name() const { return _name; }
    char const * get_description() const { return _desc; }
    Trigger get_trigger() const { return _category | _trigger; }

    // Set user value
    void set_keys(KeyMask and_mask, KeyMask not_mask) {
        _and_mask_keys = and_mask;
        _not_mask_keys = not_mask;
        _weight_keys = calculate_weight(and_mask) + calculate_weight(not_mask);
    }
    void set_user(KeyMask and_mask, KeyMask not_mask) {
        _and_mask_user = and_mask;
        _not_mask_user = not_mask;
        _weight_user = calculate_weight(and_mask) + calculate_weight(not_mask);
    }
    void unset_keys() { set_keys(NOT_SET, NOT_SET); }
    void unset_user() { set_user(NOT_SET, NOT_SET); }
    bool is_set_user() const { return _and_mask_user != NOT_SET; }

    // Get value, either user defined value or default
    KeyMask get_and_mask() const {
        if(_and_mask_user != NOT_SET) return _and_mask_user;
        if(_and_mask_keys != NOT_SET) return _and_mask_keys;
        return _and_mask_default;
    }
    KeyMask get_not_mask() const {
        // The not mask is enabled by the AND mask being set first.
        if(_and_mask_user != NOT_SET) return _not_mask_user;
        if(_and_mask_keys != NOT_SET) return _not_mask_keys;
        return NOT_SET;
    }
    // Return number of bits set for the keys
    unsigned long get_weight() const {
        if(_and_mask_user != NOT_SET) return _weight_user;
        if(_and_mask_keys != NOT_SET) return _weight_keys;
        return _weight_default;
    }

    // Generate labels such as "Shift+Ctrl" for the active modifier
    std::string get_label() const { return generate_label(get_and_mask()); }
    std::string get_category() const { return _category_names().at(_category); }

    // Configurations for saving the xml file
    bool get_config_user_disabled() const { return (_and_mask_user == NEVER); }
    std::string get_config_user_and() const { return generate_label(_and_mask_user, ","); }
    std::string get_config_user_not() const { return generate_label(_not_mask_keys, ","); }

    /**
     * Inititalizes the Modifier with the parameters.
     *
     * @param id       Goes to \c _id.
     * @param name     Goes to \c _name.
     * @param desc     Goes to \c _desc.
     */
    Modifier(char const * id,
             char const * name,
             char const * desc,
             const KeyMask and_mask,
             const Trigger category,
             const Trigger trigger) :
        _id(id),
        _name(name),
        _desc(desc),
        _and_mask_default(and_mask),
        _category(category),
        _trigger(trigger)
    {
        _weight_default = calculate_weight(and_mask);
    }

    static Type which(Trigger trigger, int button_state);
    static std::vector<Modifier const *> getList();
    bool active(int button_state) const;
    bool active(int button_state, int keyval, bool release = false) const;

    /**
     * A function to turn an enum index into a modifier object.
     *
     * @param  index  The enum index to be translated
     * @return A pointer to a modifier object or a null pointer if not found.
     */
    static Modifier *get(Type index)
    {
        try {
            return &_modifiers().at(index);
        } catch (std::out_of_range const &) {
            return nullptr;
        }
    }
    /**
     * A function to turn a string id into a modifier object.
     *
     * @param  id  The id string to be translated
     * @return A pointer to a modifier object or a NULL if not found.
     */
    static Modifier *get(char const *id);
};

} // namespace Modifiers
} // namespace Inkscape


#endif // SEEN_SP_MODIFIERS_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

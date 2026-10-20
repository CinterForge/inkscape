// SPDX-License-Identifier: GPL-2.0-or-later

// Simple paint selector widget presenting some style attributes:
//
// Fill, stroke, stroke-related attributes, markers,
// opacity, blend mode, filter(s)

#ifndef PAINT_ATTRIBUTE_H
#define PAINT_ATTRIBUTE_H

#include <glib/gi18n.h>
#include <glibmm/ustring.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <gtkmm/menubutton.h>
#include <sigc++/signal.h>

#include "color-preview.h"
#include "combo-enums.h"
#include "dash-selector.h"
#include "ink-property-grid.h"
#include "generic/spin-button.h"
#include "generic/spin-scale.h"
#include "style/marker-combo-box.h"
#include "paint-switch.h"
#include "stroke-options.h"
#include "style-internal.h"
#include "unit-menu.h"
#include "ui/widget/paint-popover-manager.h"
#include "widget-group.h"

namespace Inkscape::UI::Widget {

class PaintAttribute {
public:
    // List of all parts/attribute types that should be shown in the "PaintAttribute" widget
    enum Parts {
        NoParts = 0x00,
        FillPaint = 0x01,
        StrokePaint = 0x02,
        StrokeAttributes = 0x04,
        Opacity = 0x08,
        BlendMode = 0x10,
        AllParts = 0xff
    };
    PaintAttribute(Parts add_parts, unsigned int tag);

    void insert_widgets(InkPropertyGrid& grid);
    void set_document(SPDocument* document);
    void set_desktop(SPDesktop* desktop);
    // update UI from passed object style
    void update_from_object(SPObject* object);
    // update visibility and lock state
    void update_visibility(SPObject* object);

private:
    void set_paint(const SPObject* object, bool fill);
    //
    void update_markers(SPIMarker* markers[], SPObject* object);
    // show/hide stroke widgets
    void show_stroke(bool show);
    void update_stroke(SPItem* item);
    // true if attributes can be modified now, or false while the update is pending
    bool can_update() const;
    void update_reset_opacity_button();
    void update_reset_blend_button();
    void update_filters(SPObject* object);

    struct PaintStrip {
        PaintStrip(Glib::RefPtr<Gtk::Builder> builder, const Glib::ustring& title, bool fill, unsigned int tag);

        // set icon representing the current fill / stroke type
        void set_preview(const SPIPaint& paint, double paint_opacity, PaintMode mode);
        PaintMode update_preview_indicators(const SPObject* object);
        void set_paint(const SPObject* object);
        void set_paint(const SPIPaint& paint, double opacity, FillRule fill_rule);
        void set_fill_rule(FillRule rule);
        void set_flat_color(const Colors::Color& color);
        // mark an object as modified
        void request_update(bool update_preview);
        void show();
        void hide();
        bool can_update() const;
        std::vector<sigc::connection> connect_signals();

        Glib::RefPtr<Gtk::Builder> _builder;
        Gtk::Grid& _main;
        sigc::signal<void (bool)> _toggle_definition;
        bool _is_fill;
        Gtk::MenuButton& _paint_btn;
        PaintSwitch* _switch = nullptr;
        ColorPreview& _color_preview;
        Gtk::Image& _paint_icon;
        Gtk::Label& _label;
        InkSpinButton& _alpha;
        Gtk::Box& _box;
        Gtk::Button& _define;
        Gtk::Button& _clear;
        SPItem* _current_item = nullptr;
        SPDesktop* _desktop = nullptr;
        OperationBlocker* _update = nullptr;
        unsigned int _modified_tag;
        PaintPopoverManager::Registration _connection; // RAII token.
    };
    PaintStrip _fill;
    PaintStrip _stroke;
    Glib::RefPtr<Gtk::Builder> _builder;
    Gtk::Box& _markers;
    MarkerComboBox _marker_start = MarkerComboBox("marker-start", SP_MARKER_LOC_START);
    MarkerComboBox _marker_mid =   MarkerComboBox("marker-mid", SP_MARKER_LOC_MID);
    MarkerComboBox _marker_end =   MarkerComboBox("marker-end", SP_MARKER_LOC_END);
    DashSelector& _dash_selector;
    Gtk::MenuButton& _stroke_presets;
    Gtk::Box& _stroke_icons;
    InkSpinButton& _stroke_width;
    UnitMenu& _unit_selector;
    unsigned int _hairline_item = 0;
    Gtk::Popover& _stroke_popup;
    StrokeOptions _stroke_options;
    SpinScale& _opacity;
    ComboBoxEnum<SPBlendMode> _blend;
    Gtk::Button& _reset_blend;
    SPItem* _current_item = nullptr;
    SPObject* _current_object = nullptr;
    WidgetGroup _stroke_widgets;
    OperationBlocker _update;
    SPDesktop* _desktop = nullptr;
    const Unit* _current_unit = nullptr;
    Parts _added_parts;
    unsigned int _modified_tag;
    Gtk::Button& _visible;
};

} // namespace

#endif

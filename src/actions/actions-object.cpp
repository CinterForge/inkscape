// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for working with objects without GUI.
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include <giomm.h>  // Not <gtkmm.h>! To eventually allow a headless version!
#include <glibmm/i18n.h>

#include "actions-object.h"
#include "actions-helper.h"
#include "document-undo.h"
#include "inkscape-application.h"
#include "object/sp-star.h"
#include "preferences.h"
#include "selection.h"

#include "live_effects/effect.h"
#include "live_effects/lpe-powerclip.h"
#include "live_effects/lpe-powermask.h"
#include "object/sp-lpe-item.h"
#include "trace/potrace/inkscape-potrace.h"
#include "trace/trace.h"
#include "ui/icon-names.h"
#include "util/cast.h"

namespace {

double stod_finite(std::string const &str)
{
    double const result = std::stod(str);
    if (!std::isfinite(result)) {
        throw std::out_of_range{"stod: Inf or NaN"};
    }
    return result;
}

void object_trace(Glib::VariantBase const &value, InkscapeApplication *app)
{
    auto selection = app->get_active_selection();
    if (!selection || selection->isEmpty()) {
        show_output("action:object_trace: selection empty!", true);
        return;
    }

    auto const str = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(value);
    std::vector<Glib::ustring> const settings = Glib::Regex::split_simple(",", str.get());
    if (settings.size() != 7) {
        show_output("action:object_trace: expected argument format: {scans},{smooth[false|true]},{stack[false|true]},{remove_background[false|true],{speckles},{smooth_corners},{optimize}}", true);
        return;
    }

    int scans;
    bool smooth;
    bool stack;
    bool remove_background;
    int speckles;
    double smooth_corners;
    double optimize;
    try {
        scans = std::stoi(settings[0]);
        smooth = settings[1] == "true";
        stack = settings[2] == "true";
        remove_background = settings[3] == "true";
        speckles = std::stoi(settings[4]);
        smooth_corners = stod_finite(settings[5]);
        optimize = stod_finite(settings[6]);
    } catch (std::logic_error const &e) {
        show_output(std::string{"action:object_trace: parsing arguments failed: "} + e.what(), true);
        return;
    }

    auto tracer = std::make_unique<Inkscape::Trace::Potrace::PotraceTracingEngine>(Inkscape::Trace::Potrace::TraceType::QUANT_COLOR, false, 64,
                                                                                   0.45, 0.0, 0.65, scans, stack, smooth, remove_background);
    tracer->setOptiCurve(true);
    tracer->setTurdSize(speckles);
    tracer->setAlphaMax(smooth_corners);
    tracer->setOptTolerance(optimize);

    auto mainloop = Glib::MainLoop::create();

    auto future = Inkscape::Trace::trace(
        std::move(tracer),
        false,
        [] (double progress) {
            std::cout << "Tracing... " << std::round(100 * progress) << '%' << std::endl;
        },
        [&] {
            show_output("Tracing done.");
            mainloop->quit();
        }
    );

    if (!future) {
        show_output("Tracing failed.", true);
        return;
    }

    mainloop->run();
}


void
object_get_attribute(const Glib::VariantBase& value, InkscapeApplication *app)
{
    SPDocument* document = nullptr;
    Inkscape::Selection* selection = nullptr;
    if (!get_document_and_selection(app, &document, &selection)) {
        return;
    }
    auto const attribute = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(value).get();

    for (auto obj : selection->objects()) {
        Inkscape::XML::Node *repr = obj->getRepr();
        auto value = repr->attribute(attribute.c_str());
        show_output(value ? Glib::strescape(value) : "", false);
    }
}


void
object_get_property(const Glib::VariantBase& value, InkscapeApplication *app)
{
    SPDocument* document = nullptr;
    Inkscape::Selection* selection = nullptr;
    if (!get_document_and_selection(app, &document, &selection)) {
        return;
    }
    auto const attribute = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(value).get();

    for (auto obj : selection->objects()) {
        Inkscape::XML::Node *repr = obj->getRepr();
        SPCSSAttr *css = sp_repr_css_attr(repr, "style");
        auto value = sp_repr_css_property(css, attribute.c_str(), "");
        show_output(value ? Glib::strescape(value) : "", false);
        sp_repr_css_attr_unref(css);
    }
}

void
object_remove_attribute(Glib::VariantBase const &value, InkscapeApplication *app)
{
    Inkscape::Selection *selection = app->get_active_selection();
    if (!selection || selection->isEmpty()) {
        show_output("action:object_remove_attribute: selection empty!");
        return;
    }
    auto const attribute = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(value).get();

    for (auto obj : selection->objects()) {
        Inkscape::XML::Node *repr = obj->getRepr();
        repr->removeAttribute(attribute);
    }
    Inkscape::DocumentUndo::done(app->get_active_document(), RC_("Undo", "Action remove attribute from objects"), "");
}

void
object_remove_property(Glib::VariantBase const &value, InkscapeApplication *app)
{
    Inkscape::Selection *selection = app->get_active_selection();
    if (!selection || selection->isEmpty()) {
        show_output("action:object_remove_property: selection empty!");
        return;
    }
    auto const property = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(value).get();

    for (auto obj : selection->objects()) {
        Inkscape::XML::Node *repr = obj->getRepr();
        SPCSSAttr *css = sp_repr_css_attr(repr, "style");
        sp_repr_css_set_property(css, property.c_str(), nullptr);
        sp_repr_css_set(repr, css, "style");
        sp_repr_css_attr_unref(css);
    }
    Inkscape::DocumentUndo::done(app->get_active_document(), RC_("Undo", "Action remove property from objects"), "");
}

// No sanity checking is done... should probably add.
void
object_set_attribute(const Glib::VariantBase& value, InkscapeApplication *app)
{
    auto const argument = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(value).get();
    auto const comma_position = argument.find_first_of(',');
    if (comma_position == 0 || comma_position == Glib::ustring::npos) {
        show_output("action:object_set_attribute: requires 'attribute name, attribute value'");
        return;
    }
    auto const attribute = argument.substr(0, comma_position);
    auto const new_value = argument.substr(comma_position + 1);

    auto selection = app->get_active_selection();
    if (!selection || selection->isEmpty()) {
        show_output("action:object_set_attribute: selection empty!");
        return;
    }

    // Should this be a selection member function?
    for (auto obj : selection->objects()) {
        Inkscape::XML::Node *repr = obj->getRepr();
        repr->setAttribute(attribute, new_value);
    }

    // TODO: Needed to update repr (is this the best way?).
    Inkscape::DocumentUndo::done(app->get_active_document(), Inkscape::Util::Internal::ContextString("ActionObjectSetAttribute"), "");
}


// No sanity checking is done... should probably add.
void
object_set_property(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<Glib::ustring> s = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(value);

    std::vector<Glib::ustring> tokens = Glib::Regex::split_simple(",", s.get());
    if (tokens.size() != 2) {
        show_output("action:object_set_property: requires 'property name, property value'");
        return;
    }

    auto selection = app->get_active_selection();
    if (!selection || selection->isEmpty()) {
        show_output("action:object_set_property: selection empty!");
        return;
    }

    // Should this be a selection member function?
    for (auto obj : selection->objects()) {
        Inkscape::XML::Node *repr = obj->getRepr();
        SPCSSAttr *css = sp_repr_css_attr(repr, "style");
        sp_repr_css_set_property(css, tokens[0].c_str(), tokens[1].c_str());
        sp_repr_css_set(repr, css, "style");
        sp_repr_css_attr_unref(css);
    }

    // Needed to update repr (is this the best way?).
    Inkscape::DocumentUndo::done(app->get_active_document(), Inkscape::Util::Internal::ContextString("ActionObjectSetProperty"), "");
}


void
object_unlink_clones(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();
    if (!selection) {
        return;
    }

    selection->unlink();
}

bool
should_remove_original()
{
    return Inkscape::Preferences::get()->getBool("/options/maskobject/remove", true);
}

void
object_clip_set(InkscapeApplication *app)
{
    Inkscape::Selection *selection = app->get_active_selection();
    if (!selection) {
        return;
    }

    // Object Clip Set
    selection->setMask(true, false, should_remove_original());
    Inkscape::DocumentUndo::done(selection->document(), RC_("Undo", "Set clipping path"), "");
}

void
object_clip_set_inverse(InkscapeApplication *app)
{
    Inkscape::Selection *selection = app->get_active_selection();
    if (!selection) {
        return;
    }

    // Object Clip Set Inverse
    selection->setMask(true, false, should_remove_original());
    Inkscape::LivePathEffect::sp_inverse_powerclip(app->get_active_selection());
    Inkscape::DocumentUndo::done(app->get_active_document(), RC_("Undo", "Set Inverse Clip(LPE)"), "");
}

void
object_clip_release(InkscapeApplication *app)
{
    Inkscape::Selection *selection = app->get_active_selection();
    if (!selection) {
        return;
    }

    // Object Clip Release
    Inkscape::LivePathEffect::sp_remove_powerclip(app->get_active_selection());
    selection->unsetMask(true, true, should_remove_original());
    Inkscape::DocumentUndo::done(app->get_active_document(), RC_("Undo", "Release clipping path"), "");
}

void
object_clip_set_group(InkscapeApplication *app)
{
    Inkscape::Selection *selection = app->get_active_selection();
    if (!selection) {
        return;
    }

    selection->setClipGroup();
    // Undo added in setClipGroup().
}

void
object_mask_set(InkscapeApplication *app)
{
    Inkscape::Selection *selection = app->get_active_selection();
    if (!selection) {
        return;
    }

    // Object Mask Set
    selection->setMask(false, false, should_remove_original());
    Inkscape::DocumentUndo::done(selection->document(), RC_("Undo", "Set mask"), "");
}

void
object_mask_set_inverse(InkscapeApplication *app)
{
    Inkscape::Selection *selection = app->get_active_selection();
    if (!selection) {
        return;
    }

    // Object Mask Set Inverse
    selection->setMask(false, false, should_remove_original());
    Inkscape::LivePathEffect::sp_inverse_powermask(app->get_active_selection());
    Inkscape::DocumentUndo::done(app->get_active_document(), RC_("Undo", "Set Inverse Mask (LPE)"), "");
}

void
object_mask_release(InkscapeApplication *app)
{
    Inkscape::Selection *selection = app->get_active_selection();
    if (!selection) {
        return;
    }

    // Object Mask Release
    Inkscape::LivePathEffect::sp_remove_powermask(app->get_active_selection());
    selection->unsetMask(false, true, should_remove_original());
    Inkscape::DocumentUndo::done(app->get_active_document(), RC_("Undo", "Release mask"), "");
}

void
object_rotate_90_cw(InkscapeApplication *app)
{
    Inkscape::Selection *selection = app->get_active_selection();
    if (!selection) {
        return;
    }

    // Object Rotate 90
    auto doc = selection->document();
    selection->rotateAnchored((!doc || doc->yaxisdown()) ? 90 : -90);
}

void
object_rotate_90_ccw(InkscapeApplication *app)
{
    Inkscape::Selection *selection = app->get_active_selection();
    if (!selection) {
        return;
    }

    // Object Rotate 90 CCW
    auto doc = selection->document();
    selection->rotateAnchored((!doc || doc->yaxisdown()) ? -90 : 90);
}

void
object_flip_horizontal(InkscapeApplication *app)
{
    Inkscape::Selection *selection = app->get_active_selection();
    if (!selection) {
        return;
    }

    Geom::OptRect bbox = selection->visualBounds();
    if (!bbox) {
        return;
    }

    // Get center
    Geom::Point center;
    if (selection->center()) {
        center = *selection->center();
    } else {
        center = bbox->midpoint();
    }

    // Object Flip Horizontal
    selection->scaleRelative(center, Geom::Scale(-1.0, 1.0));
    Inkscape::DocumentUndo::done(app->get_active_document(), RC_("Undo", "Flip horizontally"), INKSCAPE_ICON("object-flip-horizontal"));
}

void
object_flip_vertical(InkscapeApplication *app)
{
    Inkscape::Selection *selection = app->get_active_selection();
    if (!selection) {
        return;
    }

    Geom::OptRect bbox = selection->visualBounds();
    if (!bbox) {
        return;
    }

    // Get center
    Geom::Point center;
    if (selection->center()) {
        center = *selection->center();
    } else {
        center = bbox->midpoint();
    }

    // Object Flip Vertical
    selection->scaleRelative(center, Geom::Scale(1.0, -1.0));
    Inkscape::DocumentUndo::done(app->get_active_document(), RC_("Undo", "Flip vertically"), INKSCAPE_ICON("object-flip-vertical"));
}

void
object_star_turn_upright(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();
    if (!selection || selection->isEmpty()) {
        show_output("action:object_star_turn_upright: selection empty!");
        return;
    }

    bool has_stars = false;
    for (auto obj : selection->objects()) {
        if (auto star = cast<SPStar>(obj)) {
            has_stars = true;
            star->turn_upright();
        }
    }

    if (!has_stars) {
        show_output("action:objects_star_turn_upright: no SPStar in selection!");
        return;
    }

    Inkscape::DocumentUndo::done(app->get_active_document(), RC_("Undo", "Turn stars upright"), INKSCAPE_ICON("object-level"));
}

void
object_to_path(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();
    if (!selection) {
        return;
    }

    selection->toCurves(false, Inkscape::Preferences::get()->getBool("/options/clonestocurvesjustunlink/value", true));
}

void
object_add_corners_lpe(InkscapeApplication *app) {
    auto selection = app->get_active_selection();
    if (!selection) {
        return;
    }

    // We should not have to do this!
    auto document  = app->get_active_document();
    if (!document) {
        return;
    }

    auto items = selection->items_vector();
    selection->clear();
    for (auto i : items) {
        if (auto lpeitem = cast<SPLPEItem>(i)) {
            if (auto lpe = lpeitem->getFirstPathEffectOfType(Inkscape::LivePathEffect::FILLET_CHAMFER)) {
                lpeitem->removePathEffect(lpe, false);
                Inkscape::DocumentUndo::done(document, RC_("Undo", "Remove Live Path Effect"), INKSCAPE_ICON("dialog-path-effects"));
            } else {
                Inkscape::LivePathEffect::Effect::createAndApply("fillet_chamfer", document, lpeitem);
                Inkscape::DocumentUndo::done(document, RC_("Undo", "Create and apply path effect"), INKSCAPE_ICON("dialog-path-effects"));
            }
            if (auto lpe = lpeitem->getCurrentLPE()) {
                lpe->refresh_widgets = true;
            }
        }
        selection->add(i);
    }
}

void
object_stroke_to_path(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();
    if (!selection) {
        return;
    }

    selection->strokesToPaths();
}

const Glib::ustring SECTION = NC_("Action Section", "Object");

std::vector<std::vector<Glib::ustring>> raw_data_object =
{
    // clang-format off
    {"app.object-set-attribute",      N_("Set Attribute"),           SECTION, N_("Set or update an attribute of selected objects; usage: object-set-attribute:attribute name, attribute value;")},
    {"app.object-set-property",       N_("Set Property"),            SECTION, N_("Set or update a property on selected objects; usage: object-set-property:property name, property value;")},
    {"app.object-get-attribute",      N_("Get Attribute"),           SECTION, N_("Get the value of an attribute of selected objects; usage: object-get-attribute:attribute name;")},
    {"app.object-get-property",       N_("Get Property"),            SECTION, N_("Get the value of a property on selected objects; usage: object-get-property:property name;")},
    {"app.object-remove-attribute",   N_("Remove Attribute"),        SECTION, N_("Remove an attribute on selected objects; usage: object-remove-attribute:property name;")},
    {"app.object-remove-property",    N_("Remove Property"),         SECTION, N_("Remove a property on selected objects; usage: object-remove-property:property name;")},

    {"app.object-unlink-clones",      N_("Unlink Clones"),           SECTION, N_("Unlink clones and symbols")},
    {"app.object-to-path",            N_("Object to Path"),          SECTION, N_("Convert shapes to paths")},
    {"app.object-add-corners-lpe",    N_("Add Corners LPE"),         SECTION, N_("Add Corners Live Path Effect to path")},
    {"app.object-stroke-to-path",     N_("Stroke to Path"),          SECTION, N_("Convert strokes to paths")},

    {"app.object-set-clip",           N_("Set Object Clipping"),         SECTION, N_("Apply clipping path to selection (using the topmost object as clipping path)")},
    {"app.object-set-inverse-clip",   N_("Set Object Inverse Clipping"), SECTION, N_("Apply inverse clipping path to selection (Power Clip LPE)")},
    {"app.object-release-clip",       N_("Release Object Clipping"),     SECTION, N_("Remove clipping path from selection")},
    {"app.object-set-clip-group",     N_("Set Object Clipping Group"),   SECTION, N_("Create a self-clipping group to which objects (not contributing to the clip-path) can be added")},
    {"app.object-set-mask",           N_("Set Object Mask"),         SECTION, N_("Apply mask to selection (using the topmost object as mask)")},
    {"app.object-set-inverse-mask",   N_("Set Object Inverse Mask"), SECTION, N_("Apply inverse mask to selection (Power Mask LPE)")},
    {"app.object-release-mask",       N_("Release Object Mask"),     SECTION, N_("Remove mask from selection")},

    {"app.object-rotate-90-cw",       N_("Rotate Object 90°"),        SECTION, N_("Rotate selected objects 90° clockwise")},
    {"app.object-rotate-90-ccw",      N_("Rotate Object 90° CCW"),    SECTION, N_("Rotate selected objects 90° counter-clockwise")},
    {"app.object-flip-horizontal",    N_("Flip Object Horizontally"),  SECTION, N_("Flip selected objects horizontally")},
    {"app.object-flip-vertical",      N_("Flip Object Vertically"),    SECTION, N_("Flip selected objects vertically")},
    {"app.object-star-turn-upright",  N_("Turn Stars/Polygons Upright"), SECTION, N_("Turn stars and polygons upright")}
    // clang-format on
};

std::vector<std::vector<Glib::ustring>> hint_data_object =
{
    // clang-format off
    {"app.object-set-attribute",        N_("Enter comma-separated string for attribute name, attribute value") },
    {"app.object-set-property",         N_("Enter comma-separated string for property name, property value")  }
    // clang-format on
};

} // namespace

void
add_actions_object(InkscapeApplication* app)
{
    Glib::VariantType Bool(  Glib::VARIANT_TYPE_BOOL);
    Glib::VariantType Int(   Glib::VARIANT_TYPE_INT32);
    Glib::VariantType Double(Glib::VARIANT_TYPE_DOUBLE);
    Glib::VariantType String(Glib::VARIANT_TYPE_STRING);

    auto *gapp = app->gio_app();

    // clang-format off
    gapp->add_action_with_parameter( "object-set-attribute",            String, sigc::bind(sigc::ptr_fun(&object_set_attribute),  app));
    gapp->add_action_with_parameter( "object-set-property",             String, sigc::bind(sigc::ptr_fun(&object_set_property),   app));
    gapp->add_action_with_parameter( "object-get-attribute",            String, sigc::bind(sigc::ptr_fun(&object_get_attribute),  app));
    gapp->add_action_with_parameter( "object-get-property",             String, sigc::bind(sigc::ptr_fun(&object_get_property),   app));
    gapp->add_action_with_parameter( "object-remove-attribute",         String, sigc::bind(sigc::ptr_fun(&object_remove_attribute),  app));
    gapp->add_action_with_parameter( "object-remove-property",          String, sigc::bind(sigc::ptr_fun(&object_remove_property),   app));
    gapp->add_action_with_parameter( "object-trace",                    String, sigc::bind(sigc::ptr_fun(&object_trace),          app));

    gapp->add_action(                "object-unlink-clones",            sigc::bind(sigc::ptr_fun(&object_unlink_clones),          app));
    gapp->add_action(                "object-to-path",                  sigc::bind(sigc::ptr_fun(&object_to_path),                app));
    gapp->add_action(                "object-add-corners-lpe",          sigc::bind(sigc::ptr_fun(&object_add_corners_lpe),        app));
    gapp->add_action(                "object-stroke-to-path",           sigc::bind(sigc::ptr_fun(&object_stroke_to_path),         app));

    gapp->add_action(                "object-set-clip",                 sigc::bind(sigc::ptr_fun(&object_clip_set),               app));
    gapp->add_action(                "object-set-inverse-clip",         sigc::bind(sigc::ptr_fun(&object_clip_set_inverse),       app));
    gapp->add_action(                "object-release-clip",             sigc::bind(sigc::ptr_fun(&object_clip_release),           app));
    gapp->add_action(                "object-set-clip-group",           sigc::bind(sigc::ptr_fun(&object_clip_set_group),         app));
    gapp->add_action(                "object-set-mask",                 sigc::bind(sigc::ptr_fun(&object_mask_set),               app));
    gapp->add_action(                "object-set-inverse-mask",         sigc::bind(sigc::ptr_fun(&object_mask_set_inverse),       app));
    gapp->add_action(                "object-release-mask",             sigc::bind(sigc::ptr_fun(&object_mask_release),           app));

    // Deprecated, see app.transform-rotate(90)
    gapp->add_action(                "object-rotate-90-cw",             sigc::bind(sigc::ptr_fun(&object_rotate_90_cw),           app));
    gapp->add_action(                "object-rotate-90-ccw",            sigc::bind(sigc::ptr_fun(&object_rotate_90_ccw),          app));
    gapp->add_action(                "object-flip-horizontal",          sigc::bind(sigc::ptr_fun(&object_flip_horizontal),        app));
    gapp->add_action(                "object-flip-vertical",            sigc::bind(sigc::ptr_fun(&object_flip_vertical),          app));
    gapp->add_action(                "object-star-turn-upright",        sigc::bind(sigc::ptr_fun(&object_star_turn_upright),      app));
    // clang-format on

    app->get_action_extra_data().add_data(raw_data_object);
    app->get_action_hint_data().add_data(hint_data_object);
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

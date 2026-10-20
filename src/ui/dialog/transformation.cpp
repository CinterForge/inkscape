// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Transform dialog - implementation.
 */
/* Authors:
 *   Bryce W. Harrington <bryce@bryceharrington.org>
 *   buliabyak@gmail.com
 *   Abhishek Sharma
 *
 * Copyright (C) 2004, 2005 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "transformation.h"

#include <glibmm/i18n.h>
#include <gtkmm/image.h>
#include <gtkmm/grid.h>
#include <gtkmm/version.h>

#include "desktop.h"
#include "document-undo.h"
#include "preferences.h"
#include "selection.h"
#include "object/sp-namedview.h"
#include "ui/icon-names.h"
#include "ui/pack.h"
#include "ui/widget/spinbutton.h"
#include "util/transform-objects.h"

using Inkscape::UI::Widget::Scalar;

namespace Inkscape::UI::Dialog {

/*########################################################################
# C O N S T R U C T O R
########################################################################*/

Transformation::Transformation()
    : DialogBase("/dialogs/transformation", "Transform"),

      _page_move              (4, 2),
      _page_scale             (4, 2),
      _page_rotate            (4, 2),
      _page_skew              (4, 2),
      _page_transform         (3, 3),

      _scalar_move_horizontal (_("_Horizontal"), _("Horizontal displacement (relative) or position (absolute)"), UNIT_TYPE_LINEAR,
                               "transform-move-horizontal", &_units_move),
      _scalar_move_vertical   (_("_Vertical"),  _("Vertical displacement (relative) or position (absolute)"), UNIT_TYPE_LINEAR,
                               "transform-move-vertical", &_units_move),
      _scalar_scale_horizontal(_("_Width"), _("Horizontal size (absolute or percentage of current)"), UNIT_TYPE_DIMENSIONLESS,
                               "transform-scale-horizontal", &_units_scale),
      _scalar_scale_vertical  (_("_Height"),  _("Vertical size (absolute or percentage of current)"), UNIT_TYPE_DIMENSIONLESS,
                               "transform-scale-vertical", &_units_scale),
      _scalar_rotate          (_("A_ngle"), _("Rotation angle (positive = counterclockwise)"), UNIT_TYPE_RADIAL,
                               "transform-rotate", &_units_rotate),
      _scalar_skew_horizontal (_("_Horizontal"), _("Horizontal skew angle (positive = counterclockwise), or absolute displacement, or percentage displacement"), UNIT_TYPE_LINEAR,
                               "transform-skew-horizontal", &_units_skew),
      _scalar_skew_vertical   (_("_Vertical"),  _("Vertical skew angle (positive = clockwise), or absolute displacement, or percentage displacement"),  UNIT_TYPE_LINEAR,
                               "transform-skew-vertical", &_units_skew),

      _scalar_transform_a     ({}, _("Transformation matrix element A")),
      _scalar_transform_b     ({}, _("Transformation matrix element B")),
      _scalar_transform_c     ({}, _("Transformation matrix element C")),
      _scalar_transform_d     ({}, _("Transformation matrix element D")),
      _scalar_transform_e     ({}, _("Transformation matrix element E"),
                               UNIT_TYPE_LINEAR, {}, &_units_transform),
      _scalar_transform_f     ({}, _("Transformation matrix element F"),
                               UNIT_TYPE_LINEAR, {}, &_units_transform),

      _check_move_relative     (_("Rela_tive move")),
      _check_scale_proportional(_("_Scale proportionally")),
      _check_apply_separately  (_("Apply to each _object separately")),
      _check_replace_matrix    (_("Edit c_urrent matrix")),

      _apply_buttons_size_group{Gtk::SizeGroup::create(Gtk::SizeGroup::Mode::HORIZONTAL)},
      applyButton{Gtk::make_managed<Gtk::Button>(_("_Apply"))},
      duplicateButton{Gtk::make_managed<Gtk::Button>(_("_Duplicate"))},
      resetButton{Gtk::make_managed<Gtk::Button>()}
{
    _scalar_move_horizontal.getLabel()->set_hexpand();
    _scalar_move_vertical.getLabel()->set_hexpand();
    _scalar_scale_horizontal.getLabel()->set_hexpand();
    _scalar_scale_vertical.getLabel()->set_hexpand();
    _scalar_skew_horizontal.getLabel()->set_hexpand();
    _scalar_skew_vertical.getLabel()->set_hexpand();

    _check_move_relative.set_use_underline();
    _check_move_relative.set_tooltip_text(_("Add the specified relative displacement to the current position; otherwise, edit the current absolute position directly"));

    _check_scale_proportional.set_use_underline();
    _check_scale_proportional.set_tooltip_text(_("Preserve the width/height ratio of the scaled objects"));

    _check_apply_separately.set_use_underline();
    _check_apply_separately.set_tooltip_text(_("Apply the scale/rotate/skew to each selected object separately; otherwise, transform the selection as a whole"));
    _check_apply_separately.set_margin_start(6);
    _check_replace_matrix.set_use_underline();
    _check_replace_matrix.set_tooltip_text(_("Edit the current transform= matrix; otherwise, post-multiply transform= by this matrix"));

    // Notebook for individual transformations
    UI::pack_start(*this, _notebook, false, false);

    _page_move.set_halign(Gtk::Align::START);
    _notebook.append_page(_page_move, _("_Move"), true);
    layoutPageMove();

    _page_scale.set_halign(Gtk::Align::START);
    _notebook.append_page(_page_scale, _("_Scale"), true);
    layoutPageScale();

    _page_rotate.set_halign(Gtk::Align::START);
    _notebook.append_page(_page_rotate, _("_Rotate"), true);
    layoutPageRotate();

    _page_skew.set_halign(Gtk::Align::START);
    _notebook.append_page(_page_skew, _("Ske_w"), true);
    layoutPageSkew();

    _page_transform.set_halign(Gtk::Align::START);
    _notebook.append_page(_page_transform, _("Matri_x"), true);
    layoutPageTransform();

    _tabSwitchConn = _notebook.signal_switch_page().connect(sigc::mem_fun(*this, &Transformation::onSwitchPage));

    // Apply separately
    UI::pack_start(*this, _check_apply_separately, false, false);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    _check_apply_separately.set_active(prefs->getBool("/dialogs/transformation/applyseparately"));
    _check_apply_separately.signal_toggled().connect(sigc::mem_fun(*this, &Transformation::onApplySeparatelyToggled));
    _check_apply_separately.set_visible(false);

#if GTKMM_CHECK_VERSION(4, 14, 0)
    // make sure all spinbuttons activate Apply on pressing Enter
    auto const apply_on_activate = [this](UI::Widget::ScalarUnit &scalar) {
        scalar.getSpinButton().signal_activate().connect([this] { _apply(false); });
    };
    apply_on_activate(_scalar_move_horizontal );
    apply_on_activate(_scalar_move_vertical   );
    apply_on_activate(_scalar_scale_horizontal);
    apply_on_activate(_scalar_scale_vertical  );
    apply_on_activate(_scalar_rotate          );
    apply_on_activate(_scalar_skew_horizontal );
    apply_on_activate(_scalar_skew_vertical   );
#endif

    resetButton->set_image_from_icon_name("reset-settings-symbolic");
    resetButton->set_size_request(30, -1);
    resetButton->set_halign(Gtk::Align::CENTER);
    resetButton->set_use_underline();
    resetButton->set_tooltip_text(_("Reset the values on the current tab to defaults"));
    resetButton->set_sensitive(true);
    resetButton->signal_clicked().connect(sigc::mem_fun(*this, &Transformation::onClear));

    duplicateButton->set_use_underline();
    duplicateButton->set_halign(Gtk::Align::CENTER);
    duplicateButton->set_tooltip_text(_("Duplicate selection and apply transformation to the copy"));
    duplicateButton->set_sensitive(false);
    duplicateButton->signal_clicked().connect([this] { _apply(true); });

    applyButton->set_use_underline();
    applyButton->set_halign(Gtk::Align::CENTER);
    applyButton->set_tooltip_text(_("Apply transformation to selection"));
    applyButton->set_sensitive(false);
    applyButton->signal_clicked().connect([this] { _apply(false); });

    _apply_buttons_size_group->add_widget(*duplicateButton);
    _apply_buttons_size_group->add_widget(*applyButton);

    auto const button_box = Gtk::make_managed<Gtk::Box>();
    button_box->set_margin_top(4);
    button_box->set_spacing(8);
    button_box->set_halign(Gtk::Align::CENTER);
    UI::pack_start(*button_box, *duplicateButton);
    UI::pack_start(*button_box, *applyButton);
    UI::pack_start(*button_box, *resetButton);
    UI::pack_start(*this, *button_box, UI::PackOptions::shrink);
}

void Transformation::selectionChanged(Inkscape::Selection *selection)
{
    setButtonsSensitive();
    updateSelection((Inkscape::UI::Dialog::Transformation::PageType)getCurrentPage(), selection);
}
void Transformation::selectionModified(Inkscape::Selection *selection, guint flags)
{
    selectionChanged(selection);
}

/*########################################################################
# U T I L I T Y
########################################################################*/

void Transformation::presentPage(Transformation::PageType page)
{
    _notebook.set_current_page(page);
    set_visible(true);
}

void Transformation::setButtonsSensitive()
{
    auto selection = getSelection();
    bool const has_selection = selection && !selection->isEmpty();

    applyButton->set_sensitive(has_selection);
    duplicateButton->set_sensitive(has_selection);
}

Geom::Affine Transformation::getCurrentMatrix()
{
    double a = _scalar_transform_a.getValue();
    double b = _scalar_transform_b.getValue();
    double c = _scalar_transform_c.getValue();
    double d = _scalar_transform_d.getValue();
    double e = _scalar_transform_e.getValue("px");
    double f = _scalar_transform_f.getValue("px");
    return Geom::Affine(a, b, c, d, e, f);
}

/*########################################################################
# S E T U P   L A Y O U T
########################################################################*/

void Transformation::layoutPageMove()
{
    _units_move.setUnitType(UNIT_TYPE_LINEAR);

    _scalar_move_horizontal.initScalar(Scalar::COMMON_MIN, Scalar::COMMON_MAX);
    _scalar_move_horizontal.setDigits(3);
    _scalar_move_horizontal.setIncrements(0.1, 1.0);
    _scalar_move_horizontal.set_hexpand();
    _scalar_move_horizontal.setWidthChars(7);

    _scalar_move_vertical.initScalar(Scalar::COMMON_MIN, Scalar::COMMON_MAX);
    _scalar_move_vertical.setDigits(3);
    _scalar_move_vertical.setIncrements(0.1, 1.0);
    _scalar_move_vertical.set_hexpand();
    _scalar_move_vertical.setWidthChars(7);

    _page_move.table().attach(_scalar_move_horizontal, 0, 0, 2, 1);
    _page_move.table().attach(_units_move,             2, 0, 1, 1);

    _page_move.table().attach(_scalar_move_vertical, 0, 1, 2, 1);

    // Relative moves
    _page_move.table().attach(_check_move_relative, 0, 2, 2, 1);

    _check_move_relative.set_active(true);
    _check_move_relative.signal_toggled()
        .connect(sigc::mem_fun(*this, &Transformation::onMoveRelativeToggled));
}

void Transformation::layoutPageScale()
{
    _units_scale.setUnitType(UNIT_TYPE_DIMENSIONLESS);
    _units_scale.setUnitType(UNIT_TYPE_LINEAR);

    _scalar_scale_horizontal.initScalar(Scalar::COMMON_MIN, Scalar::COMMON_MAX);
    _scalar_scale_horizontal.setValue(100.0, "%");
    _scalar_scale_horizontal.setDigits(3);
    _scalar_scale_horizontal.setIncrements(0.1, 1.0);
    _scalar_scale_horizontal.setAbsoluteIsIncrement(true);
    _scalar_scale_horizontal.setPercentageIsIncrement(true);
    _scalar_scale_horizontal.set_hexpand();
    _scalar_scale_horizontal.setWidthChars(7);

    _scalar_scale_vertical.initScalar(Scalar::COMMON_MIN, Scalar::COMMON_MAX);
    _scalar_scale_vertical.setValue(100.0, "%");
    _scalar_scale_vertical.setDigits(3);
    _scalar_scale_vertical.setIncrements(0.1, 1.0);
    _scalar_scale_vertical.setAbsoluteIsIncrement(true);
    _scalar_scale_vertical.setPercentageIsIncrement(true);
    _scalar_scale_vertical.set_hexpand();
    _scalar_scale_vertical.setWidthChars(7);

    _page_scale.table().attach(_scalar_scale_horizontal, 0, 0, 2, 1);

    _scalar_scale_horizontal.signal_value_changed()
        .connect(sigc::mem_fun(*this, &Transformation::onScaleXValueChanged));

    _page_scale.table().attach(_units_scale,           2, 0, 1, 1);
    _page_scale.table().attach(_scalar_scale_vertical, 0, 1, 2, 1);

    _scalar_scale_vertical.signal_value_changed()
        .connect(sigc::mem_fun(*this, &Transformation::onScaleYValueChanged));

    _page_scale.table().attach(_check_scale_proportional, 0, 2, 2, 1);

    _check_scale_proportional.set_active(false);
    _check_scale_proportional.signal_toggled()
        .connect(sigc::mem_fun(*this, &Transformation::onScaleProportionalToggled));

    //TODO: add a widget for selecting the fixed point in scaling, or honour rotation center?
}

void Transformation::layoutPageRotate()
{
    _units_rotate.setUnitType(UNIT_TYPE_RADIAL);

    _scalar_rotate.initScalar(-360.0, 360.0);
    _scalar_rotate.setDigits(3);
    _scalar_rotate.setIncrements(0.1, 1.0);
    _scalar_rotate.set_hexpand();
    _scalar_rotate.setWidthChars(7);

    _counterclockwise_rotate.set_icon_name("object-rotate-left");
    _counterclockwise_rotate.set_has_frame(false);
    _counterclockwise_rotate.set_tooltip_text(_("Rotate in a counterclockwise direction"));

    _clockwise_rotate.set_icon_name("object-rotate-right");
    _clockwise_rotate.set_has_frame(false);
    _clockwise_rotate.set_tooltip_text(_("Rotate in a clockwise direction"));
    _clockwise_rotate.set_group(_counterclockwise_rotate);

    auto const box = Gtk::make_managed<Gtk::Box>();
    _counterclockwise_rotate.set_halign(Gtk::Align::START);
    _clockwise_rotate.set_halign(Gtk::Align::START);
    UI::pack_start(*box, _counterclockwise_rotate);
    UI::pack_start(*box, _clockwise_rotate);

    _page_rotate.table().attach(_scalar_rotate, 0, 0, 1, 1);
    _page_rotate.table().attach(_units_rotate,  1, 0, 1, 1);
    _page_rotate.table().attach(*box,           1, 1, 1, 1);

    _counterclockwise_rotate.signal_clicked().connect(sigc::mem_fun(*this, &Transformation::onRotateCounterclockwiseClicked));
    _clockwise_rotate.signal_clicked().connect(sigc::mem_fun(*this, &Transformation::onRotateClockwiseClicked));

    //TODO: honour rotation center?
}

void Transformation::layoutPageSkew()
{
    _units_skew.setUnitType(UNIT_TYPE_LINEAR);
    _units_skew.setUnitType(UNIT_TYPE_DIMENSIONLESS);
    _units_skew.setUnitType(UNIT_TYPE_RADIAL);

    _scalar_skew_horizontal.initScalar(Scalar::COMMON_MIN, Scalar::COMMON_MAX);
    _scalar_skew_horizontal.setDigits(3);
    _scalar_skew_horizontal.setIncrements(0.1, 1.0);
    _scalar_skew_horizontal.set_hexpand();
    _scalar_skew_horizontal.setWidthChars(7);

    _scalar_skew_vertical.initScalar(Scalar::COMMON_MIN, Scalar::COMMON_MAX);
    _scalar_skew_vertical.setDigits(3);
    _scalar_skew_vertical.setIncrements(0.1, 1.0);
    _scalar_skew_vertical.set_hexpand();
    _scalar_skew_vertical.setWidthChars(7);

    _page_skew.table().attach(_scalar_skew_horizontal, 0, 0, 2, 1);
    _page_skew.table().attach(_units_skew,             2, 0, 1, 1);
    _page_skew.table().attach(_scalar_skew_vertical,   0, 1, 2, 1);

    //TODO: honour rotation center?
}

void Transformation::layoutPageTransform()
{
    _units_transform.setUnitType(UNIT_TYPE_LINEAR);
    _units_transform.set_tooltip_text(_("E and F units"));
    _units_transform.set_halign(Gtk::Align::END);
    _units_transform.set_margin_top(3);
    _units_transform.set_margin_bottom(3);

    UI::Widget::Scalar* labels[] = {&_scalar_transform_a, &_scalar_transform_b, &_scalar_transform_c, &_scalar_transform_d, &_scalar_transform_e, &_scalar_transform_f};
    for (auto label : labels) {
        label->hide_label();
        label->set_margin_start(2);
        label->set_margin_end(2);
        label->getWidget()->set_size_request(65, -1);
        label->setRange(-1e10, 1e10);
        label->setDigits(3);
        label->setIncrements(0.1, 1.0);
        label->setValue(1.0);
        label->setWidthChars(6);
        label->set_hexpand();
    }
    _page_transform.table().set_column_spacing(0);
    _page_transform.table().set_row_spacing(1);
    _page_transform.table().set_column_homogeneous(true);

    _page_transform.table().attach(*Gtk::make_managed<Gtk::Label>("A"), 0, 0, 1, 1);
    _page_transform.table().attach(_scalar_transform_a, 0, 1, 1, 1);

    _page_transform.table().attach(*Gtk::make_managed<Gtk::Label>("B"), 0, 2, 1, 1);
    _page_transform.table().attach(_scalar_transform_b, 0, 3, 1, 1);

    _page_transform.table().attach(*Gtk::make_managed<Gtk::Label>("C"), 1, 0, 1, 1);
    _page_transform.table().attach(_scalar_transform_c, 1, 1, 1, 1);

    _page_transform.table().attach(*Gtk::make_managed<Gtk::Label>("D"), 1, 2, 1, 1);
    _page_transform.table().attach(_scalar_transform_d, 1, 3, 1, 1);

    _page_transform.table().attach(*Gtk::make_managed<Gtk::Label>("E"), 2, 0, 1, 1);
    _page_transform.table().attach(_scalar_transform_e, 2, 1, 1, 1);

    _page_transform.table().attach(*Gtk::make_managed<Gtk::Label>("F"), 2, 2, 1, 1);
    _page_transform.table().attach(_scalar_transform_f, 2, 3, 1, 1);

    auto const img = Gtk::make_managed<Gtk::Image>();
    img->set_from_icon_name("matrix-2d");
    img->set_pixel_size(52);
    img->set_margin_top(4);
    img->set_margin_bottom(4);
    _page_transform.table().attach(*img, 0, 5, 1, 1);

    auto const descr = Gtk::make_managed<Gtk::Label>();
    descr->set_wrap();
    descr->set_wrap_mode(Pango::WrapMode::WORD);
    descr->set_text(
        _("<small>"
        "<a href=\"https://www.w3.org/TR/SVG11/coords.html#TransformMatrixDefined\">"
        "2D transformation matrix</a> that combines translation (E,F), scaling (A,D),"
        " rotation (A-D) and shearing (B,C)."
        "</small>")
    );
    descr->set_use_markup();
    _page_transform.table().attach(*descr, 1, 5, 2, 1);

    _page_transform.table().attach(_units_transform, 2, 4, 1, 1);

    // Edit existing matrix
    _page_transform.table().attach(_check_replace_matrix, 0, 4, 2, 1);

    _check_replace_matrix.set_active(false);
    _check_replace_matrix.signal_toggled()
        .connect(sigc::mem_fun(*this, &Transformation::onReplaceMatrixToggled));
}

/*########################################################################
# U P D A T E
########################################################################*/

void Transformation::updateSelection(PageType page, Inkscape::Selection *selection)
{
    bool const has_selection = selection && !selection->isEmpty();

    _check_apply_separately.set_visible(selection && selection->size() > 1);

    if (!has_selection)
        return;

    switch (page) {
        case PAGE_MOVE: {
            updatePageMove(selection);
            break;
        }
        case PAGE_SCALE: {
            updatePageScale(selection);
            break;
        }
        case PAGE_ROTATE: {
            updatePageRotate(selection);
            break;
        }
        case PAGE_SKEW: {
            updatePageSkew(selection);
            break;
        }
        case PAGE_TRANSFORM: {
            updatePageTransform(selection);
            break;
        }
        case PAGE_QTY: {
            break;
        }
    }
}

void Transformation::onSwitchPage(Gtk::Widget * /*page*/, guint pagenum)
{
    if (!getDesktop()) {
        return;
    }

    updateSelection((PageType)pagenum, getDesktop()->getSelection());
}

void Transformation::updatePageMove(Inkscape::Selection *selection)
{
    if (selection && !selection->isEmpty()) {
        if (!_check_move_relative.get_active()) {
            Geom::OptRect bbox = selection->preferredBounds();
            if (bbox) {
                double x = bbox->min()[Geom::X];
                double y = bbox->min()[Geom::Y];

                double conversion = _units_move.getConversion("px");
                _scalar_move_horizontal.setValue(x / conversion);
                _scalar_move_vertical.setValue(y / conversion);
            }
        } else {
            // do nothing, so you can apply the same relative move to many objects in turn
        }
        _page_move.set_sensitive(true);
    } else {
        _page_move.set_sensitive(false);
    }
}

void Transformation::updatePageScale(Inkscape::Selection *selection)
{
    if (selection && !selection->isEmpty()) {
        Geom::OptRect bbox = selection->preferredBounds();
        if (bbox) {
            double w = bbox->dimensions()[Geom::X];
            double h = bbox->dimensions()[Geom::Y];
            _scalar_scale_horizontal.setHundredPercent(w);
            _scalar_scale_vertical.setHundredPercent(h);
            onScaleXValueChanged(); // to update x/y proportionality if switch is on
            _page_scale.set_sensitive(true);
        } else {
            _page_scale.set_sensitive(false);
        }
    } else {
        _page_scale.set_sensitive(false);
    }
}

void Transformation::updatePageRotate(Inkscape::Selection *selection)
{
    if (selection && !selection->isEmpty()) {
        _page_rotate.set_sensitive(true);
    } else {
        _page_rotate.set_sensitive(false);
    }
}

void Transformation::updatePageSkew(Inkscape::Selection *selection)
{
    if (selection && !selection->isEmpty()) {
        Geom::OptRect bbox = selection->preferredBounds();
        if (bbox) {
            double w = bbox->dimensions()[Geom::X];
            double h = bbox->dimensions()[Geom::Y];
            _scalar_skew_vertical.setHundredPercent(w);
            _scalar_skew_horizontal.setHundredPercent(h);
            _page_skew.set_sensitive(true);
        } else {
            _page_skew.set_sensitive(false);
        }
    } else {
        _page_skew.set_sensitive(false);
    }
}

void Transformation::updatePageTransform(Inkscape::Selection *selection)
{
    if (selection && !selection->isEmpty()) {
        if (_check_replace_matrix.get_active()) {
            Geom::Affine current (selection->items().front()->transform); // take from the first item in selection

            Geom::Affine new_displayed = current;

            _scalar_transform_a.setValue(new_displayed[0]);
            _scalar_transform_b.setValue(new_displayed[1]);
            _scalar_transform_c.setValue(new_displayed[2]);
            _scalar_transform_d.setValue(new_displayed[3]);
            _scalar_transform_e.setValue(new_displayed[4], "px");
            _scalar_transform_f.setValue(new_displayed[5], "px");
        } else {
            // do nothing, so you can apply the same matrix to many objects in turn
        }
        _page_transform.set_sensitive(true);
    } else {
        _page_transform.set_sensitive(false);
    }
}

/*########################################################################
# A P P L Y
########################################################################*/

void Transformation::_apply(bool duplicate_first)
{
    auto selection = getSelection();
    if (!selection || selection->isEmpty())
        return;

    int const page = _notebook.get_current_page();

    if (page == PAGE_TRANSFORM) {
        applyPageTransform(selection, duplicate_first);
        return;
    }

    if (duplicate_first) {
        selection->duplicate();
    }

    switch (page) {
        case PAGE_MOVE: {
            applyPageMove(selection);
            break;
        }
        case PAGE_ROTATE: {
            applyPageRotate(selection);
            break;
        }
        case PAGE_SCALE: {
            applyPageScale(selection);
            break;
        }
        case PAGE_SKEW: {
            applyPageSkew(selection);
            break;
        }
    }
}

void Transformation::applyPageMove(Inkscape::Selection *selection)
{
    double x = _scalar_move_horizontal.getValue("px");
    double y = _scalar_move_vertical.getValue("px");
    auto *prefs = Inkscape::Preferences::get();
    bool relative = _check_move_relative.get_active();
    bool apply_separately = prefs->getBool("/dialogs/transformation/applyseparately");
    transform_move(selection, x, y, relative, apply_separately, getDesktop()->yaxisdir());
    DocumentUndo::done(selection->desktop()->getDocument(), RC_("Undo", "Move"), INKSCAPE_ICON("dialog-transform"));
}

void Transformation::applyPageScale(Inkscape::Selection *selection)
{
    double scaleX = _scalar_scale_horizontal.getValue("px");
    double scaleY = _scalar_scale_vertical.getValue("px");

    auto *prefs = Inkscape::Preferences::get();
    bool transform_stroke = prefs->getBool("/options/transform/stroke", true);
    bool preserve = prefs->getBool("/options/preservetransform/value", false);
    bool apply_separately = prefs->getBool("/dialogs/transformation/applyseparately");
    bool is_percent = !_units_scale.isAbsolute();

    transform_scale(selection, scaleX, scaleY, is_percent, apply_separately, transform_stroke, preserve);
    DocumentUndo::done(selection->desktop()->getDocument(), RC_("Undo", "Scale"), INKSCAPE_ICON("dialog-transform"));
}

void Transformation::applyPageRotate(Inkscape::Selection *selection)
{
    double angle = _scalar_rotate.getValue(DEG);
    auto *prefs = Inkscape::Preferences::get();
    if (!prefs->getBool("/dialogs/transformation/rotateCounterClockwise", true)) {
        angle *= -1;
    }
    bool apply_separately = prefs->getBool("/dialogs/transformation/applyseparately");
    transform_rotate(selection, angle, apply_separately);
    DocumentUndo::done(selection->desktop()->getDocument(), RC_("Undo", "Rotate"), INKSCAPE_ICON("dialog-transform"));
}

void Transformation::applyPageSkew(Inkscape::Selection *selection)
{
    auto *prefs = Inkscape::Preferences::get();
    bool apply_separately = prefs->getBool("/dialogs/transformation/applyseparately");

    SkewUnits units = SkewUnits::Absolute;
    double hx = 0.0, hy = 0.0;
    if (!_units_skew.isAbsolute()) {
        units = SkewUnits::Percent;
        hx = _scalar_skew_horizontal.getValue("%");
        hy = _scalar_skew_vertical.getValue("%");
    } else if (_units_skew.isRadial()) {
        units = SkewUnits::AngleRadians;
        hx = _scalar_skew_horizontal.getValue("rad");
        hy = _scalar_skew_vertical.getValue("rad");
    } else {
        units = SkewUnits::Absolute;
        hx = _scalar_skew_horizontal.getValue("px");
        hy = _scalar_skew_vertical.getValue("px");
    }

    transform_skew(selection, hx, hy, units, apply_separately, getDesktop()->yaxisdir());
    DocumentUndo::done(selection->desktop()->getDocument(), RC_("Undo", "Skew"), INKSCAPE_ICON("dialog-transform"));
}

void Transformation::applyPageTransform(Inkscape::Selection *selection, bool duplicate_first)
{
    Geom::Affine displayed = getCurrentMatrix(); // read values before selection changes
    if (displayed.isSingular()) {
        getDesktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Transform matrix is singular, <b>not used</b>."));
        return;
    }

    if (duplicate_first) {
        selection->duplicate();
    }

    bool replace = _check_replace_matrix.get_active();
    transform_apply_matrix(selection, displayed, replace);

    DocumentUndo::done(selection->desktop()->getDocument(), RC_("Undo", "Edit transformation matrix"), INKSCAPE_ICON("dialog-transform"));
}

/*########################################################################
# V A L U E - C H A N G E D    C A L L B A C K S
########################################################################*/

void Transformation::onMoveRelativeToggled()
{
    auto selection = getSelection();
    if (!selection || selection->isEmpty())
        return;

    double x = _scalar_move_horizontal.getValue("px");
    double y = _scalar_move_vertical.getValue("px");

    double conversion = _units_move.getConversion("px");

    //g_message("onMoveRelativeToggled: %f, %f px\n", x, y);

    Geom::OptRect bbox = selection->preferredBounds();

    if (bbox) {
        if (_check_move_relative.get_active()) {
            // From absolute to relative
            _scalar_move_horizontal.setValue((x - bbox->min()[Geom::X]) / conversion);
            _scalar_move_vertical.setValue((  y - bbox->min()[Geom::Y]) / conversion);
        } else {
            // From relative to absolute
            _scalar_move_horizontal.setValue((bbox->min()[Geom::X] + x) / conversion);
            _scalar_move_vertical.setValue((  bbox->min()[Geom::Y] + y) / conversion);
        }
    }
}

void Transformation::onScaleXValueChanged()
{
    if (_scalar_scale_horizontal.setProgrammatically) {
        _scalar_scale_horizontal.setProgrammatically = false;
        return;
    }

    if (_check_scale_proportional.get_active()) {
        if (!_units_scale.isAbsolute()) { // percentage, just copy over
            _scalar_scale_vertical.setValue(_scalar_scale_horizontal.getValue("%"));
        } else {
            double scaleXPercentage = _scalar_scale_horizontal.getAsPercentage();
            _scalar_scale_vertical.setFromPercentage (scaleXPercentage);
        }
    }
}

void Transformation::onScaleYValueChanged()
{
    if (_scalar_scale_vertical.setProgrammatically) {
        _scalar_scale_vertical.setProgrammatically = false;
        return;
    }

    if (_check_scale_proportional.get_active()) {
        if (!_units_scale.isAbsolute()) { // percentage, just copy over
            _scalar_scale_horizontal.setValue(_scalar_scale_vertical.getValue("%"));
        } else {
            double scaleYPercentage = _scalar_scale_vertical.getAsPercentage();
            _scalar_scale_horizontal.setFromPercentage (scaleYPercentage);
        }
    }
}

void Transformation::onRotateCounterclockwiseClicked()
{
    _scalar_rotate.set_tooltip_text(_("Rotation angle (positive = counterclockwise)"));
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool("/dialogs/transformation/rotateCounterClockwise", !getDesktop()->yaxisdown());
}

void Transformation::onRotateClockwiseClicked()
{
    _scalar_rotate.set_tooltip_text(_("Rotation angle (positive = clockwise)"));
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool("/dialogs/transformation/rotateCounterClockwise", getDesktop()->yaxisdown());
}

void Transformation::onReplaceMatrixToggled()
{
    auto selection = getSelection();
    if (!selection || selection->isEmpty())
        return;

    Geom::Affine current = selection->items().front()->transform; // take from the first item in selection

    Geom::Affine new_displayed;
    if (_check_replace_matrix.get_active()) {
        new_displayed = current;
    } else {
        auto displayed = getCurrentMatrix();
        new_displayed = current.inverse() * displayed;
    }

    _scalar_transform_a.setValue(new_displayed[0]);
    _scalar_transform_b.setValue(new_displayed[1]);
    _scalar_transform_c.setValue(new_displayed[2]);
    _scalar_transform_d.setValue(new_displayed[3]);
    _scalar_transform_e.setValue(new_displayed[4], "px");
    _scalar_transform_f.setValue(new_displayed[5], "px");
}

void Transformation::onScaleProportionalToggled()
{
    onScaleXValueChanged();
    if (_scalar_scale_vertical.setProgrammatically) {
        _scalar_scale_vertical.setProgrammatically = false;
    }
}

void Transformation::onClear()
{
    int const page = _notebook.get_current_page();

    switch (page) {
        case PAGE_MOVE: {
            auto selection = getSelection();
            if (!selection || selection->isEmpty() || _check_move_relative.get_active()) {
                _scalar_move_horizontal.setValue(0);
                _scalar_move_vertical.setValue(0);
            } else {
                Geom::OptRect bbox = selection->preferredBounds();
                if (bbox) {
                    _scalar_move_horizontal.setValue(bbox->min()[Geom::X], "px");
                    _scalar_move_vertical.setValue(bbox->min()[Geom::Y], "px");
                }
            }
            break;
        }
    case PAGE_ROTATE: {
        _scalar_rotate.setValue(0);
        break;
    }
    case PAGE_SCALE: {
        _scalar_scale_horizontal.setValue(100, "%");
        _scalar_scale_vertical.setValue(100, "%");
        break;
    }
    case PAGE_SKEW: {
        _scalar_skew_horizontal.setValue(0);
        _scalar_skew_vertical.setValue(0);
        break;
    }
    case PAGE_TRANSFORM: {
        _scalar_transform_a.setValue(1);
        _scalar_transform_b.setValue(0);
        _scalar_transform_c.setValue(0);
        _scalar_transform_d.setValue(1);
        _scalar_transform_e.setValue(0, "px");
        _scalar_transform_f.setValue(0, "px");
        break;
    }
    }
}

void Transformation::onApplySeparatelyToggled()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool("/dialogs/transformation/applyseparately", _check_apply_separately.get_active());
}

void Transformation::desktopReplaced()
{
    // Setting default unit to document unit
    if (auto desktop = getDesktop()) {
        SPNamedView *nv = desktop->getNamedView();
        if (nv->display_units) {
            _units_move.setUnit(nv->display_units->abbr);
            _units_transform.setUnit(nv->display_units->abbr);
        }

        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        if (prefs->getBool("/dialogs/transformation/rotateCounterClockwise", true) != desktop->yaxisdown()) {
            _counterclockwise_rotate.set_active();
            onRotateCounterclockwiseClicked();
        } else {
            _clockwise_rotate.set_active();
            onRotateClockwiseClicked();
        }

        updateSelection(PAGE_MOVE, getSelection());
    }
}

} // namespace Inkscape::UI::Dialog

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

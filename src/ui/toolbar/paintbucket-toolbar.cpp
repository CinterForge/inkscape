// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file Paint bucket toolbar
 */
/* Authors:
 *   MenTaLguY <mental@rydia.net>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Frank Felfe <innerspace@iname.com>
 *   John Cliff <simarilius@yahoo.com>
 *   David Turner <novalis@gnu.org>
 *   Josh Andler <scislac@scislac.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *   Tavmjong Bah <tavmjong@free.fr>
 *   Abhishek Sharma
 *   Kris De Gussem <Kris.DeGussem@gmail.com>
 *   Vaibhav Malik <vaibhavmalik2018@gmail.com>
 *
 * Copyright (C) 2004 David Turner
 * Copyright (C) 2003 MenTaLguY
 * Copyright (C) 1999-2011 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "paintbucket-toolbar.h"

#include <glibmm/i18n.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/liststore.h>

#include "ui/builder-utils.h"
#include "ui/tools/flood-tool.h"
#include "ui/widget/spinbutton.h"
#include "ui/widget/unit-tracker.h"

namespace Inkscape::UI::Toolbar {

PaintbucketToolbar::PaintbucketToolbar()
    : PaintbucketToolbar{create_builder("toolbar-paintbucket.ui")}
{}

PaintbucketToolbar::PaintbucketToolbar(Glib::RefPtr<Gtk::Builder> const &builder)
    : Toolbar{get_widget<Gtk::Box>(builder, "paintbucket-toolbar")}
    , _tracker{std::make_unique<UI::Widget::UnitTracker>(Inkscape::Util::UNIT_TYPE_LINEAR)}
    , _threshold_item(get_derived_widget<UI::Widget::SpinButton>(builder, "_threshold_item"))
    , _offset_item(get_derived_widget<UI::Widget::SpinButton>(builder, "_offset_item"))
    , _channels_item(get_derived_widget<UI::Widget::DropDownList>(builder, "channel-list"))
    , _autogap_item(get_derived_widget<UI::Widget::DropDownList>(builder, "autogap-list"))
{
    auto prefs = Preferences::get();

    // Setup the spin buttons.
    setup_derived_spin_button(_threshold_item, "threshold", 5, &PaintbucketToolbar::threshold_changed);
    setup_derived_spin_button(_offset_item, "offset", 0, &PaintbucketToolbar::offset_changed);

    // Values auto-calculated.
    _threshold_item.set_custom_numeric_menu_data({});
    _offset_item.set_custom_numeric_menu_data({});

    // Channel
    {
        for (auto item : Inkscape::UI::Tools::FloodTool::channel_list) {
            _channels_item.append(_(item));
        }

        int channels = prefs->getInt("/tools/paintbucket/channels", 0);
        _channels_item.set_selected(channels);

        _channels_item.signal_changed().connect([this] { channels_changed(_channels_item.get_selected()); });

        // Create the units menu.
        Glib::ustring stored_unit = prefs->getString("/tools/paintbucket/offsetunits");
        if (!stored_unit.empty()) {
            auto const u = Util::UnitTable::get().getUnit(stored_unit);
            _tracker->setActiveUnit(u);
        }
    }

    // Auto Gap
    {
        for (auto item : Inkscape::UI::Tools::FloodTool::gap_list) {
            _autogap_item.append(g_dpgettext2(nullptr, "Flood autogap", item));
        }

        int autogap = prefs->getInt("/tools/paintbucket/autogap", 0);
        _autogap_item.set_selected(autogap);

        _autogap_item.signal_changed().connect([this] { autogap_changed(_autogap_item.get_selected()); });

        auto units_menu = _tracker->create_unit_dropdown();
        get_widget<Gtk::Box>(builder, "unit_menu_box").append(*units_menu);
    }

    // Signals.
    get_widget<Gtk::Button>(builder, "reset_btn")
        .signal_clicked()
        .connect(sigc::mem_fun(*this, &PaintbucketToolbar::defaults));

    _initMenuBtns();
}

PaintbucketToolbar::~PaintbucketToolbar() = default;

void PaintbucketToolbar::setActiveUnit(Util::Unit const *unit)
{
    _tracker->setActiveUnit(unit);
}

void PaintbucketToolbar::setup_derived_spin_button(UI::Widget::SpinButton &btn, Glib::ustring const &name,
                                                   double default_value, ValueChangedMemFun const value_changed_mem_fun)
{
    auto const path = "/tools/painbucket/" + name;
    auto const val = Preferences::get()->getDouble(path, default_value);

    auto adj = btn.get_adjustment();
    adj->set_value(val);
    adj->signal_value_changed().connect(sigc::mem_fun(*this, value_changed_mem_fun));

    if (name == "offset") {
        _tracker->addAdjustment(adj->gobj());
        btn.addUnitTracker(_tracker.get());
    }

    btn.setDefocusTarget(this);
}

void PaintbucketToolbar::channels_changed(int channels)
{
    Inkscape::UI::Tools::FloodTool::set_channels(channels);
}

void PaintbucketToolbar::threshold_changed()
{
    Preferences::get()->setInt("/tools/paintbucket/threshold", _threshold_item.get_adjustment()->get_value());
}

void PaintbucketToolbar::offset_changed()
{
    auto const unit = _tracker->getActiveUnit();
    auto const prefs = Preferences::get();

    // Don't adjust the offset value because we're saving the
    // unit and it'll be correctly handled on load.
    prefs->setDouble("/tools/paintbucket/offset", (gdouble)_offset_item.get_adjustment()->get_value());

    prefs->setString("/tools/paintbucket/offsetunits", unit->abbr);
}

void PaintbucketToolbar::autogap_changed(int autogap)
{
    Preferences::get()->setInt("/tools/paintbucket/autogap", autogap);
}

void PaintbucketToolbar::defaults()
{
    // FIXME: make defaults settable via Inkscape Options
    _threshold_item.get_adjustment()->set_value(15);
    _offset_item.get_adjustment()->set_value(0.0);

    _channels_item.set_selected(Inkscape::UI::Tools::FLOOD_CHANNELS_RGB);
    _autogap_item.set_selected(0);
}

} // namespace Inkscape::UI::Toolbar

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

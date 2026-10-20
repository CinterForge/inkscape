// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 5/20/25.
//

#include "scale-bar.h"

#include <gtkmm/adjustment.h>
#include <gtkmm/gestureclick.h>
#include <gtkmm/eventcontrollermotion.h>
#include <gtkmm/eventcontrollerscroll.h>
#include <gtkmm/snapshot.h>
#include <gdk/gdkevents.h>

#include "ui/controller.h"
#include "ui/util.h"

namespace Inkscape::UI::Widget {

ScaleBar::ScaleBar() : Glib::ObjectBase{"ScaleBar"} {

    auto click = Gtk::GestureClick::create();
    click->set_button(GDK_BUTTON_PRIMARY);
    click->set_propagation_phase(Gtk::PropagationPhase::TARGET);
    click->signal_pressed().connect(Controller::use_state(sigc::mem_fun(*this, &ScaleBar::on_click_pressed), *click));
    add_controller(click);

    auto motion = Gtk::EventControllerMotion::create();
    motion->set_propagation_phase(Gtk::PropagationPhase::TARGET);
    motion->signal_motion().connect([this, &motion = *motion](auto &&...args) { on_motion(motion, args...); });
    add_controller(motion);

    auto scroll = Gtk::EventControllerScroll::create();
    scroll->set_propagation_phase(Gtk::PropagationPhase::TARGET);
    scroll->signal_scroll_begin().connect(sigc::mem_fun(*this, &ScaleBar::on_scroll_begin));
    scroll->signal_scroll().connect([this, &scroll = *scroll](auto&& ...args){ return on_scroll(scroll, args...); }, false);
    scroll->signal_scroll_end().connect(sigc::mem_fun(*this, &ScaleBar::on_scroll_end));
    scroll->set_flags(Gtk::EventControllerScroll::Flags::BOTH_AXES); // Mouse wheel is on y.
    add_controller(scroll);
}

Gtk::EventSequenceState ScaleBar::on_click_pressed(const Gtk::GestureClick& click, int n_press, double x, double y) {
    if (_adjustment && n_press == 1) {
        set_adjustment_value(x);
    }
    return Gtk::EventSequenceState::CLAIMED;
}

void ScaleBar::on_motion(const Gtk::EventControllerMotion& motion, double x, double y) {
    auto state = motion.get_current_event_state();
    if (static_cast<bool>(state & Gdk::ModifierType::BUTTON1_MASK)) {
        if (_adjustment) {
            set_adjustment_value(x);
        }
    }
}

void ScaleBar::set_adjustment(Glib::RefPtr<Gtk::Adjustment> adj) {
    _adjustment = adj;

    if (_adjustment) {
        _connection = _adjustment->property_value().signal_changed().connect([this]{ queue_draw(); });
    }
    else {
        _connection.disconnect();
    }

    queue_draw();
}

void ScaleBar::set_max_block_count(int n) {
    _block_count = std::clamp(n, 0, 1000);
    queue_draw();
}

void ScaleBar::set_block_height(int height) {
    _block_height = height;
    queue_draw();
}

void ScaleBar::snapshot_vfunc(const Glib::RefPtr<Gtk::Snapshot>& snapshot) {
    draw_scale(snapshot);
}

void ScaleBar::css_changed(GtkCssStyleChange*) {
    _selected = get_color_with_class(*this, "theme_selected_bg_color");
    _unselected = get_color_with_class(*this, "theme_fg_color");
    _unselected.set_alpha(0.16f);
    queue_draw();
}

void ScaleBar::on_scroll_begin() {
}

bool ScaleBar::on_scroll(Gtk::EventControllerScroll& scroll, double dx, double dy) {
    if (!_adjustment) return false;

    // growth direction: up or right
    auto delta = std::abs(dx) > std::abs(dy) ? -dx : dy;
    //todo: adj speed based on modifiers
    auto state = scroll.get_current_event_state();
    auto range = _adjustment->get_upper() - _adjustment->get_lower();
    if (range <= 0) return false;
    delta *= range / 100.0;
    _adjustment->set_value(_adjustment->get_value() + _adjustment->get_lower() + delta);
    return true;
}

void ScaleBar::on_scroll_end() {
}

constexpr int MIN_BLOCK_SIZE = 3;
constexpr int BLOCK_GAP = 1;

void ScaleBar::draw_scale(const Glib::RefPtr<Gtk::Snapshot>& snapshot) {
    const auto dim = Geom::IntPoint{get_width(), get_height()};
    if (!_adjustment || dim.x() < MIN_BLOCK_SIZE || dim.y() < MIN_BLOCK_SIZE) return;

    auto range = _adjustment->get_upper() - _adjustment->get_lower();
    if (range <= 0) return;

    if (!_selected.gobj()) {
        css_changed(nullptr);
    }

    auto padding = dim.y() > _block_height ? (dim.y() - _block_height) / 2 : 0;
    auto position = (_adjustment->get_value() - _adjustment->get_lower()) / range;

    const int y = padding;
    const auto block_height = dim.y() - 2 * padding;

    auto n = _block_count;
    if (n > 1) {
        auto block_width = dim.x() / n - BLOCK_GAP;
        while (block_width < MIN_BLOCK_SIZE) {
            n /= 2;
            if (!n) return;
            block_width = dim.x() / n;
        }

        for (int i = 0; i < n; ++i) {
            int x0 = i * dim.x() / n;
            int x1 = (i + 1) * dim.x() / n;
            auto rect = Gdk::Graphene::Rect(x0, y, x1 - x0 - 1, block_height);
            auto pos = (x0 + x1) / 2.0 / dim.x();
            // draw blocks and block placeholders
            auto& color = position >= pos ? _selected : _unselected;
            snapshot->append_color(color, rect);
        }
    }
    else {
        int x0 = 0;
        int len = dim.x() * position;
        // draw one block only
        if (position > 0) {
            // selected portion
            auto rect = Gdk::Graphene::Rect(x0, y, len, block_height);
            snapshot->append_color(_selected, rect);
        }
        if (position < 1) {
            // gray bar
            auto rect = Gdk::Graphene::Rect(x0 + len, y, dim.x() - len, block_height);
            snapshot->append_color(_unselected, rect);
        }
    }
}

void ScaleBar::set_adjustment_value(double x) {
    x = std::clamp(x, 0.0, static_cast<double>(get_width()));
    auto range = _adjustment->get_upper() - _adjustment->get_lower();
    auto w = get_width();
    if (w <= 0) return;

    double value = x / w * range;
    if (_block_count > 0) {
        double step = _block_count > 1 ? range / _block_count : 1.0;
        double mod = fmod(value, step);
        value -= mod;
        if (mod > step / 4) {
            value += step;
        }
    }

    _adjustment->set_value(value + _adjustment->get_lower());
}

} // namespace

// SPDX-License-Identifier: GPL-2.0-or-later

#include "simple-grid.h"
#include <algorithm>
#include <glibmm/main.h>
#include <glibmm/priorities.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/enums.h>
#include <gtkmm/eventcontrollerkey.h>
#include <gtkmm/gestureclick.h>
#include <gtkmm/scrollbar.h>
#include <gtkmm/snapshot.h>
#include <gtkmm/tooltip.h>

#include "util/drawing-utils.h"

namespace Inkscape::UI::Widget {

SimpleGrid::SimpleGrid() {
    construct();
}

SimpleGrid::SimpleGrid(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder):
    Glib::ObjectBase("SimpleGrid"),
    Bin(cobject, builder) {

    construct();
}

sigc::connection SimpleGrid::connect_tooltip(sigc::slot<Glib::ustring(int)> callback) {
    property_has_tooltip().set_value(true);
    _get_tooltip = callback;

    return signal_query_tooltip().connect([this](int x, int y, bool kbd, const Glib::RefPtr<Gtk::Tooltip>& tooltip) {
        auto cell = get_cell_index(x, y);
        if (cell < 0 || !_get_tooltip || !tooltip) return false;

        tooltip->set_text(_get_tooltip(cell));
        return true;
    }, true);
}

void SimpleGrid::construct() {
    set_name("SimpleGrid");

    _wnd.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::ALWAYS);
    _wnd.set_overlay_scrolling(false);
    _wnd.set_propagate_natural_width();
    _wnd.set_child(_area);
    _wnd.set_expand();
    _wnd.set_has_frame(false);
    _wnd.get_vadjustment()->signal_value_changed().connect([this] {
        _area.queue_draw();
    });

    _area.set_snapshot_func([this](auto& snapshot, int w, int h){ draw_content(snapshot, w, h); });
    _area.add_css_class("active-background");
    _area.set_hexpand();

    connectAfterResize([this](auto width, auto height, auto baseline) {
        resize();
    });
    set_child(_wnd);

    auto click = Gtk::GestureClick::create();
    click->signal_pressed().connect([this](int n, double x, double y) {
        _clicked_cell = get_cell_index(x, y);
        grab_focus();
        if (n > 1 && _clicked_cell >= 0) {
            open_cell(_clicked_cell);
        }
    });
    click->signal_released().connect([this](int n, double x, double y) {
        auto index = get_cell_index(x, y);
        if (index >= 0 && index == _clicked_cell && _selected_cell != index) {
            select_cell(index);
        }
        _clicked_cell = -1;
    });
    add_controller(click);

    auto kbd = Gtk::EventControllerKey::create();
    kbd->signal_key_pressed().connect([this](guint keyval, guint keycode, Gdk::ModifierType modifier) {
        switch (keyval) {
        case GDK_KEY_Left:
            move_sel(0, -1);
            break;
        case GDK_KEY_Right:
            move_sel(0, 1);
            break;
        case GDK_KEY_Down:
            move_sel(1, 0);
            break;
        case GDK_KEY_Up:
            move_sel(-1, 0);
            break;
        case GDK_KEY_Page_Up:
            move_sel(-std::max(1, _viewport_whole_rows - 1), 0);
            break;
        case GDK_KEY_Page_Down:
            move_sel(std::max(1, _viewport_whole_rows - 1), 0);
            break;
        case GDK_KEY_Home:
            move_sel_to(0);
            break;
        case GDK_KEY_End:
            move_sel_to(_cell_count - 1);
            break;
        case GDK_KEY_KP_Enter:
        case GDK_KEY_Return:
            if (_selected_cell >= 0) {
                open_cell(_selected_cell);
            }
            break;
        default:
            return false; // key not handled
        }
        return true;
    }, false);
    add_controller(kbd);
}

void SimpleGrid::move_sel(int delta_rows, int delta_cols) {
    if (_cell_count == 0 || !_layout) return;

    int cell = _selected_cell >= 0 ? _selected_cell : 0;
    int columns = _cols_rows.x();

    if (delta_rows && delta_cols) {
        cell += delta_rows * _cols_rows.x() + delta_cols;
    }
    else if (delta_rows == 0) {
        cell += delta_cols;
    }
    else {
        int delta = delta_rows * columns;
        if (delta > 0) {
            // going down
            if (cell + delta < _cell_count) {
                cell += delta;
            }
            else {
                // stop in the last row accessible from the current column
                int max = static_cast<int>(_cell_count);
                int last_row_index = _cols_rows.y() - 1; // (max - 1) / columns;
                int last_row_cols = max % columns;
                int current_col = cell % columns;
                if (current_col < last_row_cols || last_row_cols == 0) {
                    cell = last_row_index * columns + current_col;
                }
                else if (last_row_index > 0) {
                    cell = (last_row_index - 1) * columns + current_col;
                }
            }
        }
        else {
            // going up
            if (cell + delta >= 0) {
                cell += delta;
            }
            else {
                // stop in the first row
                cell = cell % columns;
            }
        }
    }

    move_sel_to(cell);
}

void SimpleGrid::move_sel_to(int cell) {
    if (_cell_count == 0) return;

    cell = std::clamp(cell, 0, static_cast<int>(_cell_count) - 1);
    if (cell != _selected_cell) {
        select_cell(cell);
    }
}

void SimpleGrid::select_cell(int index) {
    _selected_cell = index;
    scroll_to(index);
    _cell_selected.emit(index);
    _area.queue_draw();
}

void SimpleGrid::open_cell(int index) {
    _cell_open.emit(index);
}

void SimpleGrid::scroll_to(int cell) {
    if (!_layout || !_cell_count) return;

    auto vertScroll = get_vscroll_position();
    auto columns = _cols_rows.x();
    auto rows = _cols_rows.y();
    auto row = static_cast<int>(std::floor(cell / columns));
    auto firstRow = vertScroll / _cell_pitch.y();
    auto lastRow = firstRow + _viewport_whole_rows;
    auto scroll = vertScroll;
    if (row <= firstRow) {
        // scroll up
        scroll = row * _cell_pitch.y();
    }
    else if (row >= lastRow) {
        // scroll down
        scroll = (std::min(row + 1, rows) - _viewport_whole_rows) * _cell_pitch.y();
        auto max = std::max(0, _area_size.y() - _wnd.get_height());
        if (scroll > max) scroll = max;
    }

    if (scroll != vertScroll) {
        auto vsb = _wnd.get_vscrollbar();
        if (vsb) vsb->get_adjustment()->set_value(scroll);
    }
}

void SimpleGrid::set_cell_size(int width, int height) {
    auto size = Size {width, height};
    if (_cell_size != size) {
        _cell_size = size;
        invalidate();
    }
}

void SimpleGrid::set_gap(int gap_x, int gap_y) {
    // only positive or zero gap is accepted
    auto gap = Size {std::max(0, gap_x), std::max(0, gap_y)};
    if (_gap != gap) {
        _gap = gap;
        invalidate();
    }
}

void SimpleGrid::set_cell_stretch(bool stretch) {
    _stretch_cells = stretch;
    _area.queue_draw();
}

void SimpleGrid::set_cell_count(std::size_t count) {
    _selected_cell = -1;
    if (count == 0) {
        // reset offset
        if (auto vsb = _wnd.get_vscrollbar()) {
            vsb->get_adjustment()->set_value(0);
        }
    }

    if (_cell_count != count) {
        _cell_count = count;
        invalidate();
    }
}

void SimpleGrid::set_selectable(bool is_selectable) {
    // todo - readonly
}

void SimpleGrid::set_has_frame(bool frame) {
    _wnd.set_has_frame(frame);
}

void SimpleGrid::invalidate() {
    _layout = false;
    if (!_resize) {
        _resize = Glib::signal_idle().connect([this]{
            resize();
            _resize.disconnect();
            return false;
        }, Glib::PRIORITY_HIGH_IDLE);
    }
}

void SimpleGrid::clear() {
    set_cell_count(0);
    queue_draw();
}

void SimpleGrid::resize() {
    // calculate layout based on how wide the grid is in the scrolled window (that has a vertical scrollbar turned on)
    // and the height of the scrolled window (which is our viewport)
    _layout = calc_layout(_area.get_width(), _wnd.get_height());
    // set grid height to allow scrolling
    _area.set_size_request(-1, _area_size.y());
    _area.queue_draw();
}

int SimpleGrid::get_cell_index(double x, double y) const {
    if (!_layout || _cell_pitch.x() <= 0 || _cols_rows.x() <= 0) return -1;

    double width = _area.get_width();
    double height = _area.get_height();
    if (x >= 0 && y >= 0 && x < width && y < height) {
        auto col = _stretch_cells ? std::floor(x / (width / _cols_rows.x())) : std::floor(x / _cell_pitch.x());
        auto row = std::floor((y + get_vscroll_position()) / _cell_pitch.y());
        auto index = static_cast<int>(col + row * _cols_rows.x());
        if (index < _cell_count) return index;
    }

    return -1;
}

int SimpleGrid::get_vscroll_position() const {
    auto vsb = _wnd.get_vscrollbar();
    auto vert_scroll = vsb ? int(vsb->get_adjustment()->get_value()) : 0;
    return vert_scroll;
}

void SimpleGrid::draw_content(const Cairo::RefPtr<Gtk::Snapshot>& snapshot, int width, int height) {
    if (!_layout) {
        _layout = calc_layout(width, height);
    }
    if (!_layout || !_cols_rows.x() || !_cols_rows.y() || !_cell_count || !_cell_pitch.y() || !_cell_pitch.x()) return;

    auto vert_scroll = get_vscroll_position();

    auto first_row = vert_scroll / _cell_pitch.y();
    auto last_row = first_row;
    auto dy = vert_scroll % _cell_pitch.y();
    auto from_cell = first_row * _cols_rows.x();
    auto to_cell = std::min(static_cast<int>(_cell_count) - 1, from_cell + _viewport_rows * _cols_rows.x() + (dy ? _cols_rows.x() : 0) - 1);
    const auto columns = _cols_rows.x();
    auto calc_cell_pos = [this,width,columns](int column) {
        int pitch = _cell_pitch.x();
        int x = 0;
        if (_stretch_cells) {
            // distribute/stretch cells horizontally across entire width leaving no gaps
            x = column * width / columns;
            auto next = (column + 1) * width / columns;
            pitch = next - x;
        }
        else {
            // cells from left to right with a possible gap at right
            x = column * _cell_pitch.x();
        }
        return std::make_pair(x, pitch);
    };

    for (int index = from_cell; index <= to_cell; ++index) {
        auto column = index % columns;
        auto row = index / columns;
        auto [x, pitch] = calc_cell_pos(column);
        int y = row * _cell_pitch.y();
        auto rect = Geom::IntRect::from_xywh(x, y, pitch - _gap.x(), _cell_size.y());
        _draw_cell.emit(snapshot, index, rect, index == _selected_cell);

        last_row = row;
    }

    if (_gap.x() > 0 && _gap.y() > 0) {
        Gdk::RGBA fg;
        auto style = get_style_context();
        // use border color is it is defined
        if (auto border = Util::lookup_border_color(style); border.has_value()) {
            fg = border.value();
            fg.set_alpha(0.7);
        }
        else {
            // fall back to foreground color
            fg = get_color();
            fg.set_alpha(0.15);
        }

        // stay in the center of the gap
        auto center = Geom::IntPoint(_gap.x() + 1, _gap.y() + 1) / 2;

        auto last_row_cols = static_cast<int>(_cell_count) % columns;
        auto limit = last_row_cols != 0 ? std::min(last_row + 1, _cols_rows.y() - 1) : last_row + 1;
        // horizontal lines
        for (int row = first_row + 1; row <= limit; ++row) {
            auto y = row * _cell_pitch.y() - center.y();
            int x = 0;
            snapshot->append_color(fg, Gdk::Graphene::Rect(x, y, width, 1));
        }

        // vertical lines
        int y = first_row * _cell_pitch.y();
        int bottom = std::min(limit, _cols_rows.y()) * _cell_pitch.y();
        for (int col = 1; col < _cols_rows.x(); ++col) {
            auto [x, pitch] = calc_cell_pos(col);
            snapshot->append_color(fg, Gdk::Graphene::Rect(x - center.x(), y, 1, bottom - y));
        }

        if (last_row_cols != 0) {
            // the bottommost row is only partially filled with cells
            y = bottom;
            bottom += _cell_pitch.y();
            int right = 0;
            for (int col = 1; col <= last_row_cols; ++col) {
                auto [x, pitch] = calc_cell_pos(col);
                snapshot->append_color(fg, Gdk::Graphene::Rect(x - center.x(), y, 1, bottom - y));
                right = x;
            }
            y = bottom;
            int x = 0;
            snapshot->append_color(fg, Gdk::Graphene::Rect(x, y, width, 1));
        }
    }
}

bool SimpleGrid::calc_layout(int width, int height) {
    if (width <= 0 || height <= 0 || _cell_size.x() <= 0 || _cell_size.y() <= 0 || _cell_count == 0) {
        _cell_pitch = {};
        _area_size = {};
        _area.set_size_request(0, 0);
        return false;
    }
  
    auto columns = std::max(1, (width + _gap.x()) / (_cell_size.x() + _gap.x()));
    _cell_pitch.x() = _stretch_cells ? width / columns : _cell_size.x() + _gap.x();
    auto rows = static_cast<int>((_cell_count + columns - 1) / columns); // round up
    _cell_pitch.y() = _cell_size.y() + _gap.y();
    _cols_rows = {columns, rows};
    _area_size = { std::max(_cell_size.x(), width), rows * _cell_pitch.y() - _gap.y() };
    _viewport_rows = std::min(rows, (height + _cell_pitch.y() - _gap.y()) / _cell_pitch.y());
    _viewport_whole_rows = std::clamp((height + _gap.y()) / _cell_pitch.y(), 1, rows);

    return true;
}

} // namespace

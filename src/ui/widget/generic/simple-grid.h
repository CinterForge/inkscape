// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SIMPLE_GRID_HEADER
#define SIMPLE_GRID_HEADER

#include <cstdint>
#include <gtkmm/scrolledwindow.h>

#include "bin.h"
#include "snapshot-widget.h"
#include "2geom/int-point.h"
#include "2geom/int-rect.h"

namespace Inkscape::UI::Widget {

// Simple "virtual" grid that arranges rectangular cells in columns and rows
// and delegates cell ownership and drawing to a client.
// It is lightweight and can handle millions of cells uniform in size.
// It provides no caching.
// It can track one cell (selected one) and tell when it changes.

class SimpleGrid: public Bin {
public:
    SimpleGrid();
    SimpleGrid(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);

    // establish the size of all cells in pixels
    void set_cell_size(int width, int height);
    // set column and row cell gap to allow drawing separating lines
    void set_gap(int gap_x, int gap_y);
    // if true, cells will be stretched to fill up available space
    void set_cell_stretch(bool stretch);
    // total number of cells to present in a grid
    void set_cell_count(std::size_t count);
    // should cells be selectable?
    void set_selectable(bool is_selectable);
    // add or remove a frame around the grid
    void set_has_frame(bool frame = true);
    // repaint the entire grid after cells have changed
    void invalidate();
    // remove cells, clear the grid
    void clear();

    // register callback to draw cells, one at a time, given context, cell index, area and selected status
    sigc::connection set_draw_func(sigc::slot<void (const Glib::RefPtr<Gtk::Snapshot>& ctx, std::uint32_t, const Geom::IntRect&, bool)> callback) {
        return _draw_cell.connect(callback);
    }
    sigc::connection connect_tooltip(sigc::slot<Glib::ustring (int)> callback);
    // connect a callback to invoke when selected cell has changed
    void connect_cell_selected(const sigc::slot<void (int)>& callback) { _cell_selected.connect(callback); }
    // connect a callback when the user tries to "open" a cell by double-clicking or pressing Enter key
    void connect_cell_open(const sigc::slot<void (int)>& callback) { _cell_open.connect(callback); }

private:
    void construct();
    sigc::signal<void (const Glib::RefPtr<Gtk::Snapshot>& snapshot, std::uint32_t, const Geom::IntRect&, bool)> _draw_cell;
    sigc::signal<void (int)> _cell_selected;
    sigc::signal<void (int)> _cell_open;

    void draw_content(const Cairo::RefPtr<Gtk::Snapshot>& snapshot, int width, int height);
    bool calc_layout(int width, int height);
    void resize();
    int get_cell_index(double x, double y) const;
    int get_vscroll_position() const;
    void move_sel(int delta_rows, int delta_cols);
    void move_sel_to(int cell);
    void select_cell(int index);
    void open_cell(int index);
    void scroll_to(int index);

    Gtk::ScrolledWindow _wnd;
    SnapshotWidget _area;
    using Size = Geom::IntPoint;
    Size _cell_size;  // requested cell size
    Size _gap;        // requested gap
    std::size_t _cell_count = 0; // requested number of cells in a grid
    int _selected_cell = -1;
    // layout
    Size _cols_rows;    // size of grid in columns and rows
    Size _cell_pitch;   // calculated cell pitch
    int _viewport_rows = 0;  // number of visible rows
    int _viewport_whole_rows = 0;  // number of whole rows
    bool _layout = false;
    bool _stretch_cells = true;
    Size _area_size;
    sigc::scoped_connection _resize;
    int _clicked_cell = -1;
    sigc::slot<Glib::ustring (std::uint32_t)> _get_tooltip;
};

} // namespace

#endif

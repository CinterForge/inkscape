// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * A combo widget to choose a font size and unit together.
 */

#ifndef INKSCAPE_UI_WIDGET_FONT_SIZE_SELECTOR_H
#define INKSCAPE_UI_WIDGET_FONT_SIZE_SELECTOR_H

#include <gtkmm/box.h>

#include "preferences.h"
#include "unit-tracker.h"
#include "ui/widget/gtk-registry.h"
#include "ui/widget/generic/number-combo-box.h"

namespace Inkscape {
namespace UI {
namespace Widget {
class NumberComboBox;
class UnitMenu;
class UnitTracker;
} // namespace Widget
} // namespace UI
} // namespace Inkscape

namespace Inkscape::UI::Widget {

class FontSizeSelector : public BuildableWidget<FontSizeSelector, Gtk::Box>
{
public:
    FontSizeSelector();
    explicit FontSizeSelector(GtkBox* cobject, const Glib::RefPtr<Gtk::Builder>& builder = {});

    // These sizes are relative to the current configured unit
    double getSize() const;
    void setSize(double size);
    int getUnit() const; // should always mirror the global pref

    // This signal sends both the size and unit
    sigc::signal<void (double, int)> &signal_size_changed() { return _signal_size_changed; }

    void setPopupPosition(Gtk::PositionType pos);

protected:
    NumberComboBox* _size_combo;
    UnitMenu *_unit_combo;
    std::unique_ptr<UI::Widget::UnitTracker> _tracker;
    Pref<int> _max_size;
    Pref<int> _unit;
    std::optional<int> _previous_unit;

    sigc::signal<void (double, int)> _signal_size_changed;

    void _construct();
    void _sizeChanged(double size);
    void _unitChanged();
    void _updateUnit();
};

} // namespace Inkscape::UI::Widget

#endif // INKSCAPE_UI_WIDGET_FONT_SIZE_SETTINGS_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :

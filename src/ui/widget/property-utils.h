// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PROPERTY_UTILS_H
#define PROPERTY_UTILS_H
#include <glibmm/ustring.h>

namespace Gtk {
class Button;
}

namespace Inkscape::UI::Widget {
class InkSpinButton;
}

namespace Inkscape::UI::Utils {
struct AdjustmentDef {
    double min = 0, max = 1;
    double inc = 1, page_inc = 1;
    int digits = 0;
    double scale = 1.0;
};

enum Suffix {None = 0, Degree, Percent};

struct SpinPropertyDef {
    Widget::InkSpinButton* button = nullptr;
    AdjustmentDef adjustment;
    const char* label = nullptr;
    const char* tooltip = nullptr;
    Suffix unit = None;
    Gtk::Button* reset = nullptr;
};

enum PropertyButton { Reset, Edit, Add, Remove };

// Initialize property button adding proper icon and other attributes
void init_property_button(Gtk::Button& button, PropertyButton type, const Glib::ustring& tooltip = Glib::ustring());

// Helper function used to initialize InkSpinButton using provided metadata
void init_spin_button(const SpinPropertyDef& def);

}

#endif //PROPERTY_UTILS_H

// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_DISPLAY_CONTROL_CTRL_HANDLE_MANAGER_H
#define INKSCAPE_DISPLAY_CONTROL_CTRL_HANDLE_MANAGER_H
/*
 * Authors:
 *   PBS <pbs3141@gmail.com>
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <memory>
#include <string>
#include <vector>
#include <sigc++/connection.h>
#include <sigc++/slot.h>

namespace Inkscape::Handles {

class Css;

struct ColorTheme {
    std::string file_name;  // CSS file to load
    std::string title;      // display name
    bool positive;          // normal (true), inverted colors (false)
    unsigned int rgb_accent_color; // dominant color
};

class Manager
{
public:
    static Manager &get();

    std::shared_ptr<Css const> getCss() const;

    sigc::connection connectCssUpdated(sigc::slot<void()> &&slot);

    void select_theme(int index);

    int get_selected_theme() const { return current_theme; }

    const std::vector<ColorTheme>& get_handle_themes() const;

protected:
    ~Manager() = default;
    int current_theme = 0;
};

} // namespace Inkscape::Handles

#endif // INKSCAPE_DISPLAY_CONTROL_CTRL_HANDLE_MANAGER_H

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

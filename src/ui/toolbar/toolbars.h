// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 *
 * A container for toolbars, displaying one toolbar at a time.
 *
 *//*
 * Authors: Tavmjong Bah
 *
 * Copyright (C) 2023 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_TOOLBARS_H
#define SEEN_TOOLBARS_H

#include <string>
#include <unordered_map>
#include <gtkmm/box.h>

class SPDesktop;
namespace Inkscape::UI::Tools { class ToolBase; }
namespace Inkscape::Util { class Unit; }

namespace Inkscape::UI::Toolbar {

class Toolbar;

/**
 * \brief A container for toolbars.
 *
 * Displays one toolbar at a time.
 */
class Toolbars : public Gtk::Box
{
public:
    Toolbars();
    ~Toolbars() override;

    void setTool(Tools::ToolBase *tool);
    void setActiveUnit(Util::Unit const *unit);
    Toolbar *get_current_toolbar();

private:
    std::unordered_map<std::string, std::unique_ptr<Toolbar>> _toolbars;
    Toolbar *_current_toolbar = nullptr;
};

} // namespace Inkscape::UI::Toolbar

#endif // SEEN_TOOLBARS_H

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

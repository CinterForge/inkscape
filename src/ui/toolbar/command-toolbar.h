// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Toolbar for Commands.
 */
/*
 * Authors:
 *   Tavmjong Bah
 *   Vaibhav Malik <vaibhavmalik2018@gmail.com>
 *
 * Copyright (C) 2023 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_TOOLBAR_COMMAND_TOOLBAR_H
#define INKSCAPE_UI_TOOLBAR_COMMAND_TOOLBAR_H

#include "toolbar.h"

namespace Inkscape::UI::Toolbar {

class CommandToolbar : public ToolbarWidget
{
public:
    CommandToolbar();
    ~CommandToolbar() override;
};

} // namespace Inkscape::UI::Toolbar

#endif // INKSCAPE_UI_TOOLBAR_COMMAND_TOOLBAR_H

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

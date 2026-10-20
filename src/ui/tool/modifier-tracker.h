// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Fine-grained modifier tracker for event handling.
 */
/* Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *
 * Copyright (C) 2009 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_TOOL_MODIFIER_TRACKER_H
#define INKSCAPE_UI_TOOL_MODIFIER_TRACKER_H

namespace Inkscape { struct CanvasEvent; }
namespace Inkscape::UI {

class ModifierTracker
{
public:
    ModifierTracker() = default;

    void event(CanvasEvent const &event);

    bool leftShift() const { return _left_shift; }
    bool rightShift() const { return _right_shift; }
    bool leftControl() const { return _left_ctrl; }
    bool rightControl() const { return _right_ctrl; }
    bool leftAlt() const { return _left_alt; }
    bool rightAlt() const { return _right_alt; }

private:
    bool _left_shift = false;
    bool _right_shift = false;
    bool _left_ctrl = false;
    bool _right_ctrl = false;
    bool _left_alt = false;
    bool _right_alt = false;
};

} // namespace Inkscape::UI

#endif // INKSCAPE_UI_TOOL_MODIFIER_TRACKER_H

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

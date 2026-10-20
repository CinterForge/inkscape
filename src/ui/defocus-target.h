// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_DEFOCUS_TARGET_H
#define INKSCAPE_UI_DEFOCUS_TARGET_H

namespace Inkscape::UI {

/**
 * Interface for objects that would like to be informed when another widget loses focus.
 */
class DefocusTarget
{
public:
    virtual void onDefocus() = 0;

protected:
    ~DefocusTarget() = default;
};

} // namespace Inskcape::UI

#endif // INKSCAPE_UI_DEFOCUS_TARGET_H

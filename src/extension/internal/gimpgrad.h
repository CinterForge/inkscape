// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Ted Gould <ted@gould.cx>
 *
 * Copyright (C) 2004-2005 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_EXTENSION_INTERNAL_GIMPGRAD_H
#define INKSCAPE_EXTENSION_INTERNAL_GIMPGRAD_H

#include "extension/implementation/implementation.h"

namespace Inkscape::Extension::Internal {

/**
 * Implementation class of the GIMP gradient plugin.
 * This mostly just creates a namespace for the GIMP gradient plugin today.
 */
class GimpGrad : public Inkscape::Extension::Implementation::Implementation
{
public:
    bool load(Inkscape::Extension::Extension *module) override;
    void unload(Inkscape::Extension::Extension *module) override;
    std::unique_ptr<SPDocument> open(Inkscape::Extension::Input *module, char const *filename, bool is_importing) override;

    static void init();
};

} // namespace Inkscape::Extension::Internal

#endif // INKSCAPE_EXTENSION_INTERNAL_GIMPGRAD_H

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

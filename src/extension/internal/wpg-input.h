// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This code abstracts the libwpg interfaces into the Inkscape
 * input extension interface.
 *
 * Authors:
 *   Ted Gould <ted@gould.cx>
 *
 * Copyright (C) 2006 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_EXTENSION_INTERNAL_WPGOUTPUT_H
#define SEEN_EXTENSION_INTERNAL_WPGOUTPUT_H

#include "extension/implementation/implementation.h"

namespace Inkscape::Extension::Internal {

class WpgInput : public Inkscape::Extension::Implementation::Implementation
{
public:
    std::unique_ptr<SPDocument> open(Inkscape::Extension::Input *mod, char const *uri, bool is_importing) override;
    static void init();
};

} // namespace Inkscape::Extension::Internal

#endif // SEEN_EXTENSION_INTERNAL_WPGOUTPUT_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

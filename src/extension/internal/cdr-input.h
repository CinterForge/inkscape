// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * This code abstracts the libwpg interfaces into the Inkscape
 * input extension interface.
 *
 *  This file came from libwpg as a source, their utility wpg2svg
 *  specifically.  It has been modified to work as an Inkscape extension.
 *  The Inkscape extension code is covered by this copyright, but the
 *  rest is covered by the one below.
 */
/* Authors:
 *   Fridrich Strba (fridrich.strba@bluewin.ch)
 *
 * Copyright (C) 2012 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_EXTENSION_INTERNAL_CDRINPUT_H
#define INKSCAPE_EXTENSION_INTERNAL_CDRINPUT_H

#include "extension/implementation/implementation.h"

namespace Inkscape::Extension::Internal {

class CdrInput : public Inkscape::Extension::Implementation::Implementation
{
public:
    std::unique_ptr<SPDocument> open(Inkscape::Extension::Input *mod, char const *uri, bool is_importing) override;
    static void init();
};

} // namespace Inkscape::Extension::Internal

#endif // INKSCAPE_EXTENSION_INTERNAL_CDRINPUT_H

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

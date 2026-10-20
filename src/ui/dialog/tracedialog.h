// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Bitmap tracing settings dialog - second implementation.
 */
/* Authors:
 *   Bob Jamison
 *   Marc Jeanmougin <marc.jeanmougin@telecom-paristech.fr>
 *   PBS <pbs3141@gmail.com>
 *   Others - see git history.
 *
 * Copyright (C) 2019-2022 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_DIALOG_TRACE_H
#define INKSCAPE_UI_DIALOG_TRACE_H

#include "ui/dialog/dialog-base.h"

namespace Inkscape::UI::Dialog  {

class TraceDialog : public DialogBase
{
public:
    static std::unique_ptr<TraceDialog> create();

protected:
    TraceDialog() : DialogBase("/dialogs/trace", "Trace") {}
};

} // namespace Inkscape::UI::Dialog

#endif // INKSCAPE_UI_DIALOG_TRACE_H

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

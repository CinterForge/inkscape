// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * This is the C++ glue between Inkscape and Autotrace
 *//*
 *
 * Authors:
 *   Marc Jeanmougin
 *
 * Copyright (C) 2018 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 *
 * Autotrace is available at http://github.com/autotrace/autotrace.
 *
 */
#ifndef INKSCAPE_TRACE_AUTOTRACE_H
#define INKSCAPE_TRACE_AUTOTRACE_H

#include "trace/trace.h"
using at_fitting_opts_type = struct _at_fitting_opts_type;

namespace Inkscape::Trace::Autotrace {

class AutotraceTracingEngine final
    : public TracingEngine
{
public:
    AutotraceTracingEngine();
    ~AutotraceTracingEngine() override;

    TraceResult trace(Glib::RefPtr<Gdk::Pixbuf> const &pixbuf, Async::Progress<double> &progress) override;
    Glib::RefPtr<Gdk::Pixbuf> preview(Glib::RefPtr<Gdk::Pixbuf> const &pixbuf) override;

    void setColorCount(unsigned);
    void setCenterLine(bool);
    void setPreserveWidth(bool);
    void setFilterIterations(unsigned);
    void setErrorThreshold(float);

private:
    at_fitting_opts_type *opts;
};

} // namespace Inkscape::Trace::Autotrace

#endif // INKSCAPE_TRACE_AUTOTRACE_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

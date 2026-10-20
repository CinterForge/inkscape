// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Bob Jamison <rjamison@titan.com>
 *
 * Copyright (C) 2004-2006 Bob Jamison
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_TRACE_H
#define INKSCAPE_TRACE_H

#include <memory>
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include <2geom/pathvector.h>
#include <gdkmm/pixbuf.h>

#include "async/channel.h"
#include "object/weakptr.h"
#include "object/sp-image.h"

namespace Inkscape {

namespace Async { template <typename... T> class Progress; }

namespace Trace {

struct TraceResultItem
{
    TraceResultItem(std::string style_, Geom::PathVector path_)
        : style(std::move(style_))
        , path(std::move(path_)) {}

    std::string style;
    Geom::PathVector path;
};

using TraceResult = std::vector<TraceResultItem>;

/**
 * A generic interface for plugging different autotracers into Inkscape.
 */
class TracingEngine
{
public:
    TracingEngine() = default;
    virtual ~TracingEngine() = default;

    /**
     * This is the working method of this interface, and all implementing classes. Take a
     * GdkPixbuf, trace it, and return a style attribute and the path data that is
     * compatible with the d="" attribute of an SVG <path> element.
     *
     * This function will be called off-main-thread, so is required to be thread-safe.
     * The lack of const however indicates that it is not required to be re-entrant.
     */
    virtual TraceResult trace(Glib::RefPtr<Gdk::Pixbuf> const &pixbuf, Async::Progress<double> &progress) = 0;

    /**
     * Generate a quick preview without any actual tracing. Like trace(), this must be thread-safe.
     */
    virtual Glib::RefPtr<Gdk::Pixbuf> preview(Glib::RefPtr<Gdk::Pixbuf> const &pixbuf) = 0;

    /**
     * Return true if the user should be checked with before tracing because the image is too big.
     */
    virtual bool check_image_size(Geom::IntPoint const &size) const { return false; }
};

namespace detail { struct TraceFutureCreate; }

class TraceFuture
{
public:
    void cancel() { channel.close(); image_watcher.reset(); }
    explicit operator bool() const { return (bool)channel; }

private:
    Async::Channel::Dest channel;
    std::shared_ptr<SPWeakPtr<SPImage>> image_watcher;
    friend class detail::TraceFutureCreate;
};

/**
 * Launch an asynchronous trace operation taking as input \a engine and \a sioxEnabled.
 * If this returns null, the task failed to launch and no further action will be taken.
 * Otherwise, a background task is launched which will call \a onprogress some number of times
 * followed by \a onfinished exactly once. Both callbacks are invoked from the GTK main loop.
 */
TraceFuture trace(std::unique_ptr<TracingEngine> engine,
                  bool sioxEnabled,
                  std::function<void(double)> onprogress,
                  std::function<void()> onfinished);

/**
 * Similar to \a trace(), but computes the preview and passes it to \a onfinished when done.
 */
TraceFuture preview(std::unique_ptr<TracingEngine> engine,
                    bool sioxEnabled,
                    std::function<void(Glib::RefPtr<Gdk::Pixbuf>)> onfinished);

} // namespace Trace
} // namespace Inkscape

#endif // INKSCAPE_TRACE_H

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

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * The context in which a single CanvasItem tree exists. Holds the root node and common state.
 */

#ifndef SEEN_CANVAS_ITEM_CONTEXT_H
#define SEEN_CANVAS_ITEM_CONTEXT_H

#include <2geom/affine.h>
#include <sigc++/scoped_connection.h>
#include "util/funclog.h"

namespace Inkscape {

namespace UI::Widget { class Canvas; }
namespace Handles { class Css; }

class CanvasItemGroup;

class CanvasItemContext final
{
public:
    CanvasItemContext(UI::Widget::Canvas *canvas);
    CanvasItemContext(CanvasItemContext const &) = delete;
    CanvasItemContext &operator=(CanvasItemContext const &) = delete;
    ~CanvasItemContext();

    // Structure
    UI::Widget::Canvas *canvas() const { return _canvas; }
    CanvasItemGroup *root() const { return _root; }

    // Geometry
    Geom::Affine const &affine() const { return _affine; }
    void setAffine(Geom::Affine const &affine) { _affine = affine; }

    bool yaxisdown() const { return _yaxisdown; }
    void setYaxisdown(bool yaxisdown) { _yaxisdown = yaxisdown; }

    // Control handle styling
    std::shared_ptr<Handles::Css const> const &handlesCss() const { return _handles_css; }

    // Snapshotting
    void snapshot();
    void unsnapshot();
    bool snapshotted() const { return _snapshotted; }

    template<typename F>
    void defer(F &&f) { _snapshotted ? _funclog.emplace(std::forward<F>(f)) : f(); }

private:
    // Structure
    UI::Widget::Canvas *_canvas;
    CanvasItemGroup *_root;

    // Geometry
    Geom::Affine _affine;
    bool _yaxisdown = true;

    // Control handle styling
    std::shared_ptr<Handles::Css const> _handles_css;
    sigc::scoped_connection _css_updated_conn;
    sigc::connection _device_scale_conn;

    // Snapshotting
    char _cacheline_separator[127];

    bool _snapshotted = false;
    Util::FuncLog _funclog;
};

} // namespace Inkscape

#endif // SEEN_CANVAS_ITEM_CONTEXT_H

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

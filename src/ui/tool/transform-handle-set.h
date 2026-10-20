// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Affine transform handles component
 */
/* Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2009 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_TOOL_TRANSFORM_HANDLE_SET_H
#define INKSCAPE_UI_TOOL_TRANSFORM_HANDLE_SET_H

#include "ui/tool/manipulator.h"

class SPDesktop;

namespace Inkscape {

class CanvasItemGroup;
class CanvasItemRect;

namespace UI {

class RotateHandle;
class SkewHandle;
class ScaleCornerHandle;
class ScaleSideHandle;
class RotationCenter;

class TransformHandleSet : public Manipulator
{
public:
    enum Mode
    {
        MODE_SCALE,
        MODE_ROTATE_SKEW
    };

    TransformHandleSet(SPDesktop *d, Inkscape::CanvasItemGroup *th_group);
    ~TransformHandleSet() override;
    bool event(Inkscape::UI::Tools::ToolBase *tool, CanvasEvent const &event) override;

    bool visible() const { return _visible; }
    Mode mode() const { return _mode; }
    Geom::Rect bounds() const;
    void setVisible(bool v);

    /** Sets the mode of transform handles (scale or rotate). */
    void setMode(Mode m);

    void setBounds(Geom::Rect const &, bool preserve_center = false);

    bool transforming() { return _in_transform; }

    ControlPoint const &rotationCenter() const;
    ControlPoint &rotationCenter();

    sigc::signal<void (Geom::Affine const &)> signal_transform;
    sigc::signal<void (CommitEvent)> signal_commit;

private:
    void _emitTransform(Geom::Affine const &);
    void _setActiveHandle(ControlPoint *h);
    void _clearActiveHandle();

    /** Update the visibility of transformation handles according to settings and the dimensions
     * of the bounding box. It hides the handles that would have no effect or lead to
     * discontinuities. Additionally, side handles for which there is no space are not shown.
     */
    void _updateVisibility(bool v);

    // TODO unions must GO AWAY:
    union {
        ControlPoint *_handles[17];
        struct {
            ScaleCornerHandle *_scale_corners[4];
            ScaleSideHandle *_scale_sides[4];
            RotateHandle *_rot_corners[4];
            SkewHandle *_skew_sides[4];
            RotationCenter *_center;
        };
    };

    ControlPoint *_active;
    Inkscape::CanvasItemGroup *_transform_handle_group;
    Inkscape::CanvasItemRect *_trans_outline;
    Mode _mode;
    bool _in_transform;
    bool _visible;
    friend class TransformHandle;
    friend class RotationCenter;
};

/** Base class for node transform handles to simplify implementation. */
class TransformHandle : public ControlPoint
{
public:
    TransformHandle(TransformHandleSet &th, SPAnchorType anchor, Inkscape::CanvasItemCtrlType type);
    void getNextClosestPoint(bool reverse);

protected:
    virtual void startTransform() {}
    virtual void endTransform() {}
    virtual Geom::Affine computeTransform(Geom::Point const &pos, MotionEvent const &event) = 0;
    virtual CommitEvent getCommitEvent() const = 0;

    Geom::Affine _last_transform;
    Geom::Point _origin;
    TransformHandleSet &_th;
    std::vector<Inkscape::SnapCandidatePoint> _snap_points;
    std::vector<Inkscape::SnapCandidatePoint> _unselected_points;
    std::vector<Inkscape::SnapCandidatePoint> _all_snap_sources_sorted;
    std::vector<Inkscape::SnapCandidatePoint>::iterator _all_snap_sources_iter;

private:
    bool grabbed(MotionEvent const &event) override;
    void dragged(Geom::Point &new_pos, MotionEvent const &event) override;
    void ungrabbed(ButtonReleaseEvent const *event) override;
};

} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_TOOL_TRANSFORM_HANDLE_SET_H

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

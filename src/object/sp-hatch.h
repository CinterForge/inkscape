// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * SVG <hatch> implementation
 */
/*
 * Authors:
 *   Tomasz Boczkowski <penginsbacon@gmail.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2014 Tomasz Boczkowski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_SP_HATCH_H
#define SEEN_SP_HATCH_H

#include <vector>
#include <glibmm/ustring.h>
#include <sigc++/connection.h>

#include "object-view.h"
#include "svg/svg-length.h"
#include "svg/svg-angle.h"
#include "sp-paint-server.h"
#include "uri-references.h"

class SPHatchReference;
class SPHatchPath;
class SPItem;

namespace Inkscape {
class Drawing;
class DrawingPattern;
namespace XML { class Node; }
} // namespace Inkscape

class SPHatchReference : public Inkscape::URIReference
{
public:
    SPHatchReference(SPHatch *obj);
    SPHatch *getObject() const;

protected:
    bool _acceptObject(SPObject *obj) const override;
};

class SPHatch final : public SPPaintServer
{
public:
    enum class HatchUnits
    {
        UserSpaceOnUse,
        ObjectBoundingBox
    };

    struct RenderInfo
    {
        Geom::Affine child_transform;
        Geom::Affine pattern_to_user_transform;
        Geom::Rect tile_rect;

        int overflow_steps = 0;
        Geom::Affine overflow_step_transform;
        Geom::Affine overflow_initial_transform;
    };

    SPHatch();
    ~SPHatch() override;
    int tag() const override { return tag_of<decltype(*this)>; }

    PaintServerType getPaintType() const override { return PaintServerType::HATCH_PATTERN; }

    // Reference (href)
    Glib::ustring href;
    SPHatchReference ref;

    double x() const;
    double y() const;
    double pitch() const;
    double rotate() const;
    HatchUnits hatchUnits() const;
    HatchUnits hatchContentUnits() const;
    Geom::Affine hatchTransform() const;
    SPHatch const *rootHatch() const;
    SPHatch *rootHatch() { return const_cast<SPHatch *>(std::as_const(*this).rootHatch()); }
    Geom::Affine get_this_transform() const;

    std::vector<SPHatchPath *> hatchPaths();
    std::vector<SPHatchPath const *> hatchPaths() const;

    SPHatch *clone_if_necessary(SPItem *item, char const *property);
    void transform_multiply(Geom::Affine const &postmul, bool set);

    bool isValid() const override;

    Inkscape::DrawingPattern *show(Inkscape::Drawing &drawing, unsigned key, Geom::OptRect const &bbox) override;
    void hide(unsigned key) override;

    RenderInfo calculateRenderInfo(unsigned key) const;
    Geom::Interval bounds() const;
    void setBBox(unsigned int key, Geom::OptRect const &bbox) override;

protected:
    void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
    void release() override;
    void child_added(Inkscape::XML::Node* child, Inkscape::XML::Node* ref) override;
    void set(SPAttr key, char const *value) override;
    void update(SPCtx* ctx, unsigned int flags) override;
    void modified(unsigned int flags) override;

private:
    using View = ObjectView<Inkscape::DrawingPattern>;
    std::vector<View> views;

    static bool _hasHatchPatchChildren(SPHatch const *hatch);

    void _updateView(View &view);
    RenderInfo _calculateRenderInfo(View const &view) const;
    Geom::OptInterval _calculateStripExtents(Geom::OptRect const &bbox) const;

    /**
     * Count how many times hatch is used by the styles of o and its descendants
    */
    int _countHrefs(SPObject *o) const;

    /**
     * Gets called when the hatch is reattached to another <hatch>
     */
    void _onRefChanged(SPObject *old_ref, SPObject *ref);

    /**
     * Gets called when the referenced <hatch> is changed
     */
    void _onRefModified(SPObject *ref, unsigned flags);

    // patternUnits and patternContentUnits attribute
    std::optional<HatchUnits> _hatch_units;
    std::optional<HatchUnits> _hatch_content_units;

    // transform attribute
    std::optional<Geom::Affine> _hatch_transform;

    // Strip
    SVGLength _x;
    SVGLength _y;
    SVGLength _pitch;
    SVGAngle _rotate;

    sigc::connection _modified_connection;
};

#endif // SEEN_SP_HATCH_H

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

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * SVG <pattern> implementation
 *//*
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_SP_PATTERN_H
#define SEEN_SP_PATTERN_H

#include <memory>
#include <vector>
#include <cstddef>
#include <glibmm/ustring.h>
#include <sigc++/connection.h>

#include "object-view.h"
#include "svg/svg-length.h"
#include "sp-paint-server.h"
#include "uri-references.h"
#include "viewbox.h"

class SPPattern;
class SPItem;

namespace Inkscape {
namespace XML {
class Node;
} // namespace XML
} // namespace Inkscape

class SPPatternReference
    : public Inkscape::URIReference
{
public:
    SPPatternReference(SPPattern *owner);
    SPPattern *getObject() const;

protected:
    bool _acceptObject(SPObject *obj) const override;
};

class SPPattern final
    : public SPPaintServer
    , public SPViewBox
{
public:
    enum PatternUnits
    {
        UNITS_USERSPACEONUSE,
        UNITS_OBJECTBOUNDINGBOX
    };

    SPPattern();
    ~SPPattern() override;
    int tag() const override { return tag_of<decltype(*this)>; }

    // Reference (href)
    Glib::ustring href;
    SPPatternReference ref;

    double x() const;
    double y() const;
    double width() const;
    double height() const;
    Geom::Rect getBox() const {
        return Geom::Rect::from_xywh(x(), y(), width(), height());
    }
    Geom::OptRect viewbox() const;
    SPPattern::PatternUnits patternUnits() const;
    SPPattern::PatternUnits patternContentUnits() const;
    Geom::Affine const &getTransform() const;
    SPPattern const *rootPattern() const;
    SPPattern *rootPattern();
    const Geom::Affine& get_this_transform() const;

    SPPattern *clone_if_necessary(SPItem *item, char const *property);
    void transform_multiply(Geom::Affine postmul, bool set);

    /**
     * @brief create a new pattern in XML tree
     * @return created pattern id
     */
    static char const *produce(std::vector<Inkscape::XML::Node*> const &reprs, Geom::Rect const &bounds,
                               SPDocument *document, Geom::Affine const &transform, Geom::Affine const &move);

    bool isValid() const override;
    bool hasItemChildren() const;

protected:
    void build(SPDocument *doc, Inkscape::XML::Node *repr) override;
    void release() override;
    void set(SPAttr key, char const *value) override;
    void update(SPCtx *ctx, unsigned flags) override;
    void modified(unsigned flags) override;

    void child_added(Inkscape::XML::Node* child, Inkscape::XML::Node* ref) override;
    void remove_child(Inkscape::XML::Node* child) override;
    void order_changed(Inkscape::XML::Node *child, Inkscape::XML::Node *old_ref, Inkscape::XML::Node *new_ref) override;

    Inkscape::DrawingPattern *show(Inkscape::Drawing &drawing, unsigned key, Geom::OptRect const &bbox) override;
    void hide(unsigned key) override;
    void setBBox(unsigned key, Geom::OptRect const &bbox) override;

private:

    SPPattern *_chain() const;

    /**
     * Count how many times pattern is used by the styles of o and its descendants
     */
    unsigned _countHrefs(SPObject *o) const;

    /**
     * Gets called when the pattern is reattached to another <pattern>
     */
    void _onRefChanged(SPObject *old_ref, SPObject *ref);

    /**
     * Gets called when the referenced <pattern> is changed
     */
    void _onRefModified(SPObject *ref, unsigned flags);

    /* patternUnits and patternContentUnits attribute */
    PatternUnits _pattern_units : 1;
    bool _pattern_units_set : 1;
    PatternUnits _pattern_content_units : 1;
    bool _pattern_content_units_set : 1;
    /* patternTransform attribute */
    Geom::Affine _pattern_transform;
    bool _pattern_transform_set : 1;
    /* Tile rectangle */
    SVGLength _x;
    SVGLength _y;
    SVGLength _width;
    SVGLength _height;

    sigc::connection _modified_connection;

    /**
     * The pattern at the end of the href chain, currently tasked with keeping our DrawingPattern
     * up to date. When 'shown' is deleted, our DrawingPattern will be unattached from it and 'shown'
     * will be nulled. Later (asynchronously), 'shown' will be re-resolved to another Pattern and our
     * DrawingPattern will be re-attached to that.
     */
    SPPattern *shown;
    sigc::connection shown_released_connection;
    void set_shown(SPPattern *new_shown);

    /**
     * Drawing items belonging to other patterns with this pattern at the end of their href chain.
     * They will be updated in sync with this pattern's children.
     */
    struct AttachedView
    {
        Inkscape::DrawingPattern *drawingitem;
        unsigned key;
    };
    std::vector<AttachedView> attached_views;
    void attach_view(Inkscape::DrawingPattern *di, unsigned key);
    void unattach_view(Inkscape::DrawingPattern *di);

    using View = ObjectView<Inkscape::DrawingPattern>;
    std::vector<View> views;
    void update_view(View &v);
};

#endif // SEEN_SP_PATTERN_H

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

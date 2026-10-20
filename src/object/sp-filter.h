// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * SVG <filter> element
 *//*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Niko Kiirala <niko@kiirala.com>
 *
 * Copyright (C) 2006,2007 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SP_FILTER_H_SEEN
#define SP_FILTER_H_SEEN

#include <memory>
#include <glibmm/ustring.h>

#include <sigc++/scoped_connection.h>
#include "number-opt-number.h"
#include "sp-dimensions.h"
#include "sp-filter-units.h"
#include "sp-item.h"
#include "sp-object.h"

namespace Inkscape {
class Drawing;
class DrawingItem;
namespace Filters { class Filter; }
} // namespace Inkscape

class SPFilterReference;
class SPFilterPrimitive;

class SPFilter
    : public SPObject
    , public SPDimensions
{
public:
    SPFilter();
    ~SPFilter() override;
    int tag() const override { return tag_of<decltype(*this)>; }

    /// Returns a renderer for this filter, for use by the DrawingItem item.
    std::unique_ptr<Inkscape::Filters::Filter> build_renderer(Inkscape::DrawingItem *item);

    /// Returns the number of filter primitives in this SPFilter object.
    int primitive_count() const;

    void update_filter_all_regions();
    void update_filter_region(SPItem *item);
    void set_filter_region(double x, double y, double width, double height);
    Geom::Rect get_automatic_filter_region(SPItem const *item) const;

    /// Checks each filter primitive to make sure the object won't cause issues
    bool valid_for(SPObject const *obj) const;

    /// Returns a result image name that is not in use inside this filter.
    Glib::ustring get_new_result_name() const;

    void show(Inkscape::DrawingItem *item);
    void hide(Inkscape::DrawingItem *item);

    SPFilterUnits filterUnits;
    bool filterUnits_set : 1;
    SPFilterUnits primitiveUnits;
    bool primitiveUnits_set : 1;
    NumberOptNumber filterRes;
    std::unique_ptr<SPFilterReference> href;
    bool auto_region;

    sigc::scoped_connection modified_connection;

    unsigned getRefCount();
    unsigned _refcount = 0;

    void invalidate_slots();
    void ensure_slots();

protected:
    void build(SPDocument *doc, Inkscape::XML::Node *repr) override;
    void release() override;
    void set(SPAttr key, char const *value) override;
    void update(SPCtx *ctx, unsigned flags) override;
    void modified(unsigned flags) override;
    Inkscape::XML::Node *write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, unsigned flags) override;

    void child_added(Inkscape::XML::Node *child, Inkscape::XML::Node *ref) override;
    void remove_child(Inkscape::XML::Node *child) override;
    void order_changed(Inkscape::XML::Node* child, Inkscape::XML::Node* old_repr, Inkscape::XML::Node* new_repr) override;

private:
    bool slots_valid = true;

    std::vector<Inkscape::DrawingItem*> views;
};

#endif // SP_FILTER_H_SEEN

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

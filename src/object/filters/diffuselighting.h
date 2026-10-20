// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief SVG diffuse lighting filter effect
 *//*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Jean-Rene Reinhard <jr@komite.net>
 *
 * Copyright (C) 2006-2007 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SP_FEDIFFUSELIGHTING_H_SEEN
#define SP_FEDIFFUSELIGHTING_H_SEEN

#include <optional>
#include <cstdint>
#include "sp-filter-primitive.h"
#include "number-opt-number.h"

namespace Inkscape {
namespace Filters {
class FilterDiffuseLighting;
} // namespace Filters
} // namespace Inkscape

class SPFeDiffuseLighting final
    : public SPFilterPrimitive
{
public:
    int tag() const override { return tag_of<decltype(*this)>; }

private:
    float surfaceScale = 1.0f;
    float diffuseConstant = 1.0f;

    std::optional<Inkscape::Colors::Color> lighting_color;

    bool surfaceScale_set = false;
    bool diffuseConstant_set = false;

    NumberOptNumber kernelUnitLength; // TODO

protected:
    void build(SPDocument *doc, Inkscape::XML::Node *repr) override;
    void set(SPAttr key, char const *value) override;
    void modified(unsigned flags) override;
    Inkscape::XML::Node *write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, unsigned flags) override;

    void child_added(Inkscape::XML::Node *child, Inkscape::XML::Node *ref) override;
    void remove_child(Inkscape::XML::Node *child) override;
    void order_changed(Inkscape::XML::Node *child, Inkscape::XML::Node *old_repr, Inkscape::XML::Node *new_repr) override;

    std::unique_ptr<Inkscape::Filters::FilterPrimitive> build_renderer(Inkscape::DrawingItem *item) const override;
};

#endif // SP_FEDIFFUSELIGHTING_H_SEEN

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

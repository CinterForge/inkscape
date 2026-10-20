// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SP_FEMERGENODE_H_SEEN
#define SP_FEMERGENODE_H_SEEN

/** \file
 * feMergeNode implementation. A feMergeNode stores information about one
 * input image for feMerge filter primitive.
 */
/*
 * Authors:
 *   Kees Cook <kees@outflux.net>
 *   Niko Kiirala <niko@kiirala.com>
 *
 * Copyright (C) 2004,2007 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <optional>
#include <string>
#include "object/sp-object.h"
#include "display/nr-filter-types.h"

class SlotResolver;

class SPFeMergeNode final
    : public SPObject
{
public:
    int tag() const override { return tag_of<decltype(*this)>; }

    int get_in() const { return in_slot; }

    void invalidate_parent_slots();
    void resolve_slots(SlotResolver const &);

protected:
    void build(SPDocument *doc, Inkscape::XML::Node *repr) override;
    void set(SPAttr key, char const *value) override;

private:
    std::optional<std::string> in_name;
    int in_slot = Inkscape::Filters::NR_FILTER_SLOT_NOT_SET;
};

#endif // SP_FEMERGENODE_H_SEEN

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

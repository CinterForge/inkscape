// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * SVG <feComponentTransfer> implementation.
 */
/*
 * Authors:
 *   hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "componenttransfer.h"

#include <vector>                                    // for vector

#include "componenttransfer-funcnode.h"              // for SPFeFuncNode
#include "document.h"                                // for SPDocument

#include "display/nr-filter-component-transfer.h"    // for FilterComponentT...
#include "object/filters/sp-filter-primitive.h"      // for SPFilterPrimitive
#include "object/sp-object.h"                        // for SPObject, SP_OBJ...

namespace Inkscape {
class DrawingItem;
namespace Filters {
class FilterPrimitive;
} // namespace Filters
namespace XML {
class Node;
} // namespace XML
} // namespace Inkscape

void SPFeComponentTransfer::build(SPDocument *document, Inkscape::XML::Node *repr)
{
	SPFilterPrimitive::build(document, repr);
    document->addResource("feComponentTransfer", this);
}

void SPFeComponentTransfer::child_added(Inkscape::XML::Node *child, Inkscape::XML::Node *ref)
{
    SPFilterPrimitive::child_added(child, ref);
    requestModified(SP_OBJECT_MODIFIED_FLAG);
}

void SPFeComponentTransfer::remove_child(Inkscape::XML::Node *child)
{
    SPFilterPrimitive::remove_child(child);
    requestModified(SP_OBJECT_MODIFIED_FLAG);
}

void SPFeComponentTransfer::release()
{
    if (document) {
        document->removeResource("feComponentTransfer", this);
    }
    SPFilterPrimitive::release();
}

void SPFeComponentTransfer::modified(unsigned flags)
{
    auto const cflags = cascade_flags(flags);

    for (auto &c : children) {
        if (cflags || (c.mflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            c.emitModified(cflags);
        }
    }
}

std::unique_ptr<Inkscape::Filters::FilterPrimitive> SPFeComponentTransfer::build_renderer(Inkscape::DrawingItem*) const
{
    auto componenttransfer = std::make_unique<Inkscape::Filters::FilterComponentTransfer>();
    build_renderer_common(componenttransfer.get());

    bool set[4] = {false, false, false, false};
    for (auto const &node : children) {
        auto funcNode = cast<SPFeFuncNode>(&node);
        if (!funcNode) {
            continue;
        }

        int i;
        switch (funcNode->channel) {
        case SPFeFuncNode::R: i = 0; break;
        case SPFeFuncNode::G: i = 1; break;
        case SPFeFuncNode::B: i = 2; break;
        case SPFeFuncNode::A: i = 3; break;
        default:
            g_warning("Unrecognized channel for component transfer.");
            goto nested_break;
        }

        componenttransfer->type[i] = funcNode->type;
        componenttransfer->tableValues[i] = funcNode->tableValues;
        componenttransfer->slope[i] = funcNode->slope;
        componenttransfer->intercept[i] = funcNode->intercept;
        componenttransfer->amplitude[i] = funcNode->amplitude;
        componenttransfer->exponent[i] = funcNode->exponent;
        componenttransfer->offset[i] = funcNode->offset;

        set[i] = true;
    }
nested_break:;

    // Set any types not explicitly set to the identity transform
    for (int i = 0; i < 4; i++) {
        if (!set[i]) {
            componenttransfer->type[i] = Inkscape::Filters::COMPONENTTRANSFER_TYPE_IDENTITY;
        }
    }

    return componenttransfer;
}

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

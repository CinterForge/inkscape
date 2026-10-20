// SPDX-License-Identifier: GPL-2.0-or-later

#include "copy-resource.h"
#include "document.h"
#include "object/sp-defs.h"
#include "object/sp-object.h"
#include "style.h"
#include "util/uri.h"
#include "xml/repr.h"

// Make a copy of referenced: fill and stroke styles, clip paths
void copy_style_links(const SPObject* source, SPDocument* src_document, SPDocument* dest_document) {

    SPCSSAttr* css = sp_css_attr_from_object(const_cast<SPObject*>(source), SP_STYLE_FLAG_ALWAYS);

    const char* fill = sp_repr_css_property(css, "fill", "none");
    if (auto link = try_extract_uri(fill)) {
        sp_copy_resource(src_document->getObjectByHref(*link), dest_document);
    }

    const char* stroke = sp_repr_css_property(css, "stroke", "none");
    if (auto link = try_extract_uri(stroke)) {
        sp_copy_resource(src_document->getObjectByHref(*link), dest_document);
    }

    sp_repr_css_attr_unref(css);

    if (auto clip = source->getAttribute("clip-path")) {
        if (auto clip_path = try_extract_uri(clip)) {
            sp_copy_resource(src_document->getObjectByHref(*clip_path), dest_document);
        }
    }

    for (auto& child : source->children) {
        copy_style_links(&child, src_document, dest_document);
    }
}


SPObject* sp_copy_resource(const SPObject* source, SPDocument* dest_document) {
    if (!source || !source->document || !dest_document) {
        return nullptr;
    }

    // make a copy of the 'source' object
    auto src_doc = source->document;
    auto dest_defs = dest_document->getDefs();
    Inkscape::XML::Document* xml_doc = dest_document->getReprDoc();
    Inkscape::XML::Node* repr = source->getRepr()->duplicate(xml_doc);
    dest_defs->getRepr()->appendChild(repr);
    auto object = dest_document->getObjectByRepr(repr);
    g_assert(object != nullptr);
    Inkscape::GC::release(repr);

    // if 'source' references another object, copy it too
    auto xhref = object->getAttribute("xlink:href");
    auto href = object->getAttribute("href");

    if (href || xhref) {
        if (!href) {
            href = xhref;
        }
        if (!dest_document->getObjectByHref(href)) {
            sp_copy_resource(src_doc->getObjectByHref(href), dest_document);
        }
    }

    // check fill and stroke for references to other objects, like gradients, and copy them too
    copy_style_links(object, src_doc, dest_document);

    return object;
}

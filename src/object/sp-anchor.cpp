// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SVG <a> element implementation
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2017 Martin Owens
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#define noSP_ANCHOR_VERBOSE

#include <glibmm/i18n.h>

#include "attributes.h"
#include "document.h"
#include "sp-anchor.h"

#include "xml/document.h"               // for Document
#include "xml/href-attribute-helper.h"  // for setHrefAttribute
#include "xml/quote.h"                  // for xml_quote_strdup

void SPAnchor::build(SPDocument *document, Inkscape::XML::Node *repr) {
    SPGroup::build(document, repr);

    this->readAttr(SPAttr::XLINK_TYPE);
    this->readAttr(SPAttr::XLINK_ROLE);
    this->readAttr(SPAttr::XLINK_ARCROLE);
    this->readAttr(SPAttr::XLINK_TITLE);
    this->readAttr(SPAttr::XLINK_SHOW);
    this->readAttr(SPAttr::XLINK_ACTUATE);
    this->readAttr(SPAttr::XLINK_HREF);
    this->readAttr(SPAttr::TARGET);
}

void SPAnchor::release() {
    if (this->href) {
        g_free(this->href);
        this->href = nullptr;
    }
    if (this->type) {
        g_free(this->type);
        this->type = nullptr;
    }
    if (this->title) {
        g_free(this->title);
        this->title = nullptr;
    }
    if (this->page) {
        g_free(this->page);
        this->page = nullptr;
    }

    SPGroup::release();
}

void SPAnchor::set(SPAttr key, const gchar* value) {
    switch (key) {
	case SPAttr::XLINK_HREF:
            g_free(this->href);
            this->href = g_strdup(value);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            this->updatePageAnchor();
            break;
	case SPAttr::XLINK_TYPE:
            g_free(this->type);
            this->type = g_strdup(value);
            this->updatePageAnchor();
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
	case SPAttr::XLINK_ROLE:
	case SPAttr::XLINK_ARCROLE:
	case SPAttr::XLINK_TITLE:
            g_free(this->title);
            this->title = g_strdup(value);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
	case SPAttr::XLINK_SHOW:
	case SPAttr::XLINK_ACTUATE:
	case SPAttr::TARGET:
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;

	default:
            SPGroup::set(key, value);
            break;
    }
}

/*
 * Detect if this anchor qualifies as a page link and append
 * the new page document to this document.
 */
void SPAnchor::updatePageAnchor() {
    if (this->type && !strcmp(this->type, "page")) {
        local_link.reset();
        if (this->href && !this->page) {
             this->page = this->document->createChildDoc(this->href);
        }
    } else if (this->href) {
        local_link = std::make_unique<Inkscape::URIReference>(this);
        local_link->try_attach(this->href);
    }
}

#define COPY_ATTR(rd,rs,key) (rd)->setAttribute((key), rs->attribute(key));

Inkscape::XML::Node* SPAnchor::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:a");
    }

    Inkscape::setHrefAttribute(*repr, this->href);
    if (this->type) repr->setAttribute("xlink:type", this->type);
    if (this->title) repr->setAttribute("xlink:title", this->title);

    if (repr != this->getRepr()) {
        // XML Tree being directly used while it shouldn't be in the
        //  below COPY_ATTR lines
        COPY_ATTR(repr, this->getRepr(), "xlink:role");
        COPY_ATTR(repr, this->getRepr(), "xlink:arcrole");
        COPY_ATTR(repr, this->getRepr(), "xlink:show");
        COPY_ATTR(repr, this->getRepr(), "xlink:actuate");
        COPY_ATTR(repr, this->getRepr(), "target");
    }

    SPGroup::write(xml_doc, repr, flags);

    return repr;
}

const char* SPAnchor::typeName() const {
    return "link";
}

const char* SPAnchor::displayName() const {
    return C_("Hyperlink|Noun", "Link");
}

gchar* SPAnchor::description() const {
    if (this->href) {
        char *quoted_href = xml_quote_strdup(this->href);
        char *ret = g_strdup_printf(_("to %s"), quoted_href);
        g_free(quoted_href);
        return ret;
    } else {
        return g_strdup (_("without URI"));
    }
}

void SPAnchor::getLinked(std::vector<SPObject *> &objects, LinkedObjectNature direction) const
{
    if (direction == LinkedObjectNature::ANY || direction == LinkedObjectNature::DEPENDENCY) {
        if (auto linked = (local_link ? local_link->getObject() : nullptr)) {
            objects.emplace_back(linked);
        }
    }
    SPObject::getLinked(objects, direction);
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

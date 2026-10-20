// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SVG <inkscape:tagref> implementation
 *
 * Authors:
 *   Theodore Janeczko
 *   Liam P White
 *
 * Copyright (C) Theodore Janeczko 2012-2014 <flutterguy317@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-tag-use.h"

#include <glibmm/i18n.h>
#include <sigc++/functors/mem_fun.h>

#include "attributes.h"                 // for SPAttr
#include "bad-uri-exception.h"          // for BadURIException
#include "sp-item.h"                    // for SPItem
#include "sp-object.h"                  // for SPObject, sp_object_unref
#include "sp-factory.h"                 // for NodeTraits, SPFactory
#include "sp-tag-use-reference.h"       // for SPTagUseReference
#include "uri.h"                        // for URI

#include "xml/document.h"               // for Document
#include "xml/href-attribute-helper.h"  // for getHrefAttribute
#include "xml/node.h"                   // for Node

class SPDocument;

SPTagUse::SPTagUse()
    : ref{std::make_unique<SPTagUseReference>(this)}
{
    _changed_connection = ref->changedSignal().connect(sigc::mem_fun(*this, &SPTagUse::href_changed));
}

SPTagUse::~SPTagUse()
{
    if (child) {
        detach(child);
        child = nullptr;
    }

    ref->detach();
    ref.reset();
}

void
SPTagUse::build(SPDocument *document, Inkscape::XML::Node *repr)
{
    SPObject::build(document, repr);
    readAttr(SPAttr::XLINK_HREF);

    // We don't need to create child here:
    // reading xlink:href will attach ref, and that will cause the changed signal to be emitted,
    // which will call sp_tag_use_href_changed, and that will take care of the child
}

void
SPTagUse::release()
{
    if (child) {
        detach(child);
        child = nullptr;
    }

    _changed_connection.disconnect();

    href.reset();

    ref->detach();

    SPObject::release();
}

void
SPTagUse::set(SPAttr key, char const *value)
{

    switch (key) {
        case SPAttr::XLINK_HREF: {
            if (value && href && *href == value) {
                /* No change, do nothing. */
            } else {
                href.reset();

                if (value) {
                    // First, set the href field, because sp_tag_use_href_changed will need it.
                    href = value;

                    // Now do the attaching, which emits the changed signal.
                    try {
                        ref->attach(Inkscape::URI(value));
                    } catch (Inkscape::BadURIException const &e) {
                        g_warning("%s", e.what());
                        ref->detach();
                    }
                } else {
                    ref->detach();
                }
            }
            break;
        }

        default:
                SPObject::set(key, value);
            break;
    }
}

Inkscape::XML::Node *
SPTagUse::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned flags)
{
    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("inkscape:tagref");
    }

    SPObject::write(xml_doc, repr, flags);
    
    if (ref->getURI()) {
        auto uri_string = ref->getURI()->str();
        auto href_key = Inkscape::getHrefAttribute(*repr).first;
        repr->setAttributeOrRemoveIfEmpty(href_key, uri_string);
    }

    return repr;
}

/**
 * Returns the ultimate original of a SPTagUse (i.e. the first object in the chain of its originals
 * which is not an SPTagUse). If no original is found, NULL is returned (it is the responsibility
 * of the caller to make sure that this is handled correctly).
 *
 * Note that the returned is the clone object, i.e. the child of an SPTagUse (of the argument one for
 * the trivial case) and not the "true original".
 */
 
SPItem * SPTagUse::root()
{
    SPObject *orig = child;
    while (orig && is<SPTagUse>(orig)) {
        orig = cast<SPTagUse>(orig)->child;
    }
    if (!orig || !is<SPItem>(orig))
        return nullptr;
    return cast<SPItem>(orig);
}

void
SPTagUse::href_changed(SPObject * /*old_ref*/, SPObject * /*ref*/)
{
    if (href) {
        SPItem *refobj = ref->getObject();
        if (refobj) {
            Inkscape::XML::Node *childrepr = refobj->getRepr();
            const std::string typeString = NodeTraits::get_type_string(*childrepr);
            SPObject* child_ = SPFactory::createObject(typeString);
            if (child_) {
                child = child_;
                attach(child_, lastChild());
                sp_object_unref(child_, nullptr);
                child_->invoke_build(this->document, childrepr, TRUE);

            }
        }
    }
}

SPItem * SPTagUse::get_original()
{
    if (ref) {
        return ref->getObject();
    }
    return nullptr;
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

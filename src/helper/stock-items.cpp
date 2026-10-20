// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Stock-items
 *
 * Stock Item management code
 *
 * Authors:
 *  John Cliff <simarilius@yahoo.com>
 *  Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright 2004 John Cliff
 *
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "stock-items.h"

#include <cstring>
#include <glibmm/fileutils.h>

#include "document.h"
#include "inkscape.h"

#include "io/resource.h"
#include "manipulation/copy-resource.h"
#include "object/sp-gradient.h"
#include "object/sp-hatch.h"
#include "object/sp-pattern.h"
#include "object/sp-marker.h"
#include "object/sp-defs.h"
#include "util/static-doc.h"

StockPaintDocuments::StockPaintDocuments()
{
    using namespace Inkscape::IO::Resource;
    auto files = get_filenames(SYSTEM, PAINT, {".svg"});
    auto share = get_filenames(SHARED, PAINT, {".svg"});
    auto user  = get_filenames(USER,   PAINT, {".svg"});
    files.insert(files.end(), user.begin(), user.end());
    files.insert(files.end(), share.begin(), share.end());
    for (auto const &file : files) {
        if (Glib::file_test(file, Glib::FileTest::IS_REGULAR)) {
            if (auto doc = SPDocument::createNewDoc(file.c_str())) {
                doc->ensureUpToDate(); // update, so patterns referencing clippaths render properly
                documents.push_back(std::move(doc));
            } else {
                g_warning("File %s not loaded.", file.c_str());
            }
        }
    }
}

std::vector<SPDocument *> StockPaintDocuments::get_paint_documents(std::function<bool (SPDocument *)> const &filter)
{
    std::vector<SPDocument *> out;

    for (auto const &doc : documents) {
        if (filter(doc.get())) {
            out.push_back(doc.get());
        }
    }

    return out;
}

static std::unique_ptr<SPDocument> load_paint_doc(char const *basename, Inkscape::IO::Resource::Type type = Inkscape::IO::Resource::PAINT)
{
    using namespace Inkscape::IO::Resource;

    for (Domain const domain : {SYSTEM, CREATE}) {
        auto const filename = get_path_string(domain, type, basename);
        if (Glib::file_test(filename, Glib::FileTest::IS_REGULAR)) {
            if (auto doc = SPDocument::createNewDoc(filename.c_str())) {
                doc->ensureUpToDate();
                return doc;
            }
        }
    }

    return nullptr;
}

// FIXME: these should be merged with the icon loading code so they
// can share a common file/doc cache.  This function should just
// take the dir to look in, and the file to check for, and cache
// against that, rather than the existing copy/paste code seen here.

static SPObject *sp_marker_load_from_svg(char const *name, SPDocument *current_doc)
{
    if (!current_doc) {
        return nullptr;
    }

    // Try to load from document.
    auto const doc = Inkscape::Util::cache_static_doc([] { return load_paint_doc("markers.svg", Inkscape::IO::Resource::MARKERS); });
    if (!doc) {
        return nullptr;
    }

    // Get the object we want.
    auto obj = doc->getObjectById(name);
    if (!is<SPMarker>(obj)) {
        return nullptr;
    }

    auto defs = current_doc->getDefs();
    auto xml_doc = current_doc->getReprDoc();
    auto repr = obj->getRepr()->duplicate(xml_doc);
    defs->getRepr()->appendChild(repr);
    auto copied = current_doc->getObjectByRepr(repr);
    Inkscape::GC::release(repr);

    return copied;
}

// return pattern or hatch object, if found
static SPObject* sp_pattern_load_from_svg(gchar const *name, SPDocument *current_doc, SPDocument* source_doc) {
    if (!current_doc || !source_doc) {
        return nullptr;
    }
    // Try to load from document
    // Get the pattern we want
    auto obj = source_doc->getObjectById(name);
    if (auto pattern = cast<SPPattern>(obj)) {
        return sp_copy_resource(pattern, current_doc);
    }
    if (auto hatch = cast<SPHatch>(obj)) {
        return sp_copy_resource(hatch, current_doc);
    }
    return nullptr;
}

static SPObject *sp_gradient_load_from_svg(char const *name, SPDocument *current_doc)
{
    if (!current_doc) {
        return nullptr;
    }

    // Try to load from document.
    auto const doc = Inkscape::Util::cache_static_doc([] { return load_paint_doc("gradients.svg"); });
    if (!doc) {
        return nullptr;
    }

    // Get the object we want.
    auto obj = doc->getObjectById(name);
    if (!is<SPGradient>(obj)) {
        return nullptr;
    }

    auto defs = current_doc->getDefs();
    auto xml_doc = current_doc->getReprDoc();
    auto repr = obj->getRepr()->duplicate(xml_doc);
    defs->getRepr()->appendChild(repr);
    auto copied = current_doc->getObjectByRepr(repr);
    Inkscape::GC::release(repr);

    return copied;
}

// get_stock_item returns a pointer to an instance of the desired stock object in the current doc
// if necessary it will import the object. Copes with name clashes through use of the inkscape:stockid property
// This should be set to be the same as the id in the library file.

SPObject *get_stock_item(gchar const *urn, bool stock, SPDocument* stock_doc)
{
    g_assert(urn != nullptr);

    /* check its an inkscape URN */
    if (!strncmp (urn, "urn:inkscape:", 13)) {

        gchar const *e = urn + 13;
        int a = 0;
        gchar * name = g_strdup(e);
        gchar *name_p = name;
        while (*name_p != ':' && *name_p != '\0'){
            name_p++;
            a++;
        }

        if (*name_p ==':') {
            name_p++;
        }

        gchar * base = g_strndup(e, a);

        SPDocument *doc = SP_ACTIVE_DOCUMENT;
        SPDefs *defs = doc->getDefs();
        if (!defs) {
            g_free(base);
            return nullptr;
        }
        SPObject *object = nullptr;
        if (!strcmp(base, "marker") && !stock) {
            for (auto& child: defs->children)
            {
                if (child.getRepr()->attribute("inkscape:stockid") &&
                    !strcmp(name_p, child.getRepr()->attribute("inkscape:stockid")) &&
                    is<SPMarker>(&child))
                {
                    object = &child;
                }
            }
        }
        else if (!strcmp(base,"pattern") && !stock)  {
            for (auto& child: defs->children)
            {
                if (child.getRepr()->attribute("inkscape:stockid") &&
                    !strcmp(name_p, child.getRepr()->attribute("inkscape:stockid")) &&
                    (is<SPPattern>(&child) || is<SPHatch>(&child))) // allow hatches too
                {
                    object = &child;
                }
            }
        }
        else if (!strcmp(base,"gradient") && !stock)  {
            for (auto& child: defs->children)
            {
                if (child.getRepr()->attribute("inkscape:stockid") &&
                    !strcmp(name_p, child.getRepr()->attribute("inkscape:stockid")) &&
                    is<SPGradient>(&child))
                {
                    object = &child;
                }
            }
        }

        if (object == nullptr) {

            if (!strcmp(base, "marker"))  {
                object = sp_marker_load_from_svg(name_p, doc);
            }
            else if (!strcmp(base, "pattern"))  {
                object = sp_pattern_load_from_svg(name_p, doc, stock_doc);
                if (object) {
                    object->getRepr()->setAttribute("inkscape:collect", "always");
                }
            }
            else if (!strcmp(base, "gradient"))  {
                object = sp_gradient_load_from_svg(name_p, doc);
            }
        }

        g_free(base);
        g_free(name);

        if (object) {
            object->setAttribute("inkscape:isstock", "true");
        }

        return object;
    }

    else {

        SPDocument *doc = SP_ACTIVE_DOCUMENT;
        SPObject *object = doc->getObjectById(urn);

        return object;
    }
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

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Routines for resolving ID clashes when importing or pasting.
 *
 * Authors:
 *   Stephen Silver <sasilver@users.sourceforge.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2008 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "id-clash.h"

#include <cstdlib>
#include <cstring>
#include <list>
#include <string>
#include <utility>
#include <glibmm/i18n.h>
#include <glibmm/regex.h>
#include <unordered_map>

#include "attributes.h"                              // for SPAttr
#include "document.h"                                // for SPDocument
#include "style-internal.h"                          // for SPIPaint, SPIShapes
#include "style.h"                                   // for SPStyle, sp_styl...
#include "util/uri.h"                                // for extract_uri

#include "live_effects/effect.h"
#include "live_effects/lpeobject.h"
#include "live_effects/parameter/parameter.h"
#include "object/sp-gradient.h"
#include "object/sp-object.h"
#include "object/sp-paint-server.h"
#include "object/sp-root.h"
#include "object/sp-use.h"

enum ID_REF_TYPE { REF_HREF, REF_STYLE, REF_SHAPES, REF_URL, REF_CLIPBOARD };

struct IdReference {
    ID_REF_TYPE type;
    Inkscape::XML::Node *repr;
    const char *attr;  // property or href-like attribute
};

typedef std::unordered_map<std::string, std::list<IdReference> > refmap_type;

typedef std::pair<SPObject*, Glib::ustring> id_changeitem_type;
typedef std::list<id_changeitem_type> id_changelist_type;

const char *href_like_attributes[] = {"inkscape:connection-end",
                                      "inkscape:connection-end-point",
                                      "inkscape:connection-start",
                                      "inkscape:connection-start-point",
                                      "inkscape:href",
                                      "inkscape:path-effect",
                                      "inkscape:perspectiveID",
                                      "inkscape:linked-fill",
                                      "inkscape:tiled-clone-of",
                                      "href",
                                      "xlink:href"};
#define NUM_HREF_LIKE_ATTRIBUTES (sizeof(href_like_attributes) / sizeof(*href_like_attributes))

const SPIPaint SPStyle::* SPIPaint_members[] = {
    //&SPStyle::color,
    reinterpret_cast<SPIPaint SPStyle::*>(&SPStyle::fill),
    reinterpret_cast<SPIPaint SPStyle::*>(&SPStyle::stroke),
};
const char* SPIPaint_properties[] = {
    //"color",
    "fill",
    "stroke",
};
#define NUM_SPIPAINT_PROPERTIES (sizeof(SPIPaint_properties) / sizeof(*SPIPaint_properties))

const SPIShapes SPStyle::* SPIShapes_members[] = {
    reinterpret_cast<SPIShapes SPStyle::*>(&SPStyle::shape_inside),
    reinterpret_cast<SPIShapes SPStyle::*>(&SPStyle::shape_subtract),
};
const char *SPIShapes_properties[] = {
    "shape-inside",
    "shape-subtract",
};
#define NUM_SPISHAPES_PROPERTIES (sizeof(SPIShapes_properties) / sizeof(*SPIShapes_properties))

const char* other_url_properties[] = {
    "clip-path",
    "color-profile",
    "cursor",
    "marker-end",
    "marker-mid",
    "marker-start",
    "mask",
};
#define NUM_OTHER_URL_PROPERTIES (sizeof(other_url_properties) / sizeof(*other_url_properties))

const char* clipboard_properties[] = {
    //"color",
    "fill",
    "filter",
    "stroke",
    "marker-end",
    "marker-mid",
    "marker-start"
};
#define NUM_CLIPBOARD_PROPERTIES (sizeof(clipboard_properties) / sizeof(*clipboard_properties))

/**
 * Given an reference (idref), make it point to to_obj instead
 */
static void
fix_ref(IdReference const &idref, SPObject *to_obj, const char *old_id) {
    switch (idref.type) {
        case REF_HREF: {
            if (idref.repr->attribute(idref.attr)) {
                gchar *new_uri = g_strdup_printf("#%s", to_obj->getId());
                Glib::ustring value = idref.repr->attribute(idref.attr);
                // look to values stores as separated id references like inkscape::path-effect or LPE satellites param
                Glib::ustring old = "#";
                old += old_id;
                size_t posid = value.find(old_id);
                if (new_uri && posid != Glib::ustring::npos) {
                    if (!g_strcmp0(idref.attr,"inkscape:linked-fill")) {
                        value = value.replace(posid, old.size() - 1, to_obj->getId());
                    } else {
                        value = value.replace(posid - 1, old.size(), new_uri);
                    }
                    idref.repr->setAttribute(idref.attr, value.c_str());
                }
                g_free(new_uri);
            }
            break;
        }
        case REF_STYLE: {
            sp_style_set_property_url_on_repr(idref.repr, idref.attr, to_obj, false);
            break;
        }
        case REF_SHAPES: {
            SPCSSAttr* css = sp_repr_css_attr (idref.repr, "style");
            std::string prop = sp_repr_css_property (css, idref.attr, nullptr);
            std::string oid; oid.append("url(#").append(old_id).append(")");
            auto pos = prop.find(oid);
            if (pos != std::string::npos) {
                std::string nid; nid.append("url(#").append(to_obj->getId()).append(")");
                prop.replace(pos, oid.size(), nid);
                sp_repr_css_set_property (css, idref.attr, prop.c_str());
                sp_repr_css_set (idref.repr, css, "style");
            } else {
                std::cerr << "Failed to switch id -- shouldn't happen" << std::endl;
            }
            break;
        }
        case REF_URL: {
            gchar *url = g_strdup_printf("url(#%s)", to_obj->getId());
            idref.repr->setAttribute(idref.attr, url);
            g_free(url);
            break;
        }
        case REF_CLIPBOARD: {
            SPCSSAttr *style = sp_repr_css_attr(idref.repr, "style");
            gchar *url = g_strdup_printf("url(#%s)", to_obj->getId());
            sp_repr_css_set_property(style, idref.attr, url);
            g_free(url);
            Glib::ustring style_string;
            sp_repr_css_write_string(style, style_string);
            idref.repr->setAttributeOrRemoveIfEmpty("style", style_string);
            break;
        }
    }
}

// Note: elem might be null in some cases (like inkscape:clipboard handling, where its SPObject
//  counterpart does not exist). So guard any uses of it.
static void _find_references_helper(SPObject *elem, Inkscape::XML::Node *repr_elem, refmap_type &refmap)
{
    if (elem && elem->cloned) return;
    if (!repr_elem) return;
    if (repr_elem->type() != Inkscape::XML::NodeType::ELEMENT_NODE) return;

    /* check for references in inkscape:clipboard elements */
    if (!std::strcmp(repr_elem->name(), "inkscape:clipboard")) {
        SPCSSAttr *css = sp_repr_css_attr(repr_elem, "style");
        if (css) {
            for (auto attr : clipboard_properties) {
                const gchar *value = sp_repr_css_property(css, attr, nullptr);
                if (value) {
                    auto uri = extract_uri(value);
                    if (uri[0] == '#') {
                        IdReference idref = { REF_CLIPBOARD, repr_elem, attr };
                        refmap[uri.c_str() + 1].push_back(idref);
                    }
                }
            }

        }
    }
    if (!std::strcmp(repr_elem->name(), "inkscape:path-effect")) {
        auto lpeobj = cast<LivePathEffectObject>(elem);
        if (lpeobj) {
            Inkscape::LivePathEffect::Effect *effect = lpeobj->get_lpe();
            if (effect) {
                for (auto &p : effect->param_vector) {
                    if (p->paramType() == Inkscape::LivePathEffect::SATELLITE || 
                        p->paramType() == Inkscape::LivePathEffect::SATELLITE_ARRAY || 
                        p->paramType() == Inkscape::LivePathEffect::PATH ||
                        p->paramType() == Inkscape::LivePathEffect::PATH_ARRAY ||
                        p->paramType() == Inkscape::LivePathEffect::ORIGINAL_PATH || 
                        p->paramType() == Inkscape::LivePathEffect::ORIGINAL_SATELLITE) 
                    {
                        const gchar *val = repr_elem->attribute(p->param_key.c_str());
                        if (val) {
                            gchar **strarray = g_strsplit(val, "|", 0);
                            if (strarray) {
                                unsigned int i = 0;
                                Glib::ustring realycopied = "";
                                bool write = false;
                                while (strarray[i]) {
                                    gchar *splitid = g_strdup(g_strstrip(strarray[i]));
                                    if (splitid[0] == '#') {
                                        std::string id(splitid + 1);
                                        if (size_t pos = id.find(",")) {
                                            if (pos != Glib::ustring::npos) {
                                                id.erase(pos);
                                            }
                                        }

                                        IdReference idref = {REF_HREF, repr_elem, p->param_key.c_str()};
                                        SPObject *refobj = elem->document->getObjectById(id);
                                        // special tweak to allow clone original LPE keep cloned on copypase without
                                        // operand also added to path parameters
                                        bool cloneoriginal = p->effectType() == Inkscape::LivePathEffect::CLONE_ORIGINAL;
                                        bool bypass = ((p->param_key == "linkeditem") && cloneoriginal);
                                        bypass = bypass || p->paramType() == Inkscape::LivePathEffect::ParamType::PATH;
                                        bypass = bypass || p->paramType() == Inkscape::LivePathEffect::ParamType::ORIGINAL_PATH;
                                        bypass = bypass || p->paramType() == Inkscape::LivePathEffect::ParamType::PATH_ARRAY;
                                        bypass = bypass && !refobj;
                                        if (refobj || bypass) {
                                            if (!bypass) {
                                                refmap[id].push_back(idref);
                                            } else {
                                                write = true;
                                            }
                                            if (!realycopied.empty()) {
                                                realycopied += " | ";
                                            }
                                            realycopied += splitid;
                                        } else {
                                            write = true;
                                        }
                                    }
                                    i++;
                                    g_free(splitid);
                                }
                                if (write) {
                                    repr_elem->setAttribute(p->param_key.c_str(), realycopied.c_str());
                                }
                                g_strfreev(strarray);
                            }
                        }
                    }
                }
            }
        }
    }
    /* check for xlink:href="#..." and similar */
    for (auto attr : href_like_attributes) {
        Glib::ustring attfixed = "";
        if (repr_elem->attribute(attr)) {
            if (!g_strcmp0(attr,"inkscape:linked-fill")) {
                attfixed += "#";
            }
            attfixed += repr_elem->attribute(attr);
        }
        const gchar *val = attfixed.c_str();
        if (!attfixed.empty() && attfixed[0] == '#') {
            gchar **strarray = g_strsplit(val, ";", 0);
            if (strarray) {
                unsigned int i = 0;
                while (strarray[i]) {
                    if (strarray[i][0] == '#') {
                        std::string id(strarray[i] + 1);
                        IdReference idref = {REF_HREF, repr_elem, attr};
                        refmap[id].push_back(idref);
                    }
                    i++;
                }
                g_strfreev(strarray);
            }
        }
    }

    /* check for other url(#...) references */
    for (auto attr : other_url_properties) {
        const gchar *value = repr_elem->attribute(attr);
        if (value) {
            auto uri = extract_uri(value);
            if (uri[0] == '#') {
                IdReference idref = { REF_URL, repr_elem, attr };
                refmap[uri.c_str() + 1].push_back(idref);
            }
        }
    }

    // After this point, we are evalutating SPObject-specific properties and children
    if (!elem) {
        return;
    }

    SPStyle *style = elem->style;

    /* check for url(#...) references in 'fill' or 'stroke' */
    for (unsigned i = 0; i < NUM_SPIPAINT_PROPERTIES; ++i) {
        const SPIPaint SPStyle::*prop = SPIPaint_members[i];
        const SPIPaint *paint = &(style->*prop);
        if (paint->isPaintserver() && paint->href) {
            const SPObject *obj = paint->href->getObject();
            if (obj) {
                const gchar *id = obj->getId();
                IdReference idref = { REF_STYLE, repr_elem, SPIPaint_properties[i] };
                refmap[id].push_back(idref);
            }
        }
    }

    /* check for shape-inside/shape-subtract that contain multiple url(#..) each */
    for (unsigned i = 0; i < NUM_SPISHAPES_PROPERTIES; ++i) {
        const SPIShapes SPStyle::*prop = SPIShapes_members[i];
        const SPIShapes *shapes = &(style->*prop);
        for (auto *href : shapes->hrefs) {
            auto obj = href->getObject();
            if (!obj)
                continue;
            auto shape_id = obj->getId();
            IdReference idref = { REF_SHAPES, repr_elem, SPIShapes_properties[i] };
            refmap[shape_id].push_back(idref);
        }
    }

    /* check for url(#...) references in 'filter' */
    const SPIFilter *filter = &(style->filter);
    if (filter->href) {
        const SPObject *obj = filter->href->getObject();
        if (obj) {
            const gchar *id = obj->getId();
            IdReference idref = { REF_STYLE, repr_elem, "filter" };
            refmap[id].push_back(idref);
        }
    }

    /* check for url(#...) references in markers */
    const gchar *markers[4] = { "", "marker-start", "marker-mid", "marker-end" };
    for (unsigned i = SP_MARKER_LOC_START; i < SP_MARKER_LOC_QTY; i++) {
        const gchar *value = style->marker_ptrs[i]->value();
        if (value) {
            auto uri = extract_uri(value);
            if (uri[0] == '#') {
                IdReference idref = { REF_STYLE, repr_elem, markers[i] };
                refmap[uri.c_str() + 1].push_back(idref);
            }
        }
    }

    // recurse
    for (auto& child: elem->children)
    {
        _find_references_helper(&child, child.getRepr(), refmap);
    }
}

/**
 *  Build a table of places where IDs are referenced, for a given element.
 *  FIXME: There are some types of references not yet dealt with here
 *         (e.g., ID selectors in CSS stylesheets, and references in scripts).
 */
static void find_references(SPDocument *doc, refmap_type &refmap)
{
    _find_references_helper(doc->getRoot(), doc->getReprRoot(), refmap);

    // Also search clipboard (which doesn't have its own SPObject, so won't be found in above call)
    if (auto clipboard = sp_repr_lookup_name(doc->getReprRoot(), "inkscape:clipboard", 1)) {
        _find_references_helper(nullptr, clipboard, refmap);
    }
}

/**
 *  Change any IDs that clash with IDs in the current document, and make
 *  a list of those changes that will require fixing up references.
 */
static void change_clashing_ids(SPDocument *imported_doc, SPDocument *current_doc, SPObject *elem,
                                refmap_type const &refmap, id_changelist_type *id_changes)
{
    const gchar *id = elem->getId();
    bool fix_clashing_ids = true;

    if (id && current_doc->getObjectById(id)) {
        // Choose a new ID.
        // To try to preserve any meaningfulness that the original ID
        // may have had, the new ID is the old ID followed by a hyphen
        // and one or more digits.

        if (is<SPGradient>(elem)) {
            SPObject *cd_obj =  current_doc->getObjectById(id);

            if (cd_obj && is<SPGradient>(cd_obj)) {
                auto cd_gr = cast<SPGradient>(cd_obj);
                if ( cd_gr->isEquivalent(cast<SPGradient>(elem))) {
                    fix_clashing_ids = false;
                 }
             }
        }

        if (fix_clashing_ids) {
            std::string old_id(id);
            std::string new_id(old_id + '-');
            for (;;) {
                new_id += "0123456789"[std::rand() % 10];
                const char *str = new_id.c_str();
                if (current_doc->getObjectById(str) == nullptr &&
                    imported_doc->getObjectById(str) == nullptr) break;
            }
            // Change to the new ID

            elem->setAttribute("id", new_id);
            // Make a note of this change, if we need to fix up refs to it
            if (refmap.find(old_id) != refmap.end()) {
                id_changes->emplace_back(elem, old_id);
            }
        }
    }


    // recurse
    for (auto& child: elem->children)
    {
        change_clashing_ids(imported_doc, current_doc, &child, refmap, id_changes);
    }
}

/**
 *  Fix up references to changed IDs.
 */
static void
fix_up_refs(refmap_type const &refmap, const id_changelist_type &id_changes)
{
    id_changelist_type::const_iterator pp;
    const id_changelist_type::const_iterator pp_end = id_changes.end();
    for (pp = id_changes.begin(); pp != pp_end; ++pp) {
        SPObject *obj = pp->first;
        refmap_type::const_iterator pos = refmap.find(pp->second);
        std::list<IdReference>::const_iterator it;
        const std::list<IdReference>::const_iterator it_end = pos->second.end();
        for (it = pos->second.begin(); it != it_end; ++it) {
            fix_ref(*it, obj, pp->second.c_str());
        }
    }
}

/**
 *  This function resolves ID clashes between the document being imported
 *  and the current open document: IDs in the imported document that would
 *  clash with IDs in the existing document are changed, and references to
 *  those IDs are updated accordingly.
 */
void prevent_id_clashes(SPDocument *imported_doc, SPDocument *current_doc)
{
    refmap_type refmap;
    id_changelist_type id_changes;
    SPObject *imported_root = imported_doc->getRoot();

    find_references(imported_doc, refmap);
    change_clashing_ids(imported_doc, current_doc, imported_root, refmap, &id_changes);
    fix_up_refs(refmap, id_changes);
}

/*
 * Change any references of svg:def from_obj into to_obj
 */
void
change_def_references(SPObject *from_obj, SPObject *to_obj)
{
    refmap_type refmap;
    SPDocument *current_doc = from_obj->document;
    std::string old_id(from_obj->getId());

    find_references(current_doc, refmap);

    refmap_type::const_iterator pos = refmap.find(old_id);
    if (pos != refmap.end()) {
        std::list<IdReference>::const_iterator it;
        const std::list<IdReference>::const_iterator it_end = pos->second.end();
        for (it = pos->second.begin(); it != it_end; ++it) {
            fix_ref(*it, to_obj, from_obj->getId());
        }
    }
}

// This is a subset of the valid XML 1.0 ID characters.
const char valid_id_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.:";

std::pair<bool, Glib::ustring> is_object_id_valid(const std::string& id) {
    if (id.empty()) {
        return std::make_pair(false, _("Invalid empty ID"));
    }
    auto c = id[0];
    if (isdigit(c)) {
        return std::make_pair(false, _("ID cannot start with a digit"));
    }
    if (c == '.') {
        return std::make_pair(false, _("ID cannot start with a period"));
    }
    if (c == '-') {
        return std::make_pair(false, _("ID cannot start with a dash"));
    }
    if (auto pos = id.find_first_not_of(valid_id_chars); pos != std::string::npos) {
        return std::make_pair(false, pos == 0 ? _("ID starts with an invalid character") : _("ID contains invalid characters"));
    }

    return std::make_pair(true, Glib::ustring());
}

/**
 * Convert string to valid XML id.
 * The input is returned unchanged if it is a valid ID and only uses ASCII characters.
 * The returned ID is not guaranteed to be unique. Different inputs can map to the same output.
 */
Glib::ustring sanitize_id(const Glib::ustring& input_id) {
    // XML allows IDs of the form [a-zA-Z_:][a-zA-Z0-9\-_\.:]* plus some UTF8 characters.
    // We restrict us to the ASCII subset for simplicity.
    // https://www.w3.org/TR/2008/REC-xml-20081126/#id
    // https://stackoverflow.com/questions/1077084/what-characters-are-allowed-in-dom-ids#1077111
    if (input_id.empty())
        return "_";

    auto id = input_id;
    for (auto pos = id.find_first_not_of(valid_id_chars);
            pos != Glib::ustring::npos;
            pos = id.find_first_not_of(valid_id_chars, pos)) {
        id.replace(pos, 1, "_");
    }
    // ID cannot start with a digit, period, or minus (https://developer.mozilla.org/en-US/docs/Web/SVG/Attribute/id)
    auto c = id[0];
    if (isdigit(c) || c == '.' || c == '-') {
        id.insert(0, "x");
    }
    return id;
}

/**
 * Modify 'base_name' to create a new ID that is not used in the 'document'
*/
Glib::ustring generate_similar_unique_id(SPDocument* document, const Glib::ustring& base_name) {
    // replace illegal chars in base_name
    auto id = base_name;
    if (id.empty()) {
        id = "id-0";
    }
    else {
        id = sanitize_id(base_name);
    }

    if (!document) {
        g_warning("No document provided in %s, ID will not be unique.", __func__);
    }

    if (document && document->getObjectById(id.c_str())) {
        // conflict; check if id ends with "-<number>", so we can increase it;
        // only accept numbers with up to 9 digits and ignore other/larger digit strings
        Glib::RefPtr<Glib::Regex> regex = Glib::Regex::create("(.*)-(\\d{1,9})");
        Glib::MatchInfo info;
        regex->match(id, info);
        unsigned long counter = 0;
        auto base = id;
        if (info.matches()) {
            base = info.fetch(1);
            counter = std::stoul(info.fetch(2));
        }
        base += '-';
        for (;;) {
            counter++;
            id = base + std::to_string(counter);
            if (document->getObjectById(id.c_str()) == nullptr) break;
        }
    }

    return id;
}

/*
 * Change the id of a SPObject to new_name
 * If there is an id clash then rename to something similar
 */
void rename_id(SPObject *elem, Glib::ustring const &new_name)
{
    if (new_name.empty()){
        g_message("Invalid Id, will not change.");
        return;
    }
    gchar *id = g_strdup(new_name.c_str()); //id is not empty here as new_name is check to be not empty
    g_strcanon (id, valid_id_chars, '_');
    Glib::ustring new_name2 = id; //will not fail as id can not be NULL, see length check on new_name
    if (!isalnum (new_name2[0])) {
        g_message("Invalid Id, will not change.");
        g_free (id);
        return;
    }

    SPDocument *current_doc = elem->document;
    refmap_type refmap;
    find_references(current_doc, refmap);

    std::string old_id(elem->getId());
    if (current_doc->getObjectById(id)) {
        // Choose a new ID.
        // To try to preserve any meaningfulness that the original ID
        // may have had, the new ID is the old ID followed by a hyphen
        // and one or more digits.
        new_name2 += '-';
        for (;;) {
            new_name2 += "0123456789"[std::rand() % 10];
            if (current_doc->getObjectById(new_name2) == nullptr)
                break;
        }
    }
    g_free (id);
    // Change to the new ID
    elem->setAttribute("id", new_name2);
    // Make a note of this change, if we need to fix up refs to it
    id_changelist_type id_changes;
    if (refmap.find(old_id) != refmap.end()) {
        id_changes.emplace_back(elem, old_id);
    }

    fix_up_refs(refmap, id_changes);
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

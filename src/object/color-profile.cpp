// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * The color profile tag in an svg document
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "color-profile.h"

#include <giomm/error.h>

#include "attributes.h"
#include "colors/cms/profile.h"
#include "document.h"
#include "inkscape.h"
#include "sp-defs.h"
#include "uri.h"
#include "xml/document.h"
#include "xml/href-attribute-helper.h"

namespace Inkscape {

ColorProfile::~ColorProfile() = default;

void ColorProfile::release()
{
    // Unregister ourselves
    if (this->document) {
        this->document->removeResource("iccprofile", this);
    }

    SPObject::release();
}

/**
 * Callback: set attributes from associated repr.
 */
void ColorProfile::build(SPDocument *document, Inkscape::XML::Node *repr)
{
    SPObject::build(document, repr);

    this->readAttr(SPAttr::XLINK_HREF);
    this->readAttr(SPAttr::LOCAL);
    this->readAttr(SPAttr::NAME);
    this->readAttr(SPAttr::RENDERING_INTENT);

    // Register
    if (document) {
        document->addResource("iccprofile", this);
    }
}

/**
 * Callback: set attribute.
 */
void ColorProfile::set(SPAttr key, gchar const *value)
{
    switch (key) {
        case SPAttr::XLINK_HREF:
            // Href is the filename or the data of the icc profile itself and is used before local
            if (value) {
                auto base = document->getDocumentBase();
                _uri = std::make_unique<Inkscape::URI>(Inkscape::URI::from_href_and_basedir(value, base));
            } else {
                _uri.reset();
            }
            break;

        case SPAttr::LOCAL:
            // Local is the ID of the profile as a hex string. Provided by Colors::CMS::Profile::getId()
            // it's only used if the href isn't set or isn't found on this system in the specified place
            _local = value ? value : "";
            break;

        case SPAttr::NAME:
            // Name is used by the icc-color format to match this profile to a color. It over-rides the
            // name given in the icc profile if it's provided.
            _name = value ? value : "";
            break;

        case SPAttr::RENDERING_INTENT:
            // There is a standard set of rendering intents, the default fallback intent is decided in the
            // color CMS system and not here.
            _intent = Colors::RenderingIntent::UNKNOWN;
            if (value) {
                for (auto &pair : Colors::intentIds) {
                    if (pair.second == value) {
                        _intent = pair.first;
                        break;
                    }
                }
            }
            break;

        default:
            return SPObject::set(key, value);
    }
    this->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

/**
 * Callback: write attributes to associated repr.
 */
Inkscape::XML::Node *ColorProfile::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags)
{
    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:color-profile");
    }

    if ((flags & SP_OBJECT_WRITE_ALL) || _uri) {
        auto base = document->getDocumentBase();
        Inkscape::setHrefAttribute(
            *repr, _uri ? (_uri->str(base ? URI::from_dirname(base).str().c_str() : nullptr).c_str()) : nullptr);
    }

    repr->setAttributeOrRemoveIfEmpty("local", _local);
    repr->setAttributeOrRemoveIfEmpty("name", _name);
    repr->setAttributeOrRemoveIfEmpty("rendering-intent", Colors::intentIds[_intent]);

    SPObject::write(xml_doc, repr, flags);
    return repr;
}

/**
 * Return the profile data, if any. Returns empty string if none
 * is available.
 */
std::string ColorProfile::getProfileData() const
{
    // Note: The returned data could be Megabytes in length, but we're
    // copying the data. We should find a way to pass the const string back
    if (_uri) {
        try {
            return _uri->getContents();
        } catch (const Gio::Error &e) {
            g_warning("Couldn't get color profile: %s", e.what());
        }
    }
    return "";
}

/**
 * Set the rendering intent for this color profile.
 */
void ColorProfile::setRenderingIntent(Colors::RenderingIntent intent)
{
    setAttribute("rendering-intent", Colors::intentIds[intent]);
}

/**
 * Create a profile for the given profile in the given document.
 *
 * @args doc - The SPDocument to add this profile into, creating a new color profile element in it's defs.
 * @args profile - The color profile object to use as the data source
 * @args name - The name to use, this over-rides the name in the profile
 * @args storage - This sets the prefered data source.
 *   - HREF_DATA - The profile is embeded as a base64 encoded stream.
 *   - HREF_FILE - The href is a relative or absolute link to the icc profile file.
 *                 the profile MUST be a file. If the document has a file and the path is close
 *                 to the icc profile, it will be relative.
 *   - LOCAL_ID  - The profile's unique id will be stored, no href will be added.
 * @args intent  - Optional, The rendering intent to store in this profile.
 */
ColorProfile *ColorProfile::createFromProfile(SPDocument *doc, Colors::CMS::Profile const &profile,
                                              std::string const &name, ColorProfileStorage storage,
                                              std::optional<Colors::RenderingIntent> intent)
{
    if (name.empty()) {
        g_error("Refusing to create a color profile with an empty name!");
        return nullptr;
    }
    if (storage == ColorProfileStorage::HREF_FILE && profile.getPath().empty()) {
        storage = ColorProfileStorage::HREF_DATA; // fallback to data
    }

    // Create new object and attach it to the document
    auto repr = doc->getReprDoc()->createElement("svg:color-profile");

    // It's expected that the color manager will hace checked for collisions before this call.
    repr->setAttributeOrRemoveIfEmpty("name", name);

    switch (storage) {
        case ColorProfileStorage::LOCAL_ID:
            repr->setAttributeOrRemoveIfEmpty("local", profile.getId());
            break;
        case ColorProfileStorage::HREF_DATA:
            Inkscape::setHrefAttribute(*repr, "data:application/vnd.iccprofile;base64," + profile.dumpBase64());
            break;
        case ColorProfileStorage::HREF_FILE: {
            auto uri = Inkscape::URI::from_native_filename(profile.getPath().c_str());
            auto base = doc->getDocumentBase();
            Inkscape::setHrefAttribute(*repr, uri.str(base ? URI::from_dirname(base).str().c_str() : nullptr).c_str());
        } break;
    }
    if (intent) {
        repr->setAttributeOrRemoveIfEmpty("rendering-intent", Colors::intentIds[*intent]);
    }
    // Complete the creation by appending to the defs. This must be done last.
    return cast<ColorProfile>(doc->getDefs()->appendChildRepr(repr));
}

} // namespace Inkscape

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

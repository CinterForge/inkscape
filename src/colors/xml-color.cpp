// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023 author
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "xml-color.h"

#include "cms/profile.h"
#include "cms/system.h"
#include "colors/color.h"
#include "document-cms.h"
#include "document.h"
#include "manager.h"
#include "spaces/base.h"
#include "spaces/cms.h"
#include "spaces/components.h"
#include "xml/node-iterators.h"
#include "xml/node.h"
#include "xml/repr.h"
#include "xml/simple-document.h"

namespace Inkscape::Colors {

/**
 * Turn a color into a color xml document, used for drag and drop.
 *
 * @arg paint - The color or none to convert into xml
 */
std::string paint_to_xml_string(Paint const &paint)
{
    auto doc = paint_to_xml(paint);
    auto ret = sp_repr_save_buf(doc);
    GC::release(doc);
    return ret;
}

/**
 * Parse an xml document into a color. Usually from a drag and drop.
 *
 * @arg xmls - A string of a color xml document
 * @arg doc - An optional document to match icc profiles
 */
Paint xml_string_to_paint(std::string const &xmls, SPDocument *doc)
{
    auto color_doc = sp_repr_read_buf(xmls, nullptr);
    auto ret = xml_to_paint(color_doc, doc);
    GC::release(color_doc);
    return ret;
}

XML::Document *paint_to_xml(Paint const &paint)
{
    auto *document = new XML::SimpleDocument();
    auto root = document->createElement("paint");
    document->appendChild(root);

    if (std::holds_alternative<NoColor>(paint)) {
        auto node = document->createElement("nocolor");
        root->appendChild(node);
        GC::release(node);
        GC::release(root);
        return document;
    }

    auto &color = std::get<Color>(paint);
    auto space = color.getSpace();

    // This format is entirely inkscape's creation and doesn't work with anything
    // outside of inkscape. It's completely safe to change at any time since the
    // data is never saved to a file.
    auto node = document->createElement("color");
    node->setAttribute("space", space->getName());
    node->setAttributeOrRemoveIfEmpty("name", color.getName());
    root->appendChild(node);

    if (auto cms = std::dynamic_pointer_cast<Space::CMS>(space)) {
        if (auto profile = cms->getProfile()) {
            // Store the unique icc profile id, so we have a chance of matching it
            node->setAttribute("icc", profile->getId());
        }
    }

    if (color.hasOpacity()) {
        node->setAttributeSvgDouble("opacity", color.getOpacity());
    }

    auto components = space->getComponents();
    for (unsigned int i = 0; i < components.size() && i < color.size(); i++) {
        node->setAttributeCssDouble(components[i].id, color[i]);
    }

    GC::release(node);
    GC::release(root);
    return document;
}

Paint xml_to_paint(XML::Document const *xml, SPDocument *doc)
{
    auto get_node = [](XML::Node const *node, std::string const &name) {
        XML::NodeConstSiblingIterator iter{node->firstChild()};
        for (; iter; ++iter) {
            if (iter->name() && name == iter->name()) {
                return &*iter;
            }
        }
        return (const Inkscape::XML::Node *)(nullptr);
    };

    if (auto const paint = get_node(xml, "paint")) {
        if (get_node(paint, "nocolor")) {
            return NoColor();
        }
        if (auto color_xml = get_node(paint, "color")) {
            auto space_name = color_xml->attribute("space");

            if (!space_name) {
                throw ColorError("Invalid color data, no space specified.");
            }

            auto space = Manager::get().find(space_name);

            if (!space && doc)
                space = doc->getDocumentCMS().getSpace(space_name);

            if (auto icc_id = color_xml->attribute("icc")) {
                // Make a temporary space for the icc information, if possible
                if (!space)
                    if (auto profile = CMS::System::get().getProfile(icc_id)) {
                        auto cms = std::make_shared<Space::CMS>(profile);
                        cms->setIntent(RenderingIntent::AUTO);
                        space = cms;
                    }

                if (auto cms = std::dynamic_pointer_cast<Space::CMS>(space)) {
                    // Check named space has a cms profile that is actually the same Id
                    if (cms->getProfile()->getId() != icc_id) {
                        g_warning("Mismatched icc profiles in color data: '%s'", space_name);
                        // Not returning, will still return something
                    }
                }
            }
            if (!space) {
                throw ColorError("No color space available.");
            }

            XML::NodeConstSiblingIterator color_iter{color_xml->firstChild()};
            std::vector<double> values;
            for (auto &comp : space->getComponents()) {
                values.emplace_back(color_xml->getAttributeDouble(comp.id));
            }
            auto color = Color(space, values);

            if (color_xml->attribute("opacity")) {
                color.setOpacity(color_xml->getAttributeDouble("opacity"));
            }
            if (auto name = color_xml->attribute("name")) {
                color.setName(name);
            }
            return color;
        }
    }
    throw ColorError("No color data found");
}

} // namespace Inkscape::Colors

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

// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @gradient stop class.
 */
/* Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 1999,2005 authors
 * Copyright (C) 2010 Jon A. Cruz
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-stop.h"
#include "style.h"

#include "attributes.h"
#include "streq.h"
#include "svg/svg.h"
#include "svg/css-ostringstream.h"

SPStop::SPStop() : SPObject() {
    this->offset = 0.0;
}

SPStop::~SPStop() = default;

void SPStop::build(SPDocument* doc, Inkscape::XML::Node* repr) {
    this->readAttr(SPAttr::STYLE);
    this->readAttr(SPAttr::OFFSET);
    this->readAttr(SPAttr::STOP_PATH); // For mesh

    SPObject::build(doc, repr);
}

/**
 * Virtual build: set stop attributes from its associated XML node.
 */

void SPStop::set(SPAttr key, const gchar* value) {
    switch (key) {
        case SPAttr::OFFSET: {
            this->offset = sp_svg_read_percentage(value, 0.0);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
            break;
        }
        case SPAttr::STOP_PATH: {
            if (value) {
                this->path_string = Glib::ustring(value);
                this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        default: {
            if (SP_ATTRIBUTE_IS_CSS(key)) {
                this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
            } else {
                SPObject::set(key, value);
            }
            // This makes sure that the parent sp-gradient is updated.
            this->requestModified(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
        }
    }
}

void SPStop::modified(guint flags)
{
    if (parent && !(flags & SP_OBJECT_PARENT_MODIFIED_FLAG)) {
        parent->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
    }
}

/**
 * Virtual set: set attribute to value.
 */

Inkscape::XML::Node* SPStop::write(Inkscape::XML::Document* xml_doc, Inkscape::XML::Node* repr, guint flags) {
    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:stop");
    }

    SPObject::write(xml_doc, repr, flags);
    repr->setAttributeCssDouble("offset", this->offset);
    /* strictly speaking, offset an SVG <number> rather than a CSS one, but exponents make no sense
     * for offset proportions. */

    return repr;
}

/**
 * Virtual write: write object attributes to repr.
 */

// A stop might have some non-stop siblings
SPStop* SPStop::getNextStop() {
    SPStop *result = nullptr;

    for (SPObject* obj = getNext(); obj && !result; obj = obj->getNext()) {
        if (is<SPStop>(obj)) {
            result = cast<SPStop>(obj);
        }
    }

    return result;
}

SPStop* SPStop::getPrevStop() {
    SPStop *result = nullptr;

    for (SPObject* obj = getPrev(); obj; obj = obj->getPrev()) {
        // The closest previous SPObject that is an SPStop *should* be ourself.
        if (is<SPStop>(obj)) {
            auto stop = cast<SPStop>(obj);
            // Sanity check to ensure we have a proper sibling structure.
            if (stop->getNextStop() == this) {
                result = stop;
            } else {
                g_warning("SPStop previous/next relationship broken");
            }
            break;
        }
    }

    return result;
}

Inkscape::Colors::Color SPStop::getColor() const
{
    // Copy color from the right place
    Inkscape::Colors::Color color = style->stop_color.currentcolor ? style->color.getColor() : style->stop_color.getColor();
    // Bundle stop opacity into the color
    color.addOpacity(style->stop_opacity.as_double());
    return color;
}

/**
 * Sets the stop color and stop opacity in the style attribute.
 */
void SPStop::setColor(Inkscape::Colors::Color const &color)
{
    setColorRepr(getRepr(), color);
}

/**
 * Set the color and opacity directly into the given xml repr.
 */
void SPStop::setColorRepr(Inkscape::XML::Node *node, Inkscape::Colors::Color const &color)
{
    Inkscape::CSSOStringStream os;
    os << "stop-color:" << color.toString(false) << ";stop-opacity:" << color.getOpacity() <<";";
    node->setAttribute("style", os.str());
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

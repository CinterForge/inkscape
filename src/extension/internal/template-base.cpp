// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Contain internal sizes of paper which can be used in various
 * functions to make and size pages.
 *
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2022 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "clear-n_.h"
#include "document.h"
#include "extension/prefdialog/parameter.h"
#include "extension/template.h"
#include "page-manager.h"
#include "template-base.h"
#include "object/sp-page.h"

namespace Inkscape::Extension::Internal {

/**
 * Return the width and height of the new page, the default is a fixed orientation.
 */
Geom::Point TemplateBase::get_template_size(Inkscape::Extension::Template *tmod) const
{
    try {
        return Geom::Point(tmod->get_param_float("width"), tmod->get_param_float("height"));
    } catch (InxParameter::param_not_float_param) {
        g_warning("Template type should provide height and width params!");
    }
    return Geom::Point(100, 100);
}

/**
 * Return the template size in the required unit.
 */
Geom::Point TemplateBase::get_template_size(Inkscape::Extension::Template *tmod, const Util::Unit *unit) const
{
    auto size = get_template_size(tmod);
    auto t_unit = this->get_template_unit(tmod);
    auto width = Util::Quantity((double)size.x(), t_unit).value(unit);
    auto height = Util::Quantity((double)size.y(), t_unit).value(unit);
    return Geom::Point(width, height);
}

/**
 * Return the unit the size is given in.
 */
const Util::Unit *TemplateBase::get_template_unit(Inkscape::Extension::Template *tmod) const
{
    auto const &unit_table = Util::UnitTable::get();
    try {
        return unit_table.getUnit(tmod->get_param_optiongroup("unit", "cm"));
    } catch (InxParameter::param_not_optiongroup_param) {
        return unit_table.getUnit(tmod->get_param_string("unit", "cm"));
    }
}

std::unique_ptr<SPDocument> TemplateBase::new_from_template(Inkscape::Extension::Template *tmod)
{
    auto unit = this->get_template_unit(tmod);
    auto size = this->get_template_size(tmod);
    auto width = Util::Quantity((double)size.x(), unit);
    auto height = Util::Quantity((double)size.y(), unit);

    // If it was a template file, modify the document according to user's input.
    auto doc = tmod->get_template_document();
    auto nv = doc->getNamedView();

    // Set the width, height and default display units for the selected template
    doc->setWidthAndHeight(width, height, true);
    nv->setAttribute("inkscape:document-units", unit->abbr);
    doc->setDocumentScale(1.0);
    return doc;
}

void TemplateBase::resize_to_template(Inkscape::Extension::Template *tmod, SPDocument *doc, SPPage *page)
{
    static auto px = Util::UnitTable::get().getUnit("px");
    auto size = this->get_template_size(tmod, px);
    doc->getPageManager().resizePage(page, size.x(), size.y());
}

bool TemplateBase::match_template_size(Inkscape::Extension::Template *tmod, double width, double height)
{
    static auto px = Util::UnitTable::get().getUnit("px");
    auto temp_size = get_template_size(tmod, px);
    auto page_size = Geom::Point(width, height);
    auto rota_size = Geom::Point(height, width);
    // We want a half a pixel tollerance to catch floating point errors
    // We also check the rotated size, as this is a valid match (for now)
    return Geom::are_near(temp_size, page_size, 0.5) || Geom::are_near(temp_size, rota_size, 0.5);
}

} // namespace Inkscape::Extension::Internal

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

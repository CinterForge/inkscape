// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * A base template generator used by internal template types.
 *
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2022 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef EXTENSION_INTERNAL_TEMPLATE_BASE_H
#define EXTENSION_INTERNAL_TEMPLATE_BASE_H

#include <2geom/point.h>
#include "extension/implementation/implementation.h"

namespace Inkscape::Util { class Unit; }

namespace Inkscape::Extension::Internal {

class TemplateBase : public Inkscape::Extension::Implementation::Implementation
{
public:
    bool check(Inkscape::Extension::Extension *module) override { return true; }

    std::unique_ptr<SPDocument> new_from_template(Inkscape::Extension::Template *tmod) override;
    void resize_to_template(Inkscape::Extension::Template *tmod, SPDocument *doc, SPPage *page) override;
    bool match_template_size(Inkscape::Extension::Template *tmod, double width, double height) override;

protected:
    virtual Geom::Point get_template_size(Inkscape::Extension::Template *tmod) const;
    virtual Geom::Point get_template_size(Inkscape::Extension::Template *tmod, const Util::Unit *unit) const;
    virtual const Util::Unit *get_template_unit(Inkscape::Extension::Template *tmod) const;
};

} // namespace Inkscape::Extension::Internal

#endif /* EXTENSION_INTERNAL_TEMPLATE_BASE_H */

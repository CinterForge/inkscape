// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Various other pixel based templates.
 *
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2021 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef EXTENSION_INTERNAL_TEMPLATE_OTHER_H
#define EXTENSION_INTERNAL_TEMPLATE_OTHER_H

#include "extension/internal/template-base.h"

namespace Inkscape::Extension::Internal {

class TemplateOther : public TemplateBase
{
public:
    static void init();

protected:
    Geom::Point get_template_size(Inkscape::Extension::Template *tmod) const override;
};

} // namespace Inkscape::Extension::Internal {

#endif /* EXTENSION_INTERNAL_TEMPLATE_OTHER_H */

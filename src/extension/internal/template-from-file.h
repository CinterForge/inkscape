// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Collect templates as svg documents and express them as usable
 * templates to the user with an icon.
 *
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2022 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef EXTENSION_INTERNAL_TEMPLATE_FROM_FILE_H
#define EXTENSION_INTERNAL_TEMPLATE_FROM_FILE_H

#include <string>                                     // for string
#include "extension/implementation/implementation.h"  // for Implementation
#include "extension/template.h"                       // for Template (ptr o...

class SPDocument;
namespace Inkscape::XML { class Node; }

namespace Inkscape::Extension::Internal {

class TemplatePresetFile : public TemplatePreset
{
public:
    TemplatePresetFile(Template *mod, std::string const &filename);

private:
    void _load_data(const Inkscape::XML::Node *root);
};

class TemplateFromFile : public Inkscape::Extension::Implementation::Implementation
{
public:
    static void init();
    bool check(Inkscape::Extension::Extension *module) override { return true; };
    std::unique_ptr<SPDocument> new_from_template(Inkscape::Extension::Template *tmod) override;

    void get_template_presets(const Template *tmod, TemplatePresets &presets) const override;
};

} // namespace Inkscape::Extension::Internal

#endif /* EXTENSION_INTERNAL_TEMPLATE_FROM_FILE_H */

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

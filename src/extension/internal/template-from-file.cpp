// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2022 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "template-from-file.h"

#include <cstring>                                  // for strcmp
#include <algorithm>                                // for replace
#include <memory>                                   // for shared_ptr, alloc...
#include <vector>                                   // for vector

#include <glib.h>                                   // for GFileTest, G_FILE...
#include <glibmm/miscutils.h>                       // for path_get_basename
#include <glibmm/ustring.h>                         // for ustring

#include "document.h"
#include "extension/extension.h"                    // for INKSCAPE_EXTENSIO...
#include "extension/internal/clear-n_.h"            // for N_, NC_
#include "extension/internal/template-from-file.h"  // for TemplatePresetFile
#include "extension/system.h"                       // for build_from_mem
#include "io/file.h"                                // for ink_file_new
#include "io/resource.h"                            // for get_filenames
#include "io/sys.h"                                 // for file_test
#include "xml/document.h"                           // for Document
#include "xml/node.h"                               // for Node
#include "xml/repr.h"                               // for sp_repr_lookup_co...

using namespace Inkscape::IO::Resource;

namespace Inkscape::Extension::Internal {

/**
 * A file based template preset.
 */
TemplatePresetFile::TemplatePresetFile(Template *mod, const std::string &filename)
    : TemplatePreset(mod, nullptr)
{
    _visibility = TEMPLATE_NEW_ICON; // No searching

    // TODO: Add cache here.
    _prefs["filename"] = filename;
    _name = Glib::path_get_basename(filename);
    std::replace(_name.begin(), _name.end(), '_', '-');
    _name.replace(_name.rfind(".svg"), 4, 1, ' ');
    
    Inkscape::XML::Document *rdoc = sp_repr_read_file(filename.c_str(), SP_SVG_NS_URI);
    if (rdoc){
        Inkscape::XML::Node *root = rdoc->root();
        if (!strcmp(root->name(), "svg:svg")) {
            Inkscape::XML::Node *templateinfo = sp_repr_lookup_name(root, "inkscape:templateinfo");
            if (!templateinfo) {
                templateinfo = sp_repr_lookup_name(root, "inkscape:_templateinfo"); // backwards-compatibility
            }
            if (templateinfo) {
                _load_data(templateinfo);
            }
        }
    }

    // Key is just the whole filename, it's unique enough.
    _key = filename;
    std::replace(_key.begin(), _key.end(), '/', '.');
    std::replace(_key.begin(), _key.end(), '\\', '.');
}

void TemplatePresetFile::_load_data(const Inkscape::XML::Node *root)
{
    _name = sp_repr_lookup_content(root, "inkscape:name", _name);
    _name = sp_repr_lookup_content(root, "inkscape:_name", _name); // backwards-compatibility
    _desc = sp_repr_lookup_content(root, "inkscape:shortdesc", _key);
    _desc = sp_repr_lookup_content(root, "inkscape:_shortdesc", _desc); // backwards-compatibility

    _label = sp_repr_lookup_content(root, "inkscape:label", N_("Custom Template"));
    _icon = sp_repr_lookup_content(root, "inkscape:icon", _icon);
    _category = sp_repr_lookup_content(root, "inkscape:category", _category);

    try {
        _priority = std::stoi(sp_repr_lookup_content(root, "inkscape:priority", "-1"));
    } catch(std::exception &err) {
        g_warning("Template priority malformed number.");
        _priority = -1;
    }

    // Original functionality not yet used...
    // _author = sp_repr_lookup_content(root, "inkscape:author");
    // _preview = sp_repr_lookup_content(root, "inkscape:preview");
    // _date = sp_repr_lookup_name(root, "inkscape:date");
    // _keywords = sp_repr_lookup_name(root, "inkscape:_keywords");
}


std::unique_ptr<SPDocument> TemplateFromFile::new_from_template(Inkscape::Extension::Template *tmod)
{
    auto filename = tmod->get_param_string("filename", "");
    if (Inkscape::IO::file_test(filename, G_FILE_TEST_EXISTS)) {
        return ink_file_new(filename);
    }
    // Default template
    g_error("Couldn't load filename I expected to exist.");
    return tmod->get_template_document();
}

void TemplateFromFile::init()
{
    // clang-format off
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">"
            "<id>org.inkscape.template.from-file</id>"
            "<name>" N_("Load from User File") "</name>"
            "<description>" N_("Custom list of templates for a folder") "</description>"
            "<category>" NC_("TemplateCategory", "Custom") "</category>"

            "<param name='filename' gui-text='" N_("Filename") "' type='string'></param>"
            "<template icon='custom' priority='-1' visibility='both'>"
              // Auto & lazy generated content (see function)
            "</template>"
        "</inkscape-extension>",
        std::make_unique<TemplateFromFile>());
}

/**
 * Generate a list of available files as selectable presets.
 */
void TemplateFromFile::get_template_presets(const Template *tmod, TemplatePresets &presets) const
{
    for(auto &filename: get_filenames(TEMPLATES, {".svg"}, {"default"})) {
        if (filename.find("icons") != Glib::ustring::npos) continue;
        presets.emplace_back(new TemplatePresetFile(const_cast<Template *>(tmod), filename));
    }
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

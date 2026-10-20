// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2022 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_EXTENSION_TEMPLATE_H__
#define INKSCAPE_EXTENSION_TEMPLATE_H__

#include <exception>
#include <map>
#include <string>
#include <giomm/file.h>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>

#include "extension/db.h"
#include "extension/extension.h"
#include "util/units.h"

class SPDocument;
class SPPage;

namespace Inkscape {
namespace Extension {

using TemplateShow = int;
enum TemplateVisibility : TemplateShow {
    TEMPLATE_ANY = -1, // Any visibility
    TEMPLATE_HIDDEN = 0,
    TEMPLATE_NEW_FROM = 1,
    TEMPLATE_NEW_WELCOME = 2,
    TEMPLATE_NEW_ICON = 3,
    TEMPLATE_SIZE_LIST = 4,
    TEMPLATE_SIZE_SEARCH = 8,
    TEMPLATE_ALL = 255 // Set as visible everywhere
};

class Template;
class TemplatePreset;
typedef std::map<std::string, std::string> TemplatePrefs;
typedef std::vector<std::shared_ptr<TemplatePreset>> TemplatePresets;

class TemplatePreset
{
public:
    TemplatePreset(Template *mod, const Inkscape::XML::Node *repr, TemplatePrefs prefs = {}, int priority = 0);
    ~TemplatePreset() {};

    std::string const &get_key() const { return _key; }
    std::string const &get_icon() const { return _icon; }
    std::string const &get_category() const { return _category; }
    std::string const &get_name() const { return _name; }
    std::string const &get_label() const { return _label; }
    std::string const &get_description() const { return _desc; }
    int get_sort_priority() const { return _priority; }
    int get_visibility() const { return _visibility; }

    bool is_visible(TemplateShow mode) {
        // Not hidden and contains the requested mode.
        return _visibility && (mode == TEMPLATE_ANY
            || ((_visibility & (int)mode) == (int)mode));
    }

    std::unique_ptr<SPDocument> new_from_template(TemplatePrefs const &others = {});
    void resize_to_template(SPDocument *doc, SPPage *page, const TemplatePrefs &others = {});
    bool match_size(double width, double height, const TemplatePrefs &others = {});

    std::string get_icon_path() const;

private:
    Template *_mod;

protected:
    std::string _key;
    std::string _icon;
    std::string _name;
    std::string _label;
    std::string _desc;
    std::string _category;
    int _priority;
    int _visibility;

    // This is a set of preferences given to the extension
    TemplatePrefs _prefs;

    std::string _get_icon_path(std::string const &name) const;
    bool setup_prefs(const TemplatePrefs &others = {});
    void _add_prefs(const TemplatePrefs &prefs);
};

class Template : public Extension
{
public:
    struct create_cancelled : public std::exception
    {
        ~create_cancelled() noexcept override = default;
        const char *what() const noexcept override { return "Create was cancelled"; }
    };

    Template(Inkscape::XML::Node *in_repr, ImplementationHolder implementation, std::string *base_directory);
    ~Template() override = default;

    bool check() override;

    std::unique_ptr<SPDocument> new_from_template();
    void resize_to_template(SPDocument *doc, SPPage *page);

    std::string get_icon() const { return _icon; }
    std::string get_description() const { return _desc; }
    std::string get_category() const { return _category; }

    bool can_resize() const { return _can_resize; }
    int get_visibility() const { return _visibility; }

    TemplatePresets get_presets(TemplateShow visibility = TEMPLATE_ANY) const;

    std::shared_ptr<TemplatePreset> get_preset(const std::string &key);
    std::shared_ptr<TemplatePreset> get_preset(double width, double height);
    static std::shared_ptr<TemplatePreset> get_any_preset(const std::string &key);
    static std::shared_ptr<TemplatePreset> get_any_preset(double width, double height);

    Glib::RefPtr<Gio::File> get_template_filename() const;
    std::unique_ptr<SPDocument> get_template_document() const;

protected:
    friend class TemplatePreset;

    static int parse_visibility(const std::string &value);
private:
    std::string _source;
    std::string _icon;
    std::string _desc;
    std::string _category;

    bool _can_resize = false; // Can this be used to resize existing pages?
    int _visibility = TEMPLATE_SIZE_SEARCH;

    TemplatePresets _presets;
};

} // namespace Extension
} // namespace Inkscape
#endif /* INKSCAPE_EXTENSION_TEMPLATE_H__ */

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

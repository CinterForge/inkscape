// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Ted Gould <ted@gould.cx>
 *
 * Copyright (C) 2002-2004 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#ifndef INKSCAPE_EXTENSION_EFFECT_H__
#define INKSCAPE_EXTENSION_EFFECT_H__

#include <glibmm/ustring.h>
#include <list>

#include <glibmm/i18n.h>
#include <string>
#include "extension.h"
#include "inkscape-application.h"

namespace Gtk {
	class Box;
}

class SPDocument;

namespace Inkscape {


namespace Extension {
class PrefDialog;

/** \brief  Effects are extensions that take a document and do something
            to it in place.  This class adds the extra functions required
            to make extensions effects.
*/
class Effect : public Extension {
    /** \brief  This is the last effect that was used.  This is used in
                a menu item to rapidly recall the same effect. */
    static Effect * _last_effect;

    Inkscape::XML::Node *find_menu (Inkscape::XML::Node * menustruct, const gchar *name);
    void get_menu(Inkscape::XML::Node * pattern, std::list<Glib::ustring>& sub_menu_list) const;

    /** \brief  Menu node created for this effect */
    Inkscape::XML::Node * _menu_node;

    /** \brief  The preference dialog if it is shown */
    PrefDialog * _prefDialog;

public:
    Effect(Inkscape::XML::Node *in_repr, ImplementationHolder implementation, std::string *base_directory, std::string* file_name);
    ~Effect() override;

    bool prefs(SPDesktop * desktop);
    void effect(SPDesktop *desktop, SPDocument *document = nullptr);

    /** \brief  Whether a working dialog should be shown */
    bool _workingDialog = true;

    /** \brief  If stderr log should be shown, when process return code is 0 */
    bool ignore_stderr = false;

    /** \brief  If changesets should be piped in via stdin */
    bool pipe_diffs = false;

    /** \brief  Static function to get the last effect used */
    static Effect *  get_last_effect () { return _last_effect; };
    static void      set_last_effect (Effect * in_effect);

    static void      place_menus ();
    void             place_menu (Inkscape::XML::Node * menus);

    bool no_doc; // if true, the effect does not process SVG document at all, so no need to save, read, and watch for errors
    bool no_live_preview; // if true, the effect does not need "live preview" checkbox in its dialog

    PrefDialog *get_pref_dialog ();
    void        set_pref_dialog (PrefDialog * prefdialog);

    void deactivate() override;

    // try to locate SVG thumbnail for this effect and return its path; empty string if not found;
    // use "default_dir" for effects created from memory, as they don't know their location
    std::string find_icon_file(const std::string& default_dir) const;

    // returns true if this extension presents input dialog before taking effect
    // or false for immediate action (no UI)
    bool takes_input() const;

    // returns true if this effect should not be presented in the UI main menu
    bool hidden_from_menu() const;

    // returns true if this extension is a filter effect
    bool is_filter_effect() const;

    // get effect's menu tip
    const Glib::ustring& get_menu_tip() const;

    // get effect's ID sanitized to alphanumeric ASCII charaters
    std::string get_sanitized_id() const;

    // get local effect menu
    std::list<Glib::ustring> get_menu_list() const;

    // apply filter effect to 'item'
    bool apply_filter(SPItem* item);

private:
    std::string _file_name; // extension file name, if provided
    bool _hidden_from_menu = false;
    bool _filter_effect = false;
    Glib::ustring _menu_tip;
    Inkscape::XML::Node* _local_effects_menu = nullptr;
    std::string _icon_path;

    static gchar *   remove_ (gchar * instr);
    static void _sanitizeId(std::string &id);
};

} }  /* namespace Inkscape, Extension */
#endif /* INKSCAPE_EXTENSION_EFFECT_H__ */

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

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Functions to keep a listing of all modules in the system.  Has its
 * own file mostly for abstraction reasons, but is pretty simple
 * otherwise.
 *
 * Authors:
 *   Ted Gould <ted@gould.cx>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2002-2004 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_MODULES_DB_H
#define SEEN_MODULES_DB_H

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

#include <glib.h>

#include "extension.h"

namespace Inkscape::Extension {

class Template; // New
class Input;    // Load
class Output;   // Save
class Effect;   // Modify

class DB {
private:
    /** This is the actual database.  It has all of the modules in it,
        indexed by their ids.  It's a hash table for faster lookups */
    std::unordered_map<std::string, std::unique_ptr<Extension>> moduledict;

public:
    DB() = default;
    DB(DB &&)            = delete; // Database is non-movable, hence also non-copyable.
    DB &operator=(DB &&) = delete;

    Extension *get(const gchar *key) const;
    void take_ownership(std::unique_ptr<Extension> module);
    void foreach(void (*in_func)(Extension * in_plug, gpointer in_data), gpointer in_data);

private:
    static void template_internal(Extension *in_plug, gpointer data);
    static void input_internal (Extension * in_plug, gpointer data);
    static void output_internal (Extension * in_plug, gpointer data);
    static void effect_internal (Extension * in_plug, gpointer data);

public:
    typedef std::list<Template *> TemplateList;
    typedef std::list<Output *> OutputList;
    typedef std::list<Input *> InputList;
    typedef std::list<Effect *> EffectList;

    TemplateList &get_template_list(TemplateList &ou_list);
    InputList  &get_input_list  (InputList &ou_list);
    OutputList &get_output_list (OutputList &ou_list);

    std::vector<Effect*> get_effect_list();
}; /* class DB */

extern DB db;

}  // namespace Inkscape::Extension

#endif // SEEN_MODULES_DB_H

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

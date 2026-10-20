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

#include "db.h"

#include "effect.h"
#include "implementation/script.h"
#include "input.h"
#include "output.h"
#include "template.h"

/* Globals */

/* Namespaces */

namespace Inkscape {
namespace Extension {

/** This is the actual database object. There is only one of these. */
DB db;

/* Types */
struct ModuleGenericCmp
{
    bool operator()(Extension *module1, Extension *module2) const
    {
        int n1 = module1->get_sort_priority();
        int n2 = module2->get_sort_priority();
        if (n1 != n2)
            return (n1 < n2);
        return (strcmp(module1->get_name(), module2->get_name()) <= 0);
    }
};

struct ModuleInputCmp {
    bool operator()(Input* module1, Input* module2) const {

        int n1 = module1->get_sort_priority();
        int n2 = module2->get_sort_priority();
        // Treat zero as not-defined for purpose of comparison.
        if (n1 || n2)
            return n1 && n2 ? (n1 < n2) : !n2;

        return (strcmp(module1->get_filetypename(), module2->get_filetypename()) <= 0);
    }
};


struct ModuleOutputCmp {
    bool operator()(Output* module1, Output* module2) const {

        int n1 = module1->get_sort_priority();
        int n2 = module2->get_sort_priority();
        // Treat zero as not-defined for purpose of comparison.
        if (n1 || n2)
            return n1 && n2 ? (n1 < n2) : !n2;

        // special case: two extensions for the same file type. I only one of them is a script, prefer the other one
        if (Glib::ustring(module1->get_extension()).lowercase() == Glib::ustring(module2->get_extension()).lowercase()) {
            bool module1_is_script = dynamic_cast<Inkscape::Extension::Implementation::Script *>(module1->get_imp());
            bool module2_is_script = dynamic_cast<Inkscape::Extension::Implementation::Script *>(module2->get_imp());
            if (module1_is_script != module2_is_script) {
                return module1_is_script ? false : true;
            }
        }
        return (strcmp(module1->get_filetypename(), module2->get_filetypename()) <= 0);
    }
};

/**
 * @brief Take the ownership of an extension to ensure that it is freed on program exit.
 * @param module An owning pointer to the extension.
 */
void DB::take_ownership(std::unique_ptr<Extension> module)
{
    if (module) {
        moduledict[module->get_id()] = std::move(module);
    }
}

/**
	\return    A reference to the Inkscape::Extension::Extension specified by the input key.
	\brief     This function looks up a Inkscape::Extension::Extension by using its unique
	           id.  It then returns a reference to that module.
	\param     key   The unique ID of the module

    Retrieves a module by name or nullptr if not found.
*/
Extension *DB::get(const gchar *key) const
{
    if (key == nullptr) return nullptr;

	auto it = moduledict.find(key);
	if (it == moduledict.end())
		return nullptr;

	Extension *mod = it->second.get();
	assert(mod);

    if (!mod || mod->deactivated())
		return nullptr;

	return mod;
}

/**
	\return    none
	\brief     A function to execute another function with every entry
	           in the database as a parameter.
	\param     in_func  The function to execute for every module
	\param     in_data  A data pointer that is also passed to in_func

 	Enumerates the modules currently in the database, calling a given
	callback for each one.
*/
void DB::foreach(void (*in_func)(Extension *, gpointer), gpointer in_data)
{
    for (auto const &item : moduledict) {
        in_func(item.second.get(), in_data);
    }
}

/**
 * @return none
 * @brief  The function to look at each module and see if it is
       a template module, then add it to the list.
 * @param in_plug - Module to be examined
 * @param data    - The list to be attached to
*/
void DB::template_internal(Extension *in_plug, gpointer data)
{
    if (auto tmod = dynamic_cast<Template *>(in_plug)) {
        auto tlist = reinterpret_cast<TemplateList *>(data);
        tlist->push_back(tmod);
    }
}

/**
	\return    none
	\brief     The function to look at each module and see if it is
	           an input module, then add it to the list.
	\param     in_plug  Module to be examined
	\param     data     The list to be attached to

	The first thing that is checked is if this module is an input
	module.  If it is, then it is added to the list which is passed
	in through \c data.
*/
void
DB::input_internal (Extension * in_plug, gpointer data)
{
    if (auto imod = dynamic_cast<Input *>(in_plug)) {
        auto ilist = reinterpret_cast<InputList *>(data);
        ilist->push_back(imod);
    }
}

/**
	\return    none
	\brief     The function to look at each module and see if it is
	           an output module, then add it to the list.
	\param     in_plug  Module to be examined
	\param     data     The list to be attached to

	The first thing that is checked is if this module is an output
	module.  If it is, then it is added to the list which is passed
	in through \c data.
*/
void
DB::output_internal (Extension * in_plug, gpointer data)
{
    if (auto *omod = dynamic_cast<Output *>(in_plug)) {
        auto olist = reinterpret_cast<OutputList *>(data);
        olist->push_back(omod);
		// printf("Added to output list: %s\n", omod->get_id());
    }
}

/**
	\return    none
	\brief     The function to look at each module and see if it is
	           an effect module, then add it to the list.
	\param     in_plug  Module to be examined
	\param     data     The list to be attached to

	The first thing that is checked is if this module is an effect
	module.  If it is, then it is added to the list which is passed
	in through \c data.
*/
void
DB::effect_internal (Extension * in_plug, gpointer data)
{
    if (Effect *emod = dynamic_cast<Effect *>(in_plug)) {
        auto elist = reinterpret_cast<EffectList *>(data);
        elist->push_back(emod);
		// printf("Added to effect list: %s\n", emod->get_id());
    }
}

/**
 * Create a list of all the Template extensions
 * @param ou_list - The list that is used to put all the extensions in
 *
 * Calls the database \c foreach function with \c template_internal.
 */
DB::TemplateList &DB::get_template_list(DB::TemplateList &ou_list)
{
    foreach (template_internal, (gpointer)&ou_list);
    ou_list.sort(ModuleGenericCmp());
    return ou_list;
}

/**
	\brief  Creates a list of all the Input extensions
	\param  ou_list  The list that is used to put all the extensions in

	Calls the database \c foreach function with \c input_internal.
*/
DB::InputList &
DB::get_input_list (DB::InputList &ou_list)
{
	foreach(input_internal, (gpointer)&ou_list);
	ou_list.sort( ModuleInputCmp() );
	return ou_list;
}

/**
	\brief  Creates a list of all the Output extensions
	\param  ou_list  The list that is used to put all the extensions in

	Calls the database \c foreach function with \c output_internal.
*/
DB::OutputList &
DB::get_output_list (DB::OutputList &ou_list)
{
	foreach(output_internal, (gpointer)&ou_list);
	ou_list.sort( ModuleOutputCmp() );
	return ou_list;
}

/**
	\brief  Creates a list of all the Effect extensions
*/
std::vector<Effect*> DB::get_effect_list() {
    std::vector<Effect*> out;
    for (auto const &item : moduledict) {
        auto ex = item.second.get();
        if (auto effect = dynamic_cast<Effect*>(ex)) {
            out.push_back(effect);
        }
    }
    return out;
}

} } /* namespace Extension, Inkscape */

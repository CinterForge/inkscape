// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_EXTENSION_INTERNAL_FILTER_FILTER_H
#define INKSCAPE_EXTENSION_INTERNAL_FILTER_FILTER_H

/*
 * Copyright (C) 2008 Authors:
 *   Ted Gould <ted@gould.cx>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/i18n.h>

#include "extension/implementation/implementation.h"

namespace Inkscape {

namespace XML {
	struct Document;
}

namespace Extension {

class Effect;
class Extension;
class ExecutionEnv;

namespace Internal {
namespace Filter {

class Filter : public Inkscape::Extension::Implementation::Implementation {
protected:
    gchar const * _filter; // TODO: fix leaking filter strings.
	virtual gchar const * get_filter_text (Inkscape::Extension::Extension * ext);

private:
	Inkscape::XML::Document * get_filter (Inkscape::Extension::Extension * ext);
    bool apply_filter(Inkscape::Extension::Effect* module, SPItem* item) override;

public:
	Filter();
	Filter(gchar const * filter);
	~Filter() override;

	bool load(Inkscape::Extension::Extension *module) override;
	Inkscape::Extension::Implementation::ImplementationDocumentCache * newDocCache (Inkscape::Extension::Extension * ext, SPDesktop * desktop) override;
    void effect(Inkscape::Extension::Effect *module, ExecutionEnv *executionEnv, SPDesktop *desktop,
                Inkscape::Extension::Implementation::ImplementationDocumentCache *docCache) override;

    static void filter_init(gchar const * id, gchar const * name, gchar const * submenu, gchar const * tip, gchar const * filter);
	static void filters_all();

	/* File loader related */
	static void filters_all_files();
	static void filters_load_node(Inkscape::XML::Node *node, gchar * menuname);

};

}; /* namespace Filter */
}; /* namespace Internal */
}; /* namespace Extension */
}; /* namespace Inkscape */

#endif // INKSCAPE_EXTENSION_INTERNAL_FILTER_FILTER_H

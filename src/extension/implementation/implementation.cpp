// SPDX-License-Identifier: GPL-2.0-or-later
/*
    Author:  Ted Gould <ted@gould.cx>
    Copyright (c) 2003-2005,2007

    Released under GNU GPL v2+, read the file 'COPYING' for more information.

    This file is the backend to the extensions system.  These are
    the parts of the system that most users will never see, but are
    important for implementing the extensions themselves.  This file
    contains the base class for all of that.
*/

#include "implementation.h"

#include <extension/output.h>
#include <extension/input.h>
#include <extension/effect.h>

#include "selection.h"
#include "desktop.h"
#include "document.h"

namespace Inkscape {
namespace Extension {
namespace Implementation {

std::unique_ptr<SPDocument> Implementation::new_from_template(Template *)
{
    return {};
}

std::unique_ptr<SPDocument> Implementation::open(Input *module, char const *filename, bool is_importing)
{
    return {};
}

Gtk::Widget *Implementation::prefs_effect(Inkscape::Extension::Effect *module, SPDesktop *desktop, sigc::signal<void ()> * changeSignal, ImplementationDocumentCache * /*docCache*/)
{
    if (module->widget_visible_count() == 0) {
        return nullptr;
    }

    SPDocument * current_document = desktop->doc();

    auto selected = desktop->getSelection()->items();
    Inkscape::XML::Node const* first_select = nullptr;
    if (!selected.empty()) {
        const SPItem * item = selected.front();
        first_select = item->getRepr();
    }

    // TODO deal with this broken const correctness:
    return module->autogui(current_document, const_cast<Inkscape::XML::Node *>(first_select), changeSignal);

} // Implementation::prefs_effect

void Implementation::effect(Inkscape::Extension::Effect *mod, ExecutionEnv *executionEnv, SPDesktop *desktop,
                            ImplementationDocumentCache * /*docCache*/)
{
    effect(mod, executionEnv, desktop->getDocument());
}

}  /* namespace Implementation */
}  /* namespace Extension */
}  /* namespace Inkscape */

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

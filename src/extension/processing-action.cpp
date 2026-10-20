// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Martin Owens
 *
 * Copyright (C) 2023 Edgewood
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "processing-action.h"

#include <glibmm/i18n.h>

#include "document.h"
#include "inkscape-application.h"
#include "preferences.h"

#include "xml/attribute-record.h"
#include "xml/node.h"

namespace Inkscape::Extension {

ProcessingAction::ProcessingAction (Inkscape::XML::Node * in_repr)
{
    if (auto action = in_repr->firstChild()->content()) {
        _action_name = action;
    }
    for (const auto &iter : in_repr->attributeList()) {
        std::string name = g_quark_to_string(iter.key);
        std::string value = std::string(iter.value);
        if (name == "pref" && !value.empty()) {
            if (value.at(0) == '!') {
                _pref_default = false;
                value.erase(value.begin());
            }
            _pref = value;
        }
    }

    return;
}

/**
 * Check if the action should be run or not (prefs etc)
 */
bool ProcessingAction::is_enabled()
{
    // System preferences
    if (!_pref.empty()) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        return prefs->getBool(_pref, _pref_default);
    }
    return true;
}

/**
 * Run the action against the given document.
 */
void ProcessingAction::run(SPDocument *doc)
{
    if (auto action = doc->getActionGroup()->lookup_action(_action_name)) {
        if (action->get_enabled()) {
            // Doc is already bound into this action so does't need to be passed in
            action->activate();
        }
    } else if (auto action = InkscapeApplication::instance()->gio_app()->lookup_action(_action_name)) {
        if (action->get_enabled()) {
            action->activate();
        }
    } else {
        g_warning("Can't find action 'doc.%s' or 'app.%s'", _action_name.c_str(), _action_name.c_str());
    }
}

} /* namespace Inkscape::Extension */

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

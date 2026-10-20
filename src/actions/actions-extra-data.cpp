// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Extra data associated with actions: Label, Section, Tooltip.
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Extra data is indexed by "detailed action names", that is an action
 * with prefix and value (if statefull). For example:
 *   "win.canvas-display-mode(1)"
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include "actions-extra-data.h"

#include <giomm/action.h>
#include <glibmm/i18n.h>
#include <glibmm/regex.h>

#include "document.h"
#include "inkscape-application.h"
#include "inkscape-window.h"

std::vector<Glib::ustring> InkActionExtraData::get_actions()
{
    std::vector<Glib::ustring> action_names;
    for (auto const &datum : data) {
        action_names.emplace_back(datum.first);
    }
    return action_names;
}

Glib::ustring
InkActionExtraData::get_label_for_action(Glib::ustring const &action_name,
                                         bool const translated) const
{
    Glib::ustring value;
    auto const search = data.find(action_name.raw());
    if (search != data.end()) {
        value = translated ? _(search->second.label.c_str())
                           :   search->second.label;
    }
    return value;
}

// TODO: Section should be translatable, too
Glib::ustring
InkActionExtraData::get_section_for_action(Glib::ustring const &action_name) const
{
    Glib::ustring value;
    auto const search = data.find(action_name.raw());
    if (search != data.end()) {
        value = search->second.section;
    }
    return value;
}

Glib::ustring InkActionExtraData::get_tooltip_for_action(Glib::ustring const &action_name,
                                                         bool const translated           ,
                                                         bool const expanded             ) const
{
    Glib::ustring value;
    auto const search = data.find(action_name.raw());
    if (search != data.end()) {
        if (expanded && strncmp(action_name.c_str(), "win:tool-switch('", 17)) {
            value = translated ? ("<b>" + Glib::ustring(_(search->second.label.c_str())) + "</b>\n" +
                                  Glib::ustring(_(search->second.tooltip.c_str())))
                               : (search->second.label + "\n" + search->second.tooltip);
        } else {
            value = translated ? _(search->second.tooltip.c_str()) : search->second.tooltip;
        }
    }
    return value;
}

void InkActionExtraData::add_data(std::vector<std::vector<Glib::ustring>> const &raw_data)
{
    data.reserve(data.size() + raw_data.size());
    for (auto const &raw : raw_data) {
        data.emplace(raw[0], InkActionExtraDatum{ raw[1], raw[2], raw[3] });
    }
}

/**
 * Return true if the action/shortcut context is the same.
 *
 * @arg action_one - The action name for the first action to compare.
 * @arg action_two - The action name for the second action to compare.
 *
 * @returns Almost always true, except for tool shortcuts which have their own contexts. Also returns true
 *          if either of the action names are empty.
 */
bool InkActionExtraData::isSameContext(Glib::ustring const &action_one, Glib::ustring const &action_two) const
{
    if (action_one.empty() || action_two.empty())
        return true;

    std::vector<Glib::ustring> ones = Glib::Regex::split_simple("." , action_one);
    std::vector<Glib::ustring> twos = Glib::Regex::split_simple("." , action_two);

    // Only tool shortcuts have a context at the moment
    if (ones.size() < 2 || ones[0] != "tool" || twos.size() < 2 || twos[0] != "tool") {
        return true;
    }
    // The same tool means the same context, or if the tool is all tools (i.e. tool-base)
    return ones[1] == twos[1] || ones[1] == "all" || twos[1] == "all";
}

bool action_requires_parameter(Glib::ustring const &action_name)
{
    auto const dot = action_name.find('.');
    auto const action_domain = std::string_view{action_name.c_str(), dot};
    auto const name_without_domain = action_name.substr(dot + 1);

    auto iapp = InkscapeApplication::instance();

    Glib::RefPtr<Gio::Action> action;
    if (action_domain == "app") {
        auto gapp = iapp->gtk_app();

        action = gapp->lookup_action(name_without_domain);
    } else if (action_domain == "doc") {
        auto const doc = iapp->get_active_document();
        auto action_group = doc->getActionGroup();

        action = action_group->lookup_action(name_without_domain);
    } else if (action_domain == "win") {
        auto const win = iapp->get_active_window();

        action = win->lookup_action(name_without_domain);
    }

    if (action != nullptr) {
        // glibmm's `get_parameter_type()` is invalid if used when the
        // parameter type is null.
        return g_action_get_parameter_type(action->gobj()) != nullptr;
    } else if (action_domain == "tool") {
        // Tools handle their own action shortcuts without parameters.
        return false;
    }

    // For any other action, assume the worst as we do not have sufficient
    // information.
    g_warning("Unknown action %s", action_name.c_str());
    return true;
}

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

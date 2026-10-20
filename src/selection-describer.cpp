// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::SelectionDescriber - shows messages describing selection
 *
 * Authors:
 *   MenTaLguY <mental@rydia.net>
 *   bulia byak <buliabyak@users.sf.net>
 *   Abhishek Sharma
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2004-2006 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <memory>
#include <unordered_set>
#include <utility>

#include <glibmm/i18n.h>

#include "selection-describer.h"

#include "layer-manager.h"
#include "selection.h"
#include "desktop.h"

#include "object/sp-flowtext.h"
#include "object/sp-image.h"
#include "object/sp-offset.h"
#include "object/sp-path.h"
#include "object/sp-symbol.h"
#include "object/sp-textpath.h"
#include "object/sp-use.h"

#include "xml/quote.h"

// Returns a list of terms for the items to be used in the statusbar
std::pair<std::string, int> collect_terms(const std::vector<SPItem*> &items)
{
    // Currently, there are roughly 25 - 40 different displayNames
    // Hence, reserving 64 locations should suffice
    std::unordered_set<std::string> seen;
    seen.reserve(64);

    std::string out;
    int count = 0;
    bool first = true;

    for (auto item : items) {
        if (!item) {
            continue;
        }

        auto name = item->displayName();
        if (!name || !*name) {
            continue;
        }

        std::string term(name);

        if (seen.insert(term).second) {
            count++;

            if (!first) {
                out += ", ";
            }

            first = false;

            out += "<b>";
            out += term;
            out += "</b>";
        }
    }

    return {out, count};
}

// Returns the number of filtered items in the list
static int count_filtered (const std::vector<SPItem*> &items)
{
    int count=0;
    for (auto item : items) {
        if (item) {
            count += item->isFiltered();
        }
    }
    return count;
}


namespace Inkscape {

SelectionDescriber::SelectionDescriber(Inkscape::Selection *selection, MessageStack &stack, char *when_selected, char *when_nothing)
    : _context{stack}
    , _when_selected{when_selected}
    , _when_nothing{when_nothing}
{
    _selection_changed_connection = selection->connectChanged(
                 sigc::mem_fun(*this, &SelectionDescriber::updateMessage));
    updateMessage(selection);
}

SelectionDescriber::~SelectionDescriber() = default;

void SelectionDescriber::updateMessage(Inkscape::Selection *selection)
{
    // Only look at "items", not just any selected object. SPItems are visual, but non-visual
    // SPObjects can also be selected (like swatches) and shoudn't be mentioned here.
    auto const items = selection->items_vector();
    if (items.empty()) { // no items
        _context.set(Inkscape::NORMAL_MESSAGE, _when_nothing);
    } else {
        SPItem *item = items.front();
        g_assert(item != nullptr);
        SPObject *layer = selection->desktop()->layerManager().layerForObject(item);
        SPObject *root = selection->desktop()->layerManager().currentRoot();

        // Layer name
        gchar *layer_name;
        if (layer == root) {
            layer_name = g_strdup(_("root"));
        } else if(!layer) {
            layer_name = g_strdup(_("none"));
        } else {
            char const *layer_label;
            bool is_label = false;
            if (layer->label()) {
                layer_label = layer->label();
                is_label = true;
            } else {
                layer_label = layer->defaultLabel();
            }
            char *quoted_layer_label = xml_quote_strdup(layer_label);
            if (is_label) {
                layer_name = g_strdup_printf(_("layer <b>%s</b>"), quoted_layer_label);
            } else {
                layer_name = g_strdup_printf(_("layer <b><i>%s</i></b>"), quoted_layer_label);
            }
            g_free(quoted_layer_label);
        }

        // Parent name
        SPObject *parent = item->parent;
        if (!parent) { // fix selector * to "svg:svg"
            return;
        }
        gchar const *parent_label = parent->getId();
        gchar *parent_name = nullptr;
        if (parent_label) {
            char *quoted_parent_label = xml_quote_strdup(parent_label);
            parent_name = g_strdup_printf(_("<i>%s</i>"), quoted_parent_label);
            g_free(quoted_parent_label);
        }

        gchar *in_phrase;
        auto const [num_layers, num_parents] = selection->selectionDistinctLayerAndParentCounts();
        if (num_layers == 1) {
            if (num_parents == 1) {
                if (layer == parent)
                    in_phrase = g_strdup_printf(_(" in %s"), layer_name);
                else if (!layer)
                    in_phrase = g_strdup_printf("%s", _(" hidden in definitions"));
                else if (parent_name)
                    in_phrase = g_strdup_printf(_(" in group %s (%s)"), parent_name, layer_name);
                else
                    in_phrase = g_strdup_printf(_(" in unnamed group (%s)"), layer_name);
            } else {
                    in_phrase = g_strdup_printf(ngettext(" in <b>%zu</b> parent (%s)", " in <b>%zu</b> parents (%s)", num_parents), num_parents, layer_name);
            }
        } else {
            in_phrase = g_strdup_printf(ngettext(" in <b>%zu</b> layer", " in <b>%zu</b> layers", num_layers), num_layers);
        }
        g_free (layer_name);
        g_free (parent_name);

        int objcount = items.size();
        if (objcount == 1) { // one item
            char *item_desc = item->detailedDescription();

            bool isUse = is<SPUse>(item);
            if (isUse && is<SPSymbol>(item->firstChild())) {
                _context.setF(Inkscape::NORMAL_MESSAGE, "%s%s. %s. %s.",
                              item_desc, in_phrase,
                              _("Convert symbol to group to edit"), _when_selected);
            } else if (is<SPSymbol>(item)) {
                _context.setF(Inkscape::NORMAL_MESSAGE, "%s%s. %s.",
                              item_desc, in_phrase,
                              _("Remove from symbols tray to edit symbol"));
            } else {
                SPOffset *offset = isUse ? nullptr : cast<SPOffset>(item);
                if (isUse || (offset && offset->sourceHref)) {
                    _context.setF(Inkscape::NORMAL_MESSAGE, "%s%s. %s. %s.",
                                  item_desc, in_phrase,
                                  _("Use <b>Shift+D</b> to look up original"), _when_selected);
                } else {
                    auto text = cast<SPText>(item);
                    if (text && text->firstChild() && is<SPText>(text->firstChild())) {
                        _context.setF(Inkscape::NORMAL_MESSAGE, "%s%s. %s. %s.",
                                      item_desc, in_phrase,
                                      _("Use <b>Shift+D</b> to look up path"), _when_selected);
                    } else {
                        auto flowtext = cast<SPFlowtext>(item);
                        if (flowtext && !flowtext->has_internal_frame()) {
                            _context.setF(Inkscape::NORMAL_MESSAGE, "%s%s. %s. %s.",
                                          item_desc, in_phrase,
                                          _("Use <b>Shift+D</b> to look up frame"), _when_selected);
                        } else {
                            _context.setF(Inkscape::NORMAL_MESSAGE, "%s%s. %s.",
                                          item_desc, in_phrase, _when_selected);
                        }
                    }
                }
            }

            g_free(item_desc);
        } else { // multiple items
            auto const &[types, num] = collect_terms(items);

            gchar *objects_str = g_strdup_printf(ngettext(
                "<b>%1$i</b> objects selected of type %2$s",
                "<b>%1$i</b> objects selected of types %2$s", num),
                 objcount, types.c_str());

            // indicate all, some, or none filtered
            gchar *filt_str = nullptr;
            int n_filt = count_filtered(items);  //all filtered
            if (n_filt) {
                filt_str = g_strdup_printf(ngettext("; <i>%d filtered object</i> ",
                                                     "; <i>%d filtered objects</i> ", n_filt), n_filt);
            } else {
                filt_str = g_strdup("");
            }

            _context.setF(Inkscape::NORMAL_MESSAGE, "%s%s%s. %s.", objects_str, filt_str, in_phrase, _when_selected);
            if (objects_str) {
                g_free(objects_str);
                objects_str = nullptr;
            }
            if (filt_str) {
                g_free(filt_str);
                filt_str = nullptr;
            }
        }

        g_free(in_phrase);
    }
}

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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

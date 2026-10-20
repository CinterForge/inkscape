// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Fatma Omara <ftomara647@gmail.com>
 *
 * Copyright (C) 2025 authors
 */

#include "recolor-art-manager.h"

#include <glibmm/main.h>
#include <gtkmm/menubutton.h>

#include "object/sp-gradient.h"
#include "object/sp-pattern.h"
#include "object/sp-text.h"
#include "object/sp-use.h"
#include "object/sp-marker.h"
#include "style.h"

namespace Inkscape::UI::Widget {
namespace {

class MoreThan1ColorChecker
{
public:
    bool operator()(SPIPaint const &paint)
    {
        if (!paint.isColor()) {
            return false;
        }

        if (!_first) {
            _first = paint.getColor();
            return false;
        } else {
            return paint.getColor() != _first;
        }
    }

private:
    std::optional<Colors::Color> _first;
};

bool has_colors_pattern(SPItem const *item)
{
    if (!item || !item->style) {
        return false;
    }

    MoreThan1ColorChecker check;
    // Search a pattern for colours, returning true when a second colour is found.
    auto search_pattern = [&] (SPPaintServer const *ps) {
        auto pat = cast<SPPattern>(ps);
        if (!pat) {
            return false;
        }

        for (auto const &child : pat->rootPattern()->children) {
            if (auto group = cast<SPGroup>(&child)) {
                for (auto const &child : group->children) {
                    if (auto c = cast<SPItem>(&child)) {
                        if (check(c->style->fill) || check(c->style->stroke)) {
                            return true;
                        }
                    }
                }
            }

            if (check(child.style->fill) || check(child.style->stroke)) {
                return true;
            }
        }

        return false;
    };

    return search_pattern(item->style->getFillPaintServer()) ||
           search_pattern(item->style->getStrokePaintServer());
}

} // namespace

RecolorArtManager &RecolorArtManager::get()
{
    static RecolorArtManager instance;
    return instance;
}

void RecolorArtManager::reparentPopoverTo(Gtk::MenuButton &button)
{
    if (popover.get_parent() == &button) {
        return;
    }

    if (auto oldbutton = dynamic_cast<Gtk::MenuButton *>(popover.get_parent())) {
        oldbutton->unset_popover();
    }

    button.set_popover(popover);

    // The previous call causes GTK to reset the popover direction to down. Override it to left.
    popover.set_position(Gtk::PositionType::LEFT);
}

RecolorArtManager::RecolorArtManager()
{
    popover.set_autohide(false);
    popover.set_child(widget);

    // Because our popover does not autohide, Gtk doesn't do it's normal good job of ensuring
    // the popup has window focus. It's easy for the button that launched the popup to still be
    // focused (which among other things, means that Escape doesn't directly close the popup).
    // So we manually trigger a "tab forward" event once the popup is shown (but we wait until
    // Gtk has settled down first in an idle callback, because if we don't do that, the popup may
    // not ready to receive focus, which seems to happen every time *but* the first popup event)
    popover.signal_show().connect([this] {
        Glib::signal_idle().connect([this] {
            popover.child_focus(Gtk::DirectionType::TAB_FORWARD);
            return false;
        });
    });
}

bool RecolorArtManager::checkSelection(Inkscape::Selection *selection)
{
    if (selection->size() > 1) {
        return true;
    }

    auto item = selection->singleItem();
    if (!item) {
        return false;
    }

    return is<SPGroup>(item) ||
           is<SPUse>(item) ||
           is<SPText>(item) ||
           item->getMaskObject() ||
           has_colors_pattern(item);
}

bool RecolorArtManager::checkMarkerObject(SPMarker *marker)
{
    if (!marker) {
        return false;
    }

    if (marker->getMaskObject()) {
        return true;
    }

    MoreThan1ColorChecker check;
    for (auto const &child : marker->children) {

        if (auto item = cast<SPItem>(&child)) {
            if (item->style) {
                if (check(item->style->fill) || check(item->style->stroke)) {
                    return true;
                }
            }
        }
        if (auto group = cast<SPGroup>(&child)) {
            for (auto const &child : group->children) {
                if (auto c = cast<SPItem>(&child)) {
                    if (check(c->style->fill) || check(c->style->stroke)) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

bool RecolorArtManager::checkMeshObject(Inkscape::Selection *selection)
{
    if (selection->size() > 1) {
        return true;
    }

    auto item = selection->singleItem();
    if (!item) {
        return false;
    }

    auto is_mesh = [] (SPPaintServer *ps) {
        auto grad = cast<SPGradient>(ps);
        return grad && grad->hasPatches();
    };

    return is_mesh(item->style->getFillPaintServer()) ||
           is_mesh(item->style->getStrokePaintServer());
}

} // namespace Inkscape::UI::Widget

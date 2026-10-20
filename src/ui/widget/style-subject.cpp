// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2007 MenTaLguY <mental@rydia.net>
 *   Abhishek Sharma
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "style-subject.h"

#include "desktop.h"
#include "desktop-style.h"
#include "selection.h"

namespace Inkscape {
namespace UI {
namespace Widget {

StyleSubject::StyleSubject() {
}

StyleSubject::~StyleSubject() {
    setDesktop(nullptr);
}

void StyleSubject::setDesktop(SPDesktop *desktop) {
    if (desktop != _desktop) {
        _desktop = desktop;
        _afterDesktopSwitch(desktop);
        if (_desktop) {
            _emitChanged(); // This updates the widgets.
        }
    }
}

StyleSubject::Selection::Selection() = default;

StyleSubject::Selection::~Selection() = default;

Inkscape::Selection *StyleSubject::Selection::_getSelection() const {
    SPDesktop *desktop = getDesktop();
    if (desktop) {
        return desktop->getSelection();
    } else {
        return nullptr;
    }
}

std::vector<SPObject*> StyleSubject::Selection::list() {
    Inkscape::Selection *selection = _getSelection();
    if(selection) {
        return std::vector<SPObject *>(selection->objects().begin(), selection->objects().end());
    }

    return std::vector<SPObject*>();
}

Geom::OptRect StyleSubject::Selection::getBounds(SPItem::BBoxType type) {
    Inkscape::Selection *selection = _getSelection();
    if (selection) {
        return selection->bounds(type);
    } else {
        return Geom::OptRect();
    }
}

int StyleSubject::Selection::queryStyle(SPStyle *query, int property) {
    SPDesktop *desktop = getDesktop();
    if (desktop) {
        return sp_desktop_query_style(desktop, query, property);
    } else {
        return QUERY_STYLE_NOTHING;
    }
}

void StyleSubject::Selection::_afterDesktopSwitch(SPDesktop *desktop) {
    _sel_changed.disconnect();
    _sel_modified.disconnect();
    if (desktop) {
        Inkscape::Selection *selection = desktop->getSelection();
        if (selection) {
            _sel_changed = selection->connectChanged(sigc::hide(sigc::mem_fun(*this, &Selection::_emitChanged)));
            _sel_modified = selection->connectModified(sigc::mem_fun(*this, &Selection::_emitModified));
        }
    }
}

void StyleSubject::Selection::setCSS(SPCSSAttr *css) {
    SPDesktop *desktop = getDesktop();
    if (desktop) {
        sp_desktop_set_style(desktop, css);
    }
}

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

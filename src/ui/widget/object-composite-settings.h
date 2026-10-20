// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_UI_WIDGET_OBJECT_COMPOSITE_SETTINGS_H
#define SEEN_UI_WIDGET_OBJECT_COMPOSITE_SETTINGS_H

/*
 * Authors:
 *   Bryce W. Harrington <bryce@bryceharrington.org>
 *   Gustav Broberg <broberg@kth.se>
 *
 * Copyright (C) 2004--2007 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtkmm/box.h>

#include "ui/widget/filter-effect-chooser.h"

class SPDesktop;
struct InkscapeApplication;

namespace Inkscape {

namespace UI {
namespace Widget {

constexpr double BLUR_MULTIPLIER = 4.0;

class StyleSubject;

/*
 * A widget for controlling object compositing (filter, opacity, etc.)
 */
class ObjectCompositeSettings : public Gtk::Box {
public:
    ObjectCompositeSettings(Glib::ustring icon_name, char const *history_prefix, int flags);
    ~ObjectCompositeSettings() override;

    void setSubject(StyleSubject *subject);

private:
    Glib::ustring   _icon_name; // Used by History dialog.

    Glib::ustring   _blend_tag;
    Glib::ustring   _blur_tag;
    Glib::ustring   _opacity_tag;
    Glib::ustring   _isolation_tag;

    StyleSubject *_subject = nullptr;

    SimpleFilterModifier _filter_modifier;

    bool _blocked;
    gulong _desktop_activated;
    sigc::connection _subject_changed;
    
    static void _on_desktop_activate(SPDesktop *desktop, ObjectCompositeSettings *w);
    static void _on_desktop_deactivate(SPDesktop *desktop, ObjectCompositeSettings *w);
    void _subjectChanged();
    void _blendBlurValueChanged();
    void _opacityValueChanged();
    void _isolationValueChanged();
};

}
}
}

#endif

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

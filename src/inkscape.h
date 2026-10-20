// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_INKSCAPE_H
#define SEEN_INKSCAPE_H

/*
 * Interface to main application
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Liam P. White <inkscapebrony@gmail.com>
 *
 * Copyright (C) 1999-2014 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <optional>
#include <set>
#include <string>
#include <vector>

class SPDesktop;
class SPDocument;

namespace Inkscape {

class Application;
class Selection;

namespace UI {
class ThemeContext;
} // namespace UI

class Application
{
public:
    static Application &instance();
    static bool exists();
    static void create(bool use_gui);

    bool use_gui() const { return _use_gui; }
    void use_gui(bool guival) { _use_gui = guival; }

    SPDocument * active_document();
    SPDesktop * active_desktop();

    Inkscape::UI::ThemeContext *themecontext = nullptr;
    
    // Inkscape desktop stuff
    void add_desktop(SPDesktop * desktop);
    void remove_desktop(SPDesktop* desktop);
    void activate_desktop (SPDesktop * desktop);
    void switch_desktops_next ();
    void switch_desktops_prev ();
    SPDesktop * find_desktop_by_dkey (unsigned int dkey);
    unsigned int maximum_dkey();
    SPDesktop * next_desktop ();
    SPDesktop * prev_desktop ();
    std::vector<SPDesktop *> &get_desktops() { return _desktops; };
    
    // Moved document add/remove functions into public inkscape.h as they are used
    // (rightly or wrongly) by console-mode functions
    void add_document(SPDocument *document);
    void remove_document(SPDocument *document);
    
    static void crash_handler(int signum);

    void set_pdf_poppler(bool p) {
        _pdf_poppler = p;
    }
    bool get_pdf_poppler() {
        return _pdf_poppler;
    }
    void set_pdf_font_strategy(int mode) {
        _pdf_font_strategy = mode;
    }
    int get_pdf_font_strategy() {
        return _pdf_font_strategy;
    }
    void set_pdf_convert_colors(bool convert) {
        _pdf_convert_colors = convert;
    }
    bool get_pdf_convert_colors() const {
        return _pdf_convert_colors;
    }
    void set_pdf_group_by(const std::string &group_by) {
        _pdf_group_by = group_by;
    }
    const std::string &get_pdf_group_by() {
        return _pdf_group_by;
    }

    void set_pages(const std::string &pages) {
        _pages = pages;
    }
    const std::string &get_pages() const {
        return _pages;
    }

private:
    class ConstructibleApplication;
    static std::optional<ConstructibleApplication> &_get();

    explicit Application(bool use_gui);
    ~Application();

    Application(Application const &) = delete;
    Application &operator=(Application const &) = delete;

    std::set<SPDocument *> _document_set;
    std::vector<SPDesktop *> _desktops;
    std::string _pages;

    bool _use_gui = false;
    bool _pdf_poppler = false;
    int _pdf_font_strategy = 0;
    bool _pdf_convert_colors = false;
    std::string _pdf_group_by;
};

} // namespace Inkscape

#define INKSCAPE (Inkscape::Application::instance())
#define SP_ACTIVE_DOCUMENT (INKSCAPE.active_document())
#define SP_ACTIVE_DESKTOP (INKSCAPE.active_desktop())

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

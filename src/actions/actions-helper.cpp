// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for selection tied to the application and without GUI.
 *
 * Copyright (C) 2018 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 * TODO: REMOVE THIS FILE It's really not necessary.
 */

#include "actions-helper.h"

#include <cstdio>
#include <glibmm/convert.h>
#include <glibmm/miscutils.h>
#include <glibmm/ustring.h>
#include <double-conversion/double-conversion.h>

#include "inkscape-application.h"
#include "xml/document.h"          // for Document
#include "xml/node.h"              // for Node
#include "xml/repr.h"              // for sp_repr_document_new, sp_repr_save...

static bool use_active_window = false;
static Inkscape::XML::Document *active_window_data = nullptr; 

// this function is called when in command line we call with parameter --active-window | -q
// is called by a auto add new start and end action that fire first this action
// and keep on till last inserted action is done
void active_window_start_helper()
{
    use_active_window = true;
    active_window_data = sp_repr_document_new("activewindowdata");
}

// this is the end of previous function. Finish the wrap of actions to active desktop
// it also save a file to allow print in the caller terminal the output to be readable by
// external programs like extensions.
void active_window_end_helper()
{
    auto const tmpfile = get_active_desktop_commands_location();
    auto const tmpfile_next = tmpfile + ".next";
    sp_repr_save_file(active_window_data, tmpfile_next.c_str());
    std::rename(tmpfile_next.c_str(), tmpfile.c_str());
    use_active_window = false;
    Inkscape::GC::release(active_window_data);
    active_window_data = nullptr;
}

std::string get_active_desktop_commands_location()
{
    return Glib::build_filename(g_get_user_cache_dir(), "inkscape-active_desktop_commands.xml");
}

void 
show_output(Glib::ustring const &data, bool const is_cerr)
{
    if (is_cerr) {
        std::cerr << data << std::endl;
    } else {
        std::cout << data << std::endl;
    }

    if (use_active_window) {
        if (auto root = active_window_data->root()) {
            Inkscape::XML::Node * node = nullptr;
            if (is_cerr) {
                node = active_window_data->createElement("cerr");
            } else {
                node = active_window_data->createElement("cout");
            }
            root->appendChild(node);
            Inkscape::GC::release(node);
            auto txtnode = active_window_data->createTextNode("", true);
            node->appendChild(txtnode);
            Inkscape::GC::release(txtnode);
            txtnode->setContent(data.c_str());
        }
    }
}

// Helper function: returns true if both document and selection found. Maybe this should
// work on current view. Or better, application could return the selection of the current view.
bool
get_document_and_selection(InkscapeApplication* app, SPDocument** document, Inkscape::Selection** selection)
{
    *document = app->get_active_document();
    if (!(*document)) {
        show_output("get_document_and_selection: No document!");
        return false;
    }

    *selection = app->get_active_selection();
    if (!*selection) {
        show_output("get_document_and_selection: No selection!");
        return false;
    }

    return true;
}

/**
 * Convert a double to a string in a way that is compatible with GTK action parsing.
 *
 * This means locale-independent conversion to the shortest possible string that still
 * has a ".0" at the end (so GTK doesn't confuse it with an integer).
 */
std::string to_string_for_actions(double x)
{
    using namespace double_conversion;
    auto const conv = DoubleToStringConverter{
        DoubleToStringConverter::UNIQUE_ZERO |
            DoubleToStringConverter::EMIT_TRAILING_DECIMAL_POINT |
            DoubleToStringConverter::EMIT_TRAILING_ZERO_AFTER_POINT,
        "inf", "NaN", 'e', -3, 6, 0, 0
    };
    auto buf = std::string(32, '\0');
    auto builder = double_conversion::StringBuilder(buf.data(), buf.size());
    conv.ToShortest(x, &builder);
    buf.resize(builder.position());
    return buf;
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

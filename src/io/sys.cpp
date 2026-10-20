// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * System abstraction utility routines
 *
 * Authors:
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2004-2005 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#include <fstream>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include <glib.h>
#include <glib/gstdio.h>
#include <glibmm/ustring.h>

#include "sys.h"

//#define INK_DUMP_FILENAME_CONV 1
#undef INK_DUMP_FILENAME_CONV

//#define INK_DUMP_FOPEN 1
#undef INK_DUMP_FOPEN

extern guint update_in_progress;


void Inkscape::IO::dump_fopen_call( char const *utf8name, char const *id )
{
#ifdef INK_DUMP_FOPEN
    Glib::ustring str;
    for ( int i = 0; utf8name[i]; i++ )
    {
        if ( utf8name[i] == '\\' )
        {
            str += "\\\\";
        }
        else if ( (utf8name[i] >= 0x20) && ((0x0ff & utf8name[i]) <= 0x7f) )
        {
            str += utf8name[i];
        }
        else
        {
            gchar tmp[32];
            g_snprintf( tmp, sizeof(tmp), "\\x%02x", (0x0ff & utf8name[i]) );
            str += tmp;
        }
    }
    g_message( "fopen call %s for [%s]", id, str.data() );
#else
    (void)utf8name;
    (void)id;
#endif
}

/**
 * Open a file with g_fopen().
 * 
 * \param utf8name Filename in UTF8 encoding
 * \param mode see g_fopen()
 * \returns see g_fopen()
 */
FILE *Inkscape::IO::fopen_utf8name( char const *utf8name, char const *mode )
{
    FILE* fp = nullptr;

    if (Glib::ustring( utf8name ) == Glib::ustring("-")) {
        // user requests to use pipes

        Glib::ustring how( mode );
        if ( how.find("w") != Glib::ustring::npos ) {
#ifdef _WIN32
            setmode(fileno(stdout), O_BINARY);
#endif
            return stdout;
        } else {
            return stdin;
        }
    }

    gchar *filename = g_filename_from_utf8( utf8name, -1, nullptr, nullptr, nullptr );
    if ( filename )
    {
        // ensure we open the file in binary mode (not needed in POSIX but doesn't hurt either)
        Glib::ustring how( mode );
        if ( how.find("b") == Glib::ustring::npos )
        {
            how.append("b");
        }
        // when opening a file for writing: create parent directories if they don't exist already
        if ( how.find("w") != Glib::ustring::npos )
        {
            gchar *dirname = g_path_get_dirname(utf8name);
            if (g_mkdir_with_parents(dirname, 0777)) {
                g_warning("Could not create directory '%s'", dirname);
            }
            g_free(dirname);
        }
        fp = g_fopen(filename, how.c_str());
        g_free(filename);
        filename = nullptr;
    }
    return fp;
}


bool Inkscape::IO::file_test( char const *utf8name, GFileTest test )
{
    bool exists = false;

    // in case the file to check is a pipe it doesn't need to exist
    if (g_strcmp0(utf8name, "-") == 0 && G_FILE_TEST_IS_REGULAR)
        return true;

    if ( utf8name ) {
        gchar *filename = nullptr;
        if (utf8name && !g_utf8_validate(utf8name, -1, nullptr)) {
            /* FIXME: Trying to guess whether or not a filename is already in utf8 is unreliable.
               If any callers pass non-utf8 data (e.g. using g_get_home_dir), then change caller to
               use simple g_file_test.  Then add g_return_val_if_fail(g_utf_validate(...), false)
               to beginning of this function. */
            filename = g_strdup(utf8name);
            // Looks like g_get_home_dir isn't safe.
            //g_warning("invalid UTF-8 detected internally. HUNT IT DOWN AND KILL IT!!!");
        } else {
            filename = g_filename_from_utf8 ( utf8name, -1, nullptr, nullptr, nullptr );
        }
        if ( filename ) {
            exists = g_file_test (filename, test);
            g_free(filename);
            filename = nullptr;
        } else {
            g_warning( "Unable to convert filename in IO:file_test" );
        }
    }

    return exists;
}

bool Inkscape::IO::file_is_writable( char const *utf8name)
{
    bool success = true;

    if ( utf8name) {
        gchar *filename = nullptr;
        if (utf8name && !g_utf8_validate(utf8name, -1, nullptr)) {
            /* FIXME: Trying to guess whether or not a filename is already in utf8 is unreliable.
               If any callers pass non-utf8 data (e.g. using g_get_home_dir), then change caller to
               use simple g_file_test.  Then add g_return_val_if_fail(g_utf_validate(...), false)
               to beginning of this function. */
            filename = g_strdup(utf8name);
            // Looks like g_get_home_dir isn't safe.
            //g_warning("invalid UTF-8 detected internally. HUNT IT DOWN AND KILL IT!!!");
        } else {
            filename = g_filename_from_utf8 ( utf8name, -1, nullptr, nullptr, nullptr );
        }
        if ( filename ) {
            GStatBuf st;
            if (g_file_test (filename, G_FILE_TEST_EXISTS)){ 
                if (g_lstat (filename, &st) == 0) {
                    success = ((st.st_mode & S_IWRITE) != 0);
                }
            }
            g_free(filename);
            filename = nullptr;
        } else {
            g_warning( "Unable to convert filename in IO:file_test" );
        }
    }

    return success;
}

Glib::ustring Inkscape::IO::sanitizeString(char const *str)
{
    if (!str) {
        return {};
    }

    if (g_utf8_validate(str, -1, nullptr)) {
        return str;
    }

    Glib::ustring result;

    for (auto p = str; *p != '\0'; p++) {
        if (*p == '\\') {
            result += "\\\\";
        } else if (*p >= 0) {
            result += *p;
        } else {
            char buf[8];
            g_snprintf(buf, sizeof(buf), "\\x%02x", unsigned{static_cast<unsigned char>(*p)});
            result += buf;
        }
    }

    return result;
}

/* 
 * Returns the file extension of a path/filename. Don't use this one unless for display.
 * Used by src/extension/internal/odf.cpp, src/extension/output.cpp. TODO Remove!
 */
Glib::ustring Inkscape::IO::get_file_extension(Glib::ustring const &path)
{
    auto loc = path.find_last_of(".");
    return loc < path.size() ? path.substr(loc) : "";
}

/*
 * Returns the file extension of a path/filename. Use this one for filenames.
 * Used by src/extension/effect.cpp, src/io/file-export-cmd.cpp, etc.
 */
std::string Inkscape::IO::get_file_extension(std::string const &path)
{
    auto loc = path.find_last_of(".");
    return loc < path.size() ? path.substr(loc) : "";
}

/**
 * Removes a file extension, if found, from the given path
 */
void Inkscape::IO::remove_file_extension(std::string &path)
{
    auto ext = Inkscape::IO::get_file_extension(path);
    if (!ext.empty()) {
        path.erase(path.size() - ext.size());
    }
}

void Inkscape::IO::swap_file_extension(std::string &path, std::string const &new_ext)
{
    remove_file_extension(path);
    path.append(new_ext);
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

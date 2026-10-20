// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Ted Gould <ted@gould.cx>
 *
 * Copyright (C) 2002-2005 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "input.h"

#include "timer.h"

#include "db.h"
#include "implementation/implementation.h"

#include "xml/attribute-record.h"
#include "xml/node.h"
#include "document.h"

/* Inkscape::Extension::Input */

namespace Inkscape {
namespace Extension {

/**
    \brief    Builds an Input object from a XML description
    \param    in_repr The XML description in a Inkscape::XML::Node tree
    \param    implementation The module to be initialized.

    Okay, so you want to build an Input object.

    This function first takes and does the build of the parent class,
    which is SPModule.  Then, it looks for the <input> section of the
    XML description.  Under there should be several fields which
    describe the input module to excruciating detail.  Those are parsed,
    copied, and put into the structure that is passed in as module.
    Overall, there are many levels of indentation, just to handle the
    levels of indentation in the XML file.
*/
Input::Input (Inkscape::XML::Node *in_repr, ImplementationHolder implementation, std::string *base_directory)
    : Extension(in_repr, std::move(implementation), base_directory)
{
    mimetype = nullptr;
    extension = nullptr;
    filetypename = nullptr;
    filetypetooltip = nullptr;

    if (repr != nullptr) {
        Inkscape::XML::Node * child_repr;

        child_repr = repr->firstChild();

        while (child_repr != nullptr) {
            if (!strcmp(child_repr->name(), INKSCAPE_EXTENSION_NS "input")) {
                // Input tag attributes
                for (const auto &iter : child_repr->attributeList()) {
                    std::string name = g_quark_to_string(iter.key);
                    std::string value = std::string(iter.value);
                    if (name == "priority")
                        set_sort_priority(strtol(value.c_str(), nullptr, 0));
                }

                child_repr = child_repr->firstChild();
                while (child_repr != nullptr) {
                    char const * chname = child_repr->name();
					if (!strncmp(chname, INKSCAPE_EXTENSION_NS_NC, strlen(INKSCAPE_EXTENSION_NS_NC))) {
						chname += strlen(INKSCAPE_EXTENSION_NS);
					}
                    if (chname[0] == '_') /* Allow _ for translation of tags */
                        chname++;
                    if (!strcmp(chname, "extension")) {
                        g_free (extension);
                        extension = g_strdup(child_repr->firstChild()->content());
                    }
                    if (!strcmp(chname, "mimetype")) {
                        g_free (mimetype);
                        mimetype = g_strdup(child_repr->firstChild()->content());
                    }
                    if (!strcmp(chname, "filetypename")) {
                        g_free (filetypename);
                        filetypename = g_strdup(child_repr->firstChild()->content());
                    }
                    if (!strcmp(chname, "filetypetooltip")) {
                        g_free (filetypetooltip);
                        filetypetooltip = g_strdup(child_repr->firstChild()->content());
                    }

                    child_repr = child_repr->next();
                }

                break;
            }

            child_repr = child_repr->next();
        }

    }

    return;
}

/**
    \return  None
    \brief   Destroys an Input extension
*/
Input::~Input ()
{
    g_free(mimetype);
    g_free(extension);
    g_free(filetypename);
    g_free(filetypetooltip);
    return;
}

/**
    \return  Whether this extension checks out
    \brief   Validate this extension

    This function checks to make sure that the input extension has
    a filename extension and a MIME type.  Then it calls the parent
    class' check function which also checks out the implementation.
*/
bool
Input::check ()
{
    if (extension == nullptr)
        return FALSE;
    if (mimetype == nullptr)
        return FALSE;

    return Extension::check();
}

/**
    \return  A new document
    \brief   This function creates a document from a file
    \param   uri  The filename to create the document from
    \param   is_importing True if the opened file is being imported

    This function acts as the first step in creating a new document
    from a file.  The first thing that this does is make sure that the
    file actually exists.  If it doesn't, a NULL is returned.  If the
    file exits, then it is opened using the implementation of this extension.
*/
std::unique_ptr<SPDocument> Input::open(char const *uri, bool is_importing)
{
    if (!loaded()) {
        set_state(Extension::STATE_LOADED);
    }
    if (!loaded()) {
        return nullptr;
    }
    timer->touch();

    return imp->open(this, uri, is_importing);
}

/**
    \return  IETF mime-type for the extension
    \brief   Get the mime-type that describes this extension
*/
gchar const *
Input::get_mimetype() const
{
    return mimetype;
}

/**
    \return  Filename extension for the extension
    \brief   Get the filename extension for this extension
*/
gchar const *
Input::get_extension() const
{
    return extension;
}

/**
    \return  True if the filename matches
    \brief   Match filename to extension that can open it.
*/
bool
Input::can_open_filename(gchar const *filename) const
{
    gchar *filenamelower = g_utf8_strdown(filename, -1);
    gchar *extensionlower = g_utf8_strdown(extension, -1);
    bool result = g_str_has_suffix(filenamelower, extensionlower);
    g_free(filenamelower);
    g_free(extensionlower);
    return result;
}

/**
    \return  The name of the filetype supported
    \brief   Get the name of the filetype supported
*/
const char *
Input::get_filetypename(bool translated) const
{
    const char *name;

    if (filetypename)
        name = filetypename;
    else
        name = get_name();

    if (name && translated && filetypename) {
        return get_translation(name);
    } else {
        return name;
    }
}

/**
    \return  Tooltip giving more information on the filetype
    \brief   Get the tooltip for more information on the filetype
*/
const char *
Input::get_filetypetooltip(bool translated) const
{
    if (filetypetooltip && translated) {
        return get_translation(filetypetooltip);
    } else {
        return filetypetooltip;
    }
}

/**
 * Get an input extension by mime-type matching.
 *
 * @arg mime - The mime to match against.
 *
 * @returns the first Input extension found or nullptr
 */
Extension *Input::find_by_mime(char const *mime)
{
    DB::InputList list;
    db.get_input_list(list);

    auto it = list.begin();
    while (true) {
        if (it == list.end()) return nullptr;
        if (std::strcmp((*it)->get_mimetype(), mime) == 0) return *it;
        ++it;
    }
}

/**
 * Get an input extension by filename matching. Does not look at file contents.
 *
 * @arg filename - The filename to match against.
 *
 * @returns the first Input extension found or nullptr
 */
Extension *Input::find_by_filename(char const *filename)
{
    DB::InputList list;

    for (auto imod : db.get_input_list(list)) {
        if (imod->can_open_filename(filename)) {
            return imod;
        }
    }
    return nullptr;
}

} }  /* namespace Inkscape, Extension */

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

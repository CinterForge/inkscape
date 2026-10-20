// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * Inkscape::Extension::Extension:
 * Frontend to certain, possibly pluggable, actions.
 * the ability to have features that are more modular so that they
 * can be added and removed easily.  This is the basis for defining
 * those actions.
 */
/*
 * Authors:
 *   Ted Gould <ted@gould.cx>
 *
 * Copyright (C) 2002-2005 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INK_EXTENSION_H
#define INK_EXTENSION_H

#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include <glibmm/ustring.h>
#include <sigc++/signal.h>

#include "util/hybrid-pointer.h"

// Push color headers to all extensions
#include "colors/color.h"

namespace Glib {
class ustring;
} // namespace Glib

namespace Gtk {
class Grid;
class Box;
class Widget;
} // namespace Gtk

/** The key that is used to identify that the I/O should be autodetected */
#define SP_MODULE_KEY_AUTODETECT "autodetect"
/** This is the key for the SVG input module */
#define SP_MODULE_KEY_INPUT_SVG "org.inkscape.input.svg"
#define SP_MODULE_KEY_INPUT_SVGZ "org.inkscape.input.svgz"
/** Specifies the input module that should be used if none are selected */
#define SP_MODULE_KEY_INPUT_DEFAULT SP_MODULE_KEY_AUTODETECT
/** The key for outputting standard W3C SVG */
#define SP_MODULE_KEY_OUTPUT_SVG "org.inkscape.output.svg.plain"
#define SP_MODULE_KEY_OUTPUT_SVGZ "org.inkscape.output.svgz.plain"
/** This is an output file that has SVG data with the Sodipodi namespace extensions */
#define SP_MODULE_KEY_OUTPUT_SVG_INKSCAPE "org.inkscape.output.svg.inkscape"
#define SP_MODULE_KEY_OUTPUT_SVGZ_INKSCAPE "org.inkscape.output.svgz.inkscape"
/** Which output module should be used? */
#define SP_MODULE_KEY_OUTPUT_DEFAULT SP_MODULE_KEY_AUTODETECT

/** Internal raster extensions */
#define SP_MODULE_KEY_RASTER_PNG "org.inkscape.output.png.inkscape"

/** Defines the key for Postscript printing */
#define SP_MODULE_KEY_PRINT_PS    "org.inkscape.print.ps"
#define SP_MODULE_KEY_PRINT_CAIRO_PS    "org.inkscape.print.ps.cairo"
#define SP_MODULE_KEY_PRINT_CAIRO_EPS    "org.inkscape.print.eps.cairo"
/** Defines the key for PDF printing */
#define SP_MODULE_KEY_PRINT_PDF    "org.inkscape.print.pdf"
#define SP_MODULE_KEY_PRINT_CAIRO_PDF    "org.inkscape.print.pdf.cairo"
/** Defines the key for LaTeX printing */
#define SP_MODULE_KEY_PRINT_LATEX    "org.inkscape.print.latex"
/** Defines the key for printing with GNOME Print */
#define SP_MODULE_KEY_PRINT_GNOME "org.inkscape.print.gnome"

/** Mime type for SVG */
#define MIME_SVG "image/svg+xml"

/** Name of the extension error file */
#define EXTENSION_ERROR_LOG_FILENAME  "extension-errors.log"


#define INKSCAPE_EXTENSION_URI   "http://www.inkscape.org/namespace/inkscape/extension"
#define INKSCAPE_EXTENSION_NS_NC "extension"
#define INKSCAPE_EXTENSION_NS    "extension:"

enum ModuleImpType
{
    MODULE_EXTENSION,   // implementation/script.h python extensions
    MODULE_XSLT,        // implementation/xslt.h xml transform extensions
    MODULE_PLUGIN,      // plugins/*/*.h C++ extensions
    MODULE_UNKNOWN_IMP  // No implementation, so nothing created.
};

enum ModuleFuncType
{
    MODULE_TEMPLATE,
    MODULE_INPUT,
    MODULE_OUTPUT,
    MODULE_FILTER,
    MODULE_PRINT,
    MODULE_PATH_EFFECT,
    MODULE_UNKNOWN_FUNC
};

class SPDocument;

namespace Inkscape {

namespace XML {
class Node;
} // namespace XML

namespace Extension {

class ExecutionEnv;
class Dependency;
class ProcessingAction;
class ExpirationTimer;
class ExpirationTimer;
class InxParameter;
class InxWidget;

namespace Implementation {
class Implementation;
} // namespace Implementation

/** The object that is the basis for the Extension system.  This object
    contains all of the information that all Extension have.  The
    individual items are detailed within. This is the interface that
    those who want to _use_ the extensions system should use.  This
    is most likely to be those who are inside the Inkscape program. */
class Extension {
public:
    /** An enumeration to identify if the Extension has been loaded or not. */
    enum state_t {
        STATE_LOADED,      /**< The extension has been loaded successfully */
        STATE_UNLOADED,    /**< The extension has not been loaded */
        STATE_DEACTIVATED  /**< The extension is missing something which makes it unusable */
    };
    using ImplementationHolder = Util::HybridPointer<Implementation::Implementation>;

private:
    std::string _id  ;                         /**< The unique identifier for the Extension */
    std::string _name;                         /**< A user friendly name for the Extension */
    state_t    _state = STATE_UNLOADED;        /**< Which state the Extension is currently in */
    int _priority = 0;                         /**< when sorted, should this come before any others */
    std::vector<std::unique_ptr<Dependency>> _deps; /**< Dependencies for this extension */
    static FILE *error_file;                   /**< This is the place where errors get reported */
    std::string _error_reason;                 /**< Short, textual explanation for the latest error */
    bool _gui = true;

    std::vector<ProcessingAction> _actions;    /**< Processing actions */

protected:
    Inkscape::XML::Node *repr;                 /**< The XML description of the Extension */

    /** An Implementation object provides the actual implementation of the Extension.
     *  We hold an owning pointer to the implementation when the implementation is created by Inkscape,
     *  and a non-owning pointer when the implementation is allocated in an external library.
     */
    ImplementationHolder imp;
    std::string _base_directory;               /**< Directory containing the .inx file,
                                                 *  relative paths in the extension should usually be relative to it */
    std::unique_ptr<ExpirationTimer> timer;    /**< Timeout to unload after a given time */
    bool _translation_enabled = true;          /**< Attempt translation of strings provided by the extension? */

private:
    char const *_translationdomain = nullptr;  /**< Domainname of gettext textdomain that should
                                                 *  be used for translation of the extension's strings */
    std::string _gettext_catalog_dir;          /**< Directory containing the gettext catalog for _translationdomain */

    void lookup_translation_catalog();

public:
    Extension(Inkscape::XML::Node *in_repr, ImplementationHolder implementation, std::string *base_directory);
    virtual ~Extension();

    void          set_state    (state_t in_state);
    state_t       get_state    ();
    bool          loaded       ();
    virtual bool  check        ();
    virtual bool prefs();
    Inkscape::XML::Node *get_repr();
    char const   *get_id       () const;
    char const   *get_name     () const;
    virtual void  deactivate   ();
    bool          deactivated  ();
    void          printFailure (Glib::ustring const &reason);
    std::string const &getErrorReason() { return _error_reason; };
    Implementation::Implementation *get_imp() { return imp.get(); }
    auto const   &get_base_directory() const { return _base_directory; };
    void          set_base_directory(std::string const &base_directory) { _base_directory = base_directory; };
    std::string   get_dependency_location(char const *name);
    char const   *get_translation(char const *msgid, char const *msgctxt = nullptr) const;
    void          set_environment(SPDocument const *doc = nullptr);
    ModuleImpType get_implementation_type();

    int get_sort_priority() const { return _priority; }
    void set_sort_priority(int priority) { _priority = priority; }

    void run_processing_actions(SPDocument *doc);

    /* Parameter Stuff */
private:
    std::vector<std::unique_ptr<InxWidget>> _widgets; /**< A list of widgets for this extension. */

public:
    /** \brief  A function to get the number of visible parameters of the extension.
        \return The number of visible parameters. */
    unsigned int widget_visible_count() const;

    /** An error class for when a parameter is looked for that just
     * simply doesn't exist */
    class param_not_exist {};

    /** no valid ID found while parsing XML representation */
    class extension_no_id{};

    /** no valid name found while parsing XML representation */
    class extension_no_name{};

    /** extension is not compatible with the current system and should not be loaded */
    class extension_not_compatible{};

    /** No implementation could be loaded for the extension. */
    class no_implementation_for_extension : public std::exception {};

    /** An error class for when a filename already exists, but the user
     * doesn't want to overwrite it */
    class no_overwrite {};

private:
    void             make_param       (Inkscape::XML::Node * paramrepr);

    /**
     * Looks up the parameter with the specified name.
     *
     * Searches the list of parameters attached to this extension,
     * looking for a parameter with a matching name.
     *
     * This function can throw a 'param_not_exist' exception if the
     * name is not found.
     *
     * @param  name Name of the parameter to search for.
     * @return Parameter with matching name.
     */
     InxParameter *get_param(char const *name);

     /// @copydoc get_param()
     InxParameter const *get_param(char const *name) const;

public:
    bool        get_param_bool          (char const *name) const;
    bool        get_param_bool          (char const *name, bool alt) const;
    int         get_param_int           (char const *name) const;
    int         get_param_int           (char const *name, int alt) const;
    double      get_param_float         (char const *name) const;
    double      get_param_float         (char const *name, double alt) const;
    char const *get_param_string        (char const *name, char const *alt) const;
    char const *get_param_string        (char const *name) const;
    char const *get_param_optiongroup   (char const *name, char const *alt) const;
    char const *get_param_optiongroup   (char const *name) const;

    Colors::Color get_param_color(char const *name) const;

    bool get_param_optiongroup_contains (char const *name, char const   *value) const;
    bool get_param_optiongroup_is(char const *name, std::string_view value, bool alt = false) const;

    bool        set_param_bool          (char const *name, bool    value);
    int         set_param_int           (char const *name, int     value);
    double      set_param_float         (char const *name, double  value);
    char const *set_param_string        (char const *name, char const   *value);
    char const *set_param_optiongroup   (char const *name, char const   *value);
    void        set_param_color         (char const *name, Colors::Color const &color);
    void set_param_any(char const *name, std::string const &value);
    void set_param_hidden(char const *name, bool hidden);

    /* Error file handling */
    static void      error_file_open ();
    static void      error_file_close();
    static void      error_file_write(Glib::ustring const &text);

    Gtk::Widget *autogui (SPDocument *doc, Inkscape::XML::Node *node, sigc::signal<void ()> *changeSignal = nullptr);
    void paramListString(std::list<std::string> &retlist) const;
    void set_gui(bool s) { _gui = s; }
    bool get_gui() const { return _gui; }

    /* Extension editor dialog stuff */
    Gtk::Box *get_params_widget();

protected:
    inline static void add_val(Glib::ustring const &labelstr, Glib::ustring const &valuestr,
                               Gtk::Grid * table, int * row);
};

/*
This is a prototype for how collections should work.  Whoever gets
around to implementing this gets to decide what a 'folder' and an
'item' really is.  That is the joy of implementing it, eh?

class Collection : public Extension {

public:
    folder  get_root (void);
    int     get_count (folder);
    thumbnail get_thumbnail(item);
    item[]  get_items(folder);
    folder[]  get_folders(folder);
    metadata get_metadata(item);
    image   get_image(item);

};
*/

} // namespace Extension
} // namespace Inkscape

#endif // INK_EXTENSION_H

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

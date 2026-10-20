// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Code for handling extensions (i.e. scripts).
 *
 * Authors:
 *   Bryce Harrington <bryce@osdl.org>
 *   Ted Gould <ted@gould.cx>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2002-2007 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "script.h"

#include <algorithm>
#include <climits>
#include <memory>
#include <ranges>
#include <glib/gstdio.h>
#include <glibmm/convert.h>
#include <glibmm/fileutils.h>
#include <glibmm/main.h>
#include <glibmm/miscutils.h>
#include <gtkmm/enums.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/textview.h>
#include <gtkmm/window.h>

#include "desktop.h"
#include "event.h"
#include "extension/db.h"
#include "extension/effect.h"
#include "extension/execution-env.h"
#include "extension/init.h"
#include "extension/input.h"
#include "extension/output.h"
#include "extension/system.h"
#include "extension/template.h"
#include "inkscape-window.h"
#include "inkscape.h"
#include "io/file.h"
#include "layer-manager.h"
#include "object/sp-namedview.h"
#include "object/sp-page.h"
#include "object/sp-root.h"
#include "preferences.h"
#include "selection.h"
#include "ui/desktop/menubar.h"
#include "ui/dialog-events.h"
#include "ui/dialog-run.h"
#include "ui/pack.h"
#include "ui/tool/control-point-selection.h"
#include "ui/tool/path-manipulator.h"
#include "ui/tools/node-tool.h"
#include "ui/util.h"
#include "xml/event.h"
#include "xml/rebase-hrefs.h"
#include "xml/repr.h"
#include "xml/simple-document.h"

// This code could maybe be reworked to use GIO's subprocess classes to
// avoid the need to use these platform-specific defines to manipulate the subprocess.
// That might not have been an option when this was written
#ifdef _WIN32
#include <windows.h>
#define KILL_PROCESS(pid) TerminateProcess(pid, 0)
#define WAIT_PROCESS(pid) WaitForSingleObject(pid, INFINITE)
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#define KILL_PROCESS(pid) kill(pid, SIGTERM)
#define WAIT_PROCESS(pid) waitpid(pid, NULL, 0)
#endif

namespace Inkscape::Extension::Implementation {

/** \brief  A table of what interpreters to call for a given language

    This table is used to keep track of all the programs to execute a
    given script.  It also tracks the preference to use to overwrite
    the given interpreter to a custom one per user.
*/
const std::map<std::string, Script::interpreter_t> Script::interpreterTab = {
    // clang-format off
#ifdef _WIN32
    { "perl",    {"perl-interpreter",    {"wperl"             }}},
    { "python",  {"python-interpreter",  {"pythonw"           }}},
#elif defined __APPLE__
    { "perl",    {"perl-interpreter",    {"perl"              }}},
    { "python",  {"python-interpreter",  {"python3"           }}},
#else
    { "perl",    {"perl-interpreter",    {"perl"              }}},
    { "python",  {"python-interpreter",  {"python3", "python" }}},
#endif
    { "python2", {"python2-interpreter", {"python2", "python" }}},
    { "ruby",    {"ruby-interpreter",    {"ruby"    }}},
    { "shell",   {"shell-interpreter",   {"sh"      }}},
    // clang-format on
};



/** \brief Look up an interpreter name, and translate to something that
    is executable
    \param interpNameArg  The name of the interpreter that we're looking
    for, should be an entry in interpreterTab
*/
std::string Script::resolveInterpreterExecutable(const Glib::ustring &interpNameArg)
{
    // 0. Do we have a supported interpreter type?
    auto interp = interpreterTab.find(interpNameArg);
    if (interp == interpreterTab.end()) {
        g_critical("Script::resolveInterpreterExecutable(): unknown script interpreter '%s'", interpNameArg.c_str());
        return "";
    }

    std::list<Glib::ustring> searchList;
    std::copy(interp->second.defaultvals.begin(), interp->second.defaultvals.end(), std::back_inserter(searchList));

    // 1. Check preferences for an override.
    auto prefs = Inkscape::Preferences::get();
    auto prefInterp = prefs->getString("/extensions/" + Glib::ustring(interp->second.prefstring));

    if (!prefInterp.empty()) {
	searchList.push_front(prefInterp);
    }

    // 2. Search for things in the path if they're there or an absolute
    for (const auto& binname : searchList) {
        auto interpreter_path = Glib::filename_from_utf8(binname);

        if (!Glib::path_is_absolute(interpreter_path)) {
            auto found_path = Glib::find_program_in_path(interpreter_path);
            if (!found_path.empty()) {
                return found_path;
            }
        } else {
            return interpreter_path;
        }
    }

    // 3. Error
    g_critical("Script::resolveInterpreterExecutable(): failed to locate script interpreter '%s'", interpNameArg.c_str());
    return "";
}

/** \brief     This function creates a script object and sets up the
               variables.
    \return    A script object

   This function just sets the command to NULL.  It should get built
   officially in the load function.  This allows for less allocation
   of memory in the unloaded state.
*/
Script::Script()
    : Implementation()
    , _canceled(false)
    , parent_window(nullptr)
{
}

/**
 *   \brief     Destructor
 */
Script::~Script()
= default;


/**
    \return   none
    \brief    This function 'loads' an extension, basically it determines
              the full command for the extension and stores that.
    \param    module  The extension to be loaded.

    The most difficult part about this function is finding the actual
    command through all of the Reprs.  Basically it is hidden down a
    couple of layers, and so the code has to move down too.  When
    the command is actually found, it has its relative directory
    solved.

    At that point all of the loops are exited, and there is an
    if statement to make sure they didn't exit because of not finding
    the command.  If that's the case, the extension doesn't get loaded
    and should error out at a higher level.
*/

bool Script::load(Inkscape::Extension::Extension *module)
{
    if (module->loaded()) {
        return true;
    }

    helper_extension = "";

    /* This should probably check to find the executable... */
    Inkscape::XML::Node *child_repr = module->get_repr()->firstChild();
    while (child_repr != nullptr) {
        if (!strcmp(child_repr->name(), INKSCAPE_EXTENSION_NS "script")) {
            for (child_repr = child_repr->firstChild(); child_repr != nullptr; child_repr = child_repr->next()) {
                if (!strcmp(child_repr->name(), INKSCAPE_EXTENSION_NS "command")) {
                    const gchar *interpretstr = child_repr->attribute("interpreter");
                    if (interpretstr != nullptr) {
                        std::string interpString = resolveInterpreterExecutable(interpretstr);
                        if (interpString.empty()) {
                            continue; // can't have a script extension with empty interpreter
                        }
                        command.push_back(interpString);
                    }
                    // TODO: we already parse commands as dependencies in extension.cpp
                    //       can can we optimize this to be less indirect?
                    const char *script_name = child_repr->firstChild()->content();
                    std::string script_location = module->get_dependency_location(script_name);
                    command.push_back(std::move(script_location));
                } else if (!strcmp(child_repr->name(), INKSCAPE_EXTENSION_NS "helper_extension")) {
                    helper_extension = child_repr->firstChild()->content();
                }
            }

            break;
        }
        child_repr = child_repr->next();
    }

    // TODO: Currently this causes extensions to fail silently, see comment in Extension::set_state()
    g_return_val_if_fail(command.size() > 0, false);

    return true;
}


/**
    \return   None.
    \brief    Unload this puppy!
    \param    module  Extension to be unloaded.

    This function just sets the module to unloaded.  It free's the
    command if it has been allocated.
*/
void Script::unload(Inkscape::Extension::Extension */*module*/)
{
    command.clear();
    helper_extension = "";
}




/**
    \return   Whether the check passed or not
    \brief    Check every dependency that was given to make sure we should keep this extension
    \param    module  The Extension in question

*/
bool Script::check(Inkscape::Extension::Extension *module)
{
    int script_count = 0;
    Inkscape::XML::Node *child_repr = module->get_repr()->firstChild();
    while (child_repr != nullptr) {
        if (!strcmp(child_repr->name(), INKSCAPE_EXTENSION_NS "script")) {
            script_count++;

            // check if all helper_extensions attached to this script were registered
            child_repr = child_repr->firstChild();
            while (child_repr != nullptr) {
                if (!strcmp(child_repr->name(), INKSCAPE_EXTENSION_NS "helper_extension")) {
                    gchar const *helper = child_repr->firstChild()->content();
                    if (Inkscape::Extension::db.get(helper) == nullptr) {
                        return false;
                    }
                }

                child_repr = child_repr->next();
            }

            break;
        }
        child_repr = child_repr->next();
    }

    if (script_count == 0) {
        return false;
    }

    return true;
}

/**
 * Create a new document based on the given template.
 */
std::unique_ptr<SPDocument> Script::new_from_template(Inkscape::Extension::Template *module)
{
    std::list<std::string> params;
    module->paramListString(params);
    module->set_environment();

    if (auto in_file = module->get_template_filename()) {
        file_listener fileout;
        execute(command, params, in_file->get_path(), fileout);
        auto svg = fileout.string();
        auto rdoc = sp_repr_read_mem(svg.c_str(), svg.length(), SP_SVG_NS_URI);
        if (rdoc) {
            auto name = Glib::ustring::compose(_("New document %1"), SPDocument::get_new_doc_number());
            return SPDocument::createDoc(rdoc, nullptr, nullptr, name.c_str());
        }
    }

    return nullptr;
}

/**
 * Take an existing document and selected page and resize or add items as needed.
 */
void Script::resize_to_template(Inkscape::Extension::Template *tmod, SPDocument *doc, SPPage *page)
{
    std::list<std::string> params;
    {
        std::string param = "--page=";
        if (page) {
            param += page->getId();
        } else {
            // This means 'resize the svg document'
            param += doc->getRoot()->getId();
        }
        params.push_back(param);
    }
    _change_extension(tmod, nullptr, doc, params, true);
}

/**
    \return  A new document that has been opened
    \brief   This function uses a filename that is put in, and calls
             the extension's command to create an SVG file which is
             returned.
    \param   module   Extension to use.
    \param   filename File to open.

    First things first, this function needs a temporary file name.  To
    create one of those the function Glib::file_open_tmp is used with
    the header of ink_ext_.

    The extension is then executed using the 'execute' function
    with the filename assigned and then the temporary filename.
    After execution the SVG should be in the temporary file.

    Finally, the temporary file is opened using the SVG input module and
    a document is returned.  That document has its filename set to
    the incoming filename (so that it's not the temporary filename).
    That document is then returned from this function.
*/
std::unique_ptr<SPDocument> Script::open(Inkscape::Extension::Input *module, char const *filenameArg, bool)
{
    std::list<std::string> params;
    module->paramListString(params);
    module->set_environment();

    std::string tempfilename_out;
    int tempfd_out = 0;
    try {
        tempfd_out = Glib::file_open_tmp(tempfilename_out, "ink_ext_XXXXXX.svg");
    } catch (...) {
        /// \todo Popup dialog here
        return nullptr;
    }

    std::string lfilename = Glib::filename_from_utf8(filenameArg);

    file_listener fileout;
    int data_read = execute(command, params, lfilename, fileout);
    fileout.toFile(tempfilename_out);

    std::unique_ptr<SPDocument> mydoc;
    if (data_read > 10) {
        if (helper_extension.size()==0) {
            mydoc = Inkscape::Extension::open(
                  Inkscape::Extension::db.get(SP_MODULE_KEY_INPUT_SVG),
                  tempfilename_out.c_str());
        } else {
            mydoc = Inkscape::Extension::open(
                  Inkscape::Extension::db.get(helper_extension.c_str()),
                  tempfilename_out.c_str());
        }
    } // data_read

    if (mydoc) {
        mydoc->setDocumentBase(nullptr);
        mydoc->changeFilenameAndHrefs(filenameArg);
    }

    // make sure we don't leak file descriptors from Glib::file_open_tmp
    close(tempfd_out);

    unlink(tempfilename_out.c_str());

    return mydoc;
} // open



/**
    \return   none
    \brief    This function uses an extension to save a document.  It first
              creates an SVG file of the document, and then runs it through
              the script.
    \param    module    Extension to be used
    \param    doc       Document to be saved
    \param    filename  The name to save the final file as
    \return   false in case of any failure writing the file, otherwise true

    Well, at some point people need to save - it is really what makes
    the entire application useful.  And, it is possible that someone
    would want to use an extension for this, so we need a function to
    do that, eh?

    First things first, the document is saved to a temporary file that
    is an SVG file.  To get the temporary filename Glib::file_open_tmp is used with
    ink_ext_ as a prefix.  Don't worry, this file gets deleted at the
    end of the function.

    After we have the SVG file, then Script::execute is called with
    the temporary file name and the final output filename.  This should
    put the output of the script into the final output file.  We then
    delete the temporary file.
*/
void Script::save(Inkscape::Extension::Output *module,
             SPDocument *doc,
             const gchar *filenameArg)
{
    std::list<std::string> params;
    module->paramListString(params);
    module->set_environment(doc);

    std::string tempfilename_in;
    int tempfd_in = 0;
    try {
        tempfd_in = Glib::file_open_tmp(tempfilename_in, "ink_ext_XXXXXX.svg");
    } catch (...) {
        /// \todo Popup dialog here
        throw Inkscape::Extension::Output::save_failed();
    }

    if (helper_extension.size() == 0) {
        Inkscape::Extension::save(
                   Inkscape::Extension::db.get(SP_MODULE_KEY_OUTPUT_SVG_INKSCAPE),
                   doc, tempfilename_in.c_str(), false, false,
                   Inkscape::Extension::FILE_SAVE_METHOD_TEMPORARY);
    } else {
        Inkscape::Extension::save(
                   Inkscape::Extension::db.get(helper_extension.c_str()),
                   doc, tempfilename_in.c_str(), false, false,
                   Inkscape::Extension::FILE_SAVE_METHOD_TEMPORARY);
    }


    file_listener fileout;
    int data_read = execute(command, params, tempfilename_in, fileout);

    bool success = false;

    if (data_read > 0) {
        std::string lfilename = Glib::filename_from_utf8(filenameArg);
        success = fileout.toFile(lfilename);
    }

    // make sure we don't leak file descriptors from Glib::file_open_tmp
    close(tempfd_in);
    // FIXME: convert to utf8 (from "filename encoding") and unlink_utf8name
    unlink(tempfilename_in.c_str());

    if (success == false) {
        throw Inkscape::Extension::Output::save_failed();
    }
}


void Script::export_raster(Inkscape::Extension::Output *module,
             const SPDocument *doc,
             const std::string &png_file,
             const gchar *filenameArg)
{
    if(!module->is_raster()) {
        g_error("Can not export raster to non-raster extension.");
        return;
    }

    std::list<std::string> params;
    module->paramListString(params);
    module->set_environment(doc);

    file_listener fileout;
    int data_read = execute(command, params, png_file, fileout);

    bool success = false;
    if (data_read > 0) {
        std::string lfilename = Glib::filename_from_utf8(filenameArg);
        success = fileout.toFile(lfilename);
    }
    if (success == false) {
        throw Inkscape::Extension::Output::save_failed();
    }
}

void Script::_setAppSensitive(bool sensitive)
{
    if (!INKSCAPE.use_gui()) {
        return;
    }

    auto application = InkscapeApplication::instance()->gtk_app();
    for (auto const &win : application->get_windows()) {
        win->set_sensitive(sensitive);
    }

#ifdef __APPLE__
    // On macOS, hide or show our global menubar to avoid users interacting with it while
    // insensitive. We could try to disable all the actions in the menu, but that would get a bit
    // messy as we keep track of which were already disabled, and would prevent the extension
    // script from being able to activate those actions on the CLI.
    if (sensitive) {
        build_menu();
    } else {
        application->set_menubar(nullptr);
    }
#endif
}

/**
    \return    none
    \brief     This function uses an extension as an effect on a document.
    \param     module         Extension to effect with.
    \param     executionEnv   Current execution environment.
    \param     desktop        Desktop this extensions run on.
    \param     doc            Document to run through the effect.

    This function is a little bit trickier than the previous two.  It
    needs two temporary files to get its work done.  Both of these
    files have random names created for them using the Glib::file_open_temp function
    with the ink_ext_ prefix in the temporary directory.  Like the other
    functions, the temporary files are deleted at the end.

    To save/load the two temporary documents (both are SVG) the internal
    modules for SVG load and save are used.  They are both used through
    the module system function by passing their keys into the functions.

    The command itself is built a little bit differently than in other
    functions because the effect support selections.  So on the command
    line a list of all the ids that are selected is included.  Currently,
    this only works for a single selected object, but there will be more.
    The command string is filled with the data, and then after the execution
    it is freed.

    The execute function is used at the core of this function
    to execute the Script on the two SVG documents (actually only one
    exists at the time, the other is created by that script).  At that
    point both should be full, and the second one is loaded.
*/
void Script::effect(Inkscape::Extension::Effect *module, ExecutionEnv *executionEnv, SPDesktop *desktop,
                    ImplementationDocumentCache *docCache)
{
    if (desktop == nullptr)
    {
        g_warning("Script::effect: Desktop not defined");
        return;
    }

    sp_namedview_document_from_window(desktop);

    if (module->no_doc) {
        // this is a no-doc extension, e.g. a Help menu command;
        // just run the command without any files, ignoring errors

        std::list<std::string> params;
        module->paramListString(params);
        module->set_environment(desktop->getDocument());

        Glib::ustring empty;
        file_listener outfile;
        execute(command, {}, empty, outfile, module->ignore_stderr, module->pipe_diffs, !module->_workingDialog);

        // Hack to allow for extension manager to reload extensions
        // TODO: Have the extension manager call this action itself
        if (!g_strcmp0(module->get_id(), "org.inkscape.extension.manager")) {
            InkscapeApplication::instance()->gio_app()->activate_action("refresh-user-extensions");
        }

        return;
    }

    std::list<std::string> params;
    if (desktop) {
        if (auto selection = desktop->getSelection()) {
            // Get current selection state
            auto state = selection->getState();

            // Add selected object IDs
            for (auto const &id : state.selected_ids) {
                std::string selected_id = "--id=";
                selected_id += id;
                params.push_back(std::move(selected_id));
            }

            // Add selected nodes
            for (auto const &node : state.selected_nodes) {
                params.push_back(Glib::ustring::compose("--selected-nodes=%1:%2:%3", node.path_id, node.subpath_index,
                                                        node.node_index));
            }
        }
    }
    _change_extension(module, executionEnv, desktop->getDocument(), params, module->ignore_stderr, module->pipe_diffs, !module->_workingDialog);
}

/**
 * Pure document version for calling an extension from the command line
 */
void Script::effect(Inkscape::Extension::Effect *mod, ExecutionEnv *executionEnv, SPDocument *document)
{
    std::list<std::string> params;
    _change_extension(mod, executionEnv, document, params, mod->ignore_stderr);
}

/**
 * Conservative budget for the maximum command-line size
 * Used to decide when to spill arguments to a file (see _change_extension).
 */
static size_t get_cmdline_budget()
{
#ifdef _WIN32
    // Windows: Length limit is fixed at 32767
    // Leave 4096 of overhead for executable and files (rough overestimate)
    return 32767 - 4096;
#else
    // Unix: Length limit is from sysconf
    // Leave 32K of overhead for executable, files, and environment (rough overestimate)
    // Also guard against sysconf failure (-1)
    long const arg_max = sysconf(_SC_ARG_MAX);
    size_t usable = 0;
    if (arg_max > 32768) {
        usable = static_cast<size_t>(arg_max) - 32768;
    }
    return std::max(size_t{_POSIX_ARG_MAX}, usable);
#endif
}

/**
 * Internally, any modification of an existing document, used by effect and resize_page extensions.
 */
void Script::_change_extension(Inkscape::Extension::Extension *module, ExecutionEnv *executionEnv, SPDocument *doc,
                               std::list<std::string> &params, bool ignore_stderr, bool pipe_diffs, bool custom_ui)
{
    module->paramListString(params);
    module->set_environment(doc);

    if (executionEnv) {
        parent_window = executionEnv->get_working_dialog();
    }

    auto tempfile_out = Inkscape::IO::TempFilename("ink_ext_XXXXXX.svg");
    auto tempfile_in = Inkscape::IO::TempFilename("ink_ext_XXXXXX.svg");

    // Save current document to a temporary file we can send to the extension
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool("/options/svgoutput/disable_optimizations", true);
    Inkscape::Extension::save(
              Inkscape::Extension::db.get(SP_MODULE_KEY_OUTPUT_SVG_INKSCAPE),
              doc, tempfile_in.get_filename().c_str(), false, false,
              Inkscape::Extension::FILE_SAVE_METHOD_TEMPORARY);
    prefs->setBool("/options/svgoutput/disable_optimizations", false);

    // Workaround for command-line length limits
    // With thousands of arguments the accumulated command line overflows
    // Spill into a temp file and replace with a single --arg-file=<path>.
    auto tempfile_sel = Inkscape::IO::TempFilename("ink_sel_XXXXXX.txt");
    size_t cmdline_size = 0;
    for (auto const &p : params) {
        cmdline_size += p.size() + 1;
    }
    if (cmdline_size > get_cmdline_budget()) {
        try {
            auto sel_channel = Glib::IOChannel::create_from_file(tempfile_sel.get_filename(), "w");
            sel_channel->set_encoding("UTF-8");
            for (auto const &p : params) {
                sel_channel->write(p + "\n");
            }
            sel_channel->close();
            params = {"--arg-file=" + tempfile_sel.get_filename()};
        } catch (Glib::Error const &e) {
            g_warning("Script::_change_extension(): failed to spill args: %s", e.what());
        }
    }

    file_listener fileout;
    int data_read = execute(command, params, tempfile_in.get_filename(), fileout, ignore_stderr, pipe_diffs, custom_ui);
    if (data_read == 0) {
        return;
    }
    fileout.toFile(tempfile_out.get_filename());

    Inkscape::XML::Document *new_xmldoc = nullptr;
    if (data_read > 10) {
        new_xmldoc = sp_repr_read_file(tempfile_out.get_filename().c_str(), SP_SVG_NS_URI);
    } // data_read

    if (new_xmldoc) {
        //uncomment if issues on ref extensions links (with previous function)
        //sp_change_hrefs(new_xmldoc, tempfile_out.get_filename().c_str(), doc->getDocumentFilename());
        doc->rebase(new_xmldoc);
    } else {
        Inkscape::UI::gui_warning(_("The output from the extension could not be parsed."), parent_window);
    }
}

/**  \brief  This function checks the stderr file, and if it has data,
             shows it in a warning dialog to the user
     \param  filename  Filename of the stderr file
*/
void Script::showPopupError (const Glib::ustring &data,
                             Gtk::MessageType type,
                             const Glib::ustring &message)
{
    Gtk::MessageDialog warning(message, false, type, Gtk::ButtonsType::OK, true);
    warning.set_resizable(true);
    if (parent_window) {
        warning.set_transient_for(*parent_window);
    } else {
        sp_transientize(warning);
    }

    auto const textview = Gtk::make_managed<Gtk::TextView>();
    textview->set_editable(false);
    textview->set_wrap_mode(Gtk::WrapMode::WORD);
    textview->get_buffer()->set_text(data);

    auto const scrollwindow = Gtk::make_managed<Gtk::ScrolledWindow>();
    scrollwindow->set_child(*textview);

    scrollwindow->set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
    scrollwindow->set_has_frame(true);
    scrollwindow->set_size_request(0, 60);

    auto const vbox = warning.get_content_area();
    UI::pack_start(*vbox, *scrollwindow, true, true, 5 /* fix these */);

    Inkscape::UI::dialog_run(warning);
}

bool Script::cancelProcessing () {
    KILL_PROCESS(_pid);

    _canceled = true;
    if (_main_loop) {
        _main_loop->quit();
    }

    return true;
}

/** \brief    This is the core of the extension file as it actually does
              the execution of the extension.
    \param    in_command  The command to be executed
    \param    filein      Filename coming in
    \param    fileout     Filename of the out file
    \return   Number of bytes that were read into the output file.

    The first thing that this function does is build the command to be
    executed.  This consists of the first string (in_command) and then
    the filename for input (filein).  This file is put on the command
    line.

    The next thing that this function does is open a pipe to the
    command and get the file handle in the ppipe variable.  It then
    opens the output file with the output file handle.  Both of these
    operations are checked extensively for errors.

    After both are opened, then the data is copied from the output
    of the pipe into the file out using \a fread and \a fwrite.  These two
    functions are used because of their primitive nature - they make
    no assumptions about the data.  A buffer is used in the transfer,
    but the output of \a fread is stored so the exact number of bytes
    is handled gracefully.

    At the very end (after the data has been copied) both of the files
    are closed, and we return to what we were doing.
*/
int Script::execute(std::list<std::string> const &in_command, std::list<std::string> const &in_params,
                    Glib::ustring const &filein, file_listener &fileout, bool ignore_stderr, bool pipe_diffs,
                    bool custom_ui)
{
    g_return_val_if_fail(!in_command.empty(), 0);

    std::vector<std::string> argv;

    bool interpreted = (in_command.size() == 2);
    std::string program = in_command.front();
    std::string script = interpreted ? in_command.back() : "";
    std::string working_directory = "";

    auto const desktop = SP_ACTIVE_DESKTOP;
    auto const document = desktop ? desktop->doc() : nullptr;
    if (!document) {
        pipe_diffs = false; // pipe_diffs mode requires a desktop and document to attach to
    }

    // We should always have an absolute path here:
    //  - For interpreted scripts, see Script::resolveInterpreterExecutable()
    //  - For "normal" scripts this should be done as part of the dependency checking, see Dependency::check()
    if (!Glib::path_is_absolute(program)) {
        g_critical("Script::execute(): Got unexpected relative path '%s'. Please report a bug.", program.c_str());
        return 0;
    }
    argv.push_back(program);

    if (interpreted) {
        // On Windows, Python garbles Unicode command line parameters
        // in an useless way. This means extensions fail when Inkscape
        // is run from an Unicode directory.
        // As a workaround, we set the working directory to the one
        // containing the script.
        working_directory = Glib::path_get_dirname(script);
        script = Glib::path_get_basename(script);
        argv.push_back(script);
    }

    // assemble the rest of argv
    std::copy(in_params.begin(), in_params.end(), std::back_inserter(argv));
    if (!filein.empty()) {
        auto filein_native = Glib::filename_from_utf8(filein);
        if (!Glib::path_is_absolute(filein_native))
            filein_native = Glib::build_filename(Glib::get_current_dir(), filein_native);
        argv.push_back(filein_native);
    }

    int stdout_pipe, stderr_pipe, stdin_pipe;

    GPid local_pid;
    try {
        // DO_NOT_REAP_CHILD is required on Windows at least to get the PID from
        // spawn_async_with_pipes.
        auto spawn_flags = Glib::SpawnFlags::DO_NOT_REAP_CHILD;
        if (Glib::getenv("SNAP") != "") {
            // If we are running within the Linux "snap" package format,
            // we need different spawn flags to avoid that Inkscape hangs when
            // starting an extension.
            spawn_flags = Glib::SpawnFlags::LEAVE_DESCRIPTORS_OPEN;
        }
        Glib::spawn_async_with_pipes(working_directory, // working directory
                                     argv,              // arg v
                                     spawn_flags,       // spawn flags
                                     sigc::slot<void()>(),
                                     &local_pid,         // Pid
                                     &stdin_pipe,   // STDIN
                                     &stdout_pipe,  // STDOUT
                                     &stderr_pipe); // STDERR
    } catch (Glib::Error const &e) {
        g_critical("Script::execute(): failed to execute program '%s'.\n\tReason: %s", program.c_str(), e.what());
        return 0;
    }

    // Save the pid. (This function is reentrant, so _pid could be overwritten.)
    _pid = local_pid;

    // Use GTK's MainContext, so that it can process events and the UI is not flagged as frozen.
    // But we'll set all windows as insensitive below while we run, to avoid any race conditions
    // from interacting with Inkscape while an extension is running (like quitting before an
    // extension can finish writing a file).
    _main_loop = Glib::MainLoop::create();

    file_listener fileerr;
    fileout.init(stdout_pipe, _main_loop);
    fileerr.init(stderr_pipe, _main_loop);

    std::optional<PreviewObserver> watch;
    bool lost_document = false;
    std::vector<sigc::scoped_connection> conns;

    if (pipe_diffs) {
        auto stdin_channel = Glib::IOChannel::create_from_fd(stdin_pipe);
        stdin_channel->set_close_on_unref(false);
        stdin_channel->set_encoding();
#ifndef _WIN32
        // does not seem to be needed on Windows
        stdin_channel->set_flags(static_cast<Glib::IOFlags>(G_IO_FLAG_NONBLOCK));
#endif
        stdin_channel->set_buffered(false);

        watch.emplace(std::move(stdin_channel));
        (*watch).connect(desktop, document);
        auto on_lose_document = [&] {
            KILL_PROCESS(local_pid);
            (*watch).disconnect(document);
            lost_document = true;
            conns.clear();
        };
        conns.emplace_back(desktop->connectDestroy([=] (auto...) { on_lose_document(); }));
        conns.emplace_back(desktop->connectDocumentReplaced([=] (auto...) { on_lose_document(); }));
        conns.emplace_back(document->connectDestroy(on_lose_document));
    }

    if (!pipe_diffs) {
        _setAppSensitive(false);
    }
    _canceled = false;
    _main_loop->run();

    if (pipe_diffs && !lost_document) {
        (*watch).disconnect(document);
    }

    // Ensure all the data is out of the pipe
    while (!fileout.isDead()) {
        fileout.read(Glib::IOCondition::IO_IN);
    }
    while (!fileerr.isDead()) {
        fileerr.read(Glib::IOCondition::IO_IN);
    }

    Glib::spawn_close_pid(local_pid);

    _main_loop.reset();
    if (!pipe_diffs) {
        _setAppSensitive(true);
    }

    WAIT_PROCESS(local_pid);
    if (_canceled) {
        // std::cout << "Script Canceled" << std::endl;
        return 0;
    }

    if (pipe_diffs && lost_document) {
        throw Inkscape::Extension::Output::lost_document{};
    }

    Glib::ustring stderr_data = fileerr.string();
    if (!stderr_data.empty()) {
        // If we are using a GUI, flag the error to the user if asked, otherwise just print to the console for help
        // when debugging / reporting a bug. (Unless we are in non-GUI mode, in which case we only print if asked.)
        if (INKSCAPE.use_gui() && !ignore_stderr) {
            showPopupError(stderr_data, Gtk::MessageType::INFO,
                                 _("Inkscape has received additional data from the script executed.  "
                                   "The script did not return an error, but this may indicate the results will not be as expected."));
        } else if (INKSCAPE.use_gui() || !ignore_stderr) {
            std::cerr << "Script Error\n----\n" << stderr_data.c_str() << "\n----\n";
        }
    }

    Glib::ustring stdout_data = fileout.string();
    return stdout_data.length();
}

Script::file_listener::~file_listener() = default;

void Script::file_listener::init(int fd, Glib::RefPtr<Glib::MainLoop> main) {
    _channel = Glib::IOChannel::create_from_fd(fd);
    _channel->set_close_on_unref(true);
    _channel->set_encoding();
    _conn = main->get_context()->signal_io().connect(sigc::mem_fun(*this, &file_listener::read), _channel, Glib::IOCondition::IO_IN | Glib::IOCondition::IO_HUP | Glib::IOCondition::IO_ERR);
    _main_loop = main;
}

bool Script::file_listener::read(Glib::IOCondition condition) {
    if (condition != Glib::IOCondition::IO_IN) {
        _main_loop->quit();
        return false;
    }

    Glib::IOStatus status;
    Glib::ustring out;
    status = _channel->read_line(out);
    _string += out;

    if (status != Glib::IOStatus::NORMAL) {
        _main_loop->quit();
        _dead = true;
        return false;
    }

    return true;
}

/**
 * @param name File path.
 *             Value is in UTF8 encoding.
 */
bool Script::file_listener::toFile(const Glib::ustring &name) {
    return toFile(Glib::filename_from_utf8(name));
}

/**
 * @param name File path.
 *             Value is in platform-native encoding (see Glib::filename_to_utf8).
 */
bool Script::file_listener::toFile(const std::string &name) {
    try {
        Glib::RefPtr<Glib::IOChannel> stdout_file = Glib::IOChannel::create_from_file(name, "w");
        stdout_file->set_encoding();
        stdout_file->write(_string);
    } catch (Glib::FileError &e) {
        return false;
    }
    return true;
}

Script::PreviewObserver::PreviewObserver(Glib::RefPtr<Glib::IOChannel> channel)
    : _channel{std::move(channel)}
{}

void Script::PreviewObserver::connect(SPDesktop const *desktop, SPDocument *document)
{
    document->addUndoObserver(*this);
    auto selection = desktop->getSelection();
    _select_changed =
        selection->connectChanged([this](Inkscape::Selection *selection) { selectionChanged(selection); });
    // We don't want to spam deselect / select events
    // while document reconstruction is ongoing.
    // The selection is restored after the reconstruction, so
    // we will emit an event there anyway.
    _reconstruction_start_connection =
        document->connectReconstructionStart([this]() { _pause_select_events = true; }, true);
    _reconstruction_finish_connection =
        document->connectReconstructionFinish([this]() { _pause_select_events = false; });
}

void Script::PreviewObserver::disconnect(SPDocument *document)
{
    document->removeUndoObserver(*this);
    _select_changed.disconnect();
    _reconstruction_start_connection.disconnect();
    _reconstruction_finish_connection.disconnect();
}

void Script::PreviewObserver::createAndSendEvent(
    std::function<void(Inkscape::XML::Document *, Inkscape::XML::Node *)> const &eventPopulator)
{
    Inkscape::XML::Document *doc = new Inkscape::XML::SimpleDocument();
    Inkscape::XML::Node *event_node = doc->createElement("event");
    doc->addChildAtPos(event_node, 0);
    Inkscape::GC::release(event_node);

    eventPopulator(doc, event_node);

    Glib::ustring xml_output = sp_repr_write_buf(event_node, 0, true, GQuark(0), 0, 0);
    _channel->write(xml_output + "\n");

    Inkscape::GC::release(doc);
    delete doc;
}

void Script::PreviewObserver::selectionChanged(Inkscape::Selection *selection)
{
    if (_pause_select_events) {
        return;
    }
    createAndSendEvent([&](Inkscape::XML::Document *doc, Inkscape::XML::Node *event_node) {
        event_node->setAttribute("type", "updateSelection");
        for (auto objsel : selection->objects()) {
            Inkscape::XML::Node *item = event_node->document()->createElement("selObj");
            item->setAttribute("id", objsel->getId());
            event_node->appendChild(item);
            Inkscape::GC::release(item);
        }
    });
}

void Script::PreviewObserver::notifyUndoCommitEvent(Event *ee)
{
    std::vector<XML::Event *> events;

    // First collect all events
    for (auto e = ee->event; e; e = e->next) {
        events.push_back(e);
    }

    // Process events in reverse order (chronological order)
    for (auto e : events | std::views::reverse) {
        createAndSendEvent([&](Inkscape::XML::Document *doc, Inkscape::XML::Node *event_node) {
            if (auto eadd = dynamic_cast<XML::EventAdd *>(e)) {
                event_node->setAttribute("type", "add");
                if (eadd->ref) {
                    event_node->setAttribute("after", eadd->ref->attribute("id"));
                }
                if (eadd->child) {
                    Inkscape::XML::Node *new_child = eadd->child->duplicate(doc);

                    event_node->appendChild(new_child);
                    Inkscape::GC::release(new_child);
                }
                if (eadd->repr) {
                    event_node->setAttribute("parent", eadd->repr->attribute("id"));
                }
            } else if (auto edel = dynamic_cast<XML::EventDel *>(e)) {
                event_node->setAttribute("type", "delete");
                if (edel->repr && edel->repr->attribute("id")) {
                    event_node->setAttribute("parent", edel->repr->attribute("id"));
                }
                if (edel->ref) {
                    event_node->setAttribute("after", edel->ref->attribute("id"));
                }
                if (edel->child) {
                    event_node->setAttribute("child", edel->child->attribute("id"));
                }
            } else if (auto echga = dynamic_cast<XML::EventChgAttr *>(e)) {
                event_node->setAttribute("type", "attribute_change");
                if (echga->repr && e->repr->attribute("id")) {
                    event_node->setAttribute("element-id", echga->repr->attribute("id"));
                }
                event_node->setAttribute("attribute-name", g_quark_to_string(echga->key));
                event_node->setAttribute("old-value", &*(echga->oldval));
                event_node->setAttribute("new-value", &*(echga->newval));
            } else if (auto echgc = dynamic_cast<XML::EventChgContent *>(e)) {
                event_node->setAttribute("type", "content_change");
                if (e->repr && e->repr->attribute("id")) {
                    event_node->setAttribute("element-id", e->repr->attribute("id"));
                }
                event_node->setAttribute("old-content", &*(echgc->oldval));
                event_node->setAttribute("new-content", &*(echgc->newval));
            } else if (auto echgo = dynamic_cast<XML::EventChgOrder *>(e)) {
                event_node->setAttribute("type", "order_change");
                if (echgo->repr && echgo->repr->attribute("id")) {
                    event_node->setAttribute("element-id", e->repr->attribute("id"));
                }
                event_node->setAttribute("child", echgo->child->attribute("id"));
                if (echgo->oldref) {
                    event_node->setAttribute("old-ref", echgo->oldref->attribute("id"));
                }
                if (echgo->newref) {
                    event_node->setAttribute("new-ref", echgo->newref->attribute("id"));
                }
            } else if (auto echgn = dynamic_cast<XML::EventChgElementName *>(e)) {
                event_node->setAttribute("type", "element_name_change");
                if (echgn->repr && echgn->repr->attribute("id")) {
                    event_node->setAttribute("element-id", e->repr->attribute("id"));
                }
                event_node->setAttribute("old-name", g_quark_to_string(echgn->old_name));
                event_node->setAttribute("new-name", g_quark_to_string(echgn->new_name));
            } else {
                event_node->setAttribute("type", "unknown");
            }
        });
    }
}

void Script::PreviewObserver::notifyUndoEvent(Event *e)
{
    notifyUndoCommitEvent(e);
}

void Script::PreviewObserver::notifyRedoEvent(Event *e)
{
    notifyUndoCommitEvent(e);
}

void Script::PreviewObserver::notifyClearUndoEvent()
{
    // do nothing
}

void Script::PreviewObserver::notifyClearRedoEvent()
{
    // do nothing
}

void Script::PreviewObserver::notifyUndoExpired(Event *e)
{
    // do nothing
}

} // namespace Inkscape::Extension::Implementation

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :

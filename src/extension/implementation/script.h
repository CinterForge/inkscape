// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Code for handling extensions (i.e., scripts)
 *
 * Authors:
 *   Bryce Harrington <bryce@osdl.org>
 *   Ted Gould <ted@gould.cx>
 *
 * Copyright (C) 2002-2005 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_EXTENSION_IMPEMENTATION_SCRIPT_H_SEEN
#define INKSCAPE_EXTENSION_IMPEMENTATION_SCRIPT_H_SEEN

#include <list>
#include <map>
#include <string>
#include <vector>
#include <glibmm/iochannel.h>
#include <glibmm/refptr.h>
#include <glibmm/spawn.h>
#include <glibmm/ustring.h>
#include <gtkmm/enums.h>
#include <sigc++/scoped_connection.h>

#include "implementation.h"
#include "selection.h"
#include "undo-stack-observer.h"
#include "xml/node.h"

namespace Glib {
class MainLoop;
} // namespace Glib

namespace Gtk {
class Window;
} // namespace Gtk

namespace Inkscape {

namespace XML {
class Node;
} // namespace XML

namespace Extension::Implementation {

/**
 * Utility class used for loading and launching script extensions
 */
class Script : public Implementation {
public:
    Script();
    ~Script() override;

    bool load(Inkscape::Extension::Extension *module) override;
    void unload(Inkscape::Extension::Extension *module) override;
    bool check(Inkscape::Extension::Extension *module) override;

    std::unique_ptr<SPDocument> new_from_template(Inkscape::Extension::Template *module) override;
    void resize_to_template(Inkscape::Extension::Template *tmod, SPDocument *doc, SPPage *page) override;

    std::unique_ptr<SPDocument> open(Inkscape::Extension::Input *module, char const *filename, bool is_importing) override;
    void save(Inkscape::Extension::Output *module, SPDocument *doc, gchar const *filename) override;
    void export_raster(Inkscape::Extension::Output *module,
            const SPDocument *doc, std::string const &png_file, gchar const *filename) override;
    void effect(Inkscape::Extension::Effect *module, ExecutionEnv *executionEnv, SPDesktop *desktop,
                ImplementationDocumentCache *docCache) override;
    void effect(Inkscape::Extension::Effect *module, ExecutionEnv *executionEnv, SPDocument *document) override;
    bool cancelProcessing() override;

private:
    bool _canceled;
    Glib::Pid _pid;
    Glib::RefPtr<Glib::MainLoop> _main_loop;

    void _change_extension(Inkscape::Extension::Extension *mod, ExecutionEnv *executionEnv, SPDocument *doc,
                           std::list<std::string> &params, bool ignore_stderr, bool pipe_diffs = false, bool custom_ui = true);

    void _setAppSensitive(bool sensitive);

    /**
     * The command that has been derived from
     * the configuration file with appropriate directories
     */
    std::list<std::string> command;

     /**
      * This is the extension that will be used
      * as the helper to read in or write out the
      * data
      */
    Glib::ustring helper_extension;

     /**
      * The window which should be considered as "parent window" of the script execution,
      * e.g. when showin warning messages
      *
      * If set to NULL the main window of the currently active document is used.
      */
    Gtk::Window *parent_window;

    void showPopupError (Glib::ustring const& filename, Gtk::MessageType type, Glib::ustring const& message);

    class file_listener {
        Glib::ustring _string;
        sigc::scoped_connection _conn;
        Glib::RefPtr<Glib::IOChannel> _channel;
        Glib::RefPtr<Glib::MainLoop> _main_loop;
        bool _dead = false;

    public:
        virtual ~file_listener();

        bool isDead () { return _dead; }
        void init(int fd, Glib::RefPtr<Glib::MainLoop> main);
        bool read(Glib::IOCondition condition);
        Glib::ustring string () { return _string; };
        bool toFile(const Glib::ustring &name);
        bool toFile(const std::string &name);
    };

    class PreviewObserver : public UndoStackObserver
    {
    public:
        PreviewObserver(Glib::RefPtr<Glib::IOChannel> channel);
        void connect(SPDesktop const *desktop, SPDocument *document);
        void disconnect(SPDocument *document);

    private:
        void selectionChanged(Inkscape::Selection *selection);
        void notifyUndoCommitEvent(Event *log) override;
        void notifyUndoEvent(Event *log) override;
        void notifyRedoEvent(Event *log) override;
        void notifyClearUndoEvent() override;
        void notifyClearRedoEvent() override;
        void notifyUndoExpired(Event *log) override;
        void createAndSendEvent(
            std::function<void(Inkscape::XML::Document *doc, Inkscape::XML::Node *)> const &eventPopulator);

        sigc::connection _select_changed;
        sigc::connection _reconstruction_start_connection;
        sigc::connection _reconstruction_finish_connection;
        Glib::RefPtr<Glib::IOChannel> _channel;
        bool _pause_select_events = false;
    };

    int execute(std::list<std::string> const &in_command, std::list<std::string> const &in_params,
                Glib::ustring const &filein, file_listener &fileout, bool ignore_stderr = false,
                bool pipe_diffs = false, bool custom_ui = false);

    /** \brief  A definition of an interpreter, which can be specified
        in the INX file, but we need to know what to call */
    struct interpreter_t {
	std::string              prefstring;   /**< The preferences key that can override the default */
	std::vector<std::string> defaultvals;  /**< The default values to check if the preferences are wrong */
    };
    static const std::map<std::string, interpreter_t> interpreterTab;

    std::string resolveInterpreterExecutable(const Glib::ustring &interpNameArg);

};

} // namespace Extension::Implementation

} // namespace Inkscape

#endif // INKSCAPE_EXTENSION_IMPEMENTATION_SCRIPT_H_SEEN

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

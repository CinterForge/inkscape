// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef INKSCAPE_UI_DIALOG_MANAGER_H
#define INKSCAPE_UI_DIALOG_MANAGER_H

#include <glibmm/keyfile.h>
#include <map>
#include <optional>
#include <set>

namespace Glib {
class Keyfile;
class ustring;
} // namespace Glib

namespace Gtk {
class Window;
} // namespace Gtk

namespace Inkscape::UI::Dialog {

class DialogWindow;
class DialogBase;
class DialogContainer;

struct window_position_t
{
    int x, y, width, height;
};

// try to read window's geometry
std::optional<window_position_t> dm_get_window_position(Gtk::Window &window);

// restore window's geometry
void dm_restore_window_position(Gtk::Window &window, const window_position_t &position);

class DialogManager final
{
public:
    static DialogManager &singleton();

    // store complete dialog window state (including its container state)
    void store_state(DialogWindow &wnd);

    // return true if dialog 'type' should be opened as floating
    bool should_open_floating(const Glib::ustring& dialog_type);

    // find instance of dialog 'type' in one of currently open floating dialog windows
    DialogBase *find_floating_dialog(const Glib::ustring& dialog_type);

    // find window hosting floating dialog
    DialogWindow* find_floating_dialog_window(const Glib::ustring& dialog_type);

    // find floating window state hosting dialog 'code', if there was one
    std::shared_ptr<Glib::KeyFile> find_dialog_state(const Glib::ustring& dialog_type);

    // remove dialog floating state
    void remove_dialog_floating_state(const Glib::ustring& dialog_type);

    // save configuration of docked and floating dialogs
    void save_dialogs_state(DialogContainer *docking_container);

    // restore state of dialogs
    void restore_dialogs_state(DialogContainer *docking_container, bool include_floating);

    // find all floating dialog windows
    std::vector<DialogWindow*> get_all_floating_dialog_windows();

    // show/hide dialog window and keep track of it
    void set_floating_dialog_visibility(DialogWindow* wnd, bool show);

private:
    DialogManager() = default;
    ~DialogManager();

    std::vector<Glib::ustring> count_dialogs(const Glib::KeyFile *state) const;
    void load_transient_state(Glib::KeyFile *keyfile);
    void dialog_defaults(DialogContainer* docking_container);

    // transient dialog state for floating windows user closes
    std::map<std::string, std::shared_ptr<Glib::KeyFile>> _floating_dialogs;

    // temp set used when dialogs are hidden (F12 toggle)
    std::set<DialogWindow*> _hidden_dlg_windows;
};

} // namespace Inkscape::UI::Dialog

#endif // INKSCAPE_UI_DIALOG_MANAGER_H

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

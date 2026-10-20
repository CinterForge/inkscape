// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Manager for shared paint popovers (Fill/Stroke).
 *//*
 * Authors:
 *   Ayan Das <ayandazzz@outlook.com>
 *
 * Copyright (C) 2026 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_WIDGET_PAINT_POPOVER_MANAGER_H
#define INKSCAPE_UI_WIDGET_PAINT_POPOVER_MANAGER_H

#include <functional>
#include <memory>
#include <vector>
#include <sigc++/connection.h>

namespace Gtk { class MenuButton; class Popover; }

namespace Inkscape::UI::Widget {

class PaintSwitch;

class PaintPopoverManager {
public:
    using SetupCallback = std::function<void()>;
    using ConnectCallback = std::function<std::vector<sigc::connection>()>;

    /**
     * @brief A RAII token that manages the lifetime 
     * of a button's registration with the popover manager.
     * - Cannot be copied.
     * - Can transfer ownership from local variable to member variable.
     */
    class Registration {
    public:
        Registration(const Registration&) = delete;
        Registration& operator=(const Registration&) = delete;
        Registration(Registration&& other) noexcept;
        Registration& operator=(Registration&& other) noexcept;

        ~Registration();

    private:
        friend class PaintPopoverManager;
        Registration(PaintPopoverManager* mgr, Gtk::MenuButton* btn, bool fill);

        PaintPopoverManager* _mgr = nullptr;
        Gtk::MenuButton* _btn = nullptr;
        bool _fill = false;
    };

    /**
     * @brief Accesses the singleton instance of the PaintPopoverManager.
     * This method ensures that only one instance of the manager exists throughout the 
     * application lifecycle. It creates the instance on the first call and returns 
     * a reference to it on all subsequent calls.
     * @return Reference to the singleton PaintPopoverManager instance.
     */
    static PaintPopoverManager& get();

    /**
     * @brief Registers a MenuButton to use the shared popover mechanism.
     * Sets up a callback that reparents the popover, manages signals, and handles positioning
     * when the button is clicked.
     * @param btn The MenuButton to register.
     * @param is_fill True if this button controls Fill, false for Stroke.
     * @param setup Callback function to configure the switch state (e.g., set current color) before showing.
     * @param connect Callback function to connect signals (e.g., color changed) and return the connections list.
     */
    [[nodiscard]]
    Registration register_button(Gtk::MenuButton& btn, bool is_fill, SetupCallback setup, ConnectCallback connect);

    /**
     * @brief Retrieves the shared PaintSwitch instance, creating it immediately if necessary.
     * @param is_fill True to get the Fill switch, false for the Stroke switch.
     * @return Pointer to the shared PaintSwitch.
     */
    PaintSwitch* get_switch(bool is_fill);

    /**
     * @brief Retrieves the shared Popover instance, creating it immediately if necessary.
     * @param is_fill True to get the Fill popover, false for the Stroke popover.
     * @return Pointer to the shared Gtk::Popover.
     */
    Gtk::Popover* get_popover(bool is_fill);

    private:
    /**
     * @brief Unregisters a button, cleaning up its association with the shared popover.
     * If the button currently holds the popover, it is unset and signals are disconnected.
     * @param btn The MenuButton to unregister.
     * @param is_fill True if the button controls Fill, false for Stroke.
     */
    void unregister_button(Gtk::MenuButton& btn, bool is_fill);

    /**
     * @brief container for the shared resources associated with specific paint type.
     */
    struct SharedData {
      std::unique_ptr<Gtk::Popover> popover;      // The shared popover container.
      std::unique_ptr<PaintSwitch> paint_switch;            // The shared PaintSwitch.
      std::vector<sigc::connection> connections;  // Active signal connections for the current button.

      /**
       * @brief Creates the shared resources (PaintSwitch, Popover, ScrolledWindow) if they don't exist yet.
       * @param is_fill True if creating resources for Fill, false for Stroke.
       */
      void create_resources(bool is_fill);
      void clear_connections();
    };
    
    SharedData _fill_data;
    SharedData _stroke_data;

    PaintPopoverManager() = default;
};

} // namespace Inkscape::UI::Widget

#endif // INKSCAPE_UI_WIDGET_PAINT_POPOVER_MANAGER_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim:filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99:

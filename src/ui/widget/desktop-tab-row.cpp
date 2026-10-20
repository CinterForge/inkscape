// SPDX-License-Identifier: GPL-2.0-or-later

#include "desktop-tab-row.h"

#include <glibmm/i18n.h>
#include <glibmm/main.h>
#include <gtkmm/button.h>
#include <gtkmm/dragicon.h>
#include <gtkmm/droptarget.h>
#include <gtkmm/eventcontrollermotion.h>
#include <gtkmm/eventcontrollerscroll.h>
#include <gtkmm/gestureclick.h>
#include <gtkmm/picture.h>
#include <gtkmm/popovermenu.h>
#include <gtkmm/snapshot.h>
#include <gtkmm/tooltip.h>

#include "desktop.h"
#include "document.h"
#include "inkscape-application.h"
#include "inkscape-window.h"
#include "ui/builder-utils.h"
#include "ui/containerize.h"
#include "ui/popup-menu.h"
#include "ui/util.h"
#include "ui/widget/canvas.h"
#include "ui/widget/desktop-widget.h"
#include "util/value-utils.h"

#define BUILD(name) name{UI::get_widget<std::remove_reference_t<decltype(name)>>(builder, #name)}

using namespace Inkscape::Util;

namespace Inkscape::UI::Widget {
namespace {

struct TooltipUI
{
    TooltipUI()
        : TooltipUI{create_builder("document-tab-preview.ui")}
    {}

    TooltipUI(Glib::RefPtr<Gtk::Builder> const &builder)
        : BUILD(root)
        , BUILD(name)
        , BUILD(preview)
    {
        root.reference();

        // Clear preview on dismissal to save memory.
        root.signal_unmap().connect([this] {
            preview.set_paintable({});
            current_display_info = {};
        });
    }

    ~TooltipUI()
    {
        root.unreference();
    }

    Gtk::Box &root;
    Gtk::Label &name;
    Gtk::Picture &preview;

    struct CurrentDisplayInfo
    {
        SPDesktop *desktop;
        bool is_active_tab;
        auto operator<=>(CurrentDisplayInfo const &) const = default;
    };
    std::optional<CurrentDisplayInfo> current_display_info;
};

struct PointerTransparentWidget : Gtk::Widget
{
    bool contains_vfunc(double, double) const override { return false; }
};

Gtk::PopoverMenu to_popovermenu(Glib::RefPtr<Gio::Menu> const &menu)
{
    auto popovermenu = Gtk::PopoverMenu{menu};
    popovermenu.set_has_arrow(false);
    popovermenu.set_position(Gtk::PositionType::BOTTOM);
    popovermenu.set_halign(Gtk::Align::START);
    return popovermenu;
}

Gtk::PopoverMenu create_context_menu()
{
    auto menu = Gio::Menu::create();
    menu->append_item(Gio::MenuItem::create(_("Detach to New Window"), "tabs.detach"));
    menu->append_item(Gio::MenuItem::create(_("Close File"), "tabs.close"));
    return to_popovermenu(menu);
}

Gtk::PopoverMenu &get_context_menu()
{
    static auto menu = create_context_menu();
    return menu;
}

Gtk::PopoverMenu create_context_menu_background()
{
    auto menu = Gio::Menu::create();
    auto sec1 = Gio::Menu::create();
    sec1->append_item(Gio::MenuItem::create(_("Close"), "win.document-close"));
    menu->append_section(sec1);
    auto sec2 = Gio::Menu::create();
    sec2->append_item(Gio::MenuItem::create(_("New Document..."), "win.document-new"));
    sec2->append_item(Gio::MenuItem::create(_("Open Document..."), "win.document-open"));
    menu->append_section(sec2);
    return to_popovermenu(menu);
}

Gtk::PopoverMenu &get_context_menu_background()
{
    static auto menu = create_context_menu_background();
    return menu;
}

std::optional<Geom::Point> get_current_pointer_pos(Glib::RefPtr<Gdk::Device> const &pointer, Gtk::Widget &widget)
{
    double x, y;
    Gdk::ModifierType mask;
    auto root = widget.get_root();
    dynamic_cast<Gtk::Native &>(*root).get_surface()->get_device_position(pointer, x, y, mask);
    dynamic_cast<Gtk::Widget &>(*root).translate_coordinates(widget, x, y, x, y);
    return Geom::Point{x, y};
}

/// Returns the text to show inside a tab.
Glib::ustring get_title(SPDesktop *desktop)
{
    auto result = Glib::ustring{};

    auto const doc = desktop->doc();

    if (doc->isModifiedSinceSave()) {
        result += "*";
    }

    result += doc->getDocumentName();

    if (doc->isTemplate()) {
        result += " (template)";
    }

    if (auto const v = desktop->viewNumber(); v > 1) {
        result += ": ";
        result += Glib::ustring::format(v);
    }

    return result;
}

} // namespace

/// Global list of all DesktopTabRow instances. Its only purpose is to
/// coordinate when tab bars should be highlighted.
class DesktopTabRow::Instances
{
public:
    static Instances &get()
    {
        static Instances instance;
        return instance;
    }

    void add(DesktopTabRow *w)
    {
        _instances.push_back(w);
    }

    void remove(DesktopTabRow *w)
    {
        _instances.erase(std::find(_instances.begin(), _instances.end(), w));
    }

    void addHighlight()
    {
        for (auto w : _instances) {
            w->add_css_class("drop-highlight");
        }
    }

    void removeHighlight()
    {
        for (auto w : _instances) {
            w->remove_css_class("drop-highlight");
        }
    }

private:
    Instances() = default;

    std::vector<DesktopTabRow *> _instances;
};

/// A purely visual version of a Tab that is used as a dummy during drag-and-drop.
struct DumbTab : Gtk::Box
{
    Gtk::Label &name;
    Gtk::Button &close;

    DumbTab()
        : DumbTab{create_builder("document-tab.ui")}
    {}

    DumbTab(Glib::RefPtr<Gtk::Builder> const &builder)
        : BUILD(name)
        , BUILD(close)
    {
        set_name("DocumentTab");
        property_accessible_role().set_value(Gtk::Accessible::Role::TAB);

        append(get_widget<Gtk::Box>(builder, "root"));
    }

    void set_active()
    {
        set_state_flags(Gtk::StateFlags::CHECKED, false);
    }

    void set_inactive()
    {
        unset_state_flags(Gtk::StateFlags::CHECKED);
    }
};

/// The actual tabs that are shown in the tab bar.
struct Tab : CssNameClassInit, DumbTab
{
    SPDesktop *const desktop;
    DesktopTabRow *const parent;

    Tab(SPDesktop *desktop, DesktopTabRow *parent)
        : Glib::ObjectBase("Tab")
        , CssNameClassInit{"tab"}
        , desktop{desktop}
        , parent{parent}
    {
        set_has_tooltip(true);
    }
};

class TabDrag
{
public:
    /// Create and start a tab drag.
    /// All drags are assumed to start off local, i.e. the source and destination tab bars are the same.
    /// The result must therefore be assigned to the source tab bar's _drag_src and _drag_dst.
    TabDrag(Tab *src, Geom::Point const &offset, Glib::RefPtr<Gdk::Device> device)
        : _src{src}
        , _offset{offset}
        , _device{std::move(device)}
        , _dst{src->parent}
    {}

    /// Called by dst whenever the pointer moves, whether over it or not. This sometimes requires polling.
    /// Updates the tab's position within dst or detaches it.
    void motion(std::optional<Geom::Point> pos)
    {
        constexpr int detach_dist = 50;
        if (pos && Geom::Rect(0, 0, _dst->get_width(), _dst->get_height()).distanceSq(*pos) < Geom::sqr(detach_dist)) {
            // Pointer is still sufficiently near dst - update drop position.
            _drop_x = pos->x() - static_cast<int>(std::round(_offset.x()));
            _dst->queue_allocate();
        } else {
            // Pointer is too far away from dst - detach from it.
            cancelTick();
            _ensureDrag();
            setDst(nullptr);
        }
    }

    /// Called when the the pointer leaves dst.
    /// Starts polling so we continue to receive motion events.
    /// This is needed because we want tabs to be "sticky" once dropped onto tab bars.
    void addTick()
    {
        if (!_tick_callback) {
            _tick_callback = _dst->add_tick_callback([this] (auto&&) {
                motion(get_current_pointer_pos(_device, *_dst));
                return true;
            });
        }
    }

    void cancelTick()
    {
        if (_tick_callback) {
            _dst->remove_tick_callback(_tick_callback);
            _tick_callback = 0;
        }
    }

    /// Set a new destination tab bar, or unset by passing null.
    void setDst(DesktopTabRow *new_dst)
    {
        if (new_dst == _dst) {
            return;
        }

        if (_dst) {
            _dst->_drag_dst = {};
            _dst->queue_resize();
        }

        _dst = new_dst;

        if (_dst) {
            _dst->_drag_dst = _src->parent->_drag_src;
            _drop_x = {};
            _drop_i = {};
        }

        _queueReparent();
    }

    /// End the drag. This function unsets any _drag_src or _drag_dst references that point to this object,
    /// indirectly destroying this object. It is ensured that this function is called exactly once during
    /// every TabDrag's lifetime. Thus this function is the *only* way that a TabDrag is destroyed.
    void finish(bool cancel = false)
    {
        // Cancel the tick callback if one is being used for motion polling.
        cancelTick();

        // Detach from source and destination, keeping self alive until end of function.
        auto const self_ref = std::move(_src->parent->_drag_src);
        assert(self_ref.get() == this);
        if (_dst) {
            _dst->_drag_dst = {};
        }

        // Undo widget modifications to source and destination.
        _src->set_visible(true);
        _src->parent->queue_resize();
        if (_dst) {
            if (_widget && _widget->get_parent() == _dst) {
                _widget->unparent();
            }
            _dst->queue_resize();
        }
        DesktopTabRow::Instances::get().removeHighlight();

        if (!_dst && _src->parent->_tabs.size() == 1) {
            cancel = true; // cancel if detaching lone tab
        }

        if (cancel) {
            _src->parent->_desktop_widget->get_window()->present();
            return;
        }

        if (_drag) {
            _drag->drag_drop_done(true); // suppress drag-failed animation
        }

        if (!_dst) {
            // Detach
            InkscapeApplication::instance()->detachDesktopToNewWindow(_src->desktop);
        } else if (_dst == _src->parent) {
            // Reorder
            if (_drop_i) {
                int const from = _src->parent->positionOfTab(_src->desktop);
                _src->parent->_reorderTab(from, *_drop_i);
            }
        } else {
            // Migrate
            if (_drop_i) {
                auto const desktop = _src->desktop;
                _src->desktop->getDesktopWidget()->removeDesktop(desktop); // deletes src
                _dst->_desktop_widget->addDesktop(desktop, *_drop_i);
            }
        }
    }

    Tab *src() const { return _src; }
    DumbTab *widget() const { return _widget.get(); }
    std::optional<int> const &dropX() const { return _drop_x; }
    void setDropI(int i) { _drop_i = i; }

private:
    Tab *const _src; // The source tab.
    Geom::Point const _offset; // The point within the tab that the drag started from.
    Glib::RefPtr<Gdk::Device> const _device; // The pointing device that started the drag.

    DesktopTabRow *_dst; // The destination tabs widget, possibly null.
    std::optional<int> _drop_x; // Position within dst where tab is dropped.
    std::optional<int> _drop_i; // The index within dst where the tab is dropped.

    sigc::scoped_connection _reparent_conn; // Used to defer reparenting of widget.
    sigc::scoped_connection _cancel_conn; // Connection to the Gdk::Drag's cancel signal.
    sigc::scoped_connection _drop_conn; // Connection to the Gdk::Drag's drop-perfomed signal.
    Glib::RefPtr<Gdk::Drag> _drag; // The Gdk::Drag, lazily created at the same time as widget.
    std::unique_ptr<DumbTab> _widget; // The dummy tab that is placed inside the drag icon or dst.
    guint _tick_callback = 0; // If nonzero, the tick callback used for motion polling.

    void _ensureDrag()
    {
        if (_drag) {
            return;
        }

        // Create the Gdk drag.
        assert(_src->parent->_drag_src.get() == this);
        auto content = Gdk::ContentProvider::create(GlibValue::create<std::weak_ptr<TabDrag>>(_src->parent->_drag_src));
        _drag = _src->parent->get_native()->get_surface()->drag_begin_from_point(_device, content, Gdk::DragAction::MOVE, _offset.x(), _offset.y());

        // Handle drag cancellation.
        _cancel_conn = _drag->signal_cancel().connect([this] (auto reason) {
            finish(reason == Gdk::DragCancelReason::USER_CANCELLED);
        }, false);

        // Some buggy clients accept the drop when they shouldn't. We interpret it as a drop on nothing.
        _drop_conn = _drag->signal_drop_performed().connect([this] {
            finish();
        }, false);

        // Hide the real tab.
        _src->set_visible(false);

        // Create a visual replica of the tab.
        _widget = std::make_unique<DumbTab>();
        _widget->name.set_text(get_title(_src->desktop));
        _widget->set_active();
    }

    // Schedule widget to be reparented. GTK is picky about when this can be done, hence the need for an idle callback.
    // In particular, widget hierarchy changes aren't allowed in a tick callback or an enter/leave handler.
    void _queueReparent()
    {
        if (!_reparent_conn) {
            _reparent_conn = Glib::signal_idle().connect([this] { _reparentWidget(); return false; }, Glib::PRIORITY_HIGH);
        }
    }

    void _reparentWidget()
    {
        auto drag_icon = Gtk::DragIcon::get_for_drag(_drag);

        if (_widget.get() == drag_icon->get_child()) {
            drag_icon->unset_child();
            // Fixme: Shouldn't be needed, but works around https://gitlab.gnome.org/GNOME/gtk/-/issues/7185
            Gtk::DragIcon::set_from_paintable(_drag, to_texture(Cairo::ImageSurface::create(Cairo::ImageSurface::Format::ARGB32, 1, 1)), 0, 0);
        } else if (_widget->get_parent()) {
            assert(dynamic_cast<DesktopTabRow *>(_widget->get_parent()));
            _widget->unparent();
        }

        // Put the replica tab inside dst or the drag icon.
        if (_dst) {
            _widget->insert_before(*_dst, *_dst->_overlay);
            _dst->queue_resize();
            DesktopTabRow::Instances::get().removeHighlight();
        } else {
            drag_icon->set_child(*_widget);
            _drag->set_hotspot(_offset.x(), _offset.y());
            DesktopTabRow::Instances::get().addHighlight();
        }
    }
};

static std::shared_ptr<TabDrag> get_tab_drag(Gtk::DropTarget &droptarget)
{
    // Determine whether an in-app tab is being dragged and get information about it.
    auto const drag = droptarget.get_drop()->get_drag();
    if (!drag) {
        return {}; // not in-app
    }
    auto const content = GlibValue::from_content_provider<std::weak_ptr<TabDrag>>(*drag->get_content());
    if (!content) {
        return {}; // not a tab
    }
    return content->lock();
}

// This is used to ensure that a Tab never outlives its parent DesktopTabRow,
// which would result in Tab::parent dangling.
static SPDesktop *consume_tab_return_desktop(std::shared_ptr<Tab> tab)
{
    return tab ? tab->desktop : nullptr;
}

DesktopTabRow::DesktopTabRow(SPDesktopWidget *desktop_widget)
    : Glib::ObjectBase("DesktopTabRow")
    , CssNameClassInit{"tabs"}
    , _desktop_widget{desktop_widget}
    , _overlay{Gtk::make_managed<PointerTransparentWidget>()}
{
    set_name("DocumentTabsWidget");
    set_overflow(Gtk::Overflow::HIDDEN);
    containerize(*this);
    property_accessible_role().set_value(Gtk::Accessible::Role::TAB_LIST);

    _overlay->insert_at_end(*this); // always kept topmost
    _overlay->set_name("Overlay");

    auto click = Gtk::GestureClick::create();
    click->set_button(0);
    click->signal_pressed().connect([this, click = click.get()] (int, double x, double y) {
        // Find clicked tab.
        auto const [tab_weak, tab_pos] = _tabAtPoint({x, y});
        auto tab = tab_weak.lock();

        // Handle button actions.
        switch (click->get_current_button()) {
            case GDK_BUTTON_PRIMARY:
                if (tab) {
                    double xc, yc;
                    translate_coordinates(tab->close, x, y, xc, yc);
                    if (!tab->close.contains(xc, yc)) {
                        _desktop_widget->switchDesktop(tab->desktop);
                        _left_clicked = tab_weak;
                        _left_click_pos = {x, y};
                    }
                }
                break;
            case GDK_BUTTON_SECONDARY: {
                auto &menu = tab ? get_context_menu() : get_context_menu_background();
                auto old_parent = dynamic_cast<DesktopTabRow *>(menu.get_parent());
                if (old_parent != this) {
                    if (old_parent) {
                        menu.unparent();
                        old_parent->_popover = nullptr;
                    }
                    menu.set_parent(*this);
                    _popover = &menu;
                }
                UI::popup_at(menu, *this, x, y);
                _right_clicked = tab_weak;
                break;
            }
            case GDK_BUTTON_MIDDLE:
                if (tab) {
                    InkscapeApplication::instance()->destroyDesktop(consume_tab_return_desktop(std::move(tab)));
                }
                break;
            default:
                break;
        }
    });
    click->signal_released().connect([this] (auto&&...) {
        _left_clicked = {};
        if (_drag_src) {
            _drag_src->finish();
        }
    });
    add_controller(click);

    auto motion = Gtk::EventControllerMotion::create();
    motion->signal_motion().connect([this, &motion = *motion] (double x, double y) {
        if (!_drag_src) {
            auto const tab = _left_clicked.lock();
            if (!tab) {
                return;
            }

            constexpr int drag_initiate_dist = 5;
            if ((Geom::Point{x, y} - _left_click_pos).lengthSq() < Geom::sqr(drag_initiate_dist)) {
                return;
            }

            _left_clicked = {};

            Geom::Point offset;
            translate_coordinates(*tab, _left_click_pos.x(), _left_click_pos.y(), offset.x(), offset.y());

            // Start dragging.
            _drag_src = _drag_dst = std::make_shared<TabDrag>(
                tab.get(),
                offset,
                motion.get_current_event_device()
            );

            // Raise dragged tab to top.
            tab->insert_before(*this, *_overlay);
        }

        if (!_drag_src->widget()) {
            _drag_src->motion(Geom::Point{x, y});
        }
    });
    add_controller(motion);

    auto droptarget = Gtk::DropTarget::create(GlibValue::type<std::weak_ptr<TabDrag>>(), Gdk::DragAction::MOVE);
    auto handler = [this, &droptarget = *droptarget] (double x, double y) -> Gdk::DragAction {
        if (auto tabdrag = get_tab_drag(droptarget)) {
            tabdrag->cancelTick();
            tabdrag->setDst(this);
            tabdrag->motion(Geom::Point{x, y});
        }
        return {};
    };
    droptarget->signal_enter().connect(handler, false);
    droptarget->signal_motion().connect(handler, false);
    droptarget->signal_leave().connect([this] {
        if (_drag_dst) {
            _drag_dst->addTick();
        }
    });
    add_controller(droptarget);

    auto scroll = Gtk::EventControllerScroll::create();
    scroll->set_flags(Gtk::EventControllerScroll::Flags::VERTICAL | Gtk::EventControllerScroll::Flags::DISCRETE);
    auto scroll_handler = [this, &scroll = *scroll] (double dx, double dy) {
        if (dy < 0) {
            switchToPrevDesktop();
        } else if (dy > 0) {
            switchToNextDesktop();
        }
        return true;
    };
    scroll->signal_scroll().connect(scroll_handler, false);
    add_controller(scroll);

    auto actiongroup = Gio::SimpleActionGroup::create();
    actiongroup->add_action("detach", [this] {
        if (auto desktop = consume_tab_return_desktop(_right_clicked.lock())) {
            InkscapeApplication::instance()->detachDesktopToNewWindow(desktop);
        }
    });
    actiongroup->add_action("close", [this] {
        if (auto desktop = consume_tab_return_desktop(_right_clicked.lock())) {
            InkscapeApplication::instance()->destroyDesktop(desktop);
        }
    });
    insert_action_group("tabs", actiongroup);

    Instances::get().add(this);
}

DesktopTabRow::~DesktopTabRow()
{
    Instances::get().remove(this);

    // Note: This code will fail if DesktopTabRow becomes a managed widget, in which
    // case it must be done on signal_destroy() instead.
    if (_drag_dst) {
        _drag_dst->setDst(nullptr);
    }
    if (_drag_src) {
        _drag_src->finish(true);
    }
}

void DesktopTabRow::addTab(SPDesktop *desktop, int pos)
{
    auto tab = std::make_shared<Tab>(desktop, this);
    tab->name.set_text(get_title(desktop));

    tab->close.signal_clicked().connect([this, desktop] { InkscapeApplication::instance()->destroyDesktop(desktop); });

    tab->signal_query_tooltip().connect([this, desktop] (int, int, bool, Glib::RefPtr<Gtk::Tooltip> const &tooltip) {
        _setTooltip(desktop, tooltip);
        return true; // show the tooltip
    }, true);

    assert(positionOfTab(desktop) == -1);

    if (pos == -1) {
        pos = _tabs.size();
    }
    assert(0 <= pos && pos <= _tabs.size());

    tab->insert_before(*this, *_overlay);
    _tabs.insert(_tabs.begin() + pos, std::move(tab));
}

void DesktopTabRow::removeTab(SPDesktop *desktop)
{
    int const i = positionOfTab(desktop);
    assert(i != -1);

    if (_drag_src && _drag_src->src() == _tabs[i].get()) {
        _drag_src->finish(true);
    }

    _tabs[i]->unparent();
    _tabs.erase(_tabs.begin() + i);
}

void DesktopTabRow::switchTab(SPDesktop *desktop)
{
    auto const active = _active.lock();

    if (active && active->desktop == desktop) {
        return;
    }

    if (active) {
        active->set_inactive();
        _active = {};
    }

    int const i = positionOfTab(desktop);
    if (i != -1) {
        _tabs[i]->set_active();
        _active = _tabs[i];
    }
}

void DesktopTabRow::switchToPrevDesktop()
{
    int current = _activeTabPosition();
    if (current > 0) {
        switchToDesktop(_tabs[current - 1]->desktop);
    }
}

void DesktopTabRow::switchToNextDesktop()
{
    int current = _activeTabPosition();
    if (current >= 0 && current < _tabs.size() - 1) {
        switchToDesktop(_tabs[current + 1]->desktop);
    }
}

void DesktopTabRow::switchToDesktop(SPDesktop *desktop)
{
    _desktop_widget->switchDesktop(desktop);
}

void DesktopTabRow::refreshTitle(SPDesktop *desktop)
{
    int const i = positionOfTab(desktop);
    assert(i != -1);
    _tabs[i]->name.set_text(get_title(_tabs[i]->desktop));
}

int DesktopTabRow::positionOfTab(SPDesktop *desktop) const
{
    for (int i = 0; i < _tabs.size(); i++) {
        if (_tabs[i]->desktop == desktop) {
            return i;
        }
    }
    return -1;
}

SPDesktop *DesktopTabRow::tabAtPosition(int i) const
{
    if (i < _tabs.size()) {
        return _tabs[i]->desktop;
    } else {
        return nullptr;
    }
}

Gtk::Widget *DesktopTabRow::widgetAtPosition(int i) const
{
    if (i < _tabs.size()) {
        return _tabs[i].get();
    } else {
        return nullptr;
    }
}

int DesktopTabRow::size() const
{
    return _tabs.size();
}

Glib::ustring DesktopTabRow::tabName(int i) const
{
    return _tabs[i]->name.get_text();
}

Gtk::SizeRequestMode DesktopTabRow::get_request_mode_vfunc() const
{
    return Gtk::SizeRequestMode::CONSTANT_SIZE;
}

void DesktopTabRow::measure_vfunc(Gtk::Orientation orientation, int, int &min, int &nat, int &, int &) const
{
    nat = 0;
    auto consider = [&] (Gtk::Widget const &w) {
        auto const m = w.measure(orientation, -1);
        if (orientation == Gtk::Orientation::VERTICAL) {
            nat = std::max(nat, m.sizes.natural); // just take max size we find
        } else {
            nat += m.sizes.natural; // add all sizes together
        }
    };
    for (auto const &tab : _tabs) {
        consider(*tab);
    }
    if (_drag_src) {
        if (auto widget = _drag_src->widget()) {
            consider(*widget);
        }
    }
    if (_drag_dst) {
        if (auto widget = _drag_dst->widget()) {
            consider(*widget);
        }
    }
    min = nat;
}

void DesktopTabRow::size_allocate_vfunc(int width, int height, int)
{
    struct Drop
    {
        int x;
        int w;
        DumbTab *widget;
        bool done = false;
    };

    std::optional<Drop> drop;
    if (_drag_dst && _drag_dst->dropX()) {
        auto widget = !_drag_dst->widget()
            ? _drag_dst->src()
            : _drag_dst->widget();
        if (widget->get_parent() == this) {
            drop = Drop{
                .x = *_drag_dst->dropX(),
                .w = widget->measure(Gtk::Orientation::HORIZONTAL, -1).sizes.natural,
                .widget = widget,
            };
        }
    }

    int x = 0;
    for (int i = 0; i < _tabs.size(); i++) {
        auto const tab = _tabs[i].get();
        if (_drag_src && tab == _drag_src->src()) {
            continue;
        }
        auto const w = tab->measure(Gtk::Orientation::HORIZONTAL, -1).sizes.natural;
        if (drop && !drop->done && x + w / 2 > drop->x) {
            x += drop->w;
            _drag_dst->setDropI(i);
            drop->done = true;
        }
        tab->size_allocate(Gtk::Allocation(x, 0, w, height), -1);
        x += w;
    }

    _overlay->size_allocate(Gtk::Allocation(0, 0, width, height), -1);

    // GTK burdens custom widgets with having to implement this manually.
    if (_popover) {
        _popover->present();
    }

    if (drop) {
        if (!drop->done) {
            _drag_dst->setDropI(_tabs.size());
        }
        drop->widget->size_allocate(Gtk::Allocation(drop->x, 0, drop->w, height), -1);
    }
}

void DesktopTabRow::_setTooltip(SPDesktop *desktop, Glib::RefPtr<Gtk::Tooltip> const &tooltip)
{
    // Lazy-load tooltip ui file, shared among all instances.
    static auto const tooltip_ui = std::make_unique<TooltipUI>();

    auto const active = _active.lock();
    bool const is_active_tab = active && desktop == active->desktop;
    auto const display_info = TooltipUI::CurrentDisplayInfo{
        .desktop = desktop,
        .is_active_tab = is_active_tab
    };

    if (tooltip_ui->current_display_info != display_info) { // avoid pointless updates
        tooltip_ui->current_display_info = display_info;

        // Set name.
        tooltip_ui->name.set_label(desktop->doc()->getDocumentName());

        // Set preview.
        if (is_active_tab) {
            tooltip_ui->preview.set_paintable({}); // no preview for active tab
        } else {
            constexpr double scale = 0.2;
            auto snapshot = Gtk::Snapshot::create();
            snapshot->scale(scale, scale);
            desktop->getCanvas()->snapshot_vfunc(snapshot);
            tooltip_ui->preview.set_paintable(snapshot->to_paintable());
        }
    }

    // Embed custom widget into tooltip.
    tooltip->set_custom(tooltip_ui->root);
}

std::pair<std::weak_ptr<Tab>, Geom::Point> DesktopTabRow::_tabAtPoint(Geom::Point const &pos)
{
    double xt, yt;
    auto const it = std::find_if(_tabs.begin(), _tabs.end(), [&] (auto const &tab) {
        translate_coordinates(*tab, pos.x(), pos.y(), xt, yt);
        return tab->contains(xt, yt);
    });
    if (it == _tabs.end()) {
        return {};
    }
    return {*it, {xt, yt}};
}

void DesktopTabRow::_reorderTab(int from, int to)
{
    assert(0 <= from && from < _tabs.size());
    assert(0 <= to && to <= _tabs.size());

    if (from == to) {
        return;
    }

    auto tab = std::move(_tabs[from]);
    _tabs.erase(_tabs.begin() + from);
    _tabs.insert(_tabs.begin() + to - (to > from), std::move(tab));
}

int DesktopTabRow::_activeTabPosition()
{
    auto const active = _active.lock();
    if (!active) {
        return -1;
    }

    return positionOfTab(active->desktop);
}

} // namespace Inkscape::UI::Widget

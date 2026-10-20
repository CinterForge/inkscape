// SPDX-License-Identifier: GPL-2.0-or-later

#include "tab-strip.h"

#include <numeric>
#include <ranges>
#include <gdkmm/surface.h>
#include <glibmm/main.h>
#include <gtkmm/button.h>
#include <gtkmm/dragicon.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/droptarget.h>
#include <gtkmm/eventcontrollermotion.h>
#include <gtkmm/gestureclick.h>
#include <gtkmm/image.h>
#include <gtkmm/tooltip.h>

#include "ui/containerize.h"
#include "ui/popup-menu.h"
#include "ui/util.h"
#include "util/value-utils.h"

#define BUILD(name) name{UI::get_widget<std::remove_reference_t<decltype(name)>>(builder, #name)}

using namespace Inkscape::Util;

namespace Inkscape::UI::Widget {
namespace {

struct PointerTransparentWidget : Gtk::Widget
{
    bool contains_vfunc(double, double) const override { return false; }
};

std::optional<Geom::Point> get_current_pointer_pos(Glib::RefPtr<Gdk::Device> const &pointer, Gtk::Widget &widget)
{
    double x, y;
    Gdk::ModifierType mask;
    auto root = widget.get_root();
    dynamic_cast<Gtk::Native &>(*root).get_surface()->get_device_position(pointer, x, y, mask);
    dynamic_cast<Gtk::Widget &>(*root).translate_coordinates(widget, x, y, x, y);
    return Geom::Point{x, y};
}

constexpr int MARGIN = 4;

} // namespace


/// A purely visual version of a Tab that is used as a dummy during drag-and-drop.
struct SimpleTab : Gtk::Widget
{
    Gtk::Label _name;
    Gtk::Button _close;
    Gtk::Image _handle;
    Gtk::Image _icon;
    TabStrip::ShowLabels _show_labels = TabStrip::ShowLabels::Never;
    bool _show_close_btn = true;

    SimpleTab(const SimpleTab& src) : SimpleTab() {
        _name.set_text(src._name.get_text());
        _name.set_visible(src._name.get_visible());
        _icon.set_from_icon_name(src._icon.property_icon_name());
        _handle.set_visible(src._handle.get_visible());
        _close.set_visible(src._close.get_visible());
        _show_labels = src._show_labels;
        _show_close_btn = src._show_close_btn;
    }

    SimpleTab()
    {
        _name.set_halign(Gtk::Align::START);
        _name.set_xalign(0);
        _handle.set_from_icon_name("dnd");
        _handle.set_visible(false);

        _handle.set_can_target(false);
        _icon.set_can_target(false);
        _name.set_can_target(false);

        _close.set_visible(false);
        _close.set_has_frame(false);
        _close.add_css_class("close-button");
        _close.set_focus_on_click(false);
        _close.set_icon_name("window-close");
        _close.set_halign(Gtk::Align::CENTER);
        _close.set_valign(Gtk::Align::CENTER);

        _handle.insert_at_end(*this);
        _icon.insert_at_end(*this);
        _name.insert_at_end(*this);
        _close.insert_at_end(*this);
        containerize(*this);
        set_name("SimpleTab");
        set_overflow(Gtk::Overflow::HIDDEN);
        property_accessible_role().set_value(Gtk::Accessible::Role::TAB);
    }

    void set_active() {
        set_state_flags(Gtk::StateFlags::CHECKED, false);
        if (_show_close_btn) {
            _close.set_visible();
        }
        if (_show_labels == TabStrip::ShowLabels::ActiveOnly) {
            _name.set_visible();
        }
    }
    void set_inactive() {
        unset_state_flags(Gtk::StateFlags::CHECKED);
        if (_show_close_btn) {
            _close.set_visible(false);
        }
        if (_show_labels == TabStrip::ShowLabels::ActiveOnly) {
            _name.set_visible(false);
        }
    }
    Glib::ustring get_label() const { return _name.get_text(); }

    void show_handle(bool show) {
        _handle.set_visible(show);
    }

    void update(bool is_active) {
        _close.set_visible(_show_close_btn && is_active);

        _name.set_visible(
            _show_labels != TabStrip::ShowLabels::Never && (_show_labels == TabStrip::ShowLabels::Always || (_show_labels == TabStrip::ShowLabels::ActiveOnly && is_active))
        );
    }

    void measure_vfunc(Gtk::Orientation orientation, int, int &min, int &nat, int &, int &) const override {
        {
            auto [sizes, baselines] = _icon.measure(orientation);
            // normal icon size with margins
            auto icon_size = sizes.minimum + 2 * MARGIN;
            // let the tab shrink to roughly half the icon size (if we really open a lot of dialogs)
            min = icon_size / 2;
            // more generous natural size: twice the icon size; that's what we use when there is space
            nat =  orientation == Gtk::Orientation::VERTICAL || _name.get_visible() ? icon_size : icon_size * 2;

            // for vert measurements: all elements are in one row, so just use icon size
            if (orientation == Gtk::Orientation::VERTICAL) return;
        }
        // reserve space for the close button if there is one shown
        if (_close.get_visible()) {
            auto [sizes, baselines] = _close.measure(orientation);
            min += sizes.minimum + MARGIN;
            nat += sizes.natural + MARGIN;
        }
        // same for labels
        if (_name.get_visible()) {
            auto [sizes, baselines] = _name.measure(orientation);
            // do not inflate min size, so that labels can collapse to nothing and become hidden
            nat += sizes.natural + MARGIN;
        }
        // space for handle, if visible
        if (_handle.get_visible()) {
            auto [sizes, baselines] = _handle.measure(orientation);
            min += sizes.minimum + 2 * MARGIN;
            nat += sizes.natural + 2 * MARGIN;
        }

        if (min > nat) {
            nat = min;
        }
    }

    void size_allocate_vfunc(int full_width, int height, int) override {
        auto icon_w = _icon.measure(Gtk::Orientation::HORIZONTAL, -1).sizes.natural;

        // size_allocate - position and size all child widgets: icon, label and close button

        bool center_icon = false; // center the icon? we will if there's no text label
        int width = full_width;
        // for tracking widgets' position
        int x = MARGIN, y = 0;
        width -= 2 * MARGIN;

        auto handle_w = _handle.get_visible() ? _handle.measure(Gtk::Orientation::HORIZONTAL, -1).sizes.natural : 0;
        width -= handle_w;

        // icon on the left, we can center it later if needed
        _icon.size_allocate(Gtk::Allocation(x, y, icon_w, height), -1);
        width -= icon_w;
        x += icon_w;

        // measure the close button if it's visible, it will go to the right
        auto close_w = 0;
        if (_close.get_visible()) {
            close_w = _close.measure(Gtk::Orientation::HORIZONTAL, -1).sizes.natural;
            if (close_w >= width) {
                _close.set_opacity(0);
                close_w = 0;
            }
            else {
                _close.set_opacity(1);
                width -= close_w + MARGIN;
            }
        }

        // text label, if any
        if (_name.get_visible()) {
            auto name_w = _name.measure(Gtk::Orientation::HORIZONTAL, -1).sizes.natural;
            auto opacity = _name.get_opacity();
            auto delta = width - MARGIN - icon_w;
            bool show_label = true;
            // do we have enough space to show some text?
            if (delta == 0 && opacity == 0) { // this is a hysteresis to avoid flickering
                show_label = false;
            }
            else if (delta < 0) {
                show_label = false;
            }
            if (show_label) {
                _name.set_opacity(1);
                width -= MARGIN;
                x += MARGIN;
                int w = std::min(width, name_w);
                _name.size_allocate(Gtk::Allocation(x, y, w, height), -1);
                x += w;
                width -= w;
            }
            else {
                // not enough space - hide text using 0 opacity (cannot use visibility; it would trigger re-alloc)
                _name.set_opacity(0);
                // move the label out of the way so it doesn't interfere with the close button;
                // do not size it to 0, gtk doesn't like that and complains
                _name.size_allocate(Gtk::Allocation(MARGIN, y, 10, height), -1);
                center_icon = true;
            }
        }
        else {
            center_icon = true;
        }

        if (close_w > 0) {
            x += MARGIN;
            _close.size_allocate(Gtk::Allocation(full_width - close_w - MARGIN, y, close_w, height), -1);
        }

        // last comes dragging handle
        if (_handle.get_visible()) {
            _handle.size_allocate(Gtk::Allocation(std::max(x, full_width - handle_w - MARGIN), y, handle_w, height), -1);
            center_icon = false;
        }

        if (center_icon) {
            _icon.size_allocate(Gtk::Allocation((full_width - close_w - icon_w) / 2, y, icon_w, height), -1);
        }
    }
};

/// The actual tabs that are shown in the tab bar.
struct TabWidget : CssNameClassInit, SimpleTab
{
    TabStrip *const parent;

    TabWidget(TabStrip* parent)
        : Glib::ObjectBase("TabWidget")
        , CssNameClassInit{"tab"}
        , parent{parent}
    {
        set_has_tooltip(true);
    }
};

struct DnDTabWidget {
    int tab_position = -1;
    TabStrip* parent = nullptr;
};

class TabWidgetDrag
{
public:
    /// Create and start a tab drag.
    /// All drags are assumed to start off local, i.e. the source and destination tab bars are the same.
    /// The result must therefore be assigned to the source tab bar's _drag_src and _drag_dst.
    TabWidgetDrag(TabWidget *src, Geom::Point const &offset, Gtk::Orientation orientation, Glib::RefPtr<Gdk::Device> device)
        : _src{src}
        , _offset{offset}
        , _orientation{orientation}
        , _device{std::move(device)}
        , _dst{src->parent} {
        _src->set_cursor("grabbing");
    }
    ~TabWidgetDrag() = default;

    /// Called by dst whenever the pointer moves, whether over it or not. This sometimes requires polling.
    /// Updates the tab's position within dst or detaches it.
    void motion(std::optional<Geom::Point> pos)
    {
        constexpr int detach_dist = 25;
        if (pos && Geom::Rect(0, 0, _dst->get_width(), _dst->get_height()).distanceSq(*pos) < Geom::sqr(detach_dist)) {
            // Pointer is still sufficiently near dst - update drop position.
            _drop_pos = _orientation == Gtk::Orientation::HORIZONTAL ?
                pos->x() - static_cast<int>(std::round(_offset.x())) :
                pos->y() - static_cast<int>(std::round(_offset.y()));
            _dst->queue_allocate();
            // temporarily hide (+) button too
            _src->parent->_plus_btn.set_visible(false);
        } else if (_dst->_rearrange == TabStrip::Rearrange::Externally) {
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
    void setDst(TabStrip *new_dst)
    {
        if (new_dst == _dst
            || _src->parent->_rearrange != TabStrip::Rearrange::Externally) {
            return;
        }

        if (_dst) {
            _dst->_drag_dst = {};
            _dst->queue_resize();
        }

        _dst = new_dst;

        if (_dst) {
            _dst->_drag_dst = _src->parent->_drag_src;
            _drop_pos = {};
            _drop_i = {};
        }

        _queueReparent();
    }

    /// End the drag. This function unsets any _drag_src or _drag_dst references that point to this object,
    /// indirectly destroying this object. It is ensured that this function is called exactly once during
    /// every TabWidgetDrag's lifetime. Thus, this function is the *only* way that a TabWidgetDrag is destroyed.
    void finish(bool cancel = false)
    {
        // Cancel the tick callback if one is being used for motion polling.
        cancelTick();

        // Reset cursor BEFORE moving the shared_ptr to ensure _src is still valid
        if (_src->get_parent()) {
            Glib::RefPtr<Gdk::Cursor> null;
            _src->set_cursor(null);
        }

        // Detach from source and destination, keeping self alive until end of function.
        auto const self_ref = std::move(_src->parent->_drag_src);
        assert(self_ref.get() == this);
        if (_dst) {
            _dst->_drag_dst = {};
        }

        // Undo widget modifications to source and destination.
        _src->set_visible(true);
        _src->parent->_update_new_tab();
        _src->parent->queue_resize();
        if (_dst) {
            if (_widget && _widget->get_parent() == _dst) {
                _widget->unparent();
            }
            _dst->queue_resize();
        }
        _src->parent->_signal_dnd_end.emit(cancel);

        if (cancel) {
            return;
        }

        if (_drag) {
            _drag->drag_drop_done(true); // suppress drag-failed animation
        }

        if (!_dst) {
            // no op: let clients handle tab drop any way they want
        } else if (_dst == _src->parent) {
            // Reorder
            if (_drop_i) {
                if (_src->parent->_rearrange != TabStrip::Rearrange::Never) {
                    int const from = _src->parent->get_tab_position(*_src);
                    if (_src->parent->_reorderTab(from, *_drop_i)) {
                        _src->parent->_signal_tab_rearranged(from, *_drop_i - ( *_drop_i > from ));
                    }
                    else {
                        _src->parent->queue_resize();
                    }
                }
            }
        } else if (_src->parent->_rearrange == TabStrip::Rearrange::Externally) {
            // Migrate
            if (_drop_i) {
                _dst->_signal_move_tab.emit(*_src, _src->parent->get_tab_position(*_src), *_src->parent, *_drop_i);
            }
        }
    }

    TabWidget *src() const { return _src; }
    SimpleTab *widget() const { return _widget.get(); }
    std::optional<int> const &dropPos() const { return _drop_pos; }
    void setDropI(int i) { _drop_i = i; }

private:
    TabWidget *const _src; // The source tab.
    Geom::Point const _offset; // The point within the tab that the drag started from.
    Gtk::Orientation const _orientation; // orientation of the TabStrip
    Glib::RefPtr<Gdk::Device> const _device; // The pointing device that started the drag.

    TabStrip *_dst; // The destination tabs widget, possibly null.
    std::optional<int> _drop_pos; // Position (either x or y) within dst where tab is dropped.
    std::optional<int> _drop_i; // The index within dst where the tab is dropped.

    sigc::scoped_connection _reparent_conn; // Used to defer reparenting of widget.
    sigc::scoped_connection _cancel_conn; // Connection to the Gdk::Drag's cancel signal.
    sigc::scoped_connection _drop_conn; // Connection to the Gdk::Drag's drop-perfomed signal.
    Glib::RefPtr<Gdk::Drag> _drag; // The Gdk::Drag, lazily created at the same time as widget.
    std::unique_ptr<SimpleTab> _widget; // The dummy tab that is placed inside the drag icon or dst.
    guint _tick_callback = 0; // If nonzero, the tick callback used for motion polling.

    void _ensureDrag()
    {
        if (_drag) {
            return;
        }

        // Create the Gdk drag.
        assert(_src->parent->_drag_src.get() == this);
        std::vector providers = {
            Gdk::ContentProvider::create(Util::GlibValue::create<DnDTabWidget>(DnDTabWidget{_src->parent->get_tab_position(*_src), _src->parent})),
            Gdk::ContentProvider::create(GlibValue::create<std::weak_ptr<TabWidgetDrag>>(_src->parent->_drag_src))
        };
        auto content = Gdk::ContentProvider::create(providers);
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
        _src->parent->_update_new_tab();

        // Create a visual replica of the tab.
        _widget = std::make_unique<SimpleTab>(*_src);
        _widget->set_active();

        // fire D&D event
        _src->parent->_signal_dnd_begin.emit();
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
            assert(dynamic_cast<TabStrip *>(_widget->get_parent()));
            _widget->unparent();
        }

        // Put the replica tab inside dst or the drag icon.
        if (_dst) {
            _widget->insert_before(*_dst, *_dst->_overlay);
            _dst->queue_resize();
        } else {
            drag_icon->set_child(*_widget);
            _drag->set_hotspot(_offset.x(), _offset.y());
        }
    }
};

static std::shared_ptr<TabWidgetDrag> get_tab_drag(Gtk::DropTarget &droptarget)
{
    // Determine whether an in-app tab is being dragged and get information about it.
    auto const drag = droptarget.get_drop()->get_drag();
    if (!drag) {
        return {}; // not in-app
    }
    auto const content = GlibValue::from_content_provider<std::weak_ptr<TabWidgetDrag>>(*drag->get_content());
    if (!content) {
        return {}; // not a tab
    }
    return content->lock();
}

void TabStrip::construct()
{
    set_name("TabStrip");
    set_overflow(Gtk::Overflow::HIDDEN);
    containerize(*this);
    property_accessible_role().set_value(Gtk::Accessible::Role::TAB_LIST);

    _plus_btn.set_name("NewTabButton");
    _plus_btn.set_valign(Gtk::Align::CENTER);
    _plus_btn.set_has_frame(false);
    _plus_btn.set_focusable(false);
    _plus_btn.set_focus_on_click(false);
    _plus_btn.set_can_focus(false);
    _plus_btn.set_icon_name("list-add");
    _plus_btn.insert_at_end(*this);

    _overlay->insert_at_end(*this); // always kept topmost
    _overlay->set_name("Overlay");

    auto click = Gtk::GestureClick::create();
    click->set_button(0);
    click->signal_pressed().connect([this, click = click.get()] (int, double x, double y) {
        // Find clicked tab.
        auto const [tab_weak, tab_pos] = _tabAtPoint({x, y});
        auto tab = tab_weak.lock();
        auto btn = click->get_current_button();

        // Handle button actions.
        switch (btn) {
            case GDK_BUTTON_PRIMARY:
            case GDK_BUTTON_SECONDARY:
                // primary and secondary button presses activate a tab
                if (tab) {
                    double xc, yc;
                    translate_coordinates(tab->_close, x, y, xc, yc);
                    if (!tab->_close.contains(xc, yc)) {
                        if (btn == GDK_BUTTON_PRIMARY) {
                            // only the primary button can start tab dragging
                            _left_clicked = tab_weak;
                            _left_click_pos = {x, y};
                        }
                        _signal_select_tab.emit(*tab);
                    }
                }
                if (btn == GDK_BUTTON_SECONDARY) {
                    // secondary button press pops up contextual menu
                    if (tab && _popover && is_tab_active(*tab)) {
                        auto size = tab->get_allocation();
                        // center/bottom location for a context menu tip
                        UI::popup_at(*_popover, *tab, size.get_width() / 2, size.get_height() - 7);
                    }
                }
                break;
            case GDK_BUTTON_MIDDLE:
                if (tab) {
                    _signal_close_tab.emit(*tab);
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

            constexpr int drag_initiate_dist = 8;
            if ((Geom::Point{x, y} - _left_click_pos).lengthSq() < Geom::sqr(drag_initiate_dist)) {
                return;
            }

            _left_clicked = {};

            Geom::Point offset;
            translate_coordinates(*tab, _left_click_pos.x(), _left_click_pos.y(), offset.x(), offset.y());

            // Start dragging.
            _drag_src = _drag_dst = std::make_shared<TabWidgetDrag>(
                tab.get(),
                offset,
                get_orientation(),
                motion.get_current_event_device()
            );

            // Raise dragged tab to top.
            tab->insert_before(*this, _plus_btn);
        }

        if (!_drag_src->widget()) {
            _drag_src->motion(Geom::Point{x, y});
        }
    });
    add_controller(motion);

    auto droptarget = Gtk::DropTarget::create(GlibValue::type<std::weak_ptr<TabWidgetDrag>>(), Gdk::DragAction::MOVE);
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

    _updateVisibility();
}

TabStrip::TabStrip(Gtk::Orientation orientation)
    : Glib::ObjectBase("TabStrip")
    , CssNameClassInit{"tabs"}
    , _overlay{Gtk::make_managed<PointerTransparentWidget>()}
{
    set_orientation(orientation);
    construct();
}

TabStrip::TabStrip(GtkWidget* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
    : Glib::ObjectBase("TabStrip")
    , CssNameClassInit{"tabs"}
    , BuildableWidget(cobject, builder)
    , _overlay{Gtk::make_managed<PointerTransparentWidget>()}
{
    construct();
}

TabStrip::~TabStrip()
{
    // Note: This code will fail if TabStrip becomes a managed widget, in which
    // case it must be done on signal_destroy() instead.
    if (_drag_dst) {
        _drag_dst->setDst(nullptr);
    }
    if (_drag_src) {
        _drag_src->finish(true);
    }
}

Gtk::Widget* TabStrip::add_tab(const Glib::ustring& label, const Glib::ustring& icon, int pos)
{
    auto tab = std::make_shared<TabWidget>(this);
    tab->_handle.set_visible(_show_drag_handles);
    tab->_name.set_text(label);
    tab->_icon.set_from_icon_name(icon);
    tab->_show_close_btn = _show_close_btn;
    tab->_show_labels = _show_labels;
    tab->update(false);

    auto ptr_tab = tab.get();
    tab->_close.signal_clicked().connect([this, ptr_tab] { _signal_close_tab.emit(*ptr_tab); });

    tab->signal_query_tooltip().connect([this, ptr_tab] (int, int, bool, Glib::RefPtr<Gtk::Tooltip> const &tooltip) {
        _setTooltip(*ptr_tab, tooltip);
        return true; // show the tooltip
    }, true);

    if (pos == -1) {
        pos = _tabs.size();
    }
    else if (pos < 0 || pos >= _tabs.size()) {
        pos = _tabs.size();
    }
    assert(0 <= pos && pos <= _tabs.size());

    tab->insert_before(*this, _plus_btn);
    _tabs.insert(_tabs.begin() + pos, tab);

    _updateVisibility();
    return tab.get();
}

void TabStrip::remove_tab(const Gtk::Widget& tab)
{
    int const i = get_tab_position(tab);
    assert(i != -1);
    if (i < 0) {
        g_warning("TabStrip:remove_tab(): attempt to remove a tab that doesn't belong to this widget");
        return;
    }

    if (_drag_src && _drag_src->src() == _tabs[i].get()) {
        _drag_src->finish(true);
    }

    _tabs[i]->unparent();
    _tabs.erase(_tabs.begin() + i);

    _updateVisibility();
}

void TabStrip::remove_tab_at(int pos) {
    if (auto tab = get_tab_at(pos)) {
        remove_tab(*tab);
    }
}

bool TabStrip::is_tab_active(const Gtk::Widget& tab) const {
    auto const active = _active.lock();

    return active && active.get() == &tab;
}

void TabStrip::set_show_close_button(bool show) {
    _show_close_btn = show;
    for (auto& tab : _tabs) {
        tab->_show_close_btn = show;
        tab->update(is_tab_active(*tab));
    }
    queue_allocate();
}

GType TabStrip::get_dnd_source_type() {
    return GlibValue::type<DnDTabWidget>();
}

std::optional<std::pair<TabStrip*, int>> TabStrip::unpack_drop_source(const Glib::ValueBase& value) {

    if (G_VALUE_HOLDS(value.gobj(), get_dnd_source_type())) {
        auto val = static_cast<const Glib::Value<DnDTabWidget>&>(value).get();
        return std::make_pair(val.parent, val.tab_position);
    }

    return {};
}

void TabStrip::set_draw_handle(bool show) {
    if (_show_drag_handles == show) return;

    _show_drag_handles = show;
    // propagate changes to tabs
    for (auto& tab : _tabs) {
        tab->show_handle(show);
    }
}

void TabStrip::select_tab(const Gtk::Widget& tab)
{
    auto const active = _active.lock();

    if (active && active.get() == &tab) {
        return;
    }

    if (active) {
        active->set_inactive();
        _active = {};
    }

    int const i = get_tab_position(tab);
    if (i != -1) {
        _tabs[i]->set_active();
        _active = _tabs[i];
    }
}

void TabStrip::select_tab_at(int pos) {
    if (auto tab = get_tab_at(pos)) {
        select_tab(*tab);
    }
}

void TabStrip::set_tabs_order(std::vector<Gtk::Widget *> sorted)
{
    std::sort(_tabs.begin(), _tabs.end(), [sorted](const auto& a, const auto& b) {
        return std::find(sorted.begin(), sorted.end(), a.get())
             < std::find(sorted.begin(), sorted.end(), b.get());
    });
    queue_resize();
}

std::vector<Gtk::Widget *> TabStrip::get_tabs() const
{
    auto range = _tabs | std::views::transform([](auto& tab){ return tab.get();});
    return std::vector<Gtk::Widget*>(range.begin(), range.end());
}

int TabStrip::get_tab_position(const Gtk::Widget& tab) const
{
    for (int i = 0; i < _tabs.size(); i++) {
        if (_tabs[i].get() == &tab) {
            return i;
        }
    }
    return -1;
}

Gtk::Widget* TabStrip::get_tab_at(int i) const
{
    auto index = static_cast<size_t>(i);
    return index < _tabs.size() ? _tabs[index].get() : nullptr;
}

void TabStrip::set_new_tab_popup(Gtk::Popover* popover) {
    if (popover) {
        _plus_btn.set_popover(*popover);
    }
    else {
        _plus_btn.unset_popover();
    }
    _update_new_tab();
}

/**
 * Keep the new_tab button in sync with the popover widget.
 */
void TabStrip::_update_new_tab()
{
    // show (+) button when there's a popover, but only in horizontal layout
    _plus_btn.set_visible(_plus_btn.get_popover() && get_orientation() == Gtk::Orientation::HORIZONTAL);
}

void TabStrip::set_tabs_context_popup(Gtk::Popover* popover) {
    if (_popover) _popover->unparent();
    _popover = nullptr;
    if (popover) {
        popover->set_parent(*this);
        _popover = popover;
    }
}

void TabStrip::set_rearranging_tabs(Rearrange rearrange) {
    _rearrange = rearrange;
}

void TabStrip::set_show_labels(ShowLabels labels) {
    _show_labels = labels;
    // refresh tabs
    for (auto& tab : _tabs) {
        tab->_show_labels = labels;
        tab->update(is_tab_active(*tab));
    }
    queue_allocate();
}

void TabStrip::_updateVisibility()
{
    // always visible
    // set_visible(_tabs.size() > 1);
}

TabWidget* TabStrip::find_tab(Gtk::Widget& tab) {
    for (auto& t : _tabs) {
        if (t.get() == &tab) {
            return t.get();
        }
    }
    return nullptr;
}

Gtk::SizeRequestMode TabStrip::get_request_mode_vfunc() const
{
    return Gtk::SizeRequestMode::CONSTANT_SIZE;
}

void TabStrip::measure_vfunc(Gtk::Orientation orientation, int, int &min, int &nat, int &, int &) const
{
    // orientation of the tab strip
    auto layout = get_orientation();

    if (orientation != layout) {
        min = 0;
        auto consider = [&] (Gtk::Widget const &w) {
            auto const m = w.measure(orientation, -1);
            min = std::max(min, m.sizes.minimum);
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
        nat = min;
    } else {
        min = 0;
        nat = 0;
        for (auto const &tab : _tabs) {
            const auto [sizes, baselines] = tab->measure(layout, -1);
            min += sizes.minimum;
            nat += sizes.natural;
        }
        if (_plus_btn.is_visible()) {
            const auto [sizes, baselines] = _plus_btn.measure(layout, -1);
            min += sizes.minimum;
            nat += sizes.natural;
        }
    }
}

namespace {

struct Size {
    int minimum;
    int delta;   // value to shrink
    bool expand; // should it expand
    int index; // original location
    int size() const { return minimum + delta; }
};

// Decrease sizes until they meet target value by subtracting given amount
void shrink_sizes(std::vector<Size>& sizes, int decrease) {
    if (sizes.empty() || decrease <= 0) return;

    // sort by size, so we start shrinking by reducing size of the longest components first
    std::sort(begin(sizes), end(sizes), [](const Size& a, const Size& b){ return a.delta > b.delta; });

    // how much space can we save?
    int available = std::accumulate(begin(sizes), end(sizes), 0, [](int acc, auto& s){ acc += s.delta; return acc; });
    decrease = std::min(available, decrease);

    // sentry
    sizes.emplace_back(Size{ .minimum = 0, .delta = 0, .expand = false, .index = 99999 });

    auto entry = &sizes.front();
    while (decrease > 0) {
        //todo: improve performance
        entry->delta -= 1;
        decrease--;
        if (decrease == 0) break;

        if (entry[1].delta > entry->delta) {
            ++entry;
        }
        else {
            entry = &sizes.front();
        }
    }

    // restore order
    std::sort(begin(sizes), end(sizes), [](const Size& a, const Size& b){ return a.index < b.index; });
}

// Expand sizes until they meet target value by adding given amount
void expand_sizes(std::vector<Size>& sizes, int increase) {
    if (sizes.empty() || increase <= 0) return;

    // Sort, expanding elements first, then by current size
    std::sort(begin(sizes), end(sizes), [](const Size& a, const Size& b){
        if (a.expand != b.expand) {
            return a.expand > b.expand;
        }
        return a.size() < b.size();
    });

    // sentry
    sizes.emplace_back(Size{ .minimum = 0, .delta = 0, .expand = false, .index = 99999 });

    while (increase > 0) {
        // Find all sizes of the same size
        auto i = 0;
        while (sizes[i].size() == sizes[i+1].size() && sizes[i+1].expand) {
            i++;
        }
        // Expand until all previous match the next size up
        auto to_increase = increase / (i + 1);
        if (sizes[i+1].expand) {
            to_increase = std::min(to_increase, (sizes[i+1].size() - sizes[i].size()));
        }
        // Add to the delta for each element of the same size
        for (auto j = 0; j <= i; j++) {
            sizes[j].delta += to_increase;
            increase -= to_increase;
        }
        // If this was the final expansion, distribute any remaining pixels which is guaranteed to be <= i
        if (!sizes[i+1].expand) {
            for (auto j = 0; j <= i && increase > 0; j++) {
                sizes[j].delta += 1;
                increase--;
            }
        }
    }

    // restore order
    std::sort(begin(sizes), end(sizes), [](const Size& a, const Size& b){ return a.index < b.index; });
}

} // namespace

void Inkscape::UI::Widget::TabStrip::size_allocate_vfunc(int width, int height, int)
{
    auto layout = get_orientation();
    auto plus_w = _plus_btn.get_visible() ? _plus_btn.measure(Gtk::Orientation::HORIZONTAL, -1).sizes.natural : 0;

    _overlay->size_allocate(Gtk::Allocation(0, 0, width, height), -1);

    // limit width by removing plus button's size
    width -= plus_w;

    struct Drop
    {
        int loc;
        int size;
        SimpleTab *widget;
        bool done = false;
    };
    std::optional<Drop> drop;

    // measure all tabs and find all the deltas of natural size minus min size for each tab
    std::vector<Size> alloc;
    int minimum = 0;
    int total = 0;
    bool has_expanding = false;
    // gather all measurements
    alloc.reserve(_tabs.size() + 1);
    for (int i = 0; i < _tabs.size(); i++) {
        auto const tab = _tabs[i].get();
        auto [min, natural] = tab->measure(layout, -1).sizes;
        total += natural;
        minimum += min;
        alloc.emplace_back(Size{ .minimum = min, .delta = natural - min, .expand = tab->get_hexpand(), .index = i });
        has_expanding = has_expanding || tab->get_hexpand();
    }

    if (layout == Gtk::Orientation::VERTICAL) {
        // position tabs from top to bottom using their min size;
        // at the moment no-op
    }
    // check available width; restrict tab sizes if space is limited
    else if (width <= minimum) {
        // shrink to the minimum size, there's no wiggle room
        for (int i = 0; i < _tabs.size(); i++) {
            alloc[i].delta = 0;
        }
    }
    else if (width < total) {
        // We shall have to economise, Gromit.
        shrink_sizes(alloc, total - width);
    } else if (has_expanding) {
        // In fact, let them grow bigger and stronger than anyone else's
        expand_sizes(alloc, width - total);
    }

    if (_drag_dst && _drag_dst->dropPos()) {
        auto widget = !_drag_dst->widget()
            ? _drag_dst->src()
            : _drag_dst->widget();
        if (widget->get_parent() == this) {
            int pos = get_tab_position(*widget);
            int size = widget->measure(layout, -1).sizes.natural;
            if (pos >= 0) {
                size = alloc[pos].size();
            }
            auto limit = (layout == Gtk::Orientation::HORIZONTAL ? width : height) - size;
            auto loc = limit > 0 ? std::clamp(*_drag_dst->dropPos(), 0, limit) : 0;
            drop = Drop{ .loc = loc, .size = size, .widget = widget };
        }
    }

    // position and size tabs
    int pos = 0;
    for (int i = 0; i < _tabs.size(); i++) {
        const auto& a = alloc[i];
        auto const tab = _tabs[i].get();

        if (_drag_src && tab == _drag_src->src()) {
            continue;
        }
        int size = a.size();
        if (drop && !drop->done && pos + size / 2 > drop->loc) {
            pos += drop->size;
            _drag_dst->setDropI(i);
            drop->done = true;
        }
        if (layout == Gtk::Orientation::HORIZONTAL) {
            tab->size_allocate(Gtk::Allocation(pos, 0, size, height), -1);
        }
        else {
            tab->size_allocate(Gtk::Allocation(0, pos, width, size), -1);
        }
        pos += size;
    }

    if (_plus_btn.get_visible()) {
        _plus_btn.size_allocate(Gtk::Allocation(pos, 0, plus_w, height), -1);
    }

    // GTK burdens custom widgets with having to implement this manually.
    if (_popover) {
        _popover->present();
    }

    if (drop) {
        if (!drop->done) {
            _drag_dst->setDropI(_tabs.size());
        }
        if (layout == Gtk::Orientation::HORIZONTAL) {
            drop->widget->size_allocate(Gtk::Allocation(drop->loc, 0, drop->size, height), -1);
        }
        else {
            drop->widget->size_allocate(Gtk::Allocation(0, drop->loc, width, drop->size), -1);
        }
    }
}

void TabStrip::_setTooltip(const TabWidget& tab, Glib::RefPtr<Gtk::Tooltip> const &tooltip)
{
    tooltip->set_text(tab._name.get_text());
}

std::pair<std::weak_ptr<TabWidget>, Geom::Point> TabStrip::_tabAtPoint(Geom::Point const &pos)
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

bool TabStrip::_reorderTab(int from, int to)
{
    assert(0 <= from && from < _tabs.size());
    assert(0 <= to && to <= _tabs.size());

    if (from == to || from + 1 == to) {
        return false;
    }

    auto tab = std::move(_tabs[from]);
    _tabs.erase(_tabs.begin() + from);
    _tabs.insert(_tabs.begin() + to - (to > from), std::move(tab));
    return true;
}

} // namespace Inkscape::UI::Widget

// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * A class to render the SVG drawing.
 */

/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Rewrite of _SPCanvasArena.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <thread>

#include "canvas-item-drawing.h"

#include "desktop.h"

#include "display/drawing.h"
#include "display/drawing-context.h"
#include "display/drawing-item.h"
#include "display/drawing-group.h"

#include "helper/geom.h"
#include "ui/widget/canvas.h"
#include "ui/widget/events/canvas-event.h"
#include "ui/modifiers.h"

namespace Inkscape {

/**
 * Create the drawing. One per window!
 */
CanvasItemDrawing::CanvasItemDrawing(CanvasItemGroup *group)
    : CanvasItem(group)
    , _drawing{std::make_unique<Drawing>()}
{
    _name = "CanvasItemDrawing";
    _pickable = true;

    auto root = new DrawingGroup(*_drawing);
    root->setPickChildren(true);
    _drawing->setRoot(root);

    _drawing_updated_connection = _drawing->connectDrawingUpdated([this]() {
        request_update();
    });
    _redraw_area_connection = _drawing->connectRedrewArea([this](Geom::IntRect area) {
        get_canvas()->redraw_area(area);
    });
    _active_item_deleted = _drawing->connectItemDeleted([this](unsigned key) {
        if (_active_item && _active_item->first == key) set_active(nullptr);
    });

    _loadPrefs();
}

CanvasItemDrawing::~CanvasItemDrawing() = default;

void CanvasItemDrawing::set_active(Inkscape::DrawingItem *active)
{
    if (active) {
        _active_item = {active->key(), active};
    } else {
        _active_item.reset();
    }
}

static auto default_numthreads()
{
    auto ret = std::thread::hardware_concurrency();
    return ret == 0 ? 4 : ret; // Sensible fallback if not reported.
}

/**
 * Update the Drawing object with all the required settings from prefs
 */
void CanvasItemDrawing::_loadPrefs()
{
    auto prefs = Inkscape::Preferences::get();

    // Preference is stored in MiB; convert to bytes, taking care not to overflow.
    _drawing->setCacheBudget((size_t{1} << 20) * prefs->getIntLimited("/options/renderingcache/size", 64, 0, 4096));

    std::unordered_map<std::string, std::function<void (Preferences::Entry const &)>> actions;

    // Todo: (C++20) Eliminate this repetition by baking the preference metadata into the variables themselves using structural templates.
    actions.emplace("/options/wireframecolors/default",      [this] (auto &entry) { _drawing->setOutlineColor(entry.getColor("#000000")); });
    actions.emplace("/options/wireframecolors/clips",        [this] (auto &entry) { _drawing->setClipOutlineColor (entry.getColor("#00ff00")); });
    actions.emplace("/options/wireframecolors/masks",        [this] (auto &entry) { _drawing->setMaskOutlineColor (entry.getColor("#0000ff")); });
    actions.emplace("/options/wireframecolors/images",       [this] (auto &entry) { _drawing->setImageOutlineColor(entry.getColor("#ff0000")); });
    actions.emplace("/options/rendering/imageinoutlinemode", [this] (auto &entry) { _drawing->setImageOutlineMode(entry.getBool(false)); });
    actions.emplace("/options/filterquality/value",          [this] (auto &entry) { _drawing->setFilterQuality(entry.getIntLimited(0, Filters::FILTER_QUALITY_WORST, Filters::FILTER_QUALITY_BEST)  ); });
    actions.emplace("/options/blurquality/value",            [this] (auto &entry) { _drawing->setBlurQuality(entry.getInt(0)); });
    actions.emplace("/options/dithering/value",              [this] (auto &entry) { _drawing->setDithering(entry.getBool(true)); });
    actions.emplace("/options/selection/zeroopacity",        [this] (auto &entry) { _drawing->setSelectZeroOpacity(entry.getBool(false)); });
    actions.emplace("/options/renderingcache/size",          [this] (auto &entry) { _drawing->setCacheBudget((1 << 20) * entry.getIntLimited(64, 0, 4096)); });
    actions.emplace("/options/threading/numthreads",         [this] (auto &entry) { _drawing->setNumDispatchThreads(entry.getIntLimited(default_numthreads(), 1, 256)); });

    actions.emplace("/options/cursortolerance/value",        [this] (auto &entry) { _cursor_tolerance = entry.getDouble(1.0); });

    _pref_tracker = Inkscape::Preferences::PreferencesObserver::create("/options", [actions = std::move(actions)] (auto &entry) {
        auto it = actions.find(entry.getPath());
        if (it == actions.end()) return;
        it->second(entry);
    });

    // Set right away all the above values
    _pref_tracker->call();
}

/**
 * Returns true if point p (in canvas units) is inside some object in drawing.
 */
bool CanvasItemDrawing::contains(Geom::Point const &p, double tolerance)
{
    if (tolerance != 0) {
        std::cerr << "CanvasItemDrawing::contains: Non-zero tolerance not implemented!" << std::endl;
    }

    if(_drawing->pick(p, _cursor_tolerance, get_canvas()->get_area_world(), get_flags())) {
        // This will trigger a signal that is handled by our event handler. Seems a bit of a
        // round-about way of doing things but it matches what other pickable canvas-item classes do.
        return true;
    }

    return false;
}

/**
 * Update and redraw drawing.
 */
void CanvasItemDrawing::_update(bool)
{
    // Undo y-axis flip. This should not be here!!!!
    auto new_drawing_affine = affine();
    if (auto desktop = get_canvas()->get_desktop()) {
        new_drawing_affine = desktop->doc2dt() * new_drawing_affine;
    }

    bool affine_changed = _drawing_affine != new_drawing_affine;
    if (affine_changed) {
        _drawing_affine = new_drawing_affine;
    }

    _drawing->update(Geom::IntRect::infinite(), _drawing_affine, DrawingItem::STATE_ALL, affine_changed * DrawingItem::STATE_ALL);

    _bounds = expandedBy(_drawing->root()->drawbox(), 1); // Avoid aliasing artifacts

    if (_cursor) {
        /* Mess with enter/leave notifiers */
        auto new_drawing_item = _drawing->pick(_c, _delta, get_canvas()->get_area_world(), get_flags());
        if (!_active_item || _active_item->second != new_drawing_item) {
            // Fixme: These crossing events have no modifier state set.

            if (_active_item) {
                auto event = LeaveEvent();
                _drawing_event_signal.emit(event, _active_item->second);
            }

            set_active(new_drawing_item);

            if (_active_item) {
                auto event = EnterEvent();
                event.pos = _c;
                _drawing_event_signal.emit(event, _active_item->second);
            }
        }
    }
}

/**
 * Render drawing to screen via Cairo.
 */
void CanvasItemDrawing::_render(Inkscape::CanvasItemBuffer &buf) const
{
    auto dc = Inkscape::DrawingContext(buf.cr->cobj(), buf.rect.min());
    _drawing->render(dc, buf.rect, buf.outline_pass * DrawingItem::RENDER_OUTLINE);
}

/**
 * Handle events directed at the drawing. We first attempt to handle them here.
 */
bool CanvasItemDrawing::handle_event(CanvasEvent const &event)
{
    bool retval = false;

    inspect_event(event,
        [&] (EnterEvent const &event) {
            if (!_cursor) {
                if (_active_item) {
                    // Fixme: This warning seems to fire a lot.
                    std::cerr << "CanvasItemDrawing::event_handler: cursor entered drawing with an active item!" << std::endl;
                }
                _cursor = true;

                /* TODO ... event -> arena transform? */
                _c = event.pos;

                set_active(_drawing->pick(_c, _cursor_tolerance, get_canvas()->get_area_world(), get_flags()));
                retval = _drawing_event_signal.emit(event, get_active());
            }
        },

        [&] (LeaveEvent const &event) {
            if (_cursor) {
                retval = _drawing_event_signal.emit(event, get_active());
                set_active(nullptr);
                _cursor = false;
            }
        },

        [&] (MotionEvent const &event) {
            /* TODO ... event -> arena transform? */
            _c = event.pos;

            auto new_drawing_item = _drawing->pick(_c, _cursor_tolerance, get_canvas()->get_area_world(), get_flags());
            if (!_active_item || _active_item->second != new_drawing_item) {

                /* fixme: What is wrong? */
                if (_active_item) {
                    auto event2 = LeaveEvent();
                    event2.modifiers = event.modifiers;
                    retval = _drawing_event_signal.emit(event2, _active_item->second);
                }

                set_active(new_drawing_item);

                if (_active_item) {
                    auto event2 = EnterEvent();
                    event2.modifiers = event.modifiers;
                    event2.pos = event.pos;
                    retval = _drawing_event_signal.emit(event2, _active_item->second);
                }
            }
            retval = retval || _drawing_event_signal.emit(event, get_active());
        },

        [&] (ScrollEvent const &event) {
            if (Modifiers::Modifier::get(Modifiers::Type::CANVAS_ZOOM)->active(event.modifiers)) {
                /* Zoom is emitted by the canvas as well, ignore here */
                retval = false;
                return;
            }
            retval = _drawing_event_signal.emit(event, get_active());
        },

        [&] (CanvasEvent const &event) {
            // Just send event.
            retval = _drawing_event_signal.emit(event, get_active());
        }
    );

    return retval;
}

unsigned CanvasItemDrawing::get_flags() const
{
    return _sticky * DrawingItem::PICK_STICKY | _pick_outline * DrawingItem::PICK_OUTLINE;
}

} // namespace Inkscape

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

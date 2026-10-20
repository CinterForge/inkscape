// SPDX-License-Identifier: GPL-2.0-or-later

#include "object-picker-tool.h"
#include "actions/actions-tools.h"
#include "object/sp-page.h"
#include "ui/widget/events/canvas-event.h"

namespace Inkscape::UI::Tools {

// label text size
static const auto fontsize = 12;

ObjectPickerTool::ObjectPickerTool(SPDesktop* desktop): ToolBase(desktop, "/tools/picker", "object-pick.svg", false) {
    _zoom = desktop->signal_zoom_changed.connect([this](double){
        // erase text; it doesn't scale well
        show_text(Geom::Point(), nullptr);
    });

    // create text label and its frame
    auto group = desktop->getCanvasTemp();
    _frame = make_canvasitem<CanvasItemRect>(group);
    _label = make_canvasitem<CanvasItemText>(group);

    _label->set_fontsize(fontsize);
    _label->set_fill(0x000000'ff); // black text
    _label->set_background(0xffffff'bf); // white with some transparency
    _label->set_border(4);
    _label->set_visible(false);

    _frame->set_shadow(0x00000020, 1);
    _frame->set_stroke(0); // transparent
    _frame->set_visible(false);
}

ObjectPickerTool::~ObjectPickerTool() {
    ungrabCanvasEvents();
    signal_tool_switched.emit();
}

SPObject* get_item_at(SPDesktop* desktop, const Geom::Point& point) {
    SPObject* item = desktop->getItemAtPoint(point, false);
    if (item) return item;

    if (auto document = desktop->getDocument()) {
        auto pt = desktop->w2d(point);
        item = document->getPageManager().findPageAt(pt);
    }

    return item;
}

bool ObjectPickerTool::root_handler(const CanvasEvent& event) {
    auto handled = false;
    auto switch_back = false;
    auto desktop = _desktop;

    inspect_event(event,

    [&] (MotionEvent const &event) {
        auto cursor = event.pos;
        auto item = get_item_at(desktop, cursor);
        show_text(cursor, item ? item->getId() : nullptr);
        auto msg = item ? Glib::ustring("Pick object <b>") + item->getId() + Glib::ustring("</b>") : "Pick objects.";
        _desktop->messageStack()->flash(INFORMATION_MESSAGE, msg);
    },

    [&] (ButtonPressEvent const &event) {
        if (event.button != 1) return;

        auto cursor = event.pos;
        auto item = get_item_at(desktop, cursor);
        show_text(cursor, item ? item->getId() : nullptr);
        if (item) {
            // object picked
            if (!signal_object_picked.emit(item)) {
                switch_back = true;
            }
        }
    },

    [&] (CanvasEvent const &event) {}
    );

    if (switch_back) {
        auto last = get_last_active_tool();
        if (!last.empty()) {
            set_active_tool(_desktop, last);
        }
        handled = true;
    }

    return handled || ToolBase::root_handler(event);
}

// Set label to show "text", move it above "cursor" on canvas
void ObjectPickerTool::show_text(const Geom::Point& cursor, const char* text) {
    _label->set_visible(false);
    _frame->set_visible(false);

    auto desktop = _desktop;
    if (!desktop || !text) return;

    auto position = desktop->w2d(Geom::Point(cursor.x(), cursor.y() - 2.5 * fontsize));

    _label->set_text(text);
    _label->set_coord(position);
    _label->set_visible(true);
    _label->update(false);

    // TODO: improve text positioning and/or drop shadow drawing
    // Review comments from PBS:
    /*
    The text rectangle returned by get_text_size() is not computed immediately, but only set on update(). That means the shadow is out of date and lags the text (only in certain situations).
    I'm not sure of a nice way to fix this outside of moving the text bounds computation out of CanvasItemText into some common code and using that here. Or you could duplicate the shadow functionality from CanvasItemRect into CanvasItemText, since it already duplicates the rectangle fill.
    It would also be nice to not have to add get_text_size() at all.
    */
    auto box = Geom::Rect::from_xywh(position, _label->get_text_size().dimensions() / desktop->current_zoom());
    _frame->set_rect(box);
    _frame->set_visible(true);
}

} // namespace

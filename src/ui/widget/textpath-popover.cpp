// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * The popover menu which opens on clicking the textpath handles.
 * This popover will facilitate on canvas editing of textpath attributes.
 */
/*
 * Authors:
 *   Vaibhav Malik <vaibhavmalik2018@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "textpath-popover.h"

#include <iomanip>
#include <glibmm/i18n.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/box.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/togglebutton.h>

#include "desktop.h"
#include "document-undo.h"
#include "object/sp-shape.h"
#include "spinbutton.h"
#include "util/numeric/precision.h"
#include "ui/builder-utils.h"
#include "ui/icon-names.h"

namespace Inkscape::UI::Widget {

TextpathPopover::TextpathPopover(SPText *text, SPTextPath *textpath, SPDesktop *desktop, double offset_val)
    : _desktop(desktop)
    , _text(text)
    , _textpath(textpath)
    , _builder(create_builder("textpath-popover-box.ui"))
    , _start_offset_sb(get_derived_widget<SpinButton>(_builder, "start-offset-sb"))
    , _side_left_btn(get_widget<Gtk::ToggleButton>(_builder, "side-left-btn"))
    , _side_right_btn(get_widget<Gtk::ToggleButton>(_builder, "side-right-btn"))
{
    g_assert(text != nullptr);
    g_assert(textpath != nullptr);

    // Populate the popup.
    set_child(get_widget<Gtk::Box>(_builder, "popover-box"));

    auto start_adj = _start_offset_sb.get_adjustment();
    start_adj->set_value(offset_val);
    start_adj->signal_value_changed().connect([this, text, textpath, start_adj] {
        if (!textpath) {
            return;
        }

        // Update the `startOffset` attribute of the textpath.
        gint const precision = Util::get_default_numeric_precision();
        std::stringstream offset_stream;
        offset_stream << std::fixed << std::setprecision(precision) << start_adj->get_value() << '%';
        auto const offset_str = offset_stream.str();
        textpath->setStartOffset(offset_str.c_str());
        text->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        text->updateRepr();
        DocumentUndo::maybeDone(_desktop->getDocument(), "textpath:startOffset", RC_("Undo", "Update textpath startOffset"), "");
    });

    // Setup the flip buttons.
    _side_left_btn.signal_clicked().connect(
        sigc::bind(sigc::mem_fun(*this, &TextpathPopover::side_btn_clicked), SP_TEXT_PATH_SIDE_LEFT));
    _side_right_btn.signal_clicked().connect(
        sigc::bind(sigc::mem_fun(*this, &TextpathPopover::side_btn_clicked), SP_TEXT_PATH_SIDE_RIGHT));

    _side_right_btn.set_active(textpath->side);
}

void TextpathPopover::side_btn_clicked(TextPathSide const side) const
{
    if (side != _textpath->side) {
        _textpath->setSide(side);
        auto const icon = _textpath->side ? "text-path-right" : "text-path-left";
        DocumentUndo::done(_desktop->getDocument(), RC_("Undo", "Change textpath side"), INKSCAPE_ICON(icon));
        auto old_val = _start_offset_sb.get_value();

        if (auto const path_item = dynamic_cast<SPShape *>(sp_textpath_get_path_item(_textpath))) {
            if (auto const path_vector = path_item->curve()) {
                auto const pwd2 = Geom::paths_to_pw(*path_vector);
                auto const total_len = Geom::length(pwd2);
                auto const text_len = _text->length();
                auto const text_len_offset = text_len * 100 / total_len;

                // To keep the text on the same position on the path
                // after side change.
                old_val += (_text->resolve_flip_offset_multiplier() * text_len_offset);
            }
        }

        // Changing the side attribute reverses the direction
        // from which the offset is calculated.
        _start_offset_sb.set_value(100 - old_val);
    }
}

} // namespace Inkscape::UI::Widget

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

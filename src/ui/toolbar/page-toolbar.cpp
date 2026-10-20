// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Page aux toolbar: Temp until we convert all toolbars to ui files with Gio::Actions.
 */
/* Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *   Vaibhav Malik <vaibhavmalik2018@gmail.com>

 * Copyright (C) 2021 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "page-toolbar.h"

#include <glibmm/i18n.h>
#include <glibmm/regex.h>
#include <gtkmm/box.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/label.h>
#include <gtkmm/liststore.h>
#include <gtkmm/separator.h>

#include "desktop.h"
#include "document-undo.h"
#include "extension/template.h"
#include "io/resource.h"
#include "object/sp-page.h"
#include "ui/builder-utils.h"
#include "ui/icon-names.h"
#include "ui/popup-menu.h"
#include "ui/widget/spinbutton.h"
#include "ui/widget/unit-tracker.h"
#include "util/units.h"

using Inkscape::IO::Resource::UIS;

namespace Inkscape::UI::Toolbar {

class SearchCols : public Gtk::TreeModel::ColumnRecord
{
public:
    // These types must match those for the model in the ui file
    SearchCols()
    {
        add(name);
        add(label);
        add(key);
    }
    Gtk::TreeModelColumn<Glib::ustring> name;  // translated name
    Gtk::TreeModelColumn<Glib::ustring> label; // translated label
    Gtk::TreeModelColumn<Glib::ustring> key;
};

PageToolbar::PageToolbar()
    : PageToolbar{create_builder("toolbar-page.ui")}
{}

PageToolbar::PageToolbar(Glib::RefPtr<Gtk::Builder> const &builder)
    : Toolbar{get_widget<Gtk::Box>(builder, "page-toolbar")}
    , _combo_page_sizes(get_widget<Gtk::ComboBoxText>(builder, "_combo_page_sizes"))
    , _text_page_margins(get_widget<Gtk::Entry>(builder, "_text_page_margins"))
    , _margin_popover(get_widget<Gtk::Popover>(builder, "margin_popover"))
    , _text_page_bleeds(get_widget<Gtk::Entry>(builder, "_text_page_bleeds"))
    , _text_page_label(get_widget<Gtk::Entry>(builder, "_text_page_label"))
    , _label_page_pos(get_widget<Gtk::Label>(builder, "_label_page_pos"))
    , _btn_page_duplicate(get_widget<Gtk::Button>(builder, "_btn_page_duplicate"))
    , _btn_page_backward(get_widget<Gtk::Button>(builder, "_btn_page_backward"))
    , _btn_page_foreward(get_widget<Gtk::Button>(builder, "_btn_page_foreward"))
    , _btn_page_delete(get_widget<Gtk::Button>(builder, "_btn_page_delete"))
    , _btn_move_toggle(get_widget<Gtk::Button>(builder, "_btn_move_toggle"))
    , _sep1(get_widget<Gtk::Separator>(builder, "_sep1"))
    , _sizes_list(get_object<Gtk::ListStore>(builder, "_sizes_list"))
    , _sizes_search(get_object<Gtk::ListStore>(builder, "_sizes_search"))
    , _margin_top(UI::get_derived_widget<UI::Widget::SpinButton>(builder, "_margin_top"))
    , _margin_right(UI::get_derived_widget<UI::Widget::SpinButton>(builder, "_margin_right"))
    , _margin_bottom(UI::get_derived_widget<UI::Widget::SpinButton>(builder, "_margin_bottom"))
    , _margin_left(UI::get_derived_widget<UI::Widget::SpinButton>(builder, "_margin_left"))
    , _unit_tracker{std::make_unique<UI::Widget::UnitTracker>(Util::UNIT_TYPE_LINEAR)}
{
    set_name("PageToolbar");


    get_object<Gtk::EntryCompletion>(builder, "_sizes_searcher")
        ->signal_match_selected()
        .connect(
            [this] (Gtk::TreeModel::iterator const &iter) {
                SearchCols cols;
                Gtk::TreeModel::Row row = *iter;
                Glib::ustring preset_key = row[cols.key];
                sizeChoose(preset_key);
                return false;
            },
            false);

    _text_page_label.signal_activate().connect(sigc::mem_fun(*this, &PageToolbar::labelEdited));
    _text_page_bleeds.signal_activate().connect(sigc::mem_fun(*this, &PageToolbar::bleedsEdited));
    _text_page_margins.signal_activate().connect(sigc::mem_fun(*this, &PageToolbar::marginsEdited));

    _margin_popover.set_name("MarginPopover");
    _margin_popover.set_parent(*this);

    _text_page_margins.signal_icon_press().connect([&](Gtk::Entry::IconPosition) {
        if (_blocker.pending()) {
            return;
        }
        auto guard = _blocker.block();

        // Set the unit of the unit-selector, so the units of the margin spin-edits
        // are updated to the selected display unit.
        _unit_tracker->setActiveUnit(_document->getDisplayUnit());

        if (auto page = _document->getPageManager().getSelected()) {
            auto const &margin = page->getMarginBox();
            auto unit = _document->getDisplayUnit()->abbr;
            auto scale = _document->getDocumentScale();

            _margin_top.set_value(margin.top().toValue(unit) * scale[Geom::Y]);
            _margin_right.set_value(margin.right().toValue(unit) * scale[Geom::X]);
            _margin_bottom.set_value(margin.bottom().toValue(unit) * scale[Geom::Y]);
            _margin_left.set_value(margin.left().toValue(unit) * scale[Geom::X]);

            _text_page_bleeds.set_text(page->getBleedLabel());
        }
        UI::popup_at(_margin_popover, _text_page_margins);
    });

    auto const pairs = std::to_array({std::pair<UI::Widget::SpinButton &, BoxSide>
         {_margin_top, BoxSide::BOX_TOP},
         {_margin_right, BoxSide::BOX_RIGHT},
         {_margin_bottom, BoxSide::BOX_BOTTOM},
         {_margin_left, BoxSide::BOX_LEFT}
    });
    for (auto &[btn, side] : pairs) {
        btn.addUnitTracker(_unit_tracker.get());
        btn.signal_value_changed().connect(std::bind(&PageToolbar::marginSideEdited, this, side, std::ref(btn)));
    }

    dynamic_cast<Gtk::Entry &>(*_combo_page_sizes.get_child()).set_completion(get_object<Gtk::EntryCompletion>(builder, "_sizes_searcher"));

    _combo_page_sizes.set_id_column(2);
    _combo_page_sizes.signal_changed().connect([this] {
        std::string preset_key = _combo_page_sizes.get_active_id();
        if (!preset_key.empty()) {
            sizeChoose(preset_key);
        }
    });

    _entry_page_sizes = dynamic_cast<Gtk::Entry *>(_combo_page_sizes.get_child());
    // TODO(Gtk::DropDown migration): set up the tooltip for the dropdown's button.
    if (_entry_page_sizes) {
        _entry_page_sizes->set_placeholder_text(_("ex.: 100x100cm"));
        _entry_page_sizes->set_tooltip_text(_("Type in width & height of a page. (ex.: 15x10cm, 10in x 100mm)\n"
                                              "or choose preset from dropdown."));
        _entry_page_sizes->add_css_class("symbolic");
        _entry_page_sizes->signal_activate().connect(sigc::mem_fun(*this, &PageToolbar::sizeChanged));

        _entry_page_sizes->signal_icon_press().connect([this] (Gtk::Entry::IconPosition) {
            _document->getPageManager().changeOrientation();
            DocumentUndo::maybeDone(_document, "page-resize", RC_("Undo", "Resize Page"), INKSCAPE_ICON("tool-pages"));
            setSizeText();
        });
        _entry_page_sizes->set_icon_tooltip_text(_("Change page orientation"), Gtk::Entry::IconPosition::SECONDARY);
        _entry_page_sizes->property_has_focus().signal_changed().connect([this] {
            if (!_document)
                return;
            auto const display_only = !has_focus();
            setSizeText(nullptr, display_only);
        });

        populate_sizes();
    }

    _initMenuBtns();
}

PageToolbar::~PageToolbar() = default;

void PageToolbar::setDesktop(SPDesktop *desktop)
{
    if (_desktop) {
        // Disconnect previous page changed signal
        _page_selected.disconnect();
        _pages_changed.disconnect();
        _page_modified.disconnect();
        _document = nullptr;
    }

    Toolbar::setDesktop(desktop);

    if (_desktop) {
        _document = _desktop->getDocument();
        assert(_document);

        _doc_connection = _desktop->connectDocumentReplaced([this] (SPDesktop *, SPDocument *doc) {
            setDesktop(_desktop);
        });

        // Save the document and page_manager for future use.
        auto &page_manager = _document->getPageManager();
        // Connect the page changed signal and indicate changed
        _pages_changed = page_manager.connectPagesChanged(sigc::mem_fun(*this, &PageToolbar::pagesChanged));
        _page_selected = page_manager.connectPageSelected(sigc::mem_fun(*this, &PageToolbar::selectionChanged));
        // Update everything now.
        pagesChanged(nullptr);
    }
}

/**
 * Take all selectable page sizes and add to search and dropdowns
 */
void PageToolbar::populate_sizes()
{
    SearchCols cols;

    Inkscape::Extension::DB::TemplateList extensions;
    Inkscape::Extension::db.get_template_list(extensions);

    for (auto tmod : extensions) {
        if (!tmod->can_resize())
            continue;
        for (auto preset : tmod->get_presets()) {
            auto label = preset->get_label();
            if (!label.empty()) label = _(label.c_str());

            if (preset->is_visible(Inkscape::Extension::TEMPLATE_SIZE_LIST)) {
                // Goes into drop down
                Gtk::TreeModel::Row row = *_sizes_list->append();
                row[cols.name] = _(preset->get_name().c_str());
                row[cols.label] = " <small><span fgalpha=\"50%\">" + label + "</span></small>";
                row[cols.key] = preset->get_key();
            }
            if (preset->is_visible(Inkscape::Extension::TEMPLATE_SIZE_SEARCH)) {
                // Goes into text search
                Gtk::TreeModel::Row row = *_sizes_search->append();
                row[cols.name] = _(preset->get_name().c_str());
                row[cols.label] = label;
                row[cols.key] = preset->get_key();
            }
        }
    }
}

void PageToolbar::labelEdited()
{
    auto text = _text_page_label.get_text();
    if (auto page = _document->getPageManager().getSelected()) {
        page->setLabel(text.empty() ? nullptr : text.c_str());
        DocumentUndo::maybeDone(_document, "page-relabel", RC_("Undo", "Relabel Page"), INKSCAPE_ICON("tool-pages"));
    }
}

void PageToolbar::bleedsEdited()
{
    auto text = _text_page_bleeds.get_text();

    // And modifiction to the bleed causes pages to be enabled
    auto &pm = _document->getPageManager();
    pm.enablePages();

    if (auto page = pm.getSelected()) {
        page->setBleed(text);
        DocumentUndo::maybeDone(_document, "page-bleed", RC_("Undo", "Edit page bleed"), INKSCAPE_ICON("tool-pages"));
        _text_page_bleeds.set_text(page->getBleedLabel());
    }
}

void PageToolbar::marginsEdited()
{
    auto text = _text_page_margins.get_text();

    // And modification to the margin causes pages to be enabled
    auto &pm = _document->getPageManager();
    pm.enablePages();

    if (auto page = pm.getSelected()) {
        page->setMargin(text);
        DocumentUndo::maybeDone(_document, "page-margin", RC_("Undo", "Edit page margin"), INKSCAPE_ICON("tool-pages"));
        setMarginText(page);
    }
}

void PageToolbar::marginSideEdited(BoxSide side, UI::Widget::SpinButton const &entry)
{
    if (_blocker.pending()) {
        return;
    }
    auto guard = _blocker.block();

    // And modification to the margin causes pages to be enabled
    auto &pm = _document->getPageManager();
    pm.enablePages();

    if (auto page = pm.getSelected()) {
        page->setMarginSide(side, entry.get_text(), false);
        DocumentUndo::maybeDone(_document, "page-margin", RC_("Undo", "Edit page margin"), INKSCAPE_ICON("tool-pages"));
        setMarginText(page);
    }
}

void PageToolbar::sizeChoose(const std::string &preset_key)
{
    if (auto preset = Extension::Template::get_any_preset(preset_key)) {
        auto &pm = _document->getPageManager();
        // The page orientation is a part of the toolbar widget, so we pass this
        // as a specially named pref, the extension can then decide to use it or not.
        auto p_rect = pm.getSelectedPageRect();
        std::string orient = p_rect.width() > p_rect.height() ? "land" : "port";

        auto page = pm.getSelected();
        preset->resize_to_template(_document, page, {
            {"orientation", orient},
        });
        if (page) {
            page->setSizeLabel(preset->get_name());
        }

        setSizeText();
        DocumentUndo::maybeDone(_document, "page-resize", RC_("Undo", "Resize Page"), INKSCAPE_ICON("tool-pages"));
    } else {
        // Page not found, i.e., "Custom" was selected or user is typing in.
        _entry_page_sizes->grab_focus();
    }
}

/**
 * Convert the parsed sections of a text input into a desktop pixel value.
 */
double PageToolbar::_unit_to_size(std::string number, std::string unit_str,
                                  std::string const &backup)
{
    // We always support comma, even if not in that particular locale.
    std::replace(number.begin(), number.end(), ',', '.');
    double value = std::stod(number);

    // Get the best unit, for example 50x40cm means cm for both
    if (unit_str.empty() && !backup.empty())
        unit_str = backup;
    if (unit_str == "\"")
        unit_str = "in";
    if (unit_str == "'")
        unit_str = "ft";

    // Output is always in px as it's the most useful.
    auto px = Inkscape::Util::UnitTable::get().getUnit("px");

    // Convert from user entered unit to display unit
    if (!unit_str.empty())
        return Inkscape::Util::Quantity::convert(value, unit_str, px);

    // Default unit is the document's display unit
    auto unit = _document->getDisplayUnit();
    return Inkscape::Util::Quantity::convert(value, unit, px);
}

/**
 * A manually typed input size, parse out what we can understand from
 * the text or ignore it if the text can't be parsed.
 *
 * Format: 50cm x 40mm
 *         20',40"
 *         30,4-40.2
 */
void PageToolbar::sizeChanged()
{
    // Parse the size out of the typed text if possible.
    auto cb_text = _combo_page_sizes.get_active_text();

    // Remove parens from auto generated names
    auto pos1 = cb_text.find_first_of("(");
    auto pos2 = cb_text.find_first_of(")");
    if (pos1 != cb_text.npos && pos2 != cb_text.npos && pos1 < pos2) {
        cb_text = cb_text.substr(pos1+1, pos2-pos1-1);
    }

    // This does not support negative values, because pages can not be negatively sized.
    static auto const arg = "([0-9]+[\\.,]?[0-9]*|\\.[0-9]+) ?(px|mm|cm|m|in|\\\"|ft|')?";
    static auto const regex = Glib::Regex::create(Glib::ustring{"^ *"} + arg + " *([ *Xx×,\\-]) *" + arg + " *$", Glib::Regex::CompileFlags::OPTIMIZE);

    Glib::MatchInfo match;
    if (regex->match(cb_text, match)) {
        // Convert the desktop px back into document units for 'resizePage'
        auto const width_unit = match.fetch(2);
        auto const height_unit = match.fetch(5);
        double width = _unit_to_size(match.fetch(1), width_unit, height_unit);
        double height = _unit_to_size(match.fetch(4), height_unit, width_unit);
        if (width > 0 && height > 0) {
            _document->getPageManager().resizePage(width, height);
            DocumentUndo::done(_document, RC_("Undo", "Set page size"), INKSCAPE_ICON("tool-pages"));
        }
    }
    setSizeText();
}

/**
 * Sets the label of the page to the text box
 */
void PageToolbar::setLabelText(SPPage *page)
{
    Glib::ustring label = page && page->label() ? page->label() : "";
    if (_text_page_label.get_text() != label) {
        _text_page_label.set_text(label);
    }
}

/**
 * Sets the size of the current page into the entry page size.
 */
void PageToolbar::setSizeText(SPPage *page, bool display_only)
{
    SearchCols cols;

    if (!page)
        page = _document->getPageManager().getSelected();

    auto label = _document->getPageManager().getSizeLabel(page);

    // If this is a known size in our list, add the size paren to it.
    for (auto &row : _sizes_search->children()) {
        if (label == row[cols.name].operator Glib::ustring().raw()) {
            label = label + " (" + row[cols.label] + ")";
            break;
        }
    }
    _entry_page_sizes->set_text(label);

    // Orientation button
    auto box = page ? page->getDesktopRect() : *_document->preferredBounds();
    auto const icon = box.width() > box.height() ? "page-landscape" : "page-portrait";
    if (box.width() == box.height()) {
        _entry_page_sizes->unset_icon(Gtk::Entry::IconPosition::SECONDARY);
    } else {
        _entry_page_sizes->set_icon_from_icon_name(INKSCAPE_ICON(icon), Gtk::Entry::IconPosition::SECONDARY);
    }

    if (!display_only) {
        // The user has started editing the combo box; we set up a convenient initial state.
        // Select text if box is currently in focus.
        if (_entry_page_sizes->has_focus()) {
            _entry_page_sizes->select_region(0, -1);
        }
    }
}

void PageToolbar::setMarginText(SPPage *page)
{
    _text_page_margins.set_text(page ? page->getMarginLabel() : "");
    _text_page_margins.set_sensitive(true);
}

void PageToolbar::pagesChanged(SPPage *new_page)
{
    selectionChanged(_document->getPageManager().getSelected());
}

void PageToolbar::selectionChanged(SPPage *page)
{
    _page_modified.disconnect();
    auto &page_manager = _document->getPageManager();
    _text_page_label.set_tooltip_text(_("Page label"));

    // Set label widget content with page label.
    if (page) {
        _text_page_label.set_sensitive(true);
        _text_page_label.set_placeholder_text(page->getDefaultLabel());

        // TRANSLATORS: "%1" is replaced with the page we are on, and "%2" is the total number of pages.
        auto label = Glib::ustring::compose(_("%1/%2"), page->getPagePosition(), page_manager.getPageCount());
        _label_page_pos.set_label(label);

        _page_modified = page->connectModified([this] (SPObject *obj, unsigned flags) {
            if (auto page = cast<SPPage>(obj)) {
                // Make sure we don't 'select' on removal of the page
                if (flags & SP_OBJECT_MODIFIED_FLAG) {
                    selectionModified(page);
                }
            }
        });
    } else {
        _text_page_label.set_text("");
        _text_page_label.set_sensitive(false);
        _text_page_label.set_placeholder_text(_("Single Page Document"));
        _label_page_pos.set_label(_("1/-"));

        _page_modified = _document->connectModified([this] (unsigned) {
            selectionModified(nullptr);
        });
    }
    if (!page_manager.hasPrevPage() && !page_manager.hasNextPage()) {
        _sep1.set_visible(false);
        _label_page_pos.set_visible(false);
        _btn_page_backward.set_visible(false);
        _btn_page_foreward.set_visible(false);
        _btn_page_delete.set_visible(false);
        _btn_move_toggle.set_sensitive(false);
    } else {
        // Set the forward and backward button sensitivities
        _sep1.set_visible(true);
        _label_page_pos.set_visible(true);
        _btn_page_backward.set_visible(true);
        _btn_page_foreward.set_visible(true);
        _btn_page_backward.set_sensitive(page_manager.hasPrevPage());
        _btn_page_foreward.set_sensitive(page_manager.hasNextPage());
        _btn_page_delete.set_visible(true);
        _btn_move_toggle.set_sensitive(true);
    }
    selectionModified(page);
}

/**
 * Update all the elements that might have changed within a page
 */
void PageToolbar::selectionModified(SPPage *page)
{
    setLabelText(page);
    setMarginText(page);
    setSizeText(page);
}

} // namespace Inkscape::UI::Toolbar

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

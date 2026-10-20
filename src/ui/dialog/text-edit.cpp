// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Text editing dialog.
 */
/* Authors:
 *   Lauris Kaplinski <lauris@ximian.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Johan Engelen <goejendaagh@zonnet.nl>
 *   Abhishek Sharma
 *   John Smith
 *   Tavmjong Bah
 *
 * Copyright (C) 1999-2013 Authors
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtkmm/eventcontrollerkey.h>
#include "preferences.h"
#ifdef HAVE_CONFIG_H
# include "config.h"  // only include where actually required!
#endif

#include "text-edit.h"

#include <glibmm/markup.h>
#include <gtkmm/menubutton.h>
#include <gtkmm/notebook.h>
#include <gtkmm/popover.h>
#include <gtkmm/searchentry2.h>
#include <gtkmm/separator.h>
#include <gtkmm/textview.h>

#include "ui/tools/text-tool.h"
#ifdef WITH_LIBSPELLING
#include "ui/libspelling-wrapper.h"
#endif

#include "desktop-style.h"
#include "dialog-container.h"
#include "document-undo.h"
#include "inkscape.h"
#include "libnrtype/font-lister.h"
#include "object/sp-flowtext.h"
#include "object/sp-text.h"
#include "selection.h"
#include "svg/css-ostringstream.h"
#include "text-editing.h"
#include "ui/builder-utils.h"
#include "ui/icon-names.h"
#include "ui/pack.h"
#include "ui/util.h"
#include "ui/widget/font-selector.h"    // for FontSelector
#include "util/font-collections.h"
#include "util/recently-used-fonts.h"
#include "util/units.h"

namespace Inkscape::UI::Dialog {
namespace {

Glib::ustring const &getSamplePhrase()
{
    /* TRANSLATORS: Test string used in text and font dialog (when no
     * text has been entered) to get a preview of the font. Choose
     * some representative characters that users of your locale will be
     * interested in. */
    static auto const samplephrase = Glib::ustring{_("AaBbCcIiPpQq12369$\342\202\254\302\242?.;/()")};
    return samplephrase;
}

} // namespace

TextEdit::TextEdit(bool use_browser)
    // This class is two dialogs in one, based off the "use_browser" flag
    : DialogBase(
        use_browser ? "/dialogs/fontbrowser" : "/dialogs/textandfont",
        use_browser ? "FontBrowser" : "Text"
      )

    , builder(create_builder("dialog-text-edit.glade"))
      // Font
    , settings_and_filters_box (get_widget<Gtk::Box>        (builder, "settings_and_filters_box"))
    , filter_menu_button       (get_widget<Gtk::MenuButton> (builder, "filter_menu_button"))
    , reset_button             (get_widget<Gtk::Button>     (builder, "reset_button"))
    , search_entry             (get_widget<Gtk::SearchEntry2>(builder, "search_entry"))
    , font_count_label         (get_widget<Gtk::Label>      (builder, "font_count_label"))
    , filter_popover           (get_widget<Gtk::Popover>    (builder, "filter_popover"))
    , popover_box              (get_widget<Gtk::Box>        (builder, "popover_box"))
    , frame                    (get_widget<Gtk::Frame>      (builder, "frame"))
    , frame_label              (get_widget<Gtk::Label>      (builder, "frame_label"))
    , collection_editor_button (get_widget<Gtk::Button>     (builder, "collection_editor_button"))
    , collections_list         (get_widget<Gtk::ListBox>    (builder, "collections_list"))
    , preview_label            (get_widget<Gtk::Label>      (builder, "preview_label"))
      // Features
    , preview_label2           (get_widget<Gtk::Label>      (builder, "preview_label2"))
      // Shared
    , setasdefault_button      (get_widget<Gtk::Button>     (builder, "setasdefault_button"))
    , apply_button             (get_widget<Gtk::Button>     (builder, "apply_button"))
    , _apply_box               (get_widget<Gtk::Box>        (builder, "apply-box"))
    , _use_browser{use_browser}
    , _undo{"doc.undo"}
    , _redo{"doc.redo"}
{
    font_list = _use_browser ?
        Inkscape::UI::Widget::FontList::create_font_list("/font-selector") :
        Inkscape::UI::Widget::FontSelector::create_font_selector();

    auto font_collections = Inkscape::FontCollections::get();

    auto contents = &get_widget<Gtk::Box>     (builder, "contents");
    auto notebook = &get_widget<Gtk::Notebook>(builder, "notebook");
    auto font_box = &get_widget<Gtk::Box>     (builder, "font_box");
    auto feat_box = &get_widget<Gtk::Box>     (builder, "feat_box");

#ifdef WITH_LIBSPELLING
    text_view = Gtk::manage(Glib::wrap(GTK_TEXT_VIEW(gtk_source_view_new())));
#else
    text_view = Gtk::make_managed<Gtk::TextView>();
#endif
    text_buffer = text_view->get_buffer();
    text_view->property_height_request().set_value(64);
    text_view->set_focusable();
    text_view->set_wrap_mode(Gtk::WrapMode::WORD);
    auto &text_view_container = get_widget<Gtk::ScrolledWindow>(builder, "text_view_container");
    text_view_container.set_child(*text_view);

    if (_use_browser) {
        // hide settings and filter box
        settings_and_filters_box.set_visible(false);
        font_count_label.set_visible(false);
        preview_label.set_visible(false);
    }

    font_box->insert_child_after(*font_list->box(), font_count_label);
    UI::pack_start(*feat_box, font_features, true, true);
    feat_box->reorder_child_after(font_features, *feat_box->get_first_child());

    filter_popover.signal_show().connect([this] {
        // update font collections checkboxes
        display_font_collections();
    }, false);

#ifdef WITH_LIBSPELLING
    // TODO: Use computed xml:lang attribute of relevant element, if present, to specify the language.
    // onReadSelection() looks like a suitable place.
    auto adapter = spelling_text_buffer_adapter_create(GTK_SOURCE_BUFFER(text_view->get_buffer()->gobj()), spelling_checker_get_default());
    text_view->set_extra_menu(get_menu_model(*adapter));
    text_view->insert_action_group("spelling", as_action_group(*adapter));
    set_enabled(*adapter, true);
#endif

    append(*contents);

    /* Signal handlers */
    auto const key = Gtk::EventControllerKey::create();
    key->signal_key_pressed().connect([this, &key = *key](auto&& ...args) { return captureUndo(key, args...); }, true);
    text_view->add_controller(key);

    text_buffer->signal_changed().connect([this] { onChange(); });

    setasdefault_button.signal_clicked().connect([this] { onSetDefault(); });
    apply_button.signal_clicked().connect([this] { onApply(); });
    fontFeaturesChangedConn = font_features.connectChanged([this] { onChange(); });
    search_entry.signal_search_changed().connect([this] { on_search_entry_changed(); });
    reset_button.signal_clicked().connect([this] { on_reset_button_pressed(); });
    collection_editor_button.signal_clicked().connect([this] { on_fcm_button_clicked(); });
    Inkscape::FontLister::get_instance()->connectUpdate(sigc::mem_fun(*this, &TextEdit::change_font_count_label));
    fontCollectionsUpdate = font_collections->connect_update([this] { display_font_collections(); });
    fontCollectionsChangedSelection = font_collections->connect_selection_update([this] {
        auto font_collections = Inkscape::FontCollections::get();
        display_font_collections();
        int selected_count = font_collections->get_selected_collections_count();
        reset_button.set_sensitive(selected_count != 0);
    });

    change_font_count_label();

    notebook->signal_switch_page().connect(sigc::mem_fun(*this, &TextEdit::on_page_changed));
    _fontspec_changed = font_list->signal_fontspec_changed().connect([this](){ onChange(); apply_changes(SPEC, true); });
    _fontsize_changed = font_list->signal_fontsize_changed().connect([this](){ onChange(); apply_changes(SIZE, true); });
    _apply_font = font_list->signal_set_default().connect([this](){ onChange(); onSetDefault(); });
    _insert_text = font_list->signal_insert_text().connect([this](const auto& text) {
        if (auto desktop = getDesktop()) {
            if (auto text_tool = dynamic_cast<Tools::TextTool*>(desktop->getTool())) {
                text_tool->insertText(text);
            }
        }
    });

    onChange();
    on_page_changed(nullptr, 0);
}

TextEdit::~TextEdit() = default;

bool TextEdit::captureUndo(Gtk::EventControllerKey const &controller,
                           unsigned keyval, unsigned keycode, Gdk::ModifierType state)
{
    for (auto const accel: {&_undo, &_redo}) {
        if (accel->isTriggeredBy(controller, keyval, keycode, state)) {
            /*
             * TODO: Handle these events separately after switching to GTKMM4
             *       e.g. try to use the built-in undo/redo of GtkEditable, etc.
             * Fixes: https://gitlab.com/inkscape/inkscape/-/issues/744
             */
            return true;
        }
    }

    return false;
}

void TextEdit::onReadSelection ( bool dostyle, bool /*docontent*/ )
{
    if (blocked)
        return;

    blocked = true;

    SPItem *text = getSelectedTextItem ();

    auto phrase = getSamplePhrase();

    if (text)
    {
        guint items = getSelectedTextCount ();
        bool has_one_item = items == 1;
        text_view->set_sensitive(has_one_item);
        apply_button.set_sensitive(false);
        setasdefault_button.set_sensitive(true);

        Glib::ustring str = sp_te_get_string_multiline(text);
        if (!str.empty()) {
            if (has_one_item) {
                text_buffer->set_text(str);
                text_buffer->set_modified(false);
            }
            phrase = str;

        } else {
            text_buffer->set_text("");
        }

        text->getRepr(); // was being called but result ignored. Check this.
    } else {
        text_view->set_sensitive(false);
        apply_button.set_sensitive(false);
        setasdefault_button.set_sensitive(false);
    }

    if (dostyle && text) {
        auto *desktop = getDesktop();

        // create temporary style
        SPStyle query(desktop->getDocument());

        // Query style from desktop into it. This returns a result flag and fills query with the
        // style of subselection, if any, or selection

        int result_numbers = sp_desktop_query_style (desktop, &query, QUERY_STYLE_PROPERTY_FONTNUMBERS);

        // If querying returned nothing, read the style from the text tool prefs (default style for new texts).
        if (result_numbers == QUERY_STYLE_NOTHING) {
            query.readFromPrefs("/tools/text");
        }

        Inkscape::FontLister* font_lister = Inkscape::FontLister::get_instance();

        // Update family/style based on selection.
        font_lister->selection_update();
        Glib::ustring fontspec = font_lister->get_fontspec();
        // Update Font Face.
        // font_selector.update_font ();
        font_list->set_current_font(font_lister->get_font_family(), font_lister->get_font_style());

        // Update Size.
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        int unit = prefs->getInt("/options/font/unitType", SP_CSS_UNIT_PT);
        double size = sp_style_css_size_px_to_units(query.font_size.computed, unit);
        selected_fontsize = size;
        font_list->set_current_size(size);
        // Update font features (variant) widget
        //int result_features =
        sp_desktop_query_style (desktop, &query, QUERY_STYLE_PROPERTY_FONTVARIANTS);
        int result_features =
            sp_desktop_query_style (desktop, &query, QUERY_STYLE_PROPERTY_FONTFEATURESETTINGS);
        font_features.update( &query, result_features == QUERY_STYLE_MULTIPLE_DIFFERENT, fontspec );
        Glib::ustring features = font_features.get_markup();

        // Update Preview
        setPreviewText (fontspec, features, phrase);
    }

    blocked = false;
}


void TextEdit::setPreviewText (Glib::ustring const &font_spec, Glib::ustring const &font_features,
                               Glib::ustring const &phrase)
{
    if (font_spec.empty()) {
        preview_label.set_markup("");
        preview_label2.set_markup("");
        return;
    }

    // Limit number of lines in preview to arbitrary amount to prevent Text and Font dialog
    // from growing taller than a desktop
    const int max_lines = 4;
    // Ignore starting empty lines; they would show up as nothing
    auto start_pos = phrase.find_first_not_of(" \n\r\t");
    if (start_pos == Glib::ustring::npos) {
        start_pos = 0;
    }
    // Now take up to max_lines
    auto end_pos = Glib::ustring::npos;
    auto from = start_pos;
    for (int i = 0; i < max_lines; ++i) {
        end_pos = phrase.find("\n", from);
        if (end_pos == Glib::ustring::npos) { break; }
        from = end_pos + 1;
    }
    Glib::ustring phrase_trimmed = phrase.substr(start_pos, end_pos != Glib::ustring::npos ? end_pos - start_pos : end_pos);

    Glib::ustring font_spec_escaped = Glib::Markup::escape_text( font_spec );
    Glib::ustring phrase_escaped = Glib::Markup::escape_text(phrase_trimmed);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int unit = prefs->getInt("/options/font/unitType", SP_CSS_UNIT_PT);
    double pt_size =
        Inkscape::Util::Quantity::convert(
            sp_style_css_size_units_to_px(font_list->get_fontsize(), unit), "px", "pt");
    pt_size = std::min(pt_size, 100.0);
    // Pango font size is in 1024ths of a point
    auto const size = std::to_string(static_cast<int>(pt_size * PANGO_SCALE));

    auto font_features_attr = Glib::ustring{};
    if (!font_features.empty()) {
        font_features_attr = Glib::ustring::compose("font_features='%1'", font_features);
    }

    auto const markup = Glib::ustring::compose("<span font='%1' size='%2' %3>%4</span>",
                                               font_spec_escaped, size, font_features_attr,
                                               phrase_escaped);
    preview_label.set_markup (markup);
    preview_label2.set_markup (markup);
}

SPItem *TextEdit::getSelectedTextItem ()
{
    if (!getDesktop())
        return nullptr;

    auto tmp= getDesktop()->getSelection()->items();
	for(auto i : tmp)
    {
        if (is<SPText>(i) || is<SPFlowtext>(i))
            return i;
    }

    return nullptr;
}


unsigned TextEdit::getSelectedTextCount ()
{
    if (!getDesktop())
        return 0;

    unsigned int items = 0;

    auto tmp= getDesktop()->getSelection()->items();
	for(auto i : tmp)
    {
        if (is<SPText>(i) || is<SPFlowtext>(i))
            ++items;
    }

    return items;
}

void TextEdit::documentReplaced()
{
    onReadSelection(true, true);
}

void TextEdit::selectionChanged(Selection *selection)
{
    onReadSelection(true, true);
}

void TextEdit::selectionModified(Selection *selection, guint flags)
{
    bool style = ((flags & (SP_OBJECT_CHILD_MODIFIED_FLAG |
                            SP_OBJECT_STYLE_MODIFIED_FLAG  )) != 0 );
    bool content = ((flags & (SP_OBJECT_CHILD_MODIFIED_FLAG |
                              SP_TEXT_CONTENT_MODIFIED_FLAG  )) != 0 );
    onReadSelection (style, content);
}


void TextEdit::updateObjectText ( SPItem *text )
{
    Gtk::TextIter start, end;

    // write text
    if (text_buffer->get_modified()) {
        text_buffer->get_bounds(start, end);
        Glib::ustring str = text_buffer->get_text(start, end);
        sp_te_set_repr_text_multiline (text, str.c_str());
        text_buffer->set_modified(false);
    }
}

SPCSSAttr *TextEdit::fillTextStyle(guint flags)
{
    SPCSSAttr *css = sp_repr_css_attr_new ();

    if (flags & SPEC) {
        auto fontspec = font_list->get_fontspec();
        if (!fontspec.empty()) {
            auto fontlister = Inkscape::FontLister::get_instance();
            fontlister->fill_css(css, fontspec);
        }
    }

    if (flags & SIZE) {
        // TODO, possibly move this to FontLister::set_css to be shared.
        Inkscape::CSSOStringStream os;
        auto prefs = Inkscape::Preferences::get();
        int unit = prefs->getInt("/options/font/unitType", SP_CSS_UNIT_PT);
        if (prefs->getBool("/options/font/textOutputPx", true)) {
            os << sp_style_css_size_units_to_px(font_list->get_fontsize(), unit)
                << sp_style_get_css_unit_string(SP_CSS_UNIT_PX);
        } else {
            os << font_list->get_fontsize() << sp_style_get_css_unit_string(unit);
        }
        sp_repr_css_set_property(css, "font-size", os.str().c_str());
    }

    if (flags & FEATURES) {
        font_features.fill_css(css);
    }

    return css;
}

void TextEdit::onSetDefault()
{
    SPCSSAttr *css = fillTextStyle(SPEC | SIZE | FEATURES);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    blocked = true;
    prefs->mergeStyle("/tools/text/style", css);
    blocked = false;

    sp_repr_css_attr_unref (css);

    setasdefault_button.set_sensitive ( false );
}

void TextEdit::onApply()
{
    apply_changes(SPEC | SIZE | FEATURES, false);
}

void TextEdit::apply_changes(guint flags, bool continuous) {
    blocked = true;

    SPDesktop *desktop = getDesktop();

    unsigned items = 0;
    auto item_list = desktop->getSelection()->items();
    SPCSSAttr *css = fillTextStyle(flags);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    for(auto i : item_list){
        // apply style to the reprs of all text objects in the selection
        if (is<SPText>(i) || (is<SPFlowtext>(i)) ) {
            ++items;
        }
    }
    if (items == 1) {
        double factor = font_list->get_fontsize() / selected_fontsize;
        prefs->setDouble("/options/font/scaleLineHeightFromFontSIze", factor);
    }
    sp_desktop_set_style(desktop, css, true);

    if (items == 0) {
        // no text objects; apply style to prefs for new objects
        prefs->mergeStyle("/tools/text/style", css);
        setasdefault_button.set_sensitive ( false );

    } else if (items == 1) {
        // exactly one text object; now set its text, too
        SPItem *item = desktop->getSelection()->singleItem();
        if (is<SPText>(item) || is<SPFlowtext>(item)) {
            updateObjectText (item);
            SPStyle *item_style = item->style;
            if (is<SPText>(item) && item_style->inline_size.value == 0) {
                css = sp_css_attr_from_style(item_style, SP_STYLE_FLAG_IFSET);
                sp_repr_css_unset_property(css, "inline-size");
                item->changeCSS(css, "style");
            }
        }
    }

    // Update FontLister
    Glib::ustring fontspec = font_list->get_fontspec();
    Inkscape::FontLister *fontlister = Inkscape::FontLister::get_instance();
    if( !fontspec.empty() ) {
        fontlister->set_fontspec( fontspec, false );
    }

    auto recent_fonts = Inkscape::RecentlyUsedFonts::get();

    if (continuous && recent_fonts->get_continuous_streak()) {
        recent_fonts->pop_front();
    }

    recent_fonts->prepend_to_list(fontlister->get_font_family());
    recent_fonts->set_continuous_streak(continuous);

    // complete the transaction
    if (continuous) {
        DocumentUndo::maybeDone(desktop->getDocument(), "text-style", RC_("Undo", "Set text style"), INKSCAPE_ICON("draw-text"));
    }
    else {
        DocumentUndo::done(desktop->getDocument(), RC_("Undo", "Set text style"), INKSCAPE_ICON("draw-text"));
        apply_button.set_sensitive(false);
    }

    sp_repr_css_attr_unref (css);
    Inkscape::FontLister::get_instance()->update_font_list(desktop->getDocument());

    blocked = false;
}

void TextEdit::display_font_collections()
{
    UI::remove_all_children(collections_list);

    FontCollections *font_collections = Inkscape::FontCollections::get();

    // Insert system collections.
    for(auto const& col: font_collections->get_collections(true)) {
        auto const btn = Gtk::make_managed<Gtk::CheckButton>(col);
        btn->set_margin_bottom(2);
        btn->set_active(font_collections->is_collection_selected(col));
        btn->signal_toggled().connect([=](){
            // toggle font system collection
            font_collections->update_selected_collections(col);
        });
        auto const row = Gtk::make_managed<Gtk::ListBoxRow>();
        row->set_focusable(false);
        row->set_child(*btn);
        collections_list.append(*row);
    }

    // Insert row separator.
    auto const sep = Gtk::make_managed<Gtk::Separator>();
    sep->set_margin_bottom(2);
    auto const sep_row = Gtk::make_managed<Gtk::ListBoxRow>();
    sep_row->set_focusable(false);
    sep_row->set_child(*sep);
    collections_list.append(*sep_row);

    // Insert user collections.
    for (auto const& col: font_collections->get_collections()) {
        auto const btn = Gtk::make_managed<Gtk::CheckButton>(col);
        btn->set_margin_bottom(2);
        btn->set_active(font_collections->is_collection_selected(col));
        btn->signal_toggled().connect([=](){
            // toggle font collection
            font_collections->update_selected_collections(col);
        });
        auto const row = Gtk::make_managed<Gtk::ListBoxRow>();
        row->set_focusable(false);
        row->set_child(*btn);
        collections_list.append(*row);
    }
}

void TextEdit::on_page_changed(Gtk::Widget*, int pos)
{
    _apply_box.set_visible(pos != 0 || !_use_browser); // font browser doesn't use "Apply" button

    if (pos == 1) {
        Glib::ustring fontspec = font_list->get_fontspec();
        if (!fontspec.empty()) {
            auto res = FontFactory::get().FaceFromFontSpecification(fontspec.c_str());
            if (res) {
                font_features.update_opentype(fontspec);
            }
        }
    }
}

void TextEdit::on_search_entry_changed()
{
    auto search_txt = search_entry.get_text();
    font_list->unset_model();
    Inkscape::FontLister *font_lister = Inkscape::FontLister::get_instance();
    font_lister->show_results(search_txt);

    SPDocument *document = getDesktop()->getDocument();
    font_lister->add_document_fonts_at_top(document);
    font_list->set_model();
}

void TextEdit::on_reset_button_pressed()
{
    FontCollections *font_collections = Inkscape::FontCollections::get();
    search_entry.set_text("");

    // Un-select all the selected font collections.
    font_collections->clear_selected_collections();

    Inkscape::FontLister *font_lister = Inkscape::FontLister::get_instance();
    font_lister->init_font_families();
    font_lister->init_default_styles();
    SPDocument *document = getDesktop()->getDocument();
    font_lister->add_document_fonts_at_top(document);
}

void TextEdit::change_font_count_label()
{
    auto [_, label] = Inkscape::FontLister::get_instance()->get_font_count_label();
    font_count_label.set_label(label);
}

void TextEdit::on_fcm_button_clicked()
{
    // Inkscape::UI::Dialog::FontCollectionsManager::getInstance();
    if(auto desktop = SP_ACTIVE_DESKTOP) {
        if (auto container = desktop->getContainer()) {
            container->new_floating_dialog("FontCollections");
        }
    }
}

void TextEdit::onChange()
{
    if (blocked) {
        return;
    }

    Gtk::TextIter start, end;
    text_buffer->get_bounds(start, end);
    Glib::ustring str = text_buffer->get_text(start, end);

    Glib::ustring fontspec = font_list->get_fontspec();
    Glib::ustring features = font_features.get_markup();
    auto const &phrase = str.empty() ? getSamplePhrase() : str;
    setPreviewText(fontspec, features, phrase);

    SPItem *text = getSelectedTextItem();
    if (text) {
        apply_button.set_sensitive ( true );
    }

    setasdefault_button.set_sensitive ( true);
}

} // namespace Inkscape::UI::Dialog

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

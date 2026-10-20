// SPDX-License-Identifier: GPL-2.0-or-later
#include "rvng-import-dialog.h"

#include <glibmm/i18n.h>

#include "inkscape.h"
#include "extension/input.h"
#include "object/sp-root.h"
#include "ui/controller.h"
#include "ui/dialog-events.h"
#include "ui/dialog-run.h"
#include "ui/pack.h"
#include "util/units.h"

using namespace librevenge;

namespace Inkscape::Extension::Internal {

RvngImportDialog::RvngImportDialog(std::vector<RVNGString> const &pages)
    : _pages{pages}
{
    int num_pages = _pages.size();

    // Dialog settings
    set_title(_("Page Selector"));
    set_modal();
    sp_transientize(*this);
    set_resizable();
    property_destroy_with_parent().set_value(false);

    // Preview area
    vbox1 = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 4);
    vbox1->set_margin(4);
    UI::pack_start(*get_content_area(), *vbox1);

    // CONTROLS
    _page_selector_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 4);

    // "Select page:" label
    _labelSelect = Gtk::make_managed<Gtk::Label>(_("Select page:"));
    _labelTotalPages = Gtk::make_managed<Gtk::Label>();
    _labelSelect->set_wrap(false);
    _labelSelect->set_use_markup(false);
    _labelSelect->set_selectable(false);
    UI::pack_start(*_page_selector_box, *_labelSelect, UI::PackOptions::shrink);

    // Adjustment + spinner
    auto pageNumberSpin_adj = Gtk::Adjustment::create(1, 1, _pages.size(), 1, 10, 0);
    _pageNumberSpin = Gtk::make_managed<Gtk::SpinButton>(pageNumberSpin_adj, 1, 0);
    _pageNumberSpin->set_focusable();
    _pageNumberSpin->set_numeric(true);
    _pageNumberSpin->set_wrap(false);
    UI::pack_start(*_page_selector_box, *_pageNumberSpin, UI::PackOptions::shrink);

    _labelTotalPages->set_wrap(false);
    _labelTotalPages->set_use_markup(false);
    _labelTotalPages->set_selectable(false);
    _labelTotalPages->set_label(Glib::ustring::compose(_("out of %1"), num_pages));
    UI::pack_start(*_page_selector_box, *_labelTotalPages, UI::PackOptions::shrink);

    UI::pack_start(*vbox1, _preview, UI::PackOptions::expand_widget, 0);
    _preview.setResize(400, 400);

    UI::pack_end(*vbox1, *_page_selector_box, UI::PackOptions::shrink);

    // Buttons
    cancelbutton = Gtk::make_managed<Gtk::Button>(_("_Cancel"), true);
    okbutton    = Gtk::make_managed<Gtk::Button>(_("_OK"),    true);
    add_action_widget(*cancelbutton, Gtk::ResponseType::CANCEL);
    add_action_widget(*okbutton, Gtk::ResponseType::OK);

    // Connect signals
    _pageNumberSpin->signal_value_changed().connect(sigc::mem_fun(*this, &RvngImportDialog::_onPageNumberChanged));

    auto const click = Gtk::GestureClick::create();
    click->set_button(0); // any
    click->set_propagation_phase(Gtk::PropagationPhase::TARGET);
    click->signal_pressed().connect(sigc::mem_fun(*this, &RvngImportDialog::_onSpinButtonClickPressed));
    click->signal_released().connect(sigc::mem_fun(*this, &RvngImportDialog::_onSpinButtonClickReleased));
    _pageNumberSpin->add_controller(click);

    _setPreviewPage();
}

bool RvngImportDialog::showDialog()
{
    auto ret = UI::dialog_run(*this);
    return ret == Gtk::ResponseType::OK || ret == Gtk::ResponseType::ACCEPT;
}

void RvngImportDialog::_onPageNumberChanged()
{
    auto page = _pageNumberSpin->get_value_as_int();
    _current_page = std::clamp<int>(page, 1, _pages.size());
    _setPreviewPage();
}

void RvngImportDialog::_onSpinButtonClickReleased(int, double, double)
{
    _spinning = false;
    _setPreviewPage();
}

void RvngImportDialog::_onSpinButtonClickPressed(int, double, double)
{
    _spinning = true;
}

/**
 * @brief Renders the given page's thumbnail
 */
void RvngImportDialog::_setPreviewPage()
{
    if (_spinning) {
        return;
    }

    _preview.setDocument(nullptr);

    _doc = SPDocument::createNewDocFromMem(as_span(_pages[_current_page - 1]));
    if (!_doc) {
        g_warning("CDR import: Could not create preview for page %d", _current_page);
        auto no_preview_template = R"A(
           <svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'>
               <path d='M 82,10 18,74 m 0,-64 64,64' style='fill:none;stroke:#ff0000;stroke-width:2px;'/>
               <rect x='18' y='10' width='64' height='64' style='fill:none;stroke:#000000;stroke-width:1.5px;'/>
               <text x='50' y='92' style='font-size:10px;text-anchor:middle;font-family:sans-serif;'>%1</text>
           </svg>
        )A";
        auto no_preview = Glib::ustring::compose(no_preview_template, _("No preview"));
        _doc = SPDocument::createNewDocFromMem(no_preview.raw());
    }

    if (!_doc) {
        std::cerr << "RvngImportDialog::_setPreviewPage: No document!" << std::endl;
        return;
    }

    _preview.setDocument(_doc.get());
}

std::unique_ptr<SPDocument> rvng_open(
    char const *uri,
    bool (*is_supported)(RVNGInputStream *),
    bool (*parse)(RVNGInputStream *, RVNGDrawingInterface *))
{
#ifdef _WIN32
    // RVNGFileStream uses fopen() internally which unfortunately only uses ANSI encoding on Windows
    // therefore attempt to convert uri to the system codepage
    // even if this is not possible the alternate short (8.3) file name will be used if available
    auto converted_uri = g_win32_locale_filename_from_utf8(uri);
    auto input = RVNGFileStream(converted_uri);
    g_free(converted_uri);
#else
    auto input = RVNGFileStream(uri);
#endif

    if (!is_supported(&input)) {
        return nullptr;
    }

    RVNGStringVector output;
    RVNGSVGDrawingGenerator generator(output, "svg");

    if (!parse(&input, &generator)) {
        return nullptr;
    }

    if (output.empty()) {
        return nullptr;
    }

    std::vector<RVNGString> tmpSVGOutput;
    for (unsigned i=0; i<output.size(); ++i) {
        RVNGString tmpString("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n");
        tmpString.append(output[i]);
        tmpSVGOutput.push_back(tmpString);
    }

    unsigned page_num = 1;

    // If only one page is present, import that one without bothering user
    if (tmpSVGOutput.size() > 1 && INKSCAPE.use_gui()) {
        auto dlg = RvngImportDialog(tmpSVGOutput);
        if (!dlg.showDialog()) {
            throw Input::open_cancelled();
        }

        // Get needed page
        page_num = std::clamp<int>(dlg.getSelectedPage(), 1, tmpSVGOutput.size());
    }

    auto doc = SPDocument::createNewDocFromMem(as_span(tmpSVGOutput[page_num - 1]));

    if (doc && !doc->getRoot()->viewBox_set) {
        // Scales the document to account for 72dpi scaling in librevenge(<=0.0.4)
        doc->setWidth(Inkscape::Util::Quantity(doc->getWidth().quantity, "pt"), false);
        doc->setHeight(Inkscape::Util::Quantity(doc->getHeight().quantity, "pt"), false);
        doc->setViewBox(Geom::Rect::from_xywh(0, 0, doc->getWidth().value("pt"), doc->getHeight().value("pt")));
    }

    return doc;
}

} // namespace Inkscape::Extension::Internal

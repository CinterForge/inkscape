// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for use with <image>.
 *
 * Copyright (C) 2022 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include "actions-element-image.h"
#include "actions-helper.h"

#include <giomm.h>  // Not <gtkmm.h>! To eventually allow a headless version!
#include <glibmm/i18n.h>
#include <gtkmm.h>  // OK, we lied. We pop-up an message dialog if external editor not found and if we have a GUI.

#include "desktop.h"
#include "document.h"
#include "document-undo.h"
#include "inkscape-application.h"
#include "inkscape-window.h"
#include "object/sp-clippath.h"
#include "object/sp-image.h"
#include "object/sp-rect.h"
#include "object/sp-use.h"
#include "object/uri.h"
#include "preferences.h"
#include "selection.h"            // Selection
#include "ui/dialog-run.h"
#include "ui/tools/select-tool.h"
#include "util/format_size.h"
#include "xml/href-attribute-helper.h"

namespace {
Glib::ustring image_get_editor_name(bool is_svg)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (is_svg) {
        return prefs->getString("/options/svgeditor/value", "inkscape");
    }
    return prefs->getString("/options/bitmapeditor/value", "gimp");
}

Inkscape::URI get_base_path_uri(SPDocument const &document)
{
    if (const char *document_base = document.getDocumentBase()) {
        return Inkscape::URI::from_dirname(document_base);
    }
    return Inkscape::URI::from_dirname(Glib::get_current_dir().c_str());
}

bool has_svg_extension(std::string const &filename)
{
    return Glib::str_has_suffix(filename, ".svg") || Glib::str_has_suffix(filename, ".SVG");
}
}  // namespace

// Note that edits are external to Inkscape and thus we cannot undo them!
void image_edit(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();
    if (selection->isEmpty()) {
        // Nothing to do.
        return;
    }

    for (auto item : selection->items()) {
        // In the case of a clone of an image, edit the original image.
        if (auto const *clone = cast<SPUse>(item)) {
            item = clone->trueOriginal();
        }
        if (!is<SPImage>(item)) {
            continue;
        }

        const char *href = Inkscape::getHrefAttribute(*item->getRepr()).second;
        if (!href) {
            show_output("image_edit: no xlink:href");
            continue;
        }

        auto const uri = Inkscape::URI(href, get_base_path_uri(*selection->document()));
        if (uri.hasScheme("data")) {
            // data URL scheme, see https://www.ietf.org/rfc/rfc2397.txt
            g_warning("Edit Externally: Editing embedded images (data URL) is not supported");
            continue;
        }
        if (const char *other_scheme = uri.getScheme(); other_scheme && !uri.hasScheme("file")) {
            // any other scheme than 'file'
            g_warning("Edit Externally: Cannot edit image (scheme '%s' not supported)", other_scheme);
            continue;
        }

        std::string filename;
        try {
            filename = uri.toNativeFilename();
        } catch (Glib::ConvertError const &e) {
            g_warning("Edit Externally: %s", e.what());
            continue;
        }
        std::string const command = Glib::shell_quote(image_get_editor_name(has_svg_extension(filename))) + " " +
                                    Glib::shell_quote(filename);

        const char *const message = _("Failed to edit external image.\n<small>Note: Path to editor can be set in "
                                      "Preferences dialog.</small>");
        try {
            Glib::spawn_command_line_async(command);
        } catch (Glib::SpawnError &error) {
            if (auto window = app->get_active_window()) {
                auto dialog = std::make_unique<Gtk::MessageDialog>(*window, message, true, Gtk::MessageType::WARNING, Gtk::ButtonsType::OK);
                dialog->property_destroy_with_parent() = true;
                dialog->set_name("SetEditorDialog");
                dialog->set_title(_("External Edit Image:"));
                dialog->set_secondary_text(
                    Glib::ustring::compose(_("System error message: %1"), error.what()));
                Inkscape::UI::dialog_show_modal_and_selfdestruct(std::move(dialog));
            } else {
                show_output(Glib::ustring("image_edit: ") + message);
            }
        } catch (Glib::ShellError &error) {
            g_critical("Edit Externally: %s\n%s %s", message, _("System error message:"), error.what());
        }
    }
}

/**
 * Attempt to crop an image's physical pixels by the rectangle give
 * OR if not specified, by any applied clipping object.
 */
void image_crop(InkscapeApplication *app)
{
    auto win = app->get_active_window();
    auto doc = app->get_active_document();
    auto msg = win->get_desktop()->messageStack();
    auto const tool = win->get_desktop()->getTool();
    int done = 0;
    int bytes = 0;

    auto selection = app->get_active_selection();
    if (selection->isEmpty()) {
        msg->flash(Inkscape::ERROR_MESSAGE, _("Nothing selected."));
        return;
    }

    // Find a target rectangle, if provided.
    Geom::OptRect target;
    SPRect *rect = nullptr;
    for (auto item : selection->items()) {
        rect = cast<SPRect>(item);
        if (rect) {
            target = rect->geometricBounds(rect->i2doc_affine());
            break;
        }
    }

    // For each selected item, we loop through and attempt to crop the
    // raster image to the geometric bounds of the clipping object.
    for (auto item : selection->items()) {
        if (auto image = cast<SPImage>(item)) {
            bytes -= std::strlen(image->href);
            Geom::OptRect area;
            if (target) {
                // MODE A. Crop to selected rectangle.
                area = target;
            } else if (auto clip = image->getClipObject()) {
                // MODE B. Crop to image's xisting clip region
                area = clip->geometricBounds(image->i2doc_affine());
            }
            done += (int)(area && image->cropToArea(*area));
            bytes += std::strlen(image->href);
        }
    }
    if (rect) {
        rect->deleteObject();
    }

    // Tell the user what happened, since so many things could have changed.
    if (done) {
        // The select tool has no idea the image description needs updating. Force it.
        if (auto selector = dynamic_cast<Inkscape::UI::Tools::SelectTool*>(tool)) {
            selector->updateDescriber(selection);
        }
        std::stringstream ss;
        ss << ngettext("<b>%d</b> image cropped", "<b>%d</b> images cropped", done);
        if (bytes < 0) {
            ss << ", " << ngettext("%s byte removed", "%s bytes removed", abs(bytes));
        } else if (bytes > 0) {
            ss << ", <b>" << ngettext("%s byte added!", "%s bytes added!", bytes) << "</b>";
        }
        // Do flashing after select tool update.
        msg->flashF(Inkscape::INFORMATION_MESSAGE, ss.str().c_str(), done, Inkscape::Util::format_size(abs(bytes)).c_str());
        Inkscape::DocumentUndo::done(doc, RC_("Undo", "Crop Images"), "ActionImageCrop");
    } else {
        msg->flash(Inkscape::WARNING_MESSAGE, _("No images cropped!"));
    }
}

const Glib::ustring SECTION = NC_("Action Section", "Images");

std::vector<std::vector<Glib::ustring>> raw_data_element_image =
{
    // clang-format off
    {"app.element-image-crop",    N_("Crop image to clip"), SECTION, N_("Remove parts of the image outside the applied clipping area.") },
    {"app.element-image-edit",    N_("Edit externally"),    SECTION, N_("Edit image externally (image must be selected and not embedded).")    },
    // clang-format on
};

void
add_actions_element_image(InkscapeApplication* app)
{
    auto *gapp = app->gio_app();

    // clang-format off
    gapp->add_action(                "element-image-crop",          sigc::bind(sigc::ptr_fun(&image_crop),      app));
    gapp->add_action(                "element-image-edit",          sigc::bind(sigc::ptr_fun(&image_edit),      app));
    // clang-format on

    app->get_action_extra_data().add_data(raw_data_element_image);
}


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

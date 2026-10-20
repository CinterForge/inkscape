// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 9/9/24.
//

#include "mesh-editor.h"

#include <glib/gi18n.h>
#include <glibmm/markup.h>
#include <gtkmm/button.h>

#include "document.h"
#include "object/sp-defs.h"
#include "object/sp-object.h"
#include "object/sp-mesh-gradient.h"
#include "ui/util.h"
#include "util/object-renderer.h"

namespace Inkscape::UI::Widget {

namespace {

class ResourceItem : public Glib::Object {
public:
    std::string id;
    Glib::ustring label;
    Glib::RefPtr<Gdk::Texture> image;
    bool editable;
    SPObject* object;
    int color;

    static Glib::RefPtr<ResourceItem> create(
        const std::string& id,
        const std::string& label,
        Glib::RefPtr<Gdk::Texture> image,
        SPObject* object,
        bool editable = false,
        uint32_t rgb24color = 0
    ) {
        auto item = Glib::make_refptr_for_instance<ResourceItem>(new ResourceItem());
        item->id = id;
        item->label = label;
        item->image = image;
        item->object = object;
        item->editable = editable;
        item->color = rgb24color;
        return item;
    }

private:
    ResourceItem() {}
};

auto get_id = [](const SPObject* object) { auto id = object->getId(); return id ? id : ""; };
auto label_fmt = [](const char* label, const std::string& id) { return label && *label ? label : '#' + id; };

} // namespace

MeshEditor::MeshEditor() :
    Gtk::Box(Gtk::Orientation::VERTICAL)
{
    set_spacing(4);

    _store = Gio::ListStore<ResourceItem>::create();
    _item_factory = IconViewItemFactory::create([=](auto& ptr) -> IconViewItemFactory::ItemData {
        auto rsrc = std::dynamic_pointer_cast<ResourceItem>(ptr);
        if (!rsrc) return {};

        auto name = Glib::Markup::escape_text(rsrc->label);
        return { .label_markup = name, .image = rsrc->image, .tooltip = rsrc->label };
    });
    _selection_model = Gtk::SingleSelection::create(_store);
    _selection_model->signal_selection_changed().connect([this](auto, auto) {
        if (_update.pending() || !_document) return;
        // fire selection changed
        if (auto item = std::dynamic_pointer_cast<ResourceItem>(_selection_model->get_selected_item())) {
            if (auto mesh = cast<SPMeshGradient>(_document->getObjectById(item->id))) {
                _signal_changed.emit(mesh);
            }
        }
    });

    _gridview.add_css_class("grid-view-compact");
    _gridview.add_css_class("frame");
    _gridview.set_factory(_item_factory->get_factory());
    _gridview.set_model(_selection_model);
    // 3 columns to prevent fill popup from expanding horizontally too much
    _gridview.set_max_columns(3);
    _scroll.set_child(_gridview);
    _scroll.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    _scroll.set_vexpand();
    // some minimum value to show at least three full rows and fourth one partially
    _scroll.set_size_request(-1, 240);

    auto label = Gtk::make_managed<Gtk::Label>(_("Mesh fill"));
    label->set_margin_top(4);
    label->set_halign(Gtk::Align::START);
    append(*label);
    append(_scroll);
    auto button = Gtk::make_managed<Gtk::Button>(_("Edit on canvas"));
    button->set_halign(Gtk::Align::CENTER);
    button->signal_clicked().connect([this]() {
        // todo: fire edit event
    });
    append(*button);
}

void MeshEditor::set_document(SPDocument* document) {
    if (_document == document) return;

    _document = document;
    _gradients.disconnect();
    _defs.disconnect();

    if (!_document) {
        // we could clear the store, but editor is not shown without doc, so save processing time
        return;
    }

    _gradients = _document->connectResourcesChanged("gradient", [this]() {
        _gradients_changed = true;
        schedule_update();
    });

    _defs = _document->getDefs()->connectModified([this](SPObject* obj, unsigned flags) {
        auto mesh = cast<SPMeshGradient>(obj);
        if (mesh && mesh->getArray() == mesh && (flags & SP_OBJECT_CHILD_MODIFIED_FLAG)) {
            _gradients_changed = true;
            schedule_update();
        }
    });

    _gradients_changed = true;
    schedule_update();
}

void MeshEditor::select_mesh(SPGradient* mesh) {
    _selected = mesh;
    _selected_id = mesh ? get_id(mesh) : "";
    if (!mesh) return;

    auto id = get_id(mesh);
    // find it and select it
    auto n = _store->get_n_items();
    for (int i = 0; i < n; ++i) {
        if (auto item = std::dynamic_pointer_cast<ResourceItem>(_store->get_object(i))) {
            if (item->id == id) {
                auto scoped(_update.block());
                _selection_model->set_selected(i);
                break;
            }
        }
    }

    // Requested mesh not found on a list yet. That's OK, our store may not be up to date yet
}

SPGradient* MeshEditor::get_selected_mesh() const {
    if (!_document) return nullptr;

    if (auto item = std::dynamic_pointer_cast<ResourceItem>(_selection_model->get_selected_item())) {
        return cast<SPGradient>(_document->getObjectById(item->id));
    }

    // nothing's selected; pick first one
    if (auto first = std::dynamic_pointer_cast<ResourceItem>(_store->get_object(0))) {
        return cast<SPGradient>(_document->getObjectById(first->id));
    }

    // we shouldn't be here
    // assert(false);
    return nullptr;
}

void MeshEditor::schedule_update() {
    if (_tick_callback) return;

    _tick_callback = add_tick_callback([this](auto &&) {
        _tick_callback = 0;
        update();
        return false;
    });
}

void MeshEditor::update() {
    if (!_gradients_changed) return;

    auto list = rebuild_list();
    rebuild_store(list);
}

void MeshEditor::rebuild_store(const std::vector<SPMeshGradient*>& list) {
    object_renderer renderer;
    _store->freeze_notify();
    _store->remove_all();

    // mesh preview size
    const int width = 30;
    const int height = 30;
    auto device_scale = get_scale_factor();
    object_renderer::options opt = {};

    // track index, so we can update selection, if there is one
    int index = 0;
    int selected = -1;
    for (auto item : list) {
        auto id = get_id(item);
        if (_selected_id == id) {
            selected = index;
        }
        auto labelstr = item->getAttribute("inkscape:label");
        auto label = label_fmt(labelstr, id);
        auto image = to_texture(renderer.render(*item, width, height, device_scale, opt));
        _store->append(ResourceItem::create(id, label, image, item));
        index++;
    }

    _store->thaw_notify();

    if (selected >= 0) {
        auto scoped(_update.block());
        _selection_model->set_selected(selected);
    }
}

std::vector<SPMeshGradient*> MeshEditor::rebuild_list() {
    std::vector<SPMeshGradient*> list;
    if (!_document) return list;

    auto gradients = _document->getResourceList("gradient");

    for (auto obj : gradients) {
        // collect root mesh gradients only
        if (auto mesh = cast<SPMeshGradient>(obj); mesh && mesh->getArray() == mesh) {
            list.push_back(mesh);
        }
    }

    //todo: sort if desired
    return list;
}

} // namespace

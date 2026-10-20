// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef INKSCAPE_UI_WIDGET_PATTERN_STORE_H
#define INKSCAPE_UI_WIDGET_PATTERN_STORE_H
/*
 * Copyright (C) 2022 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <2geom/transforms.h>
#include <gtkmm/widget.h>
#include <optional>

#include "colors/color.h"
#include "ui/filtered-store.h"

class SPDocument;

namespace Inkscape::UI::Widget {

// pattern parameters
class PatternItem : public Glib::Object
{
public:
    Cairo::RefPtr<Cairo::ImageSurface> pix;
    std::string id;
    std::string label;
    bool stock = false;
    std::optional<bool> uniform_scale;
    Geom::Affine transform;
    std::optional<double> rotation;
    std::optional<double> pitch;
    std::optional<double> stroke;
    Geom::Point offset;
    std::optional<Inkscape::Colors::Color> color;
    Geom::Scale gap;
    bool editable = true;
    SPDocument* collection = nullptr;

    bool operator == (const PatternItem& item) const {
        // compare all attributes apart from pixmap preview
        return
                id == item.id &&
                label == item.label &&
                stock == item.stock &&
                uniform_scale == item.uniform_scale &&
                transform == item.transform &&
                rotation == item.rotation &&
                pitch == item.pitch &&
                stroke == item.stroke &&
                offset == item.offset &&
                color == item.color &&
                gap == item.gap &&
                editable == item.editable &&
                collection == item.collection;
    }

    static Glib::RefPtr<PatternItem> create() {
        return Glib::make_refptr_for_instance(new PatternItem());
    }

protected:
    PatternItem() = default;
};

struct PatternStore {
    Inkscape::FilteredStore<PatternItem> store;
    std::map<Gtk::Widget*, Glib::RefPtr<PatternItem>> widgets_to_pattern;
};

}

#endif

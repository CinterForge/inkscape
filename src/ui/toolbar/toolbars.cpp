// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 *
 * A container for toolbars, displaying one toolbar at a time.
 *
 *//*
 * Authors:
 *  Tavmjong Bah
 *  Alex Valavanis
 *  Mike Kowalski
 *  Vaibhav Malik
 *  PBS
 *
 * Copyright (C) 2024 PBS
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "toolbars.h"

#include "ui/shortcuts.h"

// For creating toolbars
#include "ui/toolbar/arc-toolbar.h"
#include "ui/toolbar/booleans-toolbar.h"
#include "ui/toolbar/box3d-toolbar.h"
#include "ui/toolbar/calligraphy-toolbar.h"
#include "ui/toolbar/connector-toolbar.h"
#include "ui/toolbar/dropper-toolbar.h"
#include "ui/toolbar/eraser-toolbar.h"
#include "ui/toolbar/gradient-toolbar.h"
#include "ui/toolbar/lpe-toolbar.h"
#include "ui/toolbar/marker-toolbar.h"
#include "ui/toolbar/measure-toolbar.h"
#include "ui/toolbar/mesh-toolbar.h"
#include "ui/toolbar/node-toolbar.h"
#include "ui/toolbar/objectpicker-toolbar.h"
#include "ui/toolbar/page-toolbar.h"
#include "ui/toolbar/paintbucket-toolbar.h"
#include "ui/toolbar/pencil-toolbar.h"
#include "ui/toolbar/rect-toolbar.h"
#include "ui/toolbar/select-toolbar.h"
#include "ui/toolbar/spiral-toolbar.h"
#include "ui/toolbar/spray-toolbar.h"
#include "ui/toolbar/star-toolbar.h"
#include "ui/toolbar/text-toolbar.h"
#include "ui/toolbar/tweak-toolbar.h"
#include "ui/toolbar/zoom-toolbar.h"
#include "ui/tools/tool-base.h"

namespace Inkscape::UI::Toolbar {
namespace {

// Data for building and tracking toolbars.
struct ToolbarData
{
    Glib::ustring name;
    std::unique_ptr<Toolbar> (*create)();
};

template <typename T, auto... args>
auto create = [] () -> std::unique_ptr<Toolbar> { return std::make_unique<T>(args...); };

auto const toolbar_data = std::unordered_map<std::string, ToolbarData>{
    // clang-format off
    {"/tools/select",          {"Select",       create<SelectToolbar>}},
    {"/tools/nodes",           {"Node",         create<NodeToolbar>}},
    {"/tools/booleans",        {"Booleans",     create<BooleansToolbar>}},
    {"/tools/marker",          {"Marker",       create<MarkerToolbar>}},
    {"/tools/shapes/rect",     {"Rect",         create<RectToolbar>}},
    {"/tools/shapes/arc",      {"Arc",          create<ArcToolbar>}},
    {"/tools/shapes/star",     {"Star",         create<StarToolbar>}},
    {"/tools/shapes/3dbox",    {"3DBox",        create<Box3DToolbar>}},
    {"/tools/shapes/spiral",   {"Spiral",       create<SpiralToolbar>}},
    {"/tools/freehand/pencil", {"Pencil",       create<PencilToolbar, true>}},
    {"/tools/freehand/pen",    {"Pen",          create<PencilToolbar, false>}},
    {"/tools/calligraphic",    {"Calligraphic", create<CalligraphyToolbar>}},
    {"/tools/text",            {"Text",         create<TextToolbar>}},
    {"/tools/gradient",        {"Gradient",     create<GradientToolbar>}},
    {"/tools/mesh",            {"Mesh",         create<MeshToolbar>}},
    {"/tools/zoom",            {"Zoom",         create<ZoomToolbar>}},
    {"/tools/measure",         {"Measure",      create<MeasureToolbar>}},
    {"/tools/dropper",         {"Dropper",      create<DropperToolbar>}},
    {"/tools/tweak",           {"Tweak",        create<TweakToolbar>}},
    {"/tools/spray",           {"Spray",        create<SprayToolbar>}},
    {"/tools/connector",       {"Connector",    create<ConnectorToolbar>}},
    {"/tools/pages",           {"Pages",        create<PageToolbar>}},
    {"/tools/paintbucket",     {"Paintbucket",  create<PaintbucketToolbar>}},
    {"/tools/eraser",          {"Eraser",       create<EraserToolbar>}},
    {"/tools/lpetool",         {"LPETool",      create<LPEToolbar>}},
    {"/tools/picker",          {"ObjectPicker", create<ObjectPickerToolbar>}},
    // clang-format on
};

} // namespace

Toolbars::Toolbars()
    : Gtk::Box{Gtk::Orientation::VERTICAL}
{
    set_name("Toolbars");
}

Toolbars::~Toolbars()
{
    if (_current_toolbar) {
        _current_toolbar->setDesktop(nullptr);
    }
}

void Toolbars::setTool(Tools::ToolBase *tool)
{
    // Acquire the toolbar to be shown, possibly null
    Toolbar *toolbar = nullptr;

    if (tool) {
        auto &toolbars_entry = _toolbars[tool->getPrefsPath()];

        if (!toolbars_entry) {
            // Lazily create the toolbar.
            auto const &data = toolbar_data.at(tool->getPrefsPath());
            toolbars_entry = data.create();
            toolbars_entry->set_name(data.name + "Toolbar");
            toolbars_entry->set_hexpand();
            Shortcuts::getInstance().update_gui_text_recursive(toolbars_entry.get());
            append(*toolbars_entry);
        }

        toolbar = toolbars_entry.get();
    }

    if (toolbar != _current_toolbar) {
        // Tool has changed.
        if (_current_toolbar) {
            _current_toolbar->set_visible(false);
            _current_toolbar->setDesktop(nullptr);
        }
        _current_toolbar = toolbar;
        if (_current_toolbar) {
            _current_toolbar->setDesktop(tool->getDesktop());
            _current_toolbar->set_visible(true);
        }
    } else if (_current_toolbar && tool->getDesktop() != _current_toolbar->getDesktop()) {
        // Tool has stayed the same but desktop has changed.
        _current_toolbar->setDesktop(tool->getDesktop());
    }
}

void Toolbars::setActiveUnit(Util::Unit const *unit)
{
    if (_current_toolbar) {
        _current_toolbar->setActiveUnit(unit);
    }
}

Toolbar *Toolbars::get_current_toolbar()
{
    return _current_toolbar;
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

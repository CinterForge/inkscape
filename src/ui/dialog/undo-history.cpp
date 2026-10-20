// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Undo History dialog - implementation.
 */
/* Author:
 *   Gustav Broberg <broberg@kth.se>
 *   Abhishek Sharma
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2014 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "undo-history.h"

#include "document-undo.h"
#include "document.h"

namespace Inkscape::UI::Dialog {
namespace {

struct NoFilter : CellRendererInt::Filter
{
    bool operator()(const int &) const override { return true; }
};

struct GreaterThan : CellRendererInt::Filter
{
    explicit GreaterThan(int _i) : i(_i) {}
    bool operator()(const int &x) const override { return x > i; }
    int i;
};

auto const greater_than_1 = GreaterThan{1};

} // namespace

CellRendererInt::Filter const &CellRendererInt::no_filter = NoFilter{};

UndoHistory::UndoHistory()
    : DialogBase{"/dialogs/undo-history", "UndoHistory"}
    , _event_list_selection{_event_list_view.get_selection()}
{
    auto const columns = &EventLog::getColumns();

    append(_scrolled_window);
    _scrolled_window.set_vexpand();
    _scrolled_window.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);

    _event_list_view.set_enable_search(false);
    _event_list_view.set_headers_visible(false);

    auto const icon_renderer = Gtk::make_managed<Gtk::CellRendererPixbuf>();
    icon_renderer->property_xpad() = 2;
    icon_renderer->property_width() = 24;
    int cols_count = _event_list_view.append_column("Icon", *icon_renderer);

    auto const icon_column = _event_list_view.get_column(cols_count - 1);
    icon_column->add_attribute(icon_renderer->property_icon_name(), columns->icon_name);

    auto const children_renderer = Gtk::make_managed<CellRendererInt>(greater_than_1);
    children_renderer->property_weight() = 600; // =Pango::WEIGHT_SEMIBOLD (not defined in old versions of pangomm)
    children_renderer->property_xalign() = 1.0;
    children_renderer->property_xpad() = 2;
    children_renderer->property_width() = 24;

    cols_count = _event_list_view.append_column("Children", *children_renderer);
    auto const children_column = _event_list_view.get_column(cols_count - 1);
    children_column->add_attribute(children_renderer->property_number(), columns->child_count);

    auto const description_renderer = Gtk::make_managed<Gtk::CellRendererText>();
    description_renderer->property_ellipsize() = Pango::EllipsizeMode::END;

    cols_count = _event_list_view.append_column("Description", *description_renderer);
    auto const description_column = _event_list_view.get_column(cols_count - 1);
    description_column->add_attribute(description_renderer->property_text(), columns->description);
    description_column->set_resizable();
    description_column->set_sizing(Gtk::TreeViewColumn::Sizing::AUTOSIZE);
    description_column->set_min_width (150);

    _event_list_view.set_expander_column(*_event_list_view.get_column(cols_count - 1));

    _scrolled_window.set_child(_event_list_view);
    _scrolled_window.set_overlay_scrolling(false);

    _event_list_selection->signal_changed().connect(sigc::mem_fun(*this, &Inkscape::UI::Dialog::UndoHistory::_onListSelectionChange));
    _event_list_view.signal_row_expanded().connect(sigc::mem_fun(*this, &Inkscape::UI::Dialog::UndoHistory::_onExpandEvent));
    _event_list_view.signal_row_collapsed().connect(sigc::mem_fun(*this, &Inkscape::UI::Dialog::UndoHistory::_onCollapseEvent));
}

UndoHistory::~UndoHistory()
{
    disconnectEventLog();
}

void UndoHistory::documentReplaced()
{
    disconnectEventLog();
    connectEventLog();
}

void UndoHistory::disconnectEventLog()
{
    if (_event_log) {
        auto guard = _blocker.block();
        _row_changed_conn.disconnect();
        _event_list_view.unset_model();
        _event_list_store = {};
        _event_log = nullptr;
    }
}

void UndoHistory::connectEventLog()
{
    if (auto document = getDocument()) {
        auto guard = _blocker.block();
        _event_log = document->get_event_log();
        _event_list_store = _event_log->getEventListStore();
        _event_list_view.set_model(_event_list_store);
        auto path = _event_list_store->get_path(_event_log->getCurrEvent());
        _event_list_view.expand_to_path(path);
        _event_list_selection->select(path);
        _event_list_view.scroll_to_row(path);
        _row_changed_conn = _event_log->connectRowChanged([this] { _onRowChanged(); });
    }
}

// Called when the document's undo history position just moved to a new place.
void UndoHistory::_onRowChanged()
{
    if (_blocker.pending()) {
        return;
    }
    auto guard = _blocker.block();

    auto old_iter = _event_list_selection->get_selected();
    auto old_parent = old_iter->parent() ? old_iter->parent() : old_iter;

    auto iter = _event_log->getCurrEvent();
    auto new_parent = iter->parent() ? iter->parent() : iter;

    if (old_parent && (!new_parent || old_parent != new_parent)) {
        // Collapse branches upon leaving them.
        _event_list_view.collapse_row(_event_list_store->get_path(old_parent));
    }

    auto path = _event_list_store->get_path(iter);
    _event_list_view.expand_to_path(path);
    _event_list_selection->select(path);
    _event_list_view.scroll_to_row(path);
}

// Called when the user just selected a new item in the undo history tree view.
void UndoHistory::_onListSelectionChange()
{
    if (_blocker.pending()) {
        return;
    }
    auto guard = _blocker.block();

    auto selected = _event_list_selection->get_selected();
    if (!selected) {
        // Can happen when collapsing a section that contained the selection, causing the selection to become null.
        // In this case _onCollapseEvent() will be called immediately after and re-select the correct item.
        return;
    }

    // Selecting a collapsed parent event is equal to selecting the last child of that parent's branch.
    if (!selected->children().empty() && !_event_list_view.row_expanded(_event_list_store->get_path(selected))) {
        selected = --selected->children().end();
    }

    _event_log->seekTo(selected);
}

void UndoHistory::_onExpandEvent(Gtk::TreeModel::iterator const &iter, Gtk::TreeModel::Path const &)
{
    if (_blocker.pending()) {
        return;
    }
    auto guard = _blocker.block(); // block _onListSelectionChange()

    if (iter == _event_list_selection->get_selected()) {
        _event_list_selection->select(_event_log->getCurrEvent());
    }
}

void UndoHistory::_onCollapseEvent(Gtk::TreeModel::iterator const &iter, Gtk::TreeModel::Path const &)
{
    if (_blocker.pending()) {
        return;
    }
    auto guard = _blocker.block(); // block _onListSelectionChange()

    // Collapsing a branch we're currently in is equal to stepping to the last event in that branch
    auto old_iter = _event_log->getCurrEvent();
    auto old_parent = old_iter->parent() ? old_iter->parent() : old_iter;
    if (old_parent && old_parent == iter) {
        _event_log->seekTo(--iter->children().end());
        _event_list_selection->select(iter);
    }
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

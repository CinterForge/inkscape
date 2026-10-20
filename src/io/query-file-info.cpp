// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 4/30/25.
//

#include "query-file-info.h"

#include <iostream>
#include <unordered_set>
#include <utility>

namespace Inkscape::UI {

static std::unordered_set<QueryFileInfo*> pending_operations;

QueryFileInfo::QueryFileInfo(const std::string& path_to_test, std::function<void (Glib::RefPtr<Gio::FileInfo>)> on_result):
    _on_result(std::move(on_result)) {

    _file = Gio::File::create_for_path(path_to_test);
    _operation = Gio::Cancellable::create();
    _file->query_info_async([this](auto& r) { results_callback(this, r); }, _operation);
    pending_operations.insert(this);
}

QueryFileInfo::~QueryFileInfo() {
    pending_operations.erase(this);
    _operation->cancel();
}

void QueryFileInfo::results_callback(QueryFileInfo* self, Glib::RefPtr<Gio::AsyncResult>& result) {
    if (pending_operations.contains(self)) {
        self->results(result);
        pending_operations.erase(self);
    }
}

void QueryFileInfo::results(Glib::RefPtr<Gio::AsyncResult>& result) const {
    try {
        auto info = _file->query_info_finish(result);
        _on_result(info);
    }
    catch (Glib::Error& ex) {
        if (ex.code() == 1) {
            // path points to nonexistent object
            _on_result({});
            return;
        }
        std::cerr << "Async file query error: " << ex.what() << ", " << ex.code() << std::endl;
    }
}

} // namespace

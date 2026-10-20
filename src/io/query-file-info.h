// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 4/30/25.
//
// UI utility: Async file info query. It retrieves file/directory information
// without blocking the main thread and delivers results on the same thread.

#ifndef QUERY_FILE_INFO_H
#define QUERY_FILE_INFO_H

#include <string>
#include <giomm/file.h>

namespace Inkscape::UI {

class QueryFileInfo final {
public:
    QueryFileInfo(const std::string& path_to_test, std::function<void (Glib::RefPtr<Gio::FileInfo>)> on_result);

    QueryFileInfo(const QueryFileInfo&) = delete;
    QueryFileInfo& operator = (const QueryFileInfo&) = delete;

    ~QueryFileInfo();
private:
    void results(Glib::RefPtr<Gio::AsyncResult>& result) const;
    static void results_callback(QueryFileInfo* self, Glib::RefPtr<Gio::AsyncResult>& result);

    std::function<void (Glib::RefPtr<Gio::FileInfo>)> _on_result;
    Glib::RefPtr<Gio::File> _file;
    Glib::RefPtr<Gio::Cancellable> _operation;
};

} // namespace

#endif //QUERY_FILE_INFO_H

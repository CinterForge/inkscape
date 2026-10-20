// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief C++ wrapping for libspelling C API.
 */
#ifndef INKSCAPE_UI_LIBSPELLING_WRAPPER_H
#define INKSCAPE_UI_LIBSPELLING_WRAPPER_H

#include <vector>

#include <libspelling.h>
#include <glibmm/ustring.h>
#include <giomm/actiongroup.h>
#include <giomm/menumodel.h>

#include "util/delete-with.h"
#include "util/gobjectptr.h"

namespace Inkscape::UI {

template <typename T, typename F>
void foreach(GPtrArray *arr, F &&f)
{
    g_ptr_array_foreach(arr, +[] (void *ptr, void *data) {
        auto t = reinterpret_cast<T *>(ptr);
        auto f = reinterpret_cast<F *>(data);
        f->operator()(t);
    }, &f);
}

template <typename F>
void foreach(char **strs, F &&f)
{
    if (!strs) {
        return;
    }
    for (int i = 0; strs[i]; i++) {
        f(strs[i]);
    }
}

template <typename F>
inline void list_language_names_and_codes(SpellingProvider *provider, F &&cb)
{
#if SPELLING_CHECK_VERSION(0, 3, 0)
    auto const languages = Util::delete_with<g_object_unref>(spelling_provider_list_languages(provider));
    auto const n_items = g_list_model_get_n_items(languages.get());

    for (auto i = 0U; i < n_items; ++i) {
        auto const language = static_cast<SpellingLanguage *>(g_list_model_get_item(languages.get(), i));
        auto const name = spelling_language_get_name(language);
        auto const code = spelling_language_get_code(language);
        cb(name, code);
    }
#else
    // TODO: remove when libspelling has API stability
    auto const languages = Util::delete_with<g_ptr_array_unref>(spelling_provider_list_languages(provider));
    foreach
        <SpellingLanguageInfo>(languages.get(), [&](auto language) {
            auto const name = spelling_language_info_get_name(language);
            auto const code = spelling_language_info_get_code(language);
            cb(name, code);
        });
#endif
}

inline auto list_corrections_c(SpellingChecker *checker, char const *word)
{
    return Util::delete_with<g_strfreev>(spelling_checker_list_corrections(checker, word));
}

inline auto list_corrections(SpellingChecker *checker, char const *word)
{
    std::vector<Glib::ustring> result;

    foreach(list_corrections_c(checker, word).get(), [&] (auto correction) {
        result.emplace_back(correction);
    });

    return result;
}

inline auto spelling_text_buffer_adapter_create(GtkSourceBuffer *buffer, SpellingChecker *checker)
{
    return Util::GObjectPtr(spelling_text_buffer_adapter_new(buffer, checker));
}

inline auto get_menu_model(SpellingTextBufferAdapter &adapter)
{
    return Glib::wrap(spelling_text_buffer_adapter_get_menu_model(&adapter), true);
}

inline auto as_action_group(SpellingTextBufferAdapter &adapter)
{
    return Glib::wrap(G_ACTION_GROUP(&adapter), true);
}

inline void set_enabled(SpellingTextBufferAdapter &adapter, bool enabled)
{
    spelling_text_buffer_adapter_set_enabled(&adapter, enabled);
}

} // namespace Inkscape::UI

#endif // INKSCAPE_UI_LIBSPELLING_WRAPPER_H

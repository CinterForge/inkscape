// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Nicholas Bishop <nicholasbishop@gmail.com>
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *
 * Copyright (C) 2007 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_WIDGET_COMBO_ENUMS_H
#define INKSCAPE_UI_WIDGET_COMBO_ENUMS_H

#include <glibmm/i18n.h>

#include "attr-widget.h"
#include "ui/widget/drop-down-list.h"
#include "ui/widget/labelled.h"
#include "util/enums.h"

namespace Inkscape::UI::Widget {

/**
 * Simplified management of enumerations in the UI as combobox.
 */
template <typename E> class ComboBoxEnum
    : public DropDownList
    , public AttrWidget
{
public:
    [[nodiscard]] ComboBoxEnum(E const default_value, const Util::EnumDataConverter<E>& c,
                               SPAttr const a = SPAttr::INVALID, bool const sort = true,
                               const char* translation_context = nullptr)
        : ComboBoxEnum{c, a, sort, translation_context, static_cast<unsigned>(default_value)}
    {
        set_active_by_id(default_value);
    }

    [[nodiscard]] ComboBoxEnum(Util::EnumDataConverter<E> const &c,
                               SPAttr const a = SPAttr::INVALID, bool const sort = true,
                               const char * const translation_context = nullptr)
        : ComboBoxEnum{c, a, sort, translation_context, 0u}
    {
        set_active(0);
    }

    void set_active(unsigned int pos) {
        set_selected(pos);
    }

    unsigned int get_active() const {
        return get_selected();
    }

private:
    struct Data {
        E id;
        Glib::ustring label;
        Glib::ustring key;
        bool separator = false;
    };
    std::vector<Data> _enums;

    [[nodiscard]] ComboBoxEnum(Util::EnumDataConverter<E> const &c,
                               SPAttr const a, bool const sort,
                               const char * const translation_context,
                               unsigned const default_value)
        : AttrWidget(a, default_value)
        , setProgrammatically(false)
        , _converter(c) {

        property_selected().signal_changed().connect(signal_attr_changed().make_slot());

        _enums.reserve(_converter._length);
        bool separator = false;

        for (unsigned int i = 0; i < _converter._length; ++i) {
            const auto& data = _converter.data(i);
            if (data.key == "-") {
                separator = true;
                continue;
            }

            Glib::ustring translated = translation_context ?
                g_dpgettext2(nullptr, translation_context, data.label.c_str()) :
                gettext(data.label.c_str());
            _enums.push_back(Data{data.id, translated, data.key, separator});
            separator = false;
        }

        if (sort) {
            std::sort(begin(_enums), end(_enums), [](const auto& a, const auto& b){ return a.label < b.label; });
        }

        set_row_separator_func([this](unsigned int pos){
            return pos < _enums.size() && _enums[pos].separator;
        });

        for (auto& el : _enums) {
            append(el.label);
        }
    }

public:
    [[nodiscard]] Glib::ustring get_as_attribute() const final
    {
        auto pos = get_selected();
        if (pos < _enums.size()) {
            return _enums[pos].key;
        }
        return {};
    }

    void set_from_attribute(SPObject * const o) final
    {
        setProgrammatically = true;

        if (auto const val = attribute_value(o)) {
            set_active_by_id(_converter.get_id_from_key(val));
        } else {
            set_active(get_default()->as_uint());
        }
    }

    std::optional<E> get_selected_id() const {
        auto pos = get_selected();
        if (pos < _enums.size()) {
            return _enums[pos].id;
        }
        return {};
    }

    void set_active_by_id(E id) {
        setProgrammatically = true;
        auto index = get_active_by_id(id);
        if (index >= 0) {
            set_active(index);
        }
    };

    void set_active_by_key(const Glib::ustring& key) {
        setProgrammatically = true;
        set_active_by_id( _converter.get_id_from_key(key) );
    };

    bool setProgrammatically = false;

private:
    [[nodiscard]] int get_active_by_id(E const id) const
    {
        auto it = std::find_if(begin(_enums), end(_enums), [id](const auto& el){ return el.id == id; });
        return it == end(_enums) ? -1 : std::distance(begin(_enums), it);
    }

    const Util::EnumDataConverter<E>& _converter;
};


/**
 * Simplified management of enumerations in the UI as combobox, plus the functionality of Labelled.
 */
template <typename E> class LabelledComboBoxEnum
    : public Labelled
{
public:
    [[nodiscard]] LabelledComboBoxEnum(Glib::ustring const &label,
                                       Glib::ustring const &tooltip,
                                       Util::EnumDataConverter<E> const &c,
                                       Glib::ustring const &icon = {},
                                       bool const mnemonic = true,
                                       bool const sort = true)
        : Labelled{label, tooltip,
                   Gtk::make_managed<ComboBoxEnum<E>>(c, SPAttr::INVALID, sort),
                   icon, mnemonic}
    { 
    }

    [[nodiscard]] ComboBoxEnum<E> *getCombobox()
    {
        return static_cast<ComboBoxEnum<E> *>(getWidget());
    }
};

} // namespace Inkscape::UI::Widget

#endif // INKSCAPE_UI_WIDGET_COMBO_ENUMS_H

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

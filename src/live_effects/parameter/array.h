// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::LivePathEffectParameters
 *
 * Copyright (C) Johan Engelen 2008 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_LIVEPATHEFFECT_PARAMETER_ARRAY_H
#define INKSCAPE_LIVEPATHEFFECT_PARAMETER_ARRAY_H

#include <cstddef>
#include <vector>
#include <glib.h>

#include "bad-uri-exception.h"
#include "helper/geom-nodesatellite.h"
#include "live_effects/parameter/parameter.h"
#include "live_effects/parameter/satellite-reference.h"
#include "object/uri.h"
#include "svg/stringstream.h"

namespace Inkscape::LivePathEffect {

namespace TpS {
// we need a separate namespace to avoid clashes with other LPEs
class KnotHolderEntityAttachBegin;
class KnotHolderEntityAttachEnd;
} // namespace TpS

template <typename StorageType>
class ArrayParam : public Parameter {
public:
    ArrayParam( const Glib::ustring& label,
                const Glib::ustring& tip,
                const Glib::ustring& key,
                Inkscape::UI::Widget::Registry* wr,
                Effect* effect,
                std::size_t n = 0 )
        : Parameter(label, tip, key, wr, effect), _vector(n), _default_size(n)
    {
    }

    ArrayParam(const ArrayParam&);
    ArrayParam& operator=(const ArrayParam&);

    std::vector<StorageType> const & data() const {
        return _vector;
    }

    Gtk::Widget * param_newWidget() override {
        return nullptr;
    }

    bool param_readSVGValue(char const * const strvalue) override {
        _vector.clear();
        auto const strarray = g_strsplit(strvalue, "|", 0);
        auto iter = strarray;
        
        while (*iter != nullptr) {
            Glib::ustring fixer = *iter;
            fixer.erase(0, fixer.find_first_not_of(" "));
            fixer.erase(fixer.find_last_not_of(" ")+1); 
            _vector.push_back( readsvg(fixer.c_str()) );
            iter++;
        }
        g_strfreev (strarray);
        return true;
    }

    void param_update_default(char const * default_value) override{};

    Glib::ustring param_getSVGValue() const override {
        Inkscape::SVGOStringStream os;
        writesvg(os, _vector);
        return os.str();
    }
    
    Glib::ustring param_getDefaultSVGValue() const override {
        return "";
    }

    void param_setValue(std::vector<StorageType> const &new_vector) {
        _vector = new_vector;
    }

    void param_set_default() override {
        param_setValue( std::vector<StorageType>(_default_size) );
    }

    void param_set_and_write_new_value(std::vector<StorageType> const &new_vector) {
        Inkscape::SVGOStringStream os;
        writesvg(os, new_vector);
        param_write_to_repr(os.str().c_str());
    }

    ParamType paramType() const override { return ParamType::ARRAY; };
    bool valid_index(int index) const { return _vector.size() > index; }

protected:
    friend class TpS::KnotHolderEntityAttachBegin;
    friend class TpS::KnotHolderEntityAttachEnd;
    std::vector<StorageType> _vector;
    std::size_t _default_size;

    void writesvg(SVGOStringStream &str, std::vector<StorageType> const &vector) const {
        for (std::size_t i = 0; i < vector.size(); ++i) {
            if (i != 0) {
                // separate items with pipe symbol
                str << " | ";
            }
            writesvgData(str,vector[i]);
        }
    }
    
    void writesvgData(SVGOStringStream &str, float const &vector_data) const {
        str << vector_data;
    }

    void writesvgData(SVGOStringStream &str, double const &vector_data) const {
        str << vector_data;
    }

    void writesvgData(SVGOStringStream &str, Glib::ustring const &vector_data) const {
        str << vector_data;
    }

    void writesvgData(SVGOStringStream &str, Geom::Point const &vector_data) const {
        str << vector_data;
    }

    void writesvgData(SVGOStringStream &str, std::shared_ptr<SatelliteReference> const &vector_data) const
    {
        if (vector_data && vector_data->isAttached()) {
            str << vector_data->getURI()->str();
            if (vector_data->getHasActive()) {
                str << ",";
                str << vector_data->getActive();
            }
        }
    }

    void writesvgData(SVGOStringStream &str, std::vector<NodeSatellite> const &vector_data) const
    {
        for (std::size_t i = 0; i < vector_data.size(); ++i) {
            if (i != 0) {
                // separate nodes with @ symbol ( we use | for paths)
                str << " @ ";
            }
            str << vector_data[i].getNodeSatellitesTypeGchar();
            str << ",";
            str << vector_data[i].is_time;
            str << ",";
            str << vector_data[i].selected;
            str << ",";
            str << vector_data[i].has_mirror;
            str << ",";
            str << vector_data[i].hidden;
            str << ",";
            str << vector_data[i].amount;
            str << ",";
            str << vector_data[i].angle;
            str << ",";
            str << static_cast<int>(vector_data[i].steps);
        }
    }

    StorageType readsvg(char const * str);
};

} // namespace Inkscape::LivePathEffect

#endif // INKSCAPE_LIVEPATHEFFECT_PARAMETER_ARRAY_H

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

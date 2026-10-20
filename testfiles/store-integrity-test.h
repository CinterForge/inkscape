// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * SVG store test
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL version 2 or later, read the file 'COPYING' for more information
 */

#include <gtest/gtest.h>
#include "compare-paths-test.h"
#include "xml/attribute-record.h"
#include <2geom/pathvector.h>
#include <glibmm/fileutils.h>
#include "io/sys.h"

#include "document.h"
#include "file.h"
#include "inkscape.h"
#include "inkscape-application.h"

#include "extension/init.h"
#include "extension/output.h"
#include "object/sp-root.h"
#include "svg/svg.h"
#include "util/numeric/converters.h"

using namespace Inkscape;

/* This class allow test stored items
 */

enum StoreIntegrityMode {
    NO_UPDATE,
    UPDATE_ORIGINAL,
    UPDATE_SAVED,
    UPDATE_BOTH
};

class StoreIntegrityTest : public ComparePathsTest
{
protected:

    void SetUp() override
    {
        // setup hidden dependency
        Application::create(false);
        Inkscape::Extension::init();
        svg = INKSCAPE_TESTS_DIR;
        svg += "/store_integrity_tests/store.svg"; // gitlab use this separator
    }

    // you can override custom threshold from svg file using in 
    // root svg from global and override with per shape "inkscape:test-threshold"
    void testDoc(std::string file, StoreIntegrityMode mode) 
    {
        auto doc = SPDocument::createNewDoc(file.c_str());
        ASSERT_TRUE(doc);
        doc->ensureUpToDate();
        SPLPEItem *lpeitem = doc->getRoot();
        if (mode == StoreIntegrityMode::UPDATE_ORIGINAL || mode == StoreIntegrityMode::UPDATE_BOTH) {
            lpeitem->updateRepr(SP_OBJECT_CHILD_MODIFIED_FLAG);
        }
        Inkscape::XML::Document * xmldoc = doc->getReprDoc();
        std::string svg_out = "store_integrity_test_tmp.svg";
        // Try to save the file
        // Following code needs to be reviewed
        
        FILE *filesave = Inkscape::IO::fopen_utf8name(svg_out.c_str(), "w");
        if (!filesave) {
            FAIL() << "File " << svg_out << " could not be saved.";
        }
        sp_repr_save_stream(xmldoc, filesave, SP_SVG_NS_URI);
        fclose(filesave);
        auto doc_out = SPDocument::createNewDoc(svg_out.c_str());
        ASSERT_TRUE(doc_out);
        doc_out->ensureUpToDate();
        if (mode == StoreIntegrityMode::UPDATE_SAVED || mode == StoreIntegrityMode::UPDATE_BOTH) {
            doc_out->getRoot()->updateRepr(SP_OBJECT_CHILD_MODIFIED_FLAG);
        }
        for (auto obj_out : doc_out->getObjectsBySelector("*")) {
            const gchar *id = obj_out->getId();
            if (!id) {
                continue;
            }
            if (auto obj = doc->getObjectById(id)) {
                double precision = getPrecision(lpeitem, obj);
                compareAttributes(obj->getRepr()->attributeList(), obj_out->getRepr()->attributeList(), precision, id);
            } else {
                FAIL() << "[FAILED  OBJECT NOT FOUND] " << id << std::endl; 
            }
        }
        remove(svg_out.c_str());
    }

    void compareAttributes(std::vector<Inkscape::XML::AttributeRecord, Inkscape::GC::Alloc<Inkscape::XML::AttributeRecord> > attrs_obj, 
                           std::vector<Inkscape::XML::AttributeRecord, Inkscape::GC::Alloc<Inkscape::XML::AttributeRecord> > attrs_obj_out, 
                           double precision, const gchar * id) 
    {
        for (auto const &attr_out : attrs_obj_out) {
            if (!std::any_of(attrs_obj.begin(), attrs_obj.end(), [&attr_out](Inkscape::XML::AttributeRecord const &attr){ return attr_out.key == attr.key; })) {
                FAIL() << "[FAILED REMOVED ATTRIBUTE ON SAVE] " << id << "::" << g_quark_to_string(attr_out.key) << std::endl; 
            }
        }
        for (auto const &attr : attrs_obj) {
            bool found = false;
            for (auto const &attr_out : attrs_obj_out) {
                if (attr.key == attr_out.key) {
                    found = true;
                    if (!g_strcmp0(g_quark_to_string(attr.key), "d") || 
                        !g_strcmp0(g_quark_to_string(attr.key), "inkscape-orignal-d")) 
                    {
                        pathCompare(attr.value.pointer(), attr_out.value.pointer(), id, "store.svg", precision);
                    } else {
                        double nattr;
                        unsigned int success = sp_svg_number_read_d(attr.value.pointer(), &nattr);
                        if (success == 1) {
                            double nattr_out;
                            success = sp_svg_number_read_d(attr_out.value.pointer(), &nattr_out);
                            if (!Geom::are_near(nattr, nattr_out, precision)) {
                                FAIL() << "[FAILED  ATTRIBUTE] key:" << g_quark_to_string(attr.key)  << " Attrs:" <<  attr.value.pointer() << " != " << attr_out.value.pointer() << std::endl; 
                            }
                        } else if (g_strcmp0(attr.value.pointer(), attr_out.value.pointer())) {
                            FAIL() << "[FAILED  ATTRIBUTE] key:" << g_quark_to_string(attr.key)  << " Attrs:" <<  attr.value.pointer() << " != " << attr_out.value.pointer() << std::endl; 
                        }
                    }
                    break;
                }
            }
            if (!found) {
                FAIL() << "[FAILED MISSING ATTRIBUTE ON OPEN] " << id << "::" << g_quark_to_string(attr.key) << std::endl; 
            }
        }

    }
    std::string svg = ""; 
};
/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

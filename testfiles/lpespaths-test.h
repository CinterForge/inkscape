// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * LPE test file wrapper
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2020 Authors
 *
 * Released under GNU GPL version 2 or later, read the file 'COPYING' for more information
 */

#include <gtest/gtest.h>
#include "compare-paths-test.h"
#include <2geom/pathvector.h>

#include "document.h"
#include "document-update.h"
#include "inkscape.h"
#include "inkscape-application.h"
#include "preferences.h"

#include "extension/init.h"
#include "object/sp-root.h"
#include "svg/svg.h"
#include "util/numeric/converters.h"

using namespace Inkscape;

/* This class allow test LPE's. To make possible in latest release of Inkscape
 * LPE is not updated on load (if in the future any do we must take account) so we load
 * a svg, get all "d" attribute from paths, shapes...
 * Update all path effects with root object and check equality of paths.
 * We use some helpers inside the SVG document to test:
 * inkscape:test-threshold="0.1" can be global using in root element or per item
 * inkscape:test-ignore="1" ignore this element from tests
 * Question: Maybe is better store SVG as files instead inline CPP files, there is a 
 * 1.2 started MR, I can't finish without too much work than a cmake advanced user
 */

class LPESPathsTest : public ComparePathsTest
{
protected:
    void SetUp() override
    {
        // setup hidden dependency
        Application::create(false);
        Inkscape::Extension::init();
        const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
        // Set the same precision used to originally create these files
        Inkscape::Preferences::get()->setInt("/options/svgoutput/numericprecision", 8);
        svg = test_info->file();
#ifdef INKSCAPE_TESTS_DIR
        svg = INKSCAPE_TESTS_DIR;
#else
        size_t pos = svg.find("lpespaths-test.h");
        svg.erase(pos);
#endif
        svg += "/lpe_tests/"; // gitlab use this separator
        /* svg += test_info->test_suite_name(); */
        svg += test_info->name();
        svg += ".svg";
    }

    // you can override custom threshold from svg file using in 
    // root svg from global and override with per shape "inkscape:test-threshold"
    void testDoc(std::string file) 
    {
        std::unique_ptr<SPDocument> doc{SPDocument::createNewDoc(file.c_str())};
        ASSERT_TRUE(doc != nullptr);
        SPLPEItem *lpeitem = doc->getRoot();
        std::vector<SPObject *> objs;
        std::vector<Glib::ustring> ids;
        std::vector<Glib::ustring> lpes;
        std::vector<Glib::ustring> ds;
        for (auto obj : doc->getObjectsByElement("path")) {
            objs.push_back(obj);
        }
        for (auto obj : doc->getObjectsByElement("ellipse")) {
            objs.push_back(obj);
        }
        for (auto obj : doc->getObjectsByElement("circle")) {
            objs.push_back(obj);
        }
        for (auto obj : doc->getObjectsByElement("rect")) {
            objs.push_back(obj);
        }
        for (auto obj : objs) {
            SPObject *parentobj = obj->parent;
            SPObject *layer = obj;
            while (parentobj->parent && parentobj->parent->parent) {
                layer = parentobj;
                parentobj = parentobj->parent;
            }
            if (!g_strcmp0(obj->getAttribute("d"), "M 0,0")) {
                if (obj->getAttribute("id")) {
                    std::cout << "[ WARN     ] Item with id:" << obj->getAttribute("id") << " has empty path data" << std::endl;
                }
            } else if (!layer->getAttribute("inkscape:test-ignore") && obj->getAttribute("d") && obj->getAttribute("id"))  {
                ds.push_back(Glib::ustring(obj->getAttribute("d")));
                ids.push_back(Glib::ustring(obj->getAttribute("id")));
                lpes.push_back(Glib::ustring(layer->getAttribute("inkscape:label") ? layer->getAttribute("inkscape:label") : layer->getAttribute("id")));
            }
        }
        sp_file_fix_lpe(doc.get());
        doc->ensureUpToDate();
        sp_lpe_item_update_patheffect(lpeitem, true, true, true);
        // to bypass onload
        sp_lpe_item_update_patheffect(lpeitem, true, true, true);
        size_t index = 0;
        for (auto id : ids) {
            SPObject *obj = doc->getObjectById(id);
            if (obj) {
                if (!obj->getAttribute("inkscape:test-ignore")) {
                    Glib::ustring idandlayer = obj->getAttribute("id"); 
                    idandlayer += Glib::ustring("(") + lpes[index] + ")"; // top layers has the LPE name tested in in id
                    pathCompare(ds[index].c_str(), obj->getAttribute("d"), idandlayer, svg, getPrecision(lpeitem, obj));
                } else {
                    std::cout << "[ WARN     ] Item with id:" << obj->getAttribute("id") << " ignored by inkscape:test-ignore" << std::endl;
                }
            } else {
                std::cout << "[ WARN     ] Item with id:" << id << " removed on apply LPE" << std::endl;
            }
            index++;
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

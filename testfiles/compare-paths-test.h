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

#include <2geom/pathvector.h>

#include "document.h"
#include "file.h"
#include "inkscape.h"
#include "inkscape-application.h"

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
 */

class ComparePathsTest : public ::testing::Test
{
protected:
    /*
    * @a is in LPE the path data before LPE in store integrity is the file just opened before save
    * @b is in LPE the path with LPE updated in, in integrity is the data in path after save to disk
    * @c the svg file name to compare
    */
    void pathCompare(char const *a, char const *b, Glib::ustring const &id, std::string const &svg, double precision = 0.001)
    {
        bool success = false;
        pathCompareInternal(a, b, precision, success);
        if (!success) {
            #define DIAG(x) "  " << #x << ":\n    " << x << std::endl
            std::cout << "More information about the failure:\n" << DIAG(svg) << DIAG(id) << DIAG(a) << DIAG(b);
            failed.emplace_back(id);
        }
    }

    void compareSinglePath(Geom::PathVector apv, Geom::PathVector bpv, double precision, bool &success)
    {
        size_t totala = apv.curveCount();
        size_t totalb = bpv.curveCount();

        // Depending on rounding errors the closing segment might be there, or it might be skipped resulting in
        // segment count difference.
        size_t common_count = std::min(totala, totalb);
        ASSERT_LE(totala, common_count + 1);
        ASSERT_LE(totalb, common_count + 1);
        ASSERT_EQ(1, apv.size());
        ASSERT_EQ(1, bpv.size());
        if (!apv[0].closed() || !bpv[0].closed()) {
            ASSERT_EQ(totala, totalb); // no closing path for open paths, need to match exactly
        }

        // find initial
        size_t initial = 0;
        for (size_t i = 0; i < common_count; i++) {
            Geom::Point pointa = apv.pointAt(0.0);
            Geom::Point pointb = bpv.pointAt(i);
            if (Geom::are_near(pointa, pointb, precision)) {
                initial = i;
                break;
            }
        }

        if (initial != 0 && initial == common_count) {
            std::cout << "[ WARN     ] Curve reversed. We do not block here. We reverse the path and test node positions on reverse" << std::endl;
            bpv.reverse();
        } else if (initial != 0) {
            std::cout << "[ WARN     ] Different starting node. We do not block here. We gap the origin to " << initial << " de " << totala << " and test with the pathvector reindexed" << std::endl;
        }

        for (size_t i = 0; i < common_count; i++) {
            if (initial >= common_count) {
                initial = 0;
            }
            Geom::Point pointa = apv.pointAt(i + 0.2);
            Geom::Point pointb = bpv.pointAt(initial + 0.2);
            Geom::Point pointc = apv.pointAt(i + 0.4);
            Geom::Point pointd = bpv.pointAt(initial + 0.4);
            Geom::Point pointe = apv.pointAt(i);
            Geom::Point pointf = bpv.pointAt(initial);
            ASSERT_NEAR(pointa[Geom::X], pointb[Geom::X], precision);
            ASSERT_NEAR(pointa[Geom::Y], pointb[Geom::Y], precision);
            ASSERT_NEAR(pointc[Geom::X], pointd[Geom::X], precision);
            ASSERT_NEAR(pointc[Geom::Y], pointd[Geom::Y], precision);
            ASSERT_NEAR(pointe[Geom::X], pointf[Geom::X], precision);
            ASSERT_NEAR(pointe[Geom::Y], pointf[Geom::Y], precision);
            initial++;
        }

        // If there is an extra closing segment make sure it is approximately 0.
        if (totala > common_count) {
            auto pa = apv.pointAt(totala);
            auto pb = apv.pointAt(totala - 1);
            ASSERT_NEAR(pa[Geom::X], pb[Geom::X], precision);
            ASSERT_NEAR(pa[Geom::Y], pb[Geom::Y], precision);
        }
        if (totalb > common_count) {
            auto pa = bpv.pointAt(totalb);
            auto pb = bpv.pointAt(totalb - 1);
            ASSERT_NEAR(pa[Geom::X], pb[Geom::X], precision);
            ASSERT_NEAR(pa[Geom::Y], pb[Geom::Y], precision);
        }
        success = true;
    }

    /*
     * Outputting success through reference variable instead return value due to implementation details of
     * GTEST framework. Nested function calls containing asserts need to return void.
     * See https://google.github.io/googletest/advanced.html#assertion-placement for more details.
     * Callers can't use HasFatalFailure() either because LPE tests are trying to gather all mismatched
     * object ids.
     */
    void pathCompareInternal(char const *a, char const *b, double precision, bool &success)
    {
        if (!b) { // a is never nullptr
            FAIL() << "Path not set";
        }
        bool a_empty = a[0] == '\0';
        bool b_empty = b[0] == '\0';
        if (a_empty || b_empty) {
            if (a_empty && b_empty) {
                success = true;
                return;
            } else {
                FAIL() << "Mismatching emptiness of paths";
            }
        }

        Geom::PathVector apv = sp_svg_read_pathv(a);
        Geom::PathVector bpv = sp_svg_read_pathv(b);

        if (apv.empty()) {
            FAIL() << "Couldn't parse original 'd'";
        }
        if (bpv.empty()) {
            FAIL() << "Couldn't parse 'd'";
        }

        ASSERT_EQ(apv.size(), bpv.size());
        for (size_t i = 0; i < apv.size(); i++) {
            bool match = false;
            compareSinglePath(apv[i], bpv[i], precision, match);
            ASSERT_TRUE(match);
        }

        // we do whoole function without a fail so inform tall be sacessfull
        success = true;
    }

    void TearDown() override
    { 
        Glib::ustring ids;
        for (auto const &fail : failed) {
            if (!ids.empty()) {
                ids += ",";
            }
            ids += fail;
        }
        if (!ids.empty()) {
            FAIL() << "[FAILED IDS] " << ids; 
        }
    }
    
    double getPrecision(SPObject *root, SPObject *current) {
        if (auto threshold = root->getAttribute("inkscape:test-threshold")) {
            return Util::read_number(threshold);
        }
        if (auto threshold = current->getAttribute("inkscape:test-threshold")) {
            return Util::read_number(threshold);
        }
        return 0.001;
    }
    std::vector<Glib::ustring> failed;
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

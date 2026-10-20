# SPDX-License-Identifier: GPL-2.0-or-later

function(add_rendering_test rendering_test)
    cmake_parse_arguments(ARG "" "FUZZ;DISABLE" "" ${ARGN})
    set(testname "render_${rendering_test}")
    add_test(NAME ${testname}
             COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/inkscape ${CMAKE_CURRENT_SOURCE_DIR}/${rendering_test} ${ARG_FUZZ}
             WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/testfiles/rendering_tests)
    set_tests_properties(${testname} PROPERTIES ENVIRONMENT "${INKSCAPE_TEST_PROFILE_DIR_ENV}/${testname};${CMAKE_CTEST_ENV}")
    if(${ARG_DISABLE})
        set_tests_properties(${testname} PROPERTIES DISABLED "True")
    endif()
endfunction()

find_package(PkgConfig REQUIRED)
pkg_search_module(HARFBUZZ REQUIRED harfbuzz)

# Add your test here (do not put the .svg extension)

# -- Generic tests --
add_rendering_test(test-empty)
add_rendering_test(test-dont-crash)
add_rendering_test(test-use FUZZ 0.03)
add_rendering_test(solid-color-test)

# -- Selector tests --
add_rendering_test(selector-important-002)
add_rendering_test(selector-important-003)

add_rendering_test(multi-style)
add_rendering_test(style-parsing)
add_rendering_test(test-peppercarrot-text FUZZ 0.002)

# -- Text tests --
## Many (if not all) of these tests are sensitive to the text rendering stack: FreeType, HarfBuzz, Pango.
add_rendering_test(text-flow FUZZ 0.5)

# add_rendering_test(test-baseline-shift)
## Small differences with code adapted for Pango 1.44.

# broken by a harfbuzz update. To be decided if a harfbuzz bug, a pango bug, or an Inkscape bug
add_rendering_test(test-glyph-y-pos FUZZ 0.1 DISABLE TRUE)

## to be fixed since an update happened between harfbuzz 1.5.1(OK) and 1.6.0(FAIL).
## If you re-enable the test, you may have to *slightly* fix the expected rendering (hoping the fix happens upstream).
## Please also check that the rendering with harfbuzz <=1.5.1 is not *too* wrong (for older systems)
## cf Tav's post : https://www.patreon.com/posts/into-sinkhole-19021727
## and bug https://bugzilla.gnome.org/show_bug.cgi?id=787526

# add_rendering_test(text-shaping)
## Expected rendering generated with Pango 1.44. Currently fails with
## CI as CI uses Pango 1.40. Enable after updating CI to Ubuntu 20.04.

# broken by a harfbuzz update. To be decided if a harfbuzz bug, a pango bug, or an Inkscape bug
add_rendering_test(text-glyphs-combining FUZZ 0.1 DISABLE TRUE)
## Expected rendering generated with Pango 1.44.

# broken by a harfbuzz update. To be decided if a harfbuzz bug, a pango bug, or an Inkscape bug
add_rendering_test(text-glyphs-vertical FUZZ 0.1 DISABLE TRUE)
## Expected rendering generated with Pango 1.44.

# -- LPE tests --
add_rendering_test(test-powerstroke-join FUZZ 0.1)

# <symbol> geometric properties (SVG 2.0 feature)
add_rendering_test(symbol-svg2-geometry-properties)
add_rendering_test(symbol-svg2-ref)

# Tests to run for 64-bit builds only. These fail in 32-bit builds (possibly due to rounding issues)
# TODO: Figure out actual cause and see if we can fix it
if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
    # broken by a harfbuzz update. To be decided if a harfbuzz bug, a pango bug, or an Inkscape bug
    add_rendering_test(test-rtl-vertical FUZZ 0.1 DISABLE TRUE)

    # .otf font with compressed SVG glyphs
    add_rendering_test(text-gzipped-svg-glyph FUZZ 0.03)
endif()
# custom .otf font with SVG glyphs for testing various edge cases
add_rendering_test(text-svg-glyph-custom FUZZ 0.3)

# Gradient that reference other gradients.
add_rendering_test(test-gradient-reference FUZZ 0.1)

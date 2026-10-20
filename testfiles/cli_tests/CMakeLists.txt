# SPDX-License-Identifier: GPL-2.0-or-later


# Helper function to add a CLI test
#
# Run an Inkscape command line and check for pass/fail condition (by default only exit status is checked)
#
# Command line options:
#   INPUT_FILENAME     - name of input file (optional)
#   OUTPUT_FILENAME    - name of output file (optional)
#   OUTPUT_PAGE        - index of page in multipage output (optional), starts from 0
#   FUZZ_PERCENTAGE    - maximum allowed normalized root-mean-squared distance between compared images
#   RASTER_DPI         - DPI setting for rasterizing vector formats before root-mean-squared comparison
#   SUPERSAMPLING      - Rasterise vector formats at larger size and scale down by this factor
#   PARAMETERS         - additional command line parameters to pass to Inkscape
#   REFERENCE_INKSCAPE - rasterize REFERENCE_FILENAME using inkscape
#                        Avoid this when possible! Preferably reference should be a raster image and when that's not
#                        practical rasterized using third party rasterizer instead of inkscape.
#
# Pass/fail criteria:
#   PASS_FOR_OUTPUT    - pass if output matches the given value, otherwise fail
#                        see https://cmake.org/cmake/help/latest/prop_test/PASS_REGULAR_EXPRESSION.html for details
#   FAIL_FOR_OUTPUT    - fail if output matches the given value
#                        see https://cmake.org/cmake/help/latest/prop_test/FAIL_REGULAR_EXPRESSION.html for details
#   REFERENCE_FILENAME - compare OUTPUT_FILENAME with this pre-rendered reference file
#                        both files are converted to PNG and compared with ImageMagick's 'compare'
#                        for multipage output, use OUTPUT_PAGE to specify a single page for comparison
#   EXPECTED_FILES     - verify the command produced the expected files (i.e. they exist on disk)
#   TEST_SCRIPT        - additional script to run after performing all checks and before cleaning up
#
# Other options:
#   ENVIRONMENT        - Additional environment variables to set while running the test
function(add_cli_test name)
    # Pack arguments so we can call multiple inner functions
    cmake_parse_arguments(PARSE_ARGV 0 FWD "" "" "")
    set(quotedArgs "")
    foreach(arg IN LISTS FWD_UNPARSED_ARGUMENTS)
        string(APPEND quotedArgs " [===[${arg}]===]")
    endforeach()

    set(testname cli_${name})
    cmake_language(EVAL CODE "
        add_cli_test_run(${testname} ${testname}_build ${quotedArgs})
        add_cli_test_cmp(${testname} ${testname}_check ${quotedArgs})
        add_cli_test_cln(${testname} ${testname}_clean ${quotedArgs})
    ")
endfunction()


#
# Just the run component of the above add_cli_test
#
function(add_cli_test_run fixture testname)
    # parse arguments
    set(options REFERENCE_INKSCAPE)
    set(oneValueArgs INPUT_FILENAME OUTPUT_FILENAME PASS_FOR_OUTPUT FAIL_FOR_OUTPUT REFERENCE_FILENAME)
    set(multiValueArgs PARAMETERS ENVIRONMENT EXPECTED_FILES TEST_SCRIPT)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    set(CMAKE_CTEST_ENV "INKSCAPE_FONTCONFIG=${CMAKE_CURRENT_SOURCE_DIR}/../rendering_tests/fonts/isolated.conf;${CMAKE_CTEST_ENV}")

    #disable dithering
    set(ARG_PARAMETERS ${ARG_PARAMETERS}  "--export-png-use-dithering=false")

    if(DEFINED ARG_OUTPUT_FILENAME)
        # for actions: if an export-do is present do not append export-filename option to prevent
        if(NOT ARG_PARAMETERS MATCHES "export-do")
            set(ARG_PARAMETERS ${ARG_PARAMETERS} "--export-filename=${ARG_OUTPUT_FILENAME}" "--export-make-paths")
        endif()
    endif()
    if(DEFINED ARG_INPUT_FILENAME)
        set(ARG_INPUT_FILENAME "${CMAKE_CURRENT_SOURCE_DIR}/testcases/${ARG_INPUT_FILENAME}")
        set(ARG_PARAMETERS ${ARG_PARAMETERS} ${ARG_INPUT_FILENAME})
    endif()

    set(CMAKE_CTEST_ENV "${INKSCAPE_TEST_PROFILE_DIR_ENV}/${testname};${CMAKE_CTEST_ENV}")
    if(DEFINED ARG_ENVIRONMENT)
        if(ARG_ENVIRONMENT STREQUAL "unset")
            unset(CMAKE_CTEST_ENV)
        else()
            # variables might already be set, however the last value wins
            list(APPEND CMAKE_CTEST_ENV ${ARG_ENVIRONMENT})
        endif()
    endif()

    # add test for main command line
    add_test(NAME ${testname} COMMAND inkscape ${ARG_PARAMETERS})
    set_tests_properties(${testname} PROPERTIES
        FIXTURES_SETUP ${fixture}
        ENVIRONMENT "${CMAKE_CTEST_ENV}")
    if(DEFINED ARG_PASS_FOR_OUTPUT)
        set_tests_properties(${testname} PROPERTIES PASS_REGULAR_EXPRESSION ${ARG_PASS_FOR_OUTPUT})
    endif()
    if(DEFINED ARG_FAIL_FOR_OUTPUT)
        set_tests_properties(${testname} PROPERTIES FAIL_REGULAR_EXPRESSION ${ARG_FAIL_FOR_OUTPUT})
    endif()

    if (${ARG_REFERENCE_INKSCAPE} AND DEFINED ARG_REFERENCE_FILENAME)
        file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/testcases/${ARG_REFERENCE_FILENAME}"  REFERENCE_FILENAME)
        set(ARG_PARAMETERS ${REFERENCE_FILENAME}  "--export-png-use-dithering=false"
                "--export-filename=${ARG_REFERENCE_FILENAME}.reference.png" "--export-make-paths")
        add_test(NAME ${testname}_ref COMMAND inkscape ${ARG_PARAMETERS})
        set_tests_properties(${testname}_ref PROPERTIES
                FIXTURES_SETUP ${fixture}
                ENVIRONMENT "${CMAKE_CTEST_ENV}")
    endif()
endfunction()

#
# Just the comparison component of the above add_cli_test
#
function(add_cli_test_cmp fixture testname)
    # parse arguments
    set(options REFERENCE_INKSCAPE)
    set(oneValueArgs OUTPUT_FILENAME OUTPUT_PAGE REFERENCE_FILENAME
        FUZZ_PERCENTAGE RASTER_DPI SUPERSAMPLING PNG_FILENAME)
    set(multiValueArgs EXPECTED_FILES TEST_SCRIPT)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (DEFINED ARG_PNG_FILENAME)
        set(ARG_PNG "${ARG_PNG_FILENAME}")
    elseif (DEFINED ARG_OUTPUT_FILENAME)
        set(ARG_PNG "${ARG_OUTPUT_FILENAME}-output.png")
    endif()

    if(DEFINED ARG_FUZZ_PERCENTAGE)
        set(ARG_FUZZ "${ARG_FUZZ_PERCENTAGE}")
    else()
        set(ARG_FUZZ "0")
    endif()

    # add test to check output files
    if(DEFINED ARG_REFERENCE_FILENAME OR DEFINED ARG_EXPECTED_FILES OR DEFINED ARG_TEST_SCRIPT)
        if(DEFINED ARG_REFERENCE_FILENAME)
            if (${ARG_REFERENCE_INKSCAPE})
                set(ARG_REFERENCE_FILENAME "${ARG_REFERENCE_FILENAME}.reference.png")
            else()
                file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/testcases/${ARG_REFERENCE_FILENAME}" ARG_REFERENCE_FILENAME)
            endif()
        endif()
        if(DEFINED ARG_EXPECTED_FILES)
            string(REPLACE ";" " " ARG_EXPECTED_FILES "${ARG_EXPECTED_FILES}")
        endif()
        if(DEFINED ARG_TEST_SCRIPT)
            set(ARG_TEST_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/${ARG_TEST_SCRIPT}")
        endif()

        add_test(NAME ${testname}
            COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/check_output.sh
                    "${ARG_OUTPUT_FILENAME}" "${ARG_OUTPUT_PAGE}" "${ARG_REFERENCE_FILENAME}"
                    "${ARG_EXPECTED_FILES}" "${ARG_TEST_SCRIPT}" "${ARG_PNG}"
                    "${ARG_FUZZ}" "${ARG_RASTER_DPI}" "${ARG_SUPERSAMPLING}")
        set_tests_properties(${testname} PROPERTIES
            FIXTURES_REQUIRED ${fixture}
            ENVIRONMENT "${CMAKE_CTEST_ENV}"
            SKIP_RETURN_CODE 42
        )
    endif()

endfunction()

#
# Delete all the generated files
#
function(add_cli_test_cln fixture testname)
    set(options REFERENCE_INKSCAPE)
    set(oneValueArgs INPUT_FILENAME OUTPUT_FILENAME PASS_FOR_OUTPUT FAIL_FOR_OUTPUT REFERENCE_FILENAME)
    set(multiValueArgs PARAMETERS ENVIRONMENT EXPECTED_FILES TEST_SCRIPT)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Cleanup as a test fixture
    if(DEFINED ARG_OUTPUT_FILENAME)
        list(APPEND CREATED_FILES "${ARG_OUTPUT_FILENAME}")
    endif()
    if(DEFINED ARG_EXPECTED_FILES)
        list(APPEND CREATED_FILES ${ARG_EXPECTED_FILES})
    endif()
    if (${ARG_REFERENCE_INKSCAPE} AND DEFINED ARG_REFERENCE_FILENAME)
        list(APPEND CREATED_FILES ${ARG_REFERENCE_FILENAME}.reference.png)
    endif()
    if(DEFINED CREATED_FILES)
        add_test("${testname}" rm -f ${CREATED_FILES})
        set_tests_properties(${testname} PROPERTIES
            FIXTURES_CLEANUP ${fixture})
    endif()
endfunction(add_cli_test_cln)

##### Tests follow below #####



#############################################################################################
### Command line options (basic tests for all program options as listed in --help output) ###
#############################################################################################

# --help

# --version (check if we can run inkscape and the revision is known)
add_cli_test(version       PARAMETERS --version)
add_cli_test(version_known PARAMETERS --version FAIL_FOR_OUTPUT "\\(unknown\\)")

# --system-data-directory / --user-data-directory (unset environment variables to override our override)
# TODO: Can we make these tests more specific without making too many assumptions?
add_cli_test(system-data-directory PARAMETERS --system-data-directory ENVIRONMENT unset PASS_FOR_OUTPUT "inkscape\n$")
add_cli_test(user-data-directory   PARAMETERS --user-data-directory   ENVIRONMENT unset PASS_FOR_OUTPUT "inkscape\n$")

# --pipe

# --pdf-page=PAGE

# --pdf-poppler
if(WITH_POPPLER)
    add_cli_test(pdf-poppler-mesh-import
                            PARAMETERS --pdf-poppler
                            INPUT_FILENAME pdf-mesh.pdf
                            OUTPUT_FILENAME pdf-mesh_poppler.svg
                            TEST_SCRIPT match_regex.sh pdf-mesh_poppler.svg "<image")
    add_cli_test(pdf-internal-mesh-import
                            INPUT_FILENAME pdf-mesh.pdf
                            OUTPUT_FILENAME pdf-mesh_internal.svg
                            TEST_SCRIPT match_regex_fail.sh pdf-mesh_internal.svg "<image")
endif()

# --convert-dpi-method=METHOD

# --no-convert-text-baseline-spacing

# --export-filename=FILENAME
## check for https://gitlab.com/inkscape/inkscape/-/issues/1712
install(DIRECTORY DESTINATION ${test.dir})
add_cli_test(export-type-dot-separated PARAMETERS --export-type=svg,png
                         INPUT_FILENAME empty.svg OUTPUT_FILENAME test.dir/empty
                         EXPECTED_FILES test.dir/empty.svg test.dir/empty.png)

# --export-overwrite

# --export-type=TYPE[,TYPE]*

##  test whether we can export all default types in principle (and in one command)
add_cli_test(export-type PARAMETERS --export-type=svg,png,ps,eps,pdf,emf,wmf
                         INPUT_FILENAME empty.svg OUTPUT_FILENAME empty
                         EXPECTED_FILES empty.svg empty.png empty.ps empty.eps empty.pdf empty.emf empty.wmf)
##  test whether this also works if the export types are not lowercase
add_cli_test(export-type-caseinsensitive PARAMETERS --export-type=sVg,Png,PS,eps,Pdf,emf,wmf
                         INPUT_FILENAME empty.svg OUTPUT_FILENAME empty
                         EXPECTED_FILES empty.svg empty.png empty.ps empty.eps empty.pdf empty.emf empty.wmf)

## test whether we produce sane output for the default types
add_cli_test(export-type_svg  PARAMETERS --export-type=svg INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.svg REFERENCE_FILENAME shapes.svg)
add_cli_test(export-type_png  PARAMETERS --export-type=png INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.png REFERENCE_FILENAME shapes_expected.png)
add_cli_test(export-type_ps   PARAMETERS --export-type=ps  INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.ps  REFERENCE_FILENAME shapes_expected.ps)
add_cli_test(export-type_eps  PARAMETERS --export-type=eps INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.eps REFERENCE_FILENAME shapes_expected.eps)
if(WITH_CAPYPDF)
add_cli_test(export-type_pdf  PARAMETERS --export-type=pdf INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.pdf REFERENCE_FILENAME shapes_expected.pdf FUZZ_PERCENTAGE 1)
endif()
add_cli_test(export-type_emf  PARAMETERS --export-type=emf INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.emf REFERENCE_FILENAME shapes_expected.emf)
add_cli_test(export-type_wmf  PARAMETERS --export-type=wmf INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.wmf REFERENCE_FILENAME shapes_expected.wmf FUZZ_PERCENTAGE 0.5)
# XAML is not supported by ImageMagick's convert, so simply compare binary
add_cli_test(export-type_xaml PARAMETERS --export-type=xaml INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.xaml
                              TEST_SCRIPT compare.sh shapes.xaml "${CMAKE_CURRENT_SOURCE_DIR}/testcases/shapes_expected.xaml")

# --export-area-page
add_cli_test(export-area-page_png PARAMETERS --export-area-page --export-type=png INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-page.png REFERENCE_FILENAME export-area-page_expected.png)
add_cli_test(export-area-page_svg PARAMETERS --export-area-page --export-type=svg INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-page.svg REFERENCE_FILENAME export-area-page_expected.svg)
if(WITH_CAPYPDF)
add_cli_test(export-area-page_pdf PARAMETERS --export-area-page --export-type=pdf INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-page.pdf REFERENCE_FILENAME export-area-page_expected.pdf FUZZ_PERCENTAGE 0.5)
endif()
add_cli_test(export-area-page_ps  PARAMETERS --export-area-page --export-type=ps  INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-page.ps  REFERENCE_FILENAME export-area-page_expected.ps)
# EPS: Currently not supported. Feature request: https://gitlab.com/inkscape/inkscape/-/issues/1074
# add_cli_test(export-area-page_eps PARAMETERS --export-area-page --export-type=eps INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-page.eps REFERENCE_FILENAME export-area-page_expected.eps)
add_cli_test(export-area-page_emf PARAMETERS --export-area-page --export-type=emf INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-page.emf REFERENCE_FILENAME export-area-page_expected.emf)
add_cli_test(export-area-page_wmf PARAMETERS --export-area-page --export-type=wmf INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-page.wmf REFERENCE_FILENAME export-area-page_expected.wmf FUZZ_PERCENTAGE 0.5)

# --export-area-page + --export-id  (+ --export-id-only)
add_cli_test(export-area-page_export-id_png PARAMETERS --export-area-page --export-id=MyStar --export-id-only INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-page_export-id.png REFERENCE_FILENAME export-area-page_export-id.png)
add_cli_test(export-area-page_export-id_svg PARAMETERS --export-area-page --export-id=MyStar --export-id-only INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-page_export-id.svg REFERENCE_FILENAME export-area-page_export-id.svg)
if(WITH_CAPYPDF)
add_cli_test(export-area-page_export-id_pdf PARAMETERS --export-area-page --export-id=MyStar --export-id-only INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-page_export-id.pdf REFERENCE_FILENAME export-area-page_export-id.pdf)
endif()
add_cli_test(export-area-page_export-id_ps  PARAMETERS --export-area-page --export-id=MyStar --export-id-only INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-page_export-id.ps  REFERENCE_FILENAME export-area-page_export-id.ps)
# EPS: Currently not supported. Feature request: https://gitlab.com/inkscape/inkscape/-/issues/1074
#add_cli_test(export-area-page_export-id_eps PARAMETERS --export-area-page --export-id=MyStar INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-page_export-id.eps REFERENCE_FILENAME export-area-page_export-id.eps)
# EMF, WMF: Nont supported.

# --export-area-drawing
add_cli_test(export-area-drawing_png PARAMETERS --export-area-drawing --export-type=png INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-drawing.png REFERENCE_FILENAME export-area-drawing_expected.png)
add_cli_test(export-area-drawing_svg PARAMETERS --export-area-drawing --export-type=svg INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-drawing.svg REFERENCE_FILENAME export-area-drawing_expected.svg)
if(WITH_CAPYPDF)
add_cli_test(export-area-drawing_pdf PARAMETERS --export-area-drawing --export-type=pdf INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-drawing.pdf REFERENCE_FILENAME export-area-drawing_expected.pdf FUZZ_PERCENTAGE 0.5)
endif()
add_cli_test(export-area-drawing_ps  PARAMETERS --export-area-drawing --export-type=ps  INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-drawing.ps  REFERENCE_FILENAME export-area-drawing_expected.ps)
add_cli_test(export-area-drawing_eps PARAMETERS --export-area-drawing --export-type=eps INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-drawing.eps REFERENCE_FILENAME export-area-drawing_expected.eps)
# EMF, WMF: Currently not supported. Feature request: https://gitlab.com/inkscape/inkscape/-/issues/1056
# add_cli_test(export-area-drawing_emf PARAMETERS --export-area-drawing --export-type=emf INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-drawing.emf REFERENCE_FILENAME export-area-drawing_expected.emf)
# add_cli_test(export-area-drawing_wmf PARAMETERS --export-area-drawing --export-type=wmf INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-drawing.wmf REFERENCE_FILENAME export-area-drawing_expected.wmf)

# --export-area-drawing + --export-area-page should be overriden by --export-area-page
add_cli_test(export-area-drawing-page_png PARAMETERS --export-area-drawing --export-area-page --export-type=png INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-drawing-page.png REFERENCE_FILENAME export-area-page_expected.png)
add_cli_test(export-area-drawing-page_svg PARAMETERS --export-area-drawing --export-area-page --export-type=svg INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-drawing-page.svg REFERENCE_FILENAME export-area-page_expected.svg)
if(WITH_CAPYPDF)
add_cli_test(export-area-drawing-page_pdf PARAMETERS --export-area-drawing --export-area-page --export-type=pdf INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-drawing-page.pdf REFERENCE_FILENAME export-area-page_expected.pdf FUZZ_PERCENTAGE 0.5)
endif()
add_cli_test(export-area-drawing-page_ps  PARAMETERS --export-area-drawing --export-area-page --export-type=ps  INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-drawing-page.ps  REFERENCE_FILENAME export-area-page_expected.ps)

# --export-area=x0:y0:x1:y1
add_cli_test(export-area_png PARAMETERS --export-area=150:150:350:300 --export-type=png INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area.png REFERENCE_FILENAME export-area_expected.png)
add_cli_test(export-area_svg PARAMETERS --export-area=150:150:350:300 --export-type=svg INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area.svg REFERENCE_FILENAME export-area_expected.svg)
if(WITH_CAPYPDF)
add_cli_test(export-area_pdf PARAMETERS --export-area=150:150:350:300 --export-type=pdf INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area.pdf REFERENCE_FILENAME export-area_expected.pdf)
endif()
add_cli_test(export-area_ps PARAMETERS --export-area=150:150:350:300 --export-type=ps INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area.ps REFERENCE_FILENAME export-area_expected.ps)
add_cli_test(export-area_eps PARAMETERS --export-area=150:150:350:300 --export-type=eps INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area.eps REFERENCE_FILENAME export-area_expected.eps)
# EMF, WMF: Not supported yet

# --export-area-snap
add_cli_test(export-area-snap_export-id           PARAMETERS --export-id=rect_misaligned --export-area-snap --export-type=png INPUT_FILENAME pyramids.svg OUTPUT_FILENAME export-area-snap-id.png      REFERENCE_FILENAME export-area-snap_expected.png)
add_cli_test(export-area-snap_export-area-drawing PARAMETERS --export-area-drawing       --export-area-snap --export-type=png INPUT_FILENAME pyramids.svg OUTPUT_FILENAME export-area-snap-drawing.png REFERENCE_FILENAME export-area-snap_expected.png)
add_cli_test(export-area-snap_svg                 PARAMETERS --export-id=rect_misaligned --export-area-snap --export-type=svg INPUT_FILENAME pyramids.svg OUTPUT_FILENAME export-area-snap-id.svg      REFERENCE_FILENAME export-area-snap_expected.svg)
if(WITH_CAPYPDF)
add_cli_test(export-area-snap_pdf                 PARAMETERS --export-id=rect_misaligned --export-area-snap --export-type=pdf INPUT_FILENAME pyramids.svg OUTPUT_FILENAME export-area-snap-id.pdf      REFERENCE_FILENAME export-area-snap_expected.pdf)
endif()
add_cli_test(export-area-snap_ps                  PARAMETERS --export-id=rect_misaligned --export-area-snap --export-type=ps  INPUT_FILENAME pyramids.svg OUTPUT_FILENAME export-area-snap-id.ps       REFERENCE_FILENAME export-area-snap_expected.ps)
add_cli_test(export-area-snap_eps                 PARAMETERS --export-id=rect_misaligned --export-area-snap --export-type=eps INPUT_FILENAME pyramids.svg OUTPUT_FILENAME export-area-snap-id.eps      REFERENCE_FILENAME export-area-snap_expected.eps)
# EMF, WMF: Not supported yet

# --export-dpi=DPI
add_cli_test(export-dpi_png PARAMETERS --export-dpi=12 --export-type=png INPUT_FILENAME filter.svg OUTPUT_FILENAME export-dpi.png REFERENCE_FILENAME export-dpi_expected.png)
# SVG: doesn't make sense
if(WITH_CAPYPDF)
add_cli_test(export-dpi_pdf PARAMETERS --export-dpi=12 --export-type=pdf INPUT_FILENAME filter.svg OUTPUT_FILENAME export-dpi.pdf REFERENCE_FILENAME export-dpi_expected.pdf FUZZ_PERCENTAGE 0.2)
endif()
add_cli_test(export-dpi_ps  PARAMETERS --export-dpi=12 --export-type=ps  INPUT_FILENAME filter.svg OUTPUT_FILENAME export-dpi.ps  REFERENCE_FILENAME export-dpi_expected.ps)
add_cli_test(export-dpi_eps PARAMETERS --export-dpi=12 --export-type=eps INPUT_FILENAME filter.svg OUTPUT_FILENAME export-dpi.eps REFERENCE_FILENAME export-dpi_expected.eps)
# EMF, WMF: doesn't make sense

# --export-width=WIDTH / --export-height=HEIGHT
add_cli_test(export-width                   PARAMETERS --export-width=380                                      --export-type=png INPUT_FILENAME export_hints.svg OUTPUT_FILENAME export-width.png   REFERENCE_FILENAME export-width_expected.png)
add_cli_test(export-width_export-dpi        PARAMETERS --export-width=380 --export-dpi=300                     --export-type=png INPUT_FILENAME export_hints.svg OUTPUT_FILENAME export-width2.png  REFERENCE_FILENAME export-width_expected.png)
add_cli_test(export-width_export-use-hints  PARAMETERS --export-width=380 --export-use-hints --export-id=rect1 --export-type=png INPUT_FILENAME export_hints.svg
                                            TEST_SCRIPT identify.sh "${CMAKE_CURRENT_SOURCE_DIR}/testcases/export_hints_rectangle.png" "PNG 380x285"
                                            EXPECTED_FILES "${CMAKE_CURRENT_SOURCE_DIR}/testcases/export_hints_rectangle.png")
add_cli_test(export-height                  PARAMETERS --export-height=40                                      --export-type=png INPUT_FILENAME export_hints.svg OUTPUT_FILENAME export-height.png  REFERENCE_FILENAME export-height_expected.png)
add_cli_test(export-height_export-dpi       PARAMETERS --export-height=40 --export-dpi=300                     --export-type=png INPUT_FILENAME export_hints.svg OUTPUT_FILENAME export-height2.png REFERENCE_FILENAME export-height_expected.png)
add_cli_test(export-height_export-use-hints PARAMETERS --export-height=40 --export-use-hints --export-id=rect1 --export-type=png INPUT_FILENAME export_hints.svg
                                            TEST_SCRIPT identify.sh ${CMAKE_CURRENT_SOURCE_DIR}/testcases/export_hints_rectangle.png "PNG 53x40"
                                            EXPECTED_FILES "${CMAKE_CURRENT_SOURCE_DIR}/testcases/export_hints_rectangle.png")
# SVG, PDF, PS, EPS, EMF, WMF: doesn't make sense

# --export-margin=MARGIN
# There are some gaps:
# - EPS, WMF: --export-margin is't supported. This affects all EPS, WMF tests below. Feature request: https://gitlab.com/inkscape/inkscape/-/issues/1142
# - --export-margin + --export-area does not clip to just that area. Feature request: https://gitlab.com/inkscape/inkscape/-/work_items/1142
# - --export-margin + --export-use-hints is PNG only feature.
# There is no test for --export-margin + --export-area-page combination because it's equivalent to --export-margin

## simple --export-margin (millimeter based)
add_cli_test(export-margin_mm_png PARAMETERS --export-margin=50 --export-type=png INPUT_FILENAME square_mm.svg OUTPUT_FILENAME export-margin_mm.png REFERENCE_FILENAME export-margin_mm_expected.png )
add_cli_test(export-margin_mm_svg PARAMETERS --export-margin=50 --export-type=svg INPUT_FILENAME square_mm.svg OUTPUT_FILENAME export-margin_mm.svg REFERENCE_FILENAME export-margin_mm_expected.svg )
add_cli_test(export-margin_mm_pdf PARAMETERS --export-margin=50 --export-type=pdf INPUT_FILENAME square_mm.svg OUTPUT_FILENAME export-margin_mm.pdf REFERENCE_FILENAME export-margin_mm_expected.pdf FUZZ_PERCENTAGE 0.01)
add_cli_test(export-margin_mm_ps  PARAMETERS --export-margin=50 --export-type=ps  INPUT_FILENAME square_mm.svg OUTPUT_FILENAME export-margin_mm.ps  REFERENCE_FILENAME export-margin_mm_expected.ps  )
# add_cli_test(export-margin_mm_eps PARAMETERS --export-margin=50 --export-type=eps INPUT_FILENAME square_mm.svg OUTPUT_FILENAME export-margin_mm.eps REFERENCE_FILENAME export-margin_mm_expected.eps )
add_cli_test(export-margin_mm_emf PARAMETERS --export-margin=50 --export-type=emf INPUT_FILENAME square_mm.svg OUTPUT_FILENAME export-margin_mm.emf REFERENCE_FILENAME export-margin_mm_expected.emf )
# add_cli_test(export-margin_mm_wmf PARAMETERS --export-margin=50 --export-type=wmf INPUT_FILENAME square_mm.svg OUTPUT_FILENAME export-margin_mm.wmf REFERENCE_FILENAME export-margin_mm_expected.wmf )

## simple --export-margin (pixel based)
add_cli_test(export-margin_px_png PARAMETERS --export-margin=50 --export-type=png INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_px.png REFERENCE_FILENAME export-margin_px_expected.png )
add_cli_test(export-margin_px_svg PARAMETERS --export-margin=50 --export-type=svg INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_px.svg REFERENCE_FILENAME export-margin_px_expected.svg )
add_cli_test(export-margin_px_pdf PARAMETERS --export-margin=50 --export-type=pdf INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_px.pdf REFERENCE_FILENAME export-margin_px_expected.pdf FUZZ_PERCENTAGE 0.01 )
add_cli_test(export-margin_px_ps  PARAMETERS --export-margin=50 --export-type=ps  INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_px.ps  REFERENCE_FILENAME export-margin_px_expected.ps  )
# add_cli_test(export-margin_px_eps PARAMETERS --export-margin=50 --export-type=eps INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_px.eps REFERENCE_FILENAME export-margin_px_expected.eps )
add_cli_test(export-margin_px_emf PARAMETERS --export-margin=50 --export-type=emf INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_px.emf REFERENCE_FILENAME export-margin_px_expected.emf )
# add_cli_test(export-margin_px_wmf PARAMETERS --export-margin=50 --export-type=wmf INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_px.wmf REFERENCE_FILENAME export-margin_px_expected.wmf )

## --export-margin + --export-id (pixel based)
add_cli_test(export-margin_export-id_png PARAMETERS --export-margin=50 --export-id=square-red --export-type=png INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-id.png REFERENCE_FILENAME export-margin_export-id_expected.png )
add_cli_test(export-margin_export-id_svg PARAMETERS --export-margin=50 --export-id=square-red --export-type=svg INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-id.svg REFERENCE_FILENAME export-margin_export-id_expected.svg )
add_cli_test(export-margin_export-id_pdf PARAMETERS --export-margin=50 --export-id=square-red --export-type=pdf INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-id.pdf REFERENCE_FILENAME export-margin_export-id_expected.pdf FUZZ_PERCENTAGE 0.01 )
add_cli_test(export-margin_export-id_ps  PARAMETERS --export-margin=50 --export-id=square-red --export-type=ps  INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-id.ps  REFERENCE_FILENAME export-margin_export-id_expected.ps  )
# add_cli_test(export-margin_export-id_eps PARAMETERS --export-margin=50 --export-id=square-red --export-type=eps INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-id.eps REFERENCE_FILENAME export-margin_export-id_expected.eps )
add_cli_test(export-margin_export-id_emf PARAMETERS --export-margin=50 --export-id=square-red --export-type=emf INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-id.emf REFERENCE_FILENAME export-margin_export-id_expected.emf )
# add_cli_test(export-margin_export-id_wmf PARAMETERS --export-margin=50 --export-id=square-red --export-type=wmf INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-id.wmf REFERENCE_FILENAME export-margin_export-id_expected.wmf )

## --export-margin + --export-id + export-id-only (pixel based)
add_cli_test(export-margin_export-id_export-id-only_png PARAMETERS --export-margin=50 --export-id=square-red --export-id-only --export-type=png INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-id_export-id-only.png REFERENCE_FILENAME export-margin_export-id_export-id-only_expected.png )
add_cli_test(export-margin_export-id_export-id-only_svg PARAMETERS --export-margin=50 --export-id=square-red --export-id-only --export-type=svg INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-id_export-id-only.svg REFERENCE_FILENAME export-margin_export-id_export-id-only_expected.svg )
add_cli_test(export-margin_export-id_export-id-only_pdf PARAMETERS --export-margin=50 --export-id=square-red --export-id-only --export-type=pdf INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-id_export-id-only.pdf REFERENCE_FILENAME export-margin_export-id_export-id-only_expected.pdf FUZZ_PERCENTAGE 0.01 )
add_cli_test(export-margin_export-id_export-id-only_ps  PARAMETERS --export-margin=50 --export-id=square-red --export-id-only --export-type=ps  INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-id_export-id-only.ps  REFERENCE_FILENAME export-margin_export-id_export-id-only_expected.ps  )
# add_cli_test(export-margin_export-id_export-id-only_eps PARAMETERS --export-margin=50 --export-id=square-red --export-id-only --export-type=eps INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-id_export-id-only.eps REFERENCE_FILENAME export-margin_export-id_export-id-only_expected.eps )
add_cli_test(export-margin_export-id_export-id-only_emf PARAMETERS --export-margin=50 --export-id=square-red --export-id-only --export-type=emf INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-id_export-id-only.emf REFERENCE_FILENAME export-margin_export-id_export-id-only_expected.emf )
# add_cli_test(export-margin_export-id_export-id-only_wmf PARAMETERS --export-margin=50 --export-id=square-red --export-id-only --export-type=wmf INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-id_export-id-only.wmf REFERENCE_FILENAME export-margin_export-id_export-id-only_expected.wmf )

## --export-margin + --export-area-drawing (pixel based)
add_cli_test(export-margin_export-area-drawing_png PARAMETERS --export-margin=50 --export-area-drawing --export-type=png INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_drawing.png REFERENCE_FILENAME export-margin_drawing_expected.png )
add_cli_test(export-margin_export-area-drawing_svg PARAMETERS --export-margin=50 --export-area-drawing --export-type=svg INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_drawing.svg REFERENCE_FILENAME export-margin_drawing_expected.svg )
add_cli_test(export-margin_export-area-drawing_pdf PARAMETERS --export-margin=50 --export-area-drawing --export-type=pdf INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_drawing.pdf REFERENCE_FILENAME export-margin_drawing_expected.pdf FUZZ_PERCENTAGE 0.01 )
add_cli_test(export-margin_export-area-drawing_ps  PARAMETERS --export-margin=50 --export-area-drawing --export-type=ps  INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_drawing.ps  REFERENCE_FILENAME export-margin_drawing_expected.ps  )
# add_cli_test(export-margin_export-area-drawing_eps PARAMETERS --export-margin=50 --export-area-drawing --export-type=eps INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_drawing.eps REFERENCE_FILENAME export-margin_drawing_expected.eps )
add_cli_test(export-margin_export-area-drawing_emf PARAMETERS --export-margin=50 --export-area-drawing --export-type=emf INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_drawing.emf REFERENCE_FILENAME export-margin_drawing_expected.emf )
# add_cli_test(export-margin_export-area-drawing_wmf PARAMETERS --export-margin=50 --export-area-drawing --export-type=wmf INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_drawing.wmf REFERENCE_FILENAME export-margin_drawing_expected.wmf )

## --export-margin + --export-area=x0:y0:x1:y1 (pixel based)
# add_cli_test(export-margin_export-area_png PARAMETERS --export-margin=50 --export-area=50:50:100:100 --export-type=png INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-area.png REFERENCE_FILENAME export-margin_export-area_expected.png )

## --export-margin + --export-use-hints (pixel based)
add_cli_test(export-margin_export-use-hints_export-id PARAMETERS --export-margin=50 --export-use-hints --export-id=rect1 INPUT_FILENAME export_hints.svg
                                                      TEST_SCRIPT identify.sh ${CMAKE_CURRENT_SOURCE_DIR}/testcases/export_hints_rectangle.png "PNG 231x205"
                                                      EXPECTED_FILES "${CMAKE_CURRENT_SOURCE_DIR}/testcases/export_hints_rectangle.png")

## --export-margin (millimeter based, user units rescaled via viewBox)
add_cli_test(export-margin_viewbox_png PARAMETERS --export-margin=50 --export-type=png INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox.png REFERENCE_FILENAME export-margin_mm_viewbox_page_expected.png )
add_cli_test(export-margin_viewbox_svg PARAMETERS --export-margin=50 --export-type=svg INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox.svg REFERENCE_FILENAME export-margin_mm_viewbox_page_expected.svg )
add_cli_test(export-margin_viewbox_pdf PARAMETERS --export-margin=50 --export-type=pdf INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox.pdf REFERENCE_FILENAME export-margin_mm_viewbox_page_expected.pdf FUZZ_PERCENTAGE 0.01 )
add_cli_test(export-margin_viewbox_ps  PARAMETERS --export-margin=50 --export-type=ps  INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox.ps  REFERENCE_FILENAME export-margin_mm_viewbox_page_expected.ps  )
# add_cli_test(export-margin_viewbox_eps PARAMETERS --export-margin=50 --export-type=eps INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox.eps REFERENCE_FILENAME export-margin_mm_viewbox_page_expected.eps )
add_cli_test(export-margin_viewbox_emf PARAMETERS --export-margin=50 --export-type=emf INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox.emf REFERENCE_FILENAME export-margin_mm_viewbox_page_expected.emf )
# add_cli_test(export-margin_viewbox_wmf PARAMETERS --export-margin=50 --export-type=wmf INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox.wmf REFERENCE_FILENAME export-margin_mm_viewbox_page_expected.wmf )

## --export-margin + --export-area-drawing (millimeter based, user units rescaled via viewBox)
add_cli_test(export-margin_viewbox_drawing_png PARAMETERS --export-margin=50 --export-area-drawing --export-type=png INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox_drawing.png REFERENCE_FILENAME export-margin_mm_viewbox_drawing_expected.png )
add_cli_test(export-margin_viewbox_drawing_svg PARAMETERS --export-margin=50 --export-area-drawing --export-type=svg INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox_drawing.svg REFERENCE_FILENAME export-margin_mm_viewbox_drawing_expected.svg )
add_cli_test(export-margin_viewbox_drawing_pdf PARAMETERS --export-margin=50 --export-area-drawing --export-type=pdf INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox_drawing.pdf REFERENCE_FILENAME export-margin_mm_viewbox_drawing_expected.pdf FUZZ_PERCENTAGE 0.01 )
add_cli_test(export-margin_viewbox_drawing_ps  PARAMETERS --export-margin=50 --export-area-drawing --export-type=ps  INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox_drawing.ps  REFERENCE_FILENAME export-margin_mm_viewbox_drawing_expected.ps  )
# add_cli_test(export-margin_viewbox_drawing_eps PARAMETERS --export-margin=50 --export-area-drawing --export-type=eps INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox_drawing.eps REFERENCE_FILENAME export-margin_mm_viewbox_drawing_expected.eps )
add_cli_test(export-margin_viewbox_drawing_emf PARAMETERS --export-margin=50 --export-area-drawing --export-type=emf INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox_drawing.emf REFERENCE_FILENAME export-margin_mm_viewbox_drawing_expected.emf )
# add_cli_test(export-margin_viewbox_drawing_wmf PARAMETERS --export-margin=50 --export-area-drawing --export-type=wmf INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox_drawing.wmf REFERENCE_FILENAME export-margin_mm_viewbox_drawing_expected.wmf )

## --export-margin + --export-id (millimeter based, user units rescaled via viewBox)
add_cli_test(export-margin_viewbox_id_png PARAMETERS --export-margin=50 --export-id=square-red --export-type=png INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox_id.png REFERENCE_FILENAME export-margin_mm_viewbox_id_expected.png )
add_cli_test(export-margin_viewbox_id_svg PARAMETERS --export-margin=50 --export-id=square-red --export-type=svg INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox_id.svg REFERENCE_FILENAME export-margin_mm_viewbox_id_expected.svg )
add_cli_test(export-margin_viewbox_id_pdf PARAMETERS --export-margin=50 --export-id=square-red --export-type=pdf INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox_id.pdf REFERENCE_FILENAME export-margin_mm_viewbox_id_expected.pdf FUZZ_PERCENTAGE 0.01 )
add_cli_test(export-margin_viewbox_id_ps  PARAMETERS --export-margin=50 --export-id=square-red --export-type=ps  INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox_id.ps  REFERENCE_FILENAME export-margin_mm_viewbox_id_expected.ps  )
# add_cli_test(export-margin_viewbox_id_eps PARAMETERS --export-margin=50 --export-id=square-red --export-type=eps INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox_id.eps REFERENCE_FILENAME export-margin_mm_viewbox_id_expected.eps )
add_cli_test(export-margin_viewbox_id_emf PARAMETERS --export-margin=50 --export-id=square-red --export-type=emf INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox_id.emf REFERENCE_FILENAME export-margin_mm_viewbox_id_expected.emf )
# add_cli_test(export-margin_viewbox_id_wmf PARAMETERS --export-margin=50 --export-id=square-red --export-type=wmf INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox_id.wmf REFERENCE_FILENAME export-margin_mm_viewbox_id_expected.wmf )

# --export-id=OBJECT-ID[;OBJECT-ID]*
add_cli_test(export-id_png PARAMETERS --export-id=red --export-type=png INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id.png REFERENCE_FILENAME export-id_expected.png )
add_cli_test(export-id_svg PARAMETERS --export-id=red --export-type=svg INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id.svg REFERENCE_FILENAME export-id_expected.svg )
if(WITH_CAPYPDF)
add_cli_test(export-id_pdf PARAMETERS --export-id=red --export-type=pdf INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id.pdf REFERENCE_FILENAME export-id_expected.pdf )
endif()
# add_cli_test(export-id_ps  PARAMETERS --export-id=red --export-type=ps  INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id.ps  REFERENCE_FILENAME export-id_expected.ps  )
# add_cli_test(export-id_eps PARAMETERS --export-id=red --export-type=eps INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id.eps REFERENCE_FILENAME export-id_expected.eps )
# EMF, WMF There is no support, feature request: https://gitlab.com/inkscape/inkscape/-/issues/1204
# add_cli_test(export-id_emf PARAMETERS --export-id=red --export-type=emf INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id.emf REFERENCE_FILENAME export-id_expected.emf )
# add_cli_test(export-id_wmf PARAMETERS --export-id=red --export-type=wmf INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id.wmf REFERENCE_FILENAME export-id_expected.wmf )

# --export-id with non-existing ID should skip and output no file
add_cli_test(non-existing-export-id_png PARAMETERS --export-id=not-exist --export-type=png 
             INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id-not-exist.png 
             TESTSCRIPT test_no_output_file.sh export-id-not-exist.png)

## --export-id + --export-area-page = --export-area-page
## --export-id + --export-area-drawing = --export-area-drawing
## --export-id + --export-area-snap → export-area-snap_export-id
## --export-id + --export-margin → export-margin_export-id_png|svg|pdf|ps|eps|emf|wmf
## --export-id + --export-use-hints → export-use-hints_export-id
## --export-id + --export-use-hints + --export-width | --export-height → export-width_export-use-hints

# --export-id-only
## --export-id + --export-id-only
add_cli_test(export-id_export-id-only_png PARAMETERS --export-id=blue --export-id-only --export-type=png INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only.png REFERENCE_FILENAME export-id_export-id-only_expected.png )
add_cli_test(export-id_export-id-only_svg PARAMETERS --export-id=blue --export-id-only --export-type=svg INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only.svg REFERENCE_FILENAME export-id_export-id-only_expected.svg )
add_cli_test(export-id_export-id-only_pdf PARAMETERS --export-id=blue --export-id-only --export-type=pdf INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only.pdf REFERENCE_FILENAME export-id_export-id-only_expected.pdf )
add_cli_test(export-id_export-id-only_ps  PARAMETERS --export-id=blue --export-id-only --export-type=ps  INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only.ps  REFERENCE_FILENAME export-id_export-id-only_expected.ps  )
add_cli_test(export-id_export-id-only_eps PARAMETERS --export-id=blue --export-id-only --export-type=eps INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only.eps REFERENCE_FILENAME export-id_export-id-only_expected.eps )
# EMF, WMF There is no support, feature request: https://gitlab.com/inkscape/inkscape/-/issues/1204
# add_cli_test(export-id_export-id-only_emf PARAMETERS --export-id=blue --export-id-only --export-type=emf INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only.emf REFERENCE_FILENAME export-id_export-id-only_expected.emf )
# add_cli_test(export-id_export-id-only_wmf PARAMETERS --export-id=blue --export-id-only --export-type=wmf INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only.wmf REFERENCE_FILENAME export-id_export-id-only_expected.wmf )

## --export-id + --export-id-only + --export-area-page
add_cli_test(export-id_export-id-only_export-area-page_png PARAMETERS --export-id=yellow --export-id-only --export-area-page --export-type=png INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only_export-area-page.png REFERENCE_FILENAME export-id_export-id-only_export-area-page_expected.png )
add_cli_test(export-id_export-id-only_export-area-page_svg PARAMETERS --export-id=yellow --export-id-only --export-area-page --export-type=svg INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only_export-area-page.svg REFERENCE_FILENAME export-id_export-id-only_export-area-page_expected.svg )
add_cli_test(export-id_export-id-only_export-area-page_pdf PARAMETERS --export-id=yellow --export-id-only --export-area-page --export-type=pdf INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only_export-area-page.pdf REFERENCE_FILENAME export-id_export-id-only_export-area-page_expected.pdf )
add_cli_test(export-id_export-id-only_export-area-page_ps  PARAMETERS --export-id=yellow --export-id-only --export-area-page --export-type=ps  INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only_export-area-page.ps  REFERENCE_FILENAME export-id_export-id-only_export-area-page_expected.ps  )
# EPS: Depents on https://gitlab.com/inkscape/inkscape/-/issues/1074
# add_cli_test(export-id_export-id-only_export-area-page_eps PARAMETERS --export-id=yellow --export-id-only --export-area-page --export-type=eps INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only_export-area-page.eps REFERENCE_FILENAME export-id_export-id-only_export-area-page_expected.eps )
# EMF, WMF: There is no support, feature request: https://gitlab.com/inkscape/inkscape/-/issues/1204
# add_cli_test(export-id_export-id-only_export-area-page_emf PARAMETERS --export-id=yellow --export-id-only --export-area-page --export-type=emf INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only_export-area-page.emf REFERENCE_FILENAME export-id_export-id-only_export-area-page_expected.emf )
# add_cli_test(export-id_export-id-only_export-area-page_wmf PARAMETERS --export-id=yellow --export-id-only --export-area-page --export-type=wmf INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only_export-area-page.wmf REFERENCE_FILENAME export-id_export-id-only_export-area-page_expected.wmf )

## --export-id + --export-id-only + --export-area-drawing
# PNG, PDF, PS, EPS, EMF, WMF: Feature request: https://gitlab.com/inkscape/inkscape/-/issues/1215
# add_cli_test(export-id_export-id-only_export-area-drawing_png PARAMETERS --export-id=yellow --export-id-only --export-area-drawing --export-type=png INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only_export-area-drawing.png REFERENCE_FILENAME export-id_export-id-only_export-area-drawing_expected.png )
add_cli_test(export-id_export-id-only_export-area-drawing_svg PARAMETERS --export-id=yellow --export-id-only --export-area-drawing --export-type=svg INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only_export-area-drawing.svg REFERENCE_FILENAME export-id_export-id-only_export-area-drawing_expected.svg )
# add_cli_test(export-id_export-id-only_export-area-drawing_pdf PARAMETERS --export-id=yellow --export-id-only --export-area-drawing --export-type=pdf INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only_export-area-drawing.pdf REFERENCE_FILENAME export-id_export-id-only_export-area-drawing_expected.pdf )
# add_cli_test(export-id_export-id-only_export-area-drawing_ps  PARAMETERS --export-id=yellow --export-id-only --export-area-drawing --export-type=ps  INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only_export-area-drawing.ps  REFERENCE_FILENAME export-id_export-id-only_export-area-drawing_expected.ps  )
# add_cli_test(export-id_export-id-only_export-area-drawing_eps PARAMETERS --export-id=yellow --export-id-only --export-area-drawing --export-type=eps INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only_export-area-drawing.eps REFERENCE_FILENAME export-id_export-id-only_export-area-drawing_expected.eps )
# add_cli_test(export-id_export-id-only_export-area-drawing_emf PARAMETERS --export-id=yellow --export-id-only --export-area-drawing --export-type=emf INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only_export-area-drawing.emf REFERENCE_FILENAME export-id_export-id-only_export-area-drawing_expected.emf )
# add_cli_test(export-id_export-id-only_export-area-drawing_wmf PARAMETERS --export-id=yellow --export-id-only --export-area-drawing --export-type=wmf INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only_export-area-drawing.wmf REFERENCE_FILENAME export-id_export-id-only_export-area-drawing_expected.wmf )

## --export-id + --export-id-only + --export-margin → export-margin_export-id_export-id-only_png|svg|pdf|ps|eps|emw|wmf

# --export-plain-svg
add_cli_test(export-plain-svg PARAMETERS --export-type=svg --export-plain-svg
                              INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.svg REFERENCE_FILENAME shapes.svg
                              TEST_SCRIPT match_regex_fail.sh "shapes.svg" "inkscape:|sodipodi:")

# --export-ps-level=LEVEL
add_cli_test(export-ps-level_2          PARAMETERS --export-ps-level=2 --export-type=ps INPUT_FILENAME gradient.svg OUTPUT_FILENAME export-ps-level-2.ps REFERENCE_FILENAME export-ps-level-2_expected.ps )
add_cli_test(export-ps-level_2_content  PARAMETERS --export-ps-level=2 --export-type=ps
                                        INPUT_FILENAME gradient.svg OUTPUT_FILENAME export-ps-level-2.ps
                                        TEST_SCRIPT match_regex.sh "export-ps-level-2.ps" "%%LanguageLevel: 2")
add_cli_test(export-ps-level_3          PARAMETERS --export-ps-level=3 --export-type=ps INPUT_FILENAME gradient.svg OUTPUT_FILENAME export-ps-level-3.ps REFERENCE_FILENAME export-ps-level-3_expected.ps )
add_cli_test(export-ps-level_3_content  PARAMETERS --export-ps-level=3 --export-type=ps
                                        INPUT_FILENAME gradient.svg OUTPUT_FILENAME export-ps-level-3.ps
                                        TEST_SCRIPT match_regex.sh "export-ps-level-3.ps" "%%LanguageLevel: 3")
add_cli_test(export-eps-level_2_content PARAMETERS --export-ps-level=2 --export-type=eps
                                        INPUT_FILENAME gradient.svg OUTPUT_FILENAME export-ps-level-2.ps
                                        TEST_SCRIPT match_regex.sh "export-ps-level-2.eps" "%%LanguageLevel: 2")
add_cli_test(export-eps-level_3_content PARAMETERS --export-ps-level=3 --export-type=eps
                                        INPUT_FILENAME gradient.svg OUTPUT_FILENAME export-ps-level-3.ps
                                        TEST_SCRIPT match_regex.sh "export-ps-level-3.eps" "%%LanguageLevel: 3")

# --export-pdf-version=VERSION
add_cli_test(export-pdf-version-14 PARAMETERS --export-pdf-version=1.4 --export-extension=org.inkscape.output.pdf.cairorenderer
                                   INPUT_FILENAME rects.svg OUTPUT_FILENAME export-pdf-version-14.pdf
                                   TEST_SCRIPT match_regex.sh "export-pdf-version-14.pdf" "^%PDF-1.4$")
add_cli_test(export-pdf-version-15 PARAMETERS --export-pdf-version=1.5 --export-extension=org.inkscape.output.pdf.cairorenderer
                                   INPUT_FILENAME rects.svg OUTPUT_FILENAME export-pdf-version-15.pdf
                                   TEST_SCRIPT match_regex.sh "export-pdf-version-15.pdf" "^%PDF-1.5$")

# --export-text-to-path
# PNG: Doesn't make sense.
# EMF/WMF: No way to check.
add_cli_test(preserve-text_ps        PARAMETERS --export-type=ps
                                     INPUT_FILENAME text.svg OUTPUT_FILENAME preserve-text.ps
                                     TEST_SCRIPT match_regex.sh "preserve-text.ps" "^%%BeginResource: font")
add_cli_test(preserve-text_eps       PARAMETERS --export-type=eps
                                     INPUT_FILENAME text.svg OUTPUT_FILENAME preserve-text.eps
                                     TEST_SCRIPT match_regex.sh "preserve-text.eps" "^%%BeginResource: font")
add_cli_test(export-text-to-path_svg PARAMETERS --export-text-to-path --export-type=svg
                                     INPUT_FILENAME text.svg OUTPUT_FILENAME export-text-to-path.svg
                                     TEST_SCRIPT match_regex_fail.sh "export-text-to-path.svg" "<text")
add_cli_test(export-text-to-path_pdf PARAMETERS --export-text-to-path --export-type=pdf
                                     INPUT_FILENAME text.svg OUTPUT_FILENAME export-text-to-path.pdf
                                     TEST_SCRIPT match_regex_fail.sh "export-text-to-path.pdf" "/FontDescriptor")
add_cli_test(export-text-to-path_ps  PARAMETERS --export-text-to-path --export-type=ps
                                     INPUT_FILENAME text.svg OUTPUT_FILENAME export-text-to-path.ps
                                     TEST_SCRIPT match_regex_fail.sh "export-text-to-path.ps" "^%%BeginResource: font")
add_cli_test(export-text-to-path_eps PARAMETERS --export-text-to-path --export-type=eps
                                     INPUT_FILENAME text.svg OUTPUT_FILENAME export-text-to-path.eps
                                     TEST_SCRIPT match_regex_fail.sh "export-text-to-path.eps" "^%%BeginResource: font")

# --export-text-to-path should convert texts in nested groups to paths
add_cli_test(export-text-to-path-in-group_svg PARAMETERS --export-text-to-path --export-type=svg
                                              INPUT_FILENAME text_in_group.svg OUTPUT_FILENAME export-text-to-path-in-group.svg
                                              TEST_SCRIPT match_regex_fail.sh "export-text-to-path-in-group.svg" "<text")

# --export-text-to-path should preserve shapes inside group https://gitlab.com/inkscape/inkscape/-/issues/2649
add_cli_test(export-text-to-path-in-group-rect_svg PARAMETERS --export-text-to-path --export-type=svg
                                                   INPUT_FILENAME shapes.svg OUTPUT_FILENAME export-text-to-path-in-group-rect.svg
                                                   TEST_SCRIPT match_regex.sh "export-text-to-path-in-group-rect.svg" "<rect")
add_cli_test(export-text-to-path-in-group-ellipse_svg PARAMETERS --export-text-to-path --export-type=svg
                                                      INPUT_FILENAME shapes.svg OUTPUT_FILENAME export-text-to-path-in-group-ellipse.svg
                                                      TEST_SCRIPT match_regex.sh "export-text-to-path-in-group-ellipse.svg" "<ellipse")

# Regression test for https://gitlab.com/inkscape/inkscape/-/issues/1364
add_cli_test(regression-1364         PARAMETERS --select=large,small --actions=path-intersection
                                     INPUT_FILENAME regression-1364.svg
                                     OUTPUT_FILENAME regression-1364_output.svg
                                     TEST_SCRIPT testcases/regression-1364_script.py)

# Regression test for https://gitlab.com/inkscape/inkscape/-/issues/2602
add_cli_test(regression-2602         PARAMETERS --export-text-to-path
                                     INPUT_FILENAME regression-2602.svg
                                     OUTPUT_FILENAME regression-2602_output.svg
                                     TEST_SCRIPT testcases/regression-2602_script.py)

# Regression test for https://gitlab.com/inkscape/inkscape/-/issues/2797
add_cli_test(regression-2797         INPUT_FILENAME regression-2797.svg
                                     OUTPUT_FILENAME regression-2797_output.svg
                                     TEST_SCRIPT testcases/regression-2797_script.py)

# --export-latex
# PDF, PS, EPS only function by design.
add_cli_test(export-latex PARAMETERS --export-type=ps,eps --export-latex
                          INPUT_FILENAME text.svg OUTPUT_FILENAME export-latex
                          EXPECTED_FILES export-latex.ps export-latex.ps_tex export-latex.eps export-latex.eps_tex
                          TEST_SCRIPT match_regex.sh "export-latex.eps_tex" "some text")
add_cli_test(export-latex-pdf PARAMETERS --export-latex --export-extension=org.inkscape.output.pdf.cairorenderer
                          INPUT_FILENAME text.svg OUTPUT_FILENAME export-latex
                          EXPECTED_FILES export-latex.pdf export-latex.pdf_tex
                          TEST_SCRIPT match_regex.sh "export-latex.pdf_tex" "some text")

# --export-ignore-filters
## testing exporting with filters
add_cli_test(export-with-filters_svg  PARAMETERS --export-type=svg INPUT_FILENAME offset.svg OUTPUT_FILENAME export-with-filters.svg REFERENCE_FILENAME export-with-filters_expected.svg)
add_cli_test(export-with-filters_png  PARAMETERS --export-type=png INPUT_FILENAME offset.svg OUTPUT_FILENAME export-with-filters.png REFERENCE_FILENAME export-with-filters_expected.png)
add_cli_test(export-with-filters_ps   PARAMETERS --export-type=ps  INPUT_FILENAME offset.svg OUTPUT_FILENAME export-with-filters.ps  REFERENCE_FILENAME export-with-filters_expected.ps )
add_cli_test(export-with-filters_eps  PARAMETERS --export-type=eps INPUT_FILENAME offset.svg OUTPUT_FILENAME export-with-filters.eps REFERENCE_FILENAME export-with-filters_expected.eps)
if(WITH_CAPYPDF)
add_cli_test(export-with-filters_pdf  PARAMETERS --export-type=pdf INPUT_FILENAME offset.svg OUTPUT_FILENAME export-with-filters.pdf REFERENCE_FILENAME export-with-filters_expected.pdf)
endif()
add_cli_test(export-grouped-mp PARAMETERS --export-type=pdf
                               INPUT_FILENAME export-grouped-mp.svg
                               OUTPUT_FILENAME export-grouped-mp.pdf
                               OUTPUT_PAGE 1
                               REFERENCE_FILENAME export-grouped-mp_expected.png
                               FUZZ_PERCENTAGE 3)
add_cli_test(export-with-filters-multipage PARAMETERS --export-type=pdf
                                           INPUT_FILENAME export-with-filters-multipage.svg
                                           OUTPUT_FILENAME export-with-filters-multipage.pdf
                                           OUTPUT_PAGE 1
                                           REFERENCE_FILENAME export-with-filters-multipage_expected.png
                                           FUZZ_PERCENTAGE 1)
add_cli_test(export-filtered-clones-mp PARAMETERS --export-type=pdf --export-dpi=600
                                       INPUT_FILENAME  export-filtered-clones-mp.svg
                                       OUTPUT_FILENAME export-filtered-clones-mp.pdf
                                       OUTPUT_PAGE 1
                                       RASTER_DPI 600
                                       REFERENCE_FILENAME export-filtered-clones-mp_expected.png
                                       FUZZ_PERCENTAGE 3)
add_cli_test(export-clone-group-filtered-mp PARAMETERS --export-type=pdf
                                            INPUT_FILENAME  export-clone-group-filtered-mp.svg
                                            OUTPUT_FILENAME export-clone-group-filtered-mp.pdf
                                            OUTPUT_PAGE 1
                                            REFERENCE_FILENAME export-clone-group-filtered-mp_expected.png
                                            FUZZ_PERCENTAGE 3)
add_cli_test(export-mask-with-filter PARAMETERS
                                     INPUT_FILENAME export-mask-with-filter.svg
                                     OUTPUT_FILENAME export-mask-with-filter.pdf
                                     REFERENCE_FILENAME export-mask-with-filter_expected.pdf
                                     FUZZ_PERCENTAGE 1)
add_cli_test(export-text-paintorder PARAMETERS --export-type=pdf
                                    INPUT_FILENAME  export-text-paintorder.svg
                                    OUTPUT_FILENAME export-text-paintorder.pdf
                                    RASTER_DPI 200
                                    REFERENCE_FILENAME export-text-paintorder_expected.png
                                    FUZZ_PERCENTAGE 4)
# This tests the paint order in the text-to-path conversion
add_cli_test(convert-text-paintorder PARAMETERS --export-text-to-path --export-extension=org.inkscape.output.pdf.cairorenderer
                                     INPUT_FILENAME  export-text-paintorder.svg
                                     OUTPUT_FILENAME export-text-paintorder.pdf
                                     RASTER_DPI 200
                                     REFERENCE_FILENAME export-text-paintorder_expected.png
                                     FUZZ_PERCENTAGE 3.5)

if(WITH_CAPYPDF)
add_cli_test(export-mask-viewbox PARAMETERS --export-type=pdf
                                 INPUT_FILENAME  export-mask-viewbox-scale.svg
                                 OUTPUT_FILENAME export-mask-viewbox-scale.pdf
                                 REFERENCE_FILENAME export-mask-viewbox-scale_expected.png
                                 FUZZ_PERCENTAGE 1)

add_cli_test(export-mask-on-transformed-image PARAMETERS --export-type=pdf
                                              INPUT_FILENAME    export-mask-on-transformed-image.svg
                                              OUTPUT_FILENAME   export-mask-on-transformed-image.pdf
                                              REFERENCE_FILENAME export-mask-on-transformed-image_expected.png
                                              FUZZ_PERCENTAGE 1)
endif()

# EMF, WMF: No support for exporting filters. Feature request: https://gitlab.com/inkscape/inbox/-/issues/2275
# add_cli_test(export-with-filters_emf  PARAMETERS --export-type=emf INPUT_FILENAME offset.svg OUTPUT_FILENAME export-with-filters.emf REFERENCE_FILENAME export-with-filters_expected.emf)
# add_cli_test(export-with-filters_wmf  PARAMETERS --export-type=wmf INPUT_FILENAME offset.svg OUTPUT_FILENAME export-with-filters.wmf REFERENCE_FILENAME export-with-filters_expected.wmf)
## --export-ignore-filters
# SVG, PNG: No support for --export-ignore-filters. Feature request: https://gitlab.com/inkscape/inbox/-/issues/2275
# add_cli_test(export-ignore-filters_svg  PARAMETERS --export-ignore-filters --export-type=svg INPUT_FILENAME offset.svg OUTPUT_FILENAME export-ignore-filters.svg REFERENCE_FILENAME export-ignore-filters_expected.svg)
# add_cli_test(export-ignore-filters_png  PARAMETERS --export-ignore-filters --export-type=png INPUT_FILENAME offset.svg OUTPUT_FILENAME export-ignore-filters.png REFERENCE_FILENAME export-ignore-filters_expected.png)
add_cli_test(export-ignore-filters_ps   PARAMETERS --export-ignore-filters --export-type=ps  INPUT_FILENAME offset.svg OUTPUT_FILENAME export-ignore-filters.ps  REFERENCE_FILENAME export-ignore-filters_expected.ps )
# EPS: Works, but depends on https://gitlab.com/inkscape/inkscape/-/issues/1074
# add_cli_test(export-ignore-filters_eps  PARAMETERS --export-ignore-filters --export-type=eps INPUT_FILENAME offset.svg OUTPUT_FILENAME export-ignore-filters.eps REFERENCE_FILENAME export-ignore-filters_expected.eps)
add_cli_test(export-ignore-filters_pdf  PARAMETERS --export-ignore-filters --export-type=pdf INPUT_FILENAME offset.svg OUTPUT_FILENAME export-ignore-filters.pdf REFERENCE_FILENAME export-ignore-filters_expected.pdf)
add_cli_test(export-ignore-filters_emf  PARAMETERS --export-ignore-filters --export-type=emf INPUT_FILENAME offset.svg OUTPUT_FILENAME export-ignore-filters.emf REFERENCE_FILENAME export-ignore-filters_expected.emf)
add_cli_test(export-ignore-filters_wmf  PARAMETERS --export-ignore-filters --export-type=wmf INPUT_FILENAME offset.svg OUTPUT_FILENAME export-ignore-filters.wmf REFERENCE_FILENAME export-ignore-filters_expected.wmf)

# --export-use-hints
add_cli_test(export-use-hints_export-id           PARAMETERS --export-use-hints --export-id=rect1 INPUT_FILENAME export_hints.svg
                                                  TEST_SCRIPT identify.sh ${CMAKE_CURRENT_SOURCE_DIR}/testcases/export_hints_rectangle.png "PNG 103x77"
                                                  EXPECTED_FILES "${CMAKE_CURRENT_SOURCE_DIR}/testcases/export_hints_rectangle.png")
add_cli_test(export-use-hints_export-area-drawing PARAMETERS --export-use-hints --export-area-drawing INPUT_FILENAME export_hints.svg
                                                  TEST_SCRIPT identify.sh ${CMAKE_CURRENT_SOURCE_DIR}/testcases/export_hints_drawing.png "PNG 197x69"
                                                  EXPECTED_FILES "${CMAKE_CURRENT_SOURCE_DIR}/testcases/export_hints_drawing.png")

# --export-background=COLOR
## check for default background color and opacity
# SVG, PDF, PS, EPS: Format not supoorted. Feature request: https://gitlab.com/inkscape/inbox/issues/2293
# EMF, WMF: Format does not support opacity.
 add_cli_test(export-default-background_png  PARAMETERS --export-type=png INPUT_FILENAME lambda-background.svg OUTPUT_FILENAME export-default-background.png REFERENCE_FILENAME export-default-background_expected.png)
 add_cli_test(export-default-background_svg  PARAMETERS --export-type=svg INPUT_FILENAME lambda-background.svg OUTPUT_FILENAME export-default-background.svg REFERENCE_FILENAME export-default-background_expected.svg)
# add_cli_test(export-default-background_pdf  PARAMETERS --export-type=pdf INPUT_FILENAME lambda-background.svg OUTPUT_FILENAME export-default-background.pdf REFERENCE_FILENAME export-default-background_expected.pdf)
# add_cli_test(export-default-background_ps   PARAMETERS --export-type=ps  INPUT_FILENAME lambda-background.svg OUTPUT_FILENAME export-default-background.ps  REFERENCE_FILENAME export-default-background_expected.ps )
# add_cli_test(export-default-background_eps  PARAMETERS --export-type=eps INPUT_FILENAME lambda-background.svg OUTPUT_FILENAME export-default-background.eps REFERENCE_FILENAME export-default-background_expected.eps)
## test --export-background=COLOR
 add_cli_test(export-background_png  PARAMETERS --export-background=yellow --export-type=png INPUT_FILENAME lambda.svg OUTPUT_FILENAME export-background.png REFERENCE_FILENAME export-background_expected.png)
 add_cli_test(export-background_svg  PARAMETERS --export-background=yellow --export-type=svg INPUT_FILENAME lambda.svg OUTPUT_FILENAME export-background.svg REFERENCE_FILENAME export-background_expected.svg)
# add_cli_test(export-background_pdf  PARAMETERS --export-background=yellow --export-type=pdf INPUT_FILENAME lambda.svg OUTPUT_FILENAME export-background.pdf REFERENCE_FILENAME export-background_expected.pdf)
# add_cli_test(export-background_ps   PARAMETERS --export-background=yellow --export-type=ps  INPUT_FILENAME lambda.svg OUTPUT_FILENAME export-background.ps  REFERENCE_FILENAME export-background_expected.ps )
# add_cli_test(export-background_eps  PARAMETERS --export-background=yellow --export-type=eps INPUT_FILENAME lambda.svg OUTPUT_FILENAME export-background.eps REFERENCE_FILENAME export-background_expected.eps)
# add_cli_test(export-background_emf  PARAMETERS --export-background=yellow --export-type=emf INPUT_FILENAME lambda.svg OUTPUT_FILENAME export-background.emf REFERENCE_FILENAME export-background_expected.emf)
# add_cli_test(export-background_wmf  PARAMETERS --export-background=yellow --export-type=wmf INPUT_FILENAME lambda.svg OUTPUT_FILENAME export-background.wmf REFERENCE_FILENAME export-background_expected.wmf)

# --export-background-opacity=VALUE
# SVG, PDF, PS, EPS: Format not supoorted. Feature request: https://gitlab.com/inkscape/inbox/issues/2293
# EMF, WMF: Format does not support opacity.
add_cli_test(export-background-opacity_png  PARAMETERS --export-background=yellow --export-background-opacity=0.5 --export-type=png INPUT_FILENAME lambda.svg OUTPUT_FILENAME export-background-opacity.png REFERENCE_FILENAME export-background-opacity_expected.png)
 add_cli_test(export-background-opacity_svg  PARAMETERS --export-background=yellow --export-background-opacity=0.5 --export-type=svg INPUT_FILENAME lambda.svg OUTPUT_FILENAME export-background-opacity.svg REFERENCE_FILENAME export-background-opacity_expected.svg)
# add_cli_test(export-background-opacity_pdf  PARAMETERS --export-background=yellow --export-background-opacity=0.5 --export-type=pdf INPUT_FILENAME lambda.svg OUTPUT_FILENAME export-background-opacity.pdf REFERENCE_FILENAME export-background-opacity_expected.pdf)
# add_cli_test(export-background-opacity_ps   PARAMETERS --export-background=yellow --export-background-opacity=0.5 --export-type=ps  INPUT_FILENAME lambda.svg OUTPUT_FILENAME export-background-opacity.ps  REFERENCE_FILENAME export-background-opacity_expected.ps )
# add_cli_test(export-background-opacity_eps  PARAMETERS --export-background=yellow --export-background-opacity=0.5 --export-type=eps INPUT_FILENAME lambda.svg OUTPUT_FILENAME export-background-opacity.eps REFERENCE_FILENAME export-background-opacity_expected.eps)

# --export-png-color-mode=COLOR-MODE
# SVG, PDF, PS, EPS, EMF, WMF: Vector formats - bitmap bit-depth and color-type not relevant there.
 add_cli_test(export-png-color-mode-gray-8_png  PARAMETERS --export-png-color-mode=Gray_8 --export-type=png INPUT_FILENAME areas.svg OUTPUT_FILENAME export-png-color-mode-gray-8.png REFERENCE_FILENAME export-png-color-mode-gray-8_expected.png)
 add_cli_test(export-png-color-mode-rgb-8_png  PARAMETERS --export-png-color-mode=RGB_8 --export-type=png INPUT_FILENAME areas.svg OUTPUT_FILENAME export-png-color-mode-rgb-8.png REFERENCE_FILENAME export-png-color-mode-rgb-8_expected.png)
 add_cli_test(export-png-color-mode-rgba-8_png  PARAMETERS --export-png-color-mode=RGBA_8 --export-type=png INPUT_FILENAME areas.svg OUTPUT_FILENAME export-png-color-mode-rgba-8.png REFERENCE_FILENAME export-png-color-mode-rgba-8_expected.png)

## test whether we produce correct output for default export extensions
add_cli_test(export-extension_svg  PARAMETERS --export-type=svg --export-extension=org.inkscape.output.svg.inkscape INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.svg REFERENCE_FILENAME shapes.svg)
add_cli_test(export-extension_ps   PARAMETERS --export-type=ps --export-extension=org.inkscape.print.ps.cairo INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.ps  REFERENCE_FILENAME shapes_expected.ps)
add_cli_test(export-extension_eps  PARAMETERS --export-type=eps --export-extension=org.inkscape.print.eps.cairo  INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.eps REFERENCE_FILENAME shapes_expected.eps)
if(WITH_CAPYPDF)
    add_cli_test(export-extension_pdf  PARAMETERS --export-type=pdf --export-extension=org.inkscape.output.pdf.capypdf INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.pdf REFERENCE_FILENAME shapes_expected.pdf FUZZ_PERCENTAGE 1)
endif()
add_cli_test(export-extension_emf  PARAMETERS --export-type=emf --export-extension=org.inkscape.output.emf  INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.emf REFERENCE_FILENAME shapes_expected.emf)
add_cli_test(export-extension_wmf  PARAMETERS --export-type=wmf --export-extension=org.inkscape.output.wmf INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.wmf REFERENCE_FILENAME shapes_expected.wmf FUZZ_PERCENTAGE 0.5)
add_cli_test(export-plain-extension-svg PARAMETERS --export-type=svg --export-extension=org.inkscape.output.svg.plain
                              INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.svg REFERENCE_FILENAME shapes.svg
                              TEST_SCRIPT match_regex_fail.sh "shapes.svg" "inkscape:|sodipodi:")
## guess file type from --export-extension
add_cli_test(export-type-extension_svg_guess_file_type  PARAMETERS --export-extension=org.inkscape.output.svg.inkscape INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes)

## check error messages for --export-extension
add_cli_test(export-type-extension_png_error  PARAMETERS --export-extension=org.inkscape.output.svg.inkscape INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.png
                            PASS_FOR_OUTPUT "InkFileExportCmd::do_export: The parameter --export-extension is invalid for PNG export")
add_cli_test(export-type-extension_type_error  PARAMETERS --export-extension=org.inkscape.output.svg.inkscape INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.pdf
                            PASS_FOR_OUTPUT "InkFileExportCmd::do_export: The supplied extension ID \\(org\\.inkscape\\.output\\.svg\\.inkscape\\) does not match any of the extensions available for this file type.*")
add_cli_test(export-type-extension_id_error  PARAMETERS --export-extension=whatever INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes
                            PASS_FOR_OUTPUT "InkFileExportCmd::do_export: The supplied --export-extension was not found. Specify a file extension to get a list of available extensions for this file type.")
add_cli_test(export-type_filetype_error INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.xxx
                            PASS_FOR_OUTPUT "InkFileExportCmd::do_export: Unknown export type: xxx. Allowed values: \\[.*]")

# --query-id=OBJECT-ID[,OBJECT-ID]*

# --query-all / --query-x / --query-y / --query-width / --query-height
string(CONCAT query_all_expected "rect1,10,10,80,80\n"
                                 "rect2,110,20,80,70\n"
                                 "rect3,210,30,80,60")
add_cli_test(query-all     PARAMETERS --query-id=rect2 --query-all    INPUT_FILENAME rects.svg PASS_FOR_OUTPUT ${query_all_expected})
add_cli_test(query-x       PARAMETERS --query-id=rect2 --query-x      INPUT_FILENAME rects.svg PASS_FOR_OUTPUT 110)
add_cli_test(query-y       PARAMETERS --query-id=rect2 --query-y      INPUT_FILENAME rects.svg PASS_FOR_OUTPUT 20)
add_cli_test(query-width   PARAMETERS --query-id=rect2 --query-width  INPUT_FILENAME rects.svg PASS_FOR_OUTPUT 80)
add_cli_test(query-height  PARAMETERS --query-id=rect2 --query-height INPUT_FILENAME rects.svg PASS_FOR_OUTPUT 70)

# --vacuum-defs

# --select=OBJECT-ID[,OBJECT-ID]*

# --actions / --verbs
# (see below)

# --action-list / --verb-list
add_cli_test(action-list   PARAMETERS --action-list PASS_FOR_OUTPUT "file-new")

# --with-gui

# --batch-process

# --active-window

# --shell


###############
### actions ###
###############

# action-list: test = action-list
add_cli_test(actions-action-list           PARAMETERS --actions=action-list PASS_FOR_OUTPUT "file-new")

# actions-file=FILENAME

# convert-dpi-method
# NOTE: currently convert-dpi-method must be set before file-open, otherwise it will not take effect
add_cli_test(action-convert-dpi-method-none
             PARAMETERS --export-area-page --actions=convert-dpi-method:none$<SEMICOLON>file-open:${CMAKE_CURRENT_SOURCE_DIR}/testcases/shapes_pre_0.92.svg$<SEMICOLON>export-filename:action-convert-dpi-method-none.png$<SEMICOLON>export-do
             OUTPUT_FILENAME action-convert-dpi-method-none.png
             REFERENCE_FILENAME shapes_expected.png)
add_cli_test(action-convert-dpi-method-scale-viewbox
             PARAMETERS --export-area-page --actions=convert-dpi-method:scale-viewbox$<SEMICOLON>file-open:${CMAKE_CURRENT_SOURCE_DIR}/testcases/shapes_pre_0.92.svg$<SEMICOLON>export-filename:action-convert-dpi-method-scale-viewbox.png$<SEMICOLON>export-do
             OUTPUT_FILENAME action-convert-dpi-method-scale-viewbox.png
             EXPECTED_FILES "${CMAKE_CURRENT_SOURCE_DIR}/testcases/shapes_pre_0.92_backup.svg"
             REFERENCE_FILENAME shapes_pre_0.92_scaled_expected.png)
add_cli_test(action-convert-dpi-method-scale-document
             PARAMETERS --export-area-page --actions=convert-dpi-method:scale-document$<SEMICOLON>file-open:${CMAKE_CURRENT_SOURCE_DIR}/testcases/shapes_pre_0.92.svg$<SEMICOLON>export-filename:action-convert-dpi-method-scale-document.png$<SEMICOLON>export-do
             OUTPUT_FILENAME action-convert-dpi-method-scale-document.png
             EXPECTED_FILES "${CMAKE_CURRENT_SOURCE_DIR}/testcases/shapes_pre_0.92_backup.svg"
             REFERENCE_FILENAME shapes_pre_0.92_scaled_expected.png)

# debug-info
add_cli_test(actions-debug-info            PARAMETERS --actions=debug-info PASS_FOR_OUTPUT "OS version:")

# export-area
# export-area-drawing
# export-area-page
# export-area-snap
# export-background
# export-background-opacity
# export-do
# export-dpi
# export-filename
# export-height
# export-id
# export-id-only
# export-ignore-filters
# export-latex
# export-margin
# export-overwrite
# export-pdf-version
# export-plain-svg
# export-png-color-mode
# export-ps-level
# export-text-to-path
# export-type
# export-use-hints
# export-width

# file-close
add_cli_test(actions-file-close            PARAMETERS --actions=file-new$<SEMICOLON>file-close$<SEMICOLON>file-open:${CMAKE_CURRENT_SOURCE_DIR}/testcases/theta.svg$<SEMICOLON>export-filename:actions-file-close.svg$<SEMICOLON>export-do
                                           TEST_SCRIPT match_regex.sh actions-file-close.svg "id=\"purple-cover\"")

# file-new
add_cli_test(actions-file-new1             PARAMETERS --actions=file-new$<SEMICOLON>export-filename:actions-file-new1.svg$<SEMICOLON>export-do
                                           TEST_SCRIPT match_regex.sh actions-file-new1.svg "viewBox=\"0 0 210 297\"")
add_cli_test(actions-file-new2             PARAMETERS --actions=file-new:${CMAKE_CURRENT_SOURCE_DIR}/testcases/theta.svg$<SEMICOLON>export-filename:actions-file-new2.svg$<SEMICOLON>export-do
                                           TEST_SCRIPT match_regex.sh actions-file-new2.svg "id=\"purple-cover\"")

# file-open
add_cli_test(actions-file-open             PARAMETERS --actions=file-open:${CMAKE_CURRENT_SOURCE_DIR}/testcases/theta.svg$<SEMICOLON>export-filename:actions-file-open.png$<SEMICOLON>export-do
                                           OUTPUT_FILENAME actions-file-open.png
                                           REFERENCE_FILENAME actions-file-open_expected.png)

# inkscape-version
add_cli_test(actions-inkscape-version      PARAMETERS --actions=inkscape-version
                                           PASS_FOR_OUTPUT "Inkscape [0-9]+\\.[0-9]+")

# no-convert-baseline
# object-align
add_cli_test(actions-object-align          INPUT_FILENAME lambda.svg
                                           PARAMETERS --actions=select-by-id:green$<SEMICOLON>object-align:left\ page
                                           OUTPUT_FILENAME actions-object-align.png
                                           REFERENCE_FILENAME actions-object-align_expected.png)

# object-distribute
add_cli_test(actions-object-distribute     INPUT_FILENAME rects.svg
                                           PARAMETERS --actions=select-by-element:rect$<SEMICOLON>object-distribute:vgap
                                           OUTPUT_FILENAME actions-object-distribute.png
                                           REFERENCE_FILENAME actions-object-distribute_expected.png)

# object-set-attribute
add_cli_test(actions-object-set-attribute           INPUT_FILENAME rects.svg
                                                    PARAMETERS --actions=select-by-id:rect1$<SEMICOLON>object-set-attribute:rx,15
                                                    OUTPUT_FILENAME actions-object-set-attribute.png
                                                    REFERENCE_FILENAME actions-object-set-attribute_expected.png)

add_cli_test(actions-object-set-attribute_filter    INPUT_FILENAME gradient.svg
                                                    PARAMETERS --actions=select-by-id:stop1$<SEMICOLON>object-set-attribute:offset,25%
                                                    OUTPUT_FILENAME actions-object-set-attribute.svg
                                                    TEST_SCRIPT match_regex.sh actions-object-set-attribute.svg "offset=\"25%\"")

# object-get-attribute
add_cli_test(actions-object-get-attribute              INPUT_FILENAME rects.svg
                                                       PARAMETERS --actions=select-by-id:rect1$<SEMICOLON>object-get-attribute:x
                                                       PASS_FOR_OUTPUT "10\n$")

add_cli_test(actions-object-get-attribute_nonexistant  INPUT_FILENAME rects.svg
                                                       PARAMETERS --actions=select-by-id:rect1$<SEMICOLON>object-get-attribute:z
                                                       PASS_FOR_OUTPUT "\n$")
# object-remove-attribute
add_cli_test(actions-object-remove-attribute    INPUT_FILENAME export-mask-on-transformed-image.svg
                                                PARAMETERS --actions=select-by-id:circle-48$<SEMICOLON>object-remove-attribute:style
                                                OUTPUT_FILENAME actions-object-remove-attribute.svg
                                                TEST_SCRIPT match_regex_fail.sh actions-object-remove-attribute.svg "style=\"fill: #ffff00; stroke: none;\"")

# object-set-property
add_cli_test(actions-object-set-property        INPUT_FILENAME areas.svg
                                                PARAMETERS --actions=select-by-id:MyRect$<SEMICOLON>object-set-property:fill,gold
                                                OUTPUT_FILENAME actions-object-set-property.svg
                                                TEST_SCRIPT match_regex.sh actions-object-set-property.svg "style=\"fill:gold\"")

add_cli_test(actions-object-set-property_filter INPUT_FILENAME gradient.svg
                                                PARAMETERS --actions=select-by-id:stop1$<SEMICOLON>object-set-property:stop-color,gold
                                                OUTPUT_FILENAME actions-object-set-property.svg
                                                TEST_SCRIPT match_regex.sh actions-object-set-property.svg "style=\"stop-color:gold\"")

# object-get-property
add_cli_test(actions-object-get-property               INPUT_FILENAME areas.svg
                                                       PARAMETERS --actions=select-by-id:MyStar$<SEMICOLON>object-get-attribute:fill
                                                       PASS_FOR_OUTPUT "red\n$")

add_cli_test(actions-object-get-property_nonexistant   INPUT_FILENAME areas.svg
                                                       PARAMETERS --actions=select-by-id:MyStar$<SEMICOLON>object-get-attribute:full
                                                       PASS_FOR_OUTPUT "\n$")

# object-remove-attribute
add_cli_test(actions-object-remove-property     INPUT_FILENAME export-mask-on-transformed-image.svg
                                                PARAMETERS --actions=select-by-id:circle-48$<SEMICOLON>object-remove-property:fill
                                                OUTPUT_FILENAME actions-object-remove-property.svg
                                                TEST_SCRIPT match_regex_fail.sh actions-object-remove-property.svg "fill: #ffff00;")

# path-simplify
add_cli_test(actions-path-simplify         INPUT_FILENAME path.svg
                                           PARAMETERS --actions=select-by-id:gates$<SEMICOLON>path-simplify$<SEMICOLON>export-id:gates$<SEMICOLON>export-id-only
                                           OUTPUT_FILENAME actions-object-simplify-path.png
                                           REFERENCE_FILENAME actions-object-simplify-path_expected.png FUZZ_PERCENTAGE 0.1)

# object-stroke-to-path
add_cli_test(actions-object-stroke-to-path INPUT_FILENAME path.svg
                                           PARAMETERS --actions=select-by-id:cross$<SEMICOLON>object-stroke-to-path$<SEMICOLON>select-by-selector:path\:nth-of-type\(2\)$<SEMICOLON>object-set-attribute:stroke,red$<SEMICOLON>export-id:cross$<SEMICOLON>export-id-only
                                           OUTPUT_FILENAME actions-object-stroke-to-path.png
                                           REFERENCE_FILENAME actions-object-stroke-to-path_expected.png)

add_cli_test(action_stroke-to-path_varied
                        INPUT_FILENAME stroke-to-path-variations.svg
                        PARAMETERS --actions=select-all:all$<SEMICOLON>object-stroke-to-path$<SEMICOLON>export-filename:stroke-to-path-variations_out.svg$<SEMICOLON>export-do
                        EXPECTED_FILES stroke-to-path-variations_out.svg
                        TEST_SCRIPT match_regex_fail.sh stroke-to-path-variations_out.svg "(#mixed)|(#fillet)|(<text)|(<flow)|(<use)")

add_cli_test(action_stroke-to-path_visual
                        INPUT_FILENAME stroke-to-path-variations.svg
                        PARAMETERS --actions=select-all:all$<SEMICOLON>object-stroke-to-path
                        OUTPUT_FILENAME stroke-to-path-variations.png
                        REFERENCE_FILENAME actions-object-stroke-to-path_variations_expected.png
                        FUZZ_PERCENTAGE 0.01)

# object-to-path
add_cli_test(actions-object-to-path        INPUT_FILENAME areas.svg
                                           PARAMETERS --actions=select-by-id:MyRect$<SEMICOLON>object-to-path$<SEMICOLON>export-id:MyRect$<SEMICOLON>export-id-only
                                           OUTPUT_FILENAME actions-object-to-path.svg
                                           TEST_SCRIPT match_regex.sh actions-object-to-path.svg "<path")

# object-unlink-clones
add_cli_test(actions-object-unlink-clones  INPUT_FILENAME path.svg
                                           PARAMETERS --actions=select-by-id:clone$<SEMICOLON>object-unlink-clones$<SEMICOLON>object-set-attribute:fill,red$<SEMICOLON>export-id:clone$<SEMICOLON>export-id-only
                                           OUTPUT_FILENAME actions-object-unlink-clones.png
                                           REFERENCE_FILENAME actions-object-unlink-clones_expected.png)
# open-page
add_cli_test(actions-open-page             PARAMETERS --actions=open-page:3$<SEMICOLON>file-open:${CMAKE_CURRENT_SOURCE_DIR}/testcases/pdf-pages.pdf$<SEMICOLON>export-filename:actions-open-page.png$<SEMICOLON>export-do
                                           OUTPUT_FILENAME actions-open-page.png
                                           REFERENCE_FILENAME actions-open-page_expected.png
                                           FUZZ_PERCENTAGE 0.05)

# query-all: test = query-all
add_cli_test(actions-query-all             INPUT_FILENAME rects.svg PARAMETERS --actions=select-by-id:rect2$<SEMICOLON>query-all    PASS_FOR_OUTPUT ${query_all_expected})

# query-height: test = query-height
add_cli_test(actions-query-height          INPUT_FILENAME rects.svg PARAMETERS --actions=select-by-id:rect2$<SEMICOLON>query-height PASS_FOR_OUTPUT 70)

# query-width: test = query-width
add_cli_test(actions-query-width           INPUT_FILENAME rects.svg PARAMETERS --actions=select-by-id:rect2$<SEMICOLON>query-width  PASS_FOR_OUTPUT 80)

# query-x: test = query-x
add_cli_test(actions-query-x               INPUT_FILENAME rects.svg PARAMETERS --actions=select-by-id:rect2$<SEMICOLON>query-x      PASS_FOR_OUTPUT 110)

# query-y: test = query-y
add_cli_test(actions-query-y               INPUT_FILENAME rects.svg PARAMETERS --actions=select-by-id:rect2$<SEMICOLON>query-y      PASS_FOR_OUTPUT 20)

# quit-inkscape
# Failed because of https://gitlab.com/inkscape/inkscape/-/issues/1565
# add_cli_test(actions-quit-inkscape         PARAMETERS --actions=quit-inkscape FAIL_FOR_OUTPUT ".+")

# select: it is deprecated
# select-all
# Failed because of https://gitlab.com/inkscape/inkscape/-/issues/1565
# add_cli_test(actions-select-all            INPUT_FILENAME path.svg PARAMETERS --actions=select-all$<SEMICOLON>select-list
#                                           PASS_FOR_OUTPUT "^heart .+\nbottom-left-corner .+\nsquare .+\ntop-right-corner .+\narc-big .+\narc-small .+\ngates .+\ncross .+\nclone .+$")

# select-by-class
# Failed because of https://gitlab.com/inkscape/inkscape/-/issues/1565
# add_cli_test(actions-select-by-class       INPUT_FILENAME path.svg PARAMETERS --actions=select-by-class:corner$<SEMICOLON>select-list
#                                           PASS_FOR_OUTPUT "^bottom-left-corner .+\ntop-right-corner .+$")

# select-by-element
# Failed because of https://gitlab.com/inkscape/inkscape/-/issues/1565
# add_cli_test(actions-select-by-element     INPUT_FILENAME path.svg PARAMETERS --actions=select-by-element:use$<SEMICOLON>select-list
#                                           PASS_FOR_OUTPUT "^clone .+$")

# select-by-id
# Failed because of https://gitlab.com/inkscape/inkscape/-/issues/1565
# add_cli_test(actions-select-by-id          INPUT_FILENAME path.svg PARAMETERS --actions=select-by-id:top-right-corner$<SEMICOLON>select-list
#                                           PASS_FOR_OUTPUT "^top-right-corner .+$")

# select-by-selector = svg > path:nth-child(2n) ~ *[fill=red]:nth-of-type(even)
# Failed because of https://gitlab.com/inkscape/inkscape/-/issues/1565
# add_cli_test(actions-select-by-selector    INPUT_FILENAME path.svg PARAMETERS --actions=select-by-selector:svg\ >\ path:nth-child\(2n\)\ ~\ *\[fill\=red\]:nth-of-type\(even\)$<SEMICOLON>select-list
#                                           PASS_FOR_OUTPUT "^arc-big .+\nclone .+$")

# select-clear
# Failed because of https://gitlab.com/inkscape/inkscape/-/issues/1565
# add_cli_test(actions-select-clear          INPUT_FILENAME path.svg PARAMETERS --actions=select-by-element:path$<SEMICOLON>select-clear$<SEMICOLON>select-by-id:cross$<SEMICOLON>select-list
#                                           PASS_FOR_OUTPUT "^cross .+$")

# select-invert
# Failed because of https://gitlab.com/inkscape/inkscape/-/issues/1565
# add_cli_test(actions-select-invert         INPUT_FILENAME path.svg PARAMETERS --actions=select-by-selector:path:nth-of-type\(2n\)$<SEMICOLON>select-invert:all$<SEMICOLON>select-list
#                                           PASS_FOR_OUTPUT "^heart .+\nsquare .+\narc-big .+\ngates .+\nclone .+$")

# select-list
# Failed because of https://gitlab.com/inkscape/inkscape/-/issues/1565
# add_cli_test(actions-select-list           INPUT_FILENAME path.svg PARAMETERS --actions=select-by-selector:*[stroke-width]:nth-child\(3n+3\)$<SEMICOLON>select-list
#                                           PASS_FOR_OUTPUT "^gates cloned: false ref: 1 href: 0 total href: 0\n$")

# system-data-directory: test = system-data-directory
add_cli_test(actions-system-data-directory PARAMETERS --actions=system-data-directory ENVIRONMENT unset PASS_FOR_OUTPUT "inkscape\n$")

# transform-remove
add_cli_test(actions-transform-remove      INPUT_FILENAME pyramids.svg
                                           PARAMETERS --actions=select-by-id:quad_snapped_4$<SEMICOLON>transform-rotate:2$<SEMICOLON>transform-remove
                                           OUTPUT_FILENAME actions-transform-remove.png
                                           REFERENCE_FILENAME actions-transform-remove_expected.png)

# transform-rotate
add_cli_test(actions-transform-rotate      INPUT_FILENAME pyramids.svg
                                           PARAMETERS --actions=select-by-id:rect_snapped_4$<SEMICOLON>transform-rotate:45
                                           OUTPUT_FILENAME actions-transform-rotate.png
                                           REFERENCE_FILENAME actions-transform-rotate_expected.png)

# transform-scale
add_cli_test(actions-transform-grow        INPUT_FILENAME pyramids.svg
                                           PARAMETERS --actions=select-by-id:quad_snapped_1$<SEMICOLON>transform-grow:10
                                           OUTPUT_FILENAME actions-transform-grow.png
                                           REFERENCE_FILENAME actions-transform-grow_expected.png)

# transform-translate
add_cli_test(actions-transform-translate   INPUT_FILENAME pyramids.svg
                                           PARAMETERS --actions=select-by-id:quad_snapped_1$<SEMICOLON>transform-translate:-5,-5
                                           OUTPUT_FILENAME actions-transform-translate.png
                                           REFERENCE_FILENAME actions-transform-translate_expected.png)

# unselect: it is deprecated
# unselect-by-id
# Failed because of https://gitlab.com/inkscape/inkscape/-/issues/1565
# add_cli_test(actions-unselect-by-id        INPUT_FILENAME path.svg PARAMETERS --actions=select-by-element:path$<SEMICOLON>unselect-by-id:heart,bottom-left-corner,square,top-right-corner,arc-big,arc-small,gates$<SEMICOLON>select-list
#                                           PASS_FOR_OUTPUT "^cross .+$")

# user-data-directory: test = user-data-directory
add_cli_test(actions-user-data-directory   PARAMETERS --actions=user-data-directory ENVIRONMENT unset PASS_FOR_OUTPUT "inkscape\n$")

# vacuum-defs
# window-close
# window-open

# tests x/y values are not swapped in tspans.
add_cli_test(action_test_multiline_anchoring
                        INPUT_FILENAME multiline-anchoring.svg
                        PARAMETERS --actions=select-by-id:grouped_text$<SEMICOLON>transform-translate:10,0$<SEMICOLON>transform-translate:-10,0$<SEMICOLON>export-filename:multiline-anchoring_out.svg$<SEMICOLON>export-plain-svg$<SEMICOLON>export-do
                        EXPECTED_FILES multiline-anchoring_out.svg
                        TEST_SCRIPT match_regex_fail.sh multiline-anchoring_out.svg "(x=\"[3-9][0-9]{2}\")|(x=\"[12][0-9]{3}\")|(y=\"[0-9]{2}\")|(y=\"1[0-9]{2}\")")
# test text to path with SVG in OTF fonts, at the end of test erase all text elements to make sure it doesn't pass by
# rendering the nonconverted text.
add_cli_test(actions-svginot-text-topath
    INPUT_FILENAME ../../rendering_tests/text-gzipped-svg-glyph.svg
    PARAMETERS --actions=select-all$<SEMICOLON>object-to-path$<SEMICOLON>select-clear$<SEMICOLON>select-by-element:text$<SEMICOLON>delete
    OUTPUT_FILENAME actions-svginot-text-topath-output.png
    REFERENCE_FILENAME ../../rendering_tests/expected_rendering/text-gzipped-svg-glyph.png
    FUZZ_PERCENTAGE 3
)
add_cli_test(actions-svginot-text-topath2
        INPUT_FILENAME ../../rendering_tests/text-svg-glyph-custom.svg
        PARAMETERS --actions=select-all$<SEMICOLON>object-to-path$<SEMICOLON>select-clear$<SEMICOLON>select-by-element:text$<SEMICOLON>delete
        OUTPUT_FILENAME actions-svginot-text-topath-output.png
        REFERENCE_FILENAME ../../rendering_tests/expected_rendering/text-svg-glyph-custom.png
        FUZZ_PERCENTAGE 1
)

add_cli_test(actions-text-to-glyph
        INPUT_FILENAME text_to_glyph.svg
        PARAMETERS
            --actions=select-all$<SEMICOLON>text-convert-to-glyphs$<SEMICOLON>select-clear$<SEMICOLON>select-by-id:text_o_1$<SEMICOLON>select-by-id:text_o_2$<SEMICOLON>select-by-id:text_o_3$<SEMICOLON>select-by-id:text_o_4$<SEMICOLON>delete$<SEMICOLON>
        OUTPUT_FILENAME text_to_glyph.png
        REFERENCE_FILENAME text_to_glyph_expected.svgz REFERENCE_INKSCAPE
        FUZZ_PERCENTAGE 0.2
)


# The export area type should be the last set value
add_cli_test(actions-export-area-drawing-then-page
             INPUT_FILENAME areas.svg
             PARAMETERS --actions=export-area-drawing:true$<SEMICOLON>export-area-page:true
             OUTPUT_FILENAME actions-export-area-drawing-then-page.png
             REFERENCE_FILENAME export-area-page_expected.png)
add_cli_test(actions-export-area-page-then-drawing
             INPUT_FILENAME areas.svg
             PARAMETERS --actions=export-area-page:true$<SEMICOLON>export-area-drawing:true
             OUTPUT_FILENAME actions-export-area-page-then-drawing.png
             REFERENCE_FILENAME export-area-drawing_expected.png)

# Sequential export-id-only action should succeed
# https://gitlab.com/inkscape/inkscape/-/issues/1957
add_cli_test(actions-sequential-export-id-only_svg 
             INPUT_FILENAME theta.svg
             PARAMETERS --actions=export-id-only$<SEMICOLON>export-filename:actions-sequential-export-id-only.svg$<SEMICOLON>export-id:red$<SEMICOLON>export-do$<SEMICOLON>export-filename:actions-sequential-export-id-only.svg$<SEMICOLON>export-id:blue$<SEMICOLON>export-do
             OUTPUT_FILENAME actions-sequential-export-id-only.svg
             REFERENCE_FILENAME export-id_export-id-only_expected.svg)

# EMF exports should be exact to a 100th of a milimeter (not increase to the next highest whole millimeter)
# https://gitlab.com/inkscape/inkscape/-/issues/8300
add_cli_test(export-emf-100thmm 
             INPUT_FILENAME emf-100thmm.svg
             PARAMETERS --export-type=emf --export-extension=org.inkscape.output.emf
             OUTPUT_FILENAME export-emf-100thmm.emf
             REFERENCE_FILENAME export-emf-100thmm_expected.emf)

###########################
### pdf input support   ###
###########################

function(add_pdfinput_test name pages font_strategy)
    set(oneValueArgs IN_FILE EXTRA)
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "" ${ARGN})

    if(DEFINED ARG_IN_FILE)
        set(in_file "${ARG_IN_FILE}")
    else()
        set(in_file "${name}")
    endif()

    add_cli_test(pdfinput-${name}
        INPUT_FILENAME pdfinput/${in_file}.pdf
        PARAMETERS --pages=${pages} --pdf-font-strategy=${font_strategy} ${ARG_EXTRA} --actions=export-filename:${name}.svg$<SEMICOLON>export-do
        EXPECTED_FILES "${name}.svg"
        TEST_SCRIPT compare.sh ${name}.svg "${CMAKE_CURRENT_SOURCE_DIR}/testcases/pdfinput/${name}_expected.svg")

endfunction()

# CMS testing disabled because CI builder doesn't support LCMS yet.
# add_pdfinput_test(cms 1)
if(WITH_POPPLER)
    add_pdfinput_test(font-spacing 1 draw-all)
    add_pdfinput_test(font-style 1 draw-all)
    add_pdfinput_test(gradients 1 draw-all)
    add_pdfinput_test(latex 1 draw-all)
    add_pdfinput_test(transparency-groups 1 draw-all)
    add_pdfinput_test(rotated-gradients 1 draw-all)
    add_pdfinput_test(labels-utf16 1 draw-all)
    add_pdfinput_test(multi-page-sample all draw-all)
    add_pdfinput_test(multiple-style-same-text-block 1 keep)
    add_pdfinput_test(gs_linewidth all draw-all)
    add_pdfinput_test(XObj_layer all draw-all)
    add_pdfinput_test(Annot_w_AP_RGB all draw-all IN_FILE "Annot_w_AP" EXTRA "--pdf-convert-colors")
    add_pdfinput_test(Annot_w_AP_CMYK all draw-all IN_FILE "Annot_w_AP")
    add_pdfinput_test(clipping_shift 1 keep)
    add_pdfinput_test(layers-in-xobj 1 substitute EXTRA "--pdf-group-by=layer")
    add_pdfinput_test(negative-font-size 1 draw-all)
    add_pdfinput_test(ligatures-drawn all draw-all IN_FILE "ligatures")
    add_pdfinput_test(ligatures-text all keep IN_FILE "ligatures")
endif()

function(add_pdfoutput_test name)
    set(oneValueArgs FUZZ_PERCENTAGE RASTER_DPI SUPERSAMPLING)
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "" ${ARGN})

    if(NOT DEFINED ARG_FUZZ_PERCENTAGE)
        set(ARG_FUZZ_PERCENTAGE 0.1)
    endif()

    if(DEFINED ARG_RASTER_DPI)
        set(RASTER_DPI "RASTER_DPI" "${ARG_RASTER_DPI}")
    endif()
    if(DEFINED ARG_SUPERSAMPLING)
        set(SUPERSAMPLING "SUPERSAMPLING" "${ARG_SUPERSAMPLING}")
    endif()

    set(SOURCE_SVG "${CMAKE_CURRENT_SOURCE_DIR}/testcases/pdfoutput/${name}.svg")
    execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/get-pages.py ${SOURCE_SVG}
                    ERROR_VARIABLE PAGE_NUMBERS
                    OUTPUT_VARIABLE PAGE_LABELS)
    # Watching the source SVG means changes will regenerate the CTests
    watch("${SOURCE_SVG}")

    add_cli_test_run(cli_pdfoutput-${name} cli_pdfoutput-${name}-build
        INPUT_FILENAME pdfoutput/${name}.svg
        OUTPUT_FILENAME pdfoutput/${name}.pdf
    )
    foreach(NUM LABEL IN ZIP_LISTS PAGE_NUMBERS PAGE_LABELS)
        add_cli_test_cmp(cli_pdfoutput-${name} cli_pdfoutput-${name}-${LABEL}
            pdfoutput-${name}
            OUTPUT_FILENAME pdfoutput/${name}.pdf
            OUTPUT_PAGE "${NUM}"
            PNG_FILENAME "pdfoutput/expected/${name}-${NUM}.png"
            REFERENCE_FILENAME pdfoutput/expected/${name}-${NUM}.png
            FUZZ_PERCENTAGE ${ARG_FUZZ_PERCENTAGE}
            ${RASTER_DPI}
            ${SUPERSAMPLING}
        )
    endforeach()
    add_cli_test_cln(cli_pdfoutput-${name} cli_pdfoutput-${name}-clean
        OUTPUT_FILENAME pdfoutput/${name}.pdf)
endfunction()

if(WITH_CAPYPDF)
    add_pdfoutput_test(00-one-thing)
    add_pdfoutput_test(01-test-pages)
    add_pdfoutput_test(02-test-fills FUZZ_PERCENTAGE 1)
    add_pdfoutput_test(03-test-strokes)
    add_pdfoutput_test(04-test-transforms)
    add_pdfoutput_test(05-test-transparencies)
    add_pdfoutput_test(06-test-markers)
    add_pdfoutput_test(07-test-shapes FUZZ_PERCENTAGE 0.2)
    add_pdfoutput_test(08-clones FUZZ_PERCENTAGE 1)
    add_pdfoutput_test(09-text-layout FUZZ_PERCENTAGE 0.4 SUPERSAMPLING 3)
    add_pdfoutput_test(10-text-paint)
    add_pdfoutput_test(11-text-decoration)
    add_pdfoutput_test(12-rasters FUZZ_PERCENTAGE 1)
    add_pdfoutput_test(13-clips SUPERSAMPLING 3)
    add_pdfoutput_test(14-filters FUZZ_PERCENTAGE 0.2)
    add_pdfoutput_test(15-many-items)
endif()

###########################
### file format support ###
###########################

# librevenge formats
if(WITH_LIBCDR)
  # add_cli_test(import_cdr  PARAMETERS --export-type=png   # fails to open (regression in libcdr 1.6.0)
  #                          INPUT_FILENAME librevenge_formats/corel_draw.cdr OUTPUT_FILENAME format_corel_draw.png
  #                          REFERENCE_FILENAME librevenge_formats/corel_draw_expected.png # check png size is correct
  #                          FUZZ_PERCENTAGE 3)
  add_cli_test(import_cdr2 PARAMETERS --export-type=png
                           INPUT_FILENAME librevenge_formats/corel_draw2.cdr OUTPUT_FILENAME format_corel_draw2.png
                           REFERENCE_FILENAME librevenge_formats/corel_draw2_expected.png
                           FUZZ_PERCENTAGE 3)
endif()
if(WITH_LIBVISIO)
  add_cli_test(import_vsd  PARAMETERS --export-type=png
                           INPUT_FILENAME librevenge_formats/visio.vsd OUTPUT_FILENAME format_visio.vsd.png
                           REFERENCE_FILENAME librevenge_formats/visio.vsd_expected.png
                           FUZZ_PERCENTAGE 3)
  add_cli_test(import_vsdx PARAMETERS --export-type=png
                           INPUT_FILENAME librevenge_formats/visio.vsdx OUTPUT_FILENAME format_visio.vsdx.png
                           REFERENCE_FILENAME librevenge_formats/visio.vsdx_expected.png
                           FUZZ_PERCENTAGE 3)
endif()
if(WITH_LIBWPG)
  add_cli_test(import_wpg  PARAMETERS --export-type=png
                           INPUT_FILENAME librevenge_formats/word_perfect.wpg OUTPUT_FILENAME format_word_perfect.png
                           REFERENCE_FILENAME librevenge_formats/word_perfect_expected.png
                           FUZZ_PERCENTAGE 3)
endif()



##############################
### advanced functionality ###
##############################

# check whether INKSCAPE_DATADIR / INKSCAPE_PROFILE_DIR environment variables work
# TODO: INKSCAPE_PROFILE_DIR does not seem to be sanitized at all (i.e. is used verbatim by Inkscape)
set(fancy_dir "i_certainly_do_not_exist")
file(TO_NATIVE_PATH "${fancy_dir}/inkscape" expected_dir)
string(REPLACE "\\" "\\\\" expected_dir "${expected_dir}")
add_cli_test(inkscape_datadir      PARAMETERS --system-data-directory
                                   ENVIRONMENT INKSCAPE_DATADIR=${fancy_dir}
                                   PASS_FOR_OUTPUT "${expected_dir}\n$")
add_cli_test(inkscape_profile_dir  PARAMETERS --user-data-directory
                                   ENVIRONMENT INKSCAPE_PROFILE_DIR=${fancy_dir}/inkscape
                                   PASS_FOR_OUTPUT "${fancy_dir}/inkscape\n$")
add_cli_test(inkscape_profile_dir_handle_illegal
                                   ENVIRONMENT INKSCAPE_PROFILE_DIR=invalid:dir
                                   INPUT_FILENAME empty.svg OUTPUT_FILENAME empty.svg)

# check if "systemLanguage" attribute is properly handled
add_cli_test(systemLanguage_en     ENVIRONMENT LANGUAGE=en          INPUT_FILENAME     systemLanguage.svg
                                                                    OUTPUT_FILENAME    systemLanguage_en.png
                                                                    REFERENCE_FILENAME systemLanguage_en.png)
add_cli_test(systemLanguage_fr     ENVIRONMENT LANGUAGE=fr_FR       INPUT_FILENAME     systemLanguage.svg
                                                                    OUTPUT_FILENAME    systemLanguage_fr.png
                                                                    REFERENCE_FILENAME systemLanguage_fr.png)
add_cli_test(systemLanguage_fr2    ENVIRONMENT LANGUAGE=fr_FR.UTF-8 INPUT_FILENAME     systemLanguage.svg
                                                                    OUTPUT_FILENAME    systemLanguage_fr2.png
                                                                    REFERENCE_FILENAME systemLanguage_fr.png)
add_cli_test(systemLanguage_de     ENVIRONMENT LANGUAGE=de          INPUT_FILENAME     systemLanguage.svg
                                                                    OUTPUT_FILENAME    systemLanguage_de.png
                                                                    REFERENCE_FILENAME systemLanguage_de.png)
add_cli_test(systemLanguage_de-CH  ENVIRONMENT LANGUAGE=de_CH       INPUT_FILENAME     systemLanguage.svg
                                                                    OUTPUT_FILENAME    systemLanguage_de-CH.png
                                                                    REFERENCE_FILENAME systemLanguage_de.png)
add_cli_test(systemLanguage_pt     ENVIRONMENT LANGUAGE=pt          INPUT_FILENAME     systemLanguage.svg
                                                                    OUTPUT_FILENAME    systemLanguage_pt.png
                                                                    REFERENCE_FILENAME systemLanguage_pt.png)
add_cli_test(systemLanguage_xy     ENVIRONMENT LANGUAGE=xy          INPUT_FILENAME     systemLanguage.svg
                                                                    OUTPUT_FILENAME    systemLanguage_xy.png
                                                                    REFERENCE_FILENAME systemLanguage_default.png)
add_cli_test(systemLanguage_fr_RDF ENVIRONMENT LANGUAGE=xy          INPUT_FILENAME     systemLanguage_RDF.svg
                                                                    OUTPUT_FILENAME    systemLanguage_fr_RDF.png
                                                                    REFERENCE_FILENAME systemLanguage_fr.png)

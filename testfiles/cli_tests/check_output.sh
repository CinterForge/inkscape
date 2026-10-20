#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later

MY_LOCATION=$(dirname "$0")
source "${MY_LOCATION}/../utils/functions.sh"

ensure_command "convert"
ensure_command "compare"
ensure_command "bc"
ensure_command "cp"

OUTPUT_FILENAME=$1
OUTPUT_PAGE=$2
REFERENCE_FILENAME=$3
EXPECTED_FILES=$4
TEST_SCRIPT=$5
get_outputs "$6"
FUZZ=$7
DPI=$8
SUPERSAMPLING=$9

export LC_NUMERIC=C

# check if expected files exist
for file in ${EXPECTED_FILES}; do
    test -f "${file}" || { echo "Error: Expected file '${file}' not found."; exit 1; }
done

# if reference file is given check if input files exist and continue with comparison
if [ -n "${REFERENCE_FILENAME}" ]; then
    if [ ! -f "${OUTPUT_FILENAME}" ]; then
        echo "Error: Test file '${OUTPUT_FILENAME}' not found."
        exit 1
    fi
    if [ ! -f "${REFERENCE_FILENAME}" ]; then
        echo "Error: Reference file '${REFERENCE_FILENAME}' not found."
        exit 1
    fi

    # Convert the output and reference files to the PNG format
    # - use internal MSVG delegate in SVG conversions for reproducibility reasons (avoid inkscape or rsvg delegates)
    [ "${OUTPUT_FILENAME##*.}"    = "svg" ] && delegate1=MSVG:
    [ "${REFERENCE_FILENAME##*.}" = "svg" ] && delegate2=MSVG:

    CONVERSION_OPTIONS="-colorspace RGB"

    # Extract a page from multipage PS/PDF if requested
    OUTFILE_SUFFIX=""
    if [ -n "$OUTPUT_PAGE" ]; then
        OUTFILE_SUFFIX="[${OUTPUT_PAGE}]" # Use ImageMagick's bracket operator
    fi

    if [ -z "$DPI" ]; then
        DPI=72
    fi

    if [ -z "$SUPERSAMPLING" ]; then
        SUPERSAMPLING=1
    fi

    if [[ $(identify -format "%m" "${OUTPUT_FILENAME}") != "PNG" ]] then
        echo -density "%[fx:$DPI*$SUPERSAMPLING]" ${delegate1}${OUTPUT_FILENAME}${OUTFILE_SUFFIX} ${CONVERSION_OPTIONS} -resize "%[fx:100/$SUPERSAMPLING]%" ${PNG_FILENAME}
        if ! magick -density "%[fx:$DPI*$SUPERSAMPLING]" ${delegate1}${OUTPUT_FILENAME}${OUTFILE_SUFFIX} ${CONVERSION_OPTIONS} -resize "%[fx:100/$SUPERSAMPLING]%" ${PNG_FILENAME}; then
            echo "Warning: Failed to convert test file '${OUTPUT_FILENAME}' to PNG format. Skipping comparison test."
            exit 42
        fi
    else
        cp "${OUTPUT_FILENAME}" "${PNG_FILENAME}"
    fi
    if [[ $(identify -format "%m" "${REFERENCE_FILENAME}") != "PNG" ]] then
        if ! magick -density "%[fx:$DPI*$SUPERSAMPLING]" ${delegate2}${REFERENCE_FILENAME} ${CONVERSION_OPTIONS} -resize "%[fx:100/$SUPERSAMPLING]%" ${PNG_REFERENCE}; then
            echo "Warning: Failed to convert reference file '${REFERENCE_FILENAME}' to PNG format. Skipping comparison test."
            exit 42
        fi
    else
        cp "${REFERENCE_FILENAME}" "${PNG_REFERENCE}"
    fi

    # Compare the two files
    COMPARE_OUTPUT=$(compare 2>&1 -metric RMSE "${PNG_FILENAME}" "${PNG_REFERENCE}" "${PNG_COMPARE}")
    RELATIVE_ERROR=$(get_compare_result "$COMPARE_OUTPUT")
    PERCENTAGE_ERROR=$(fraction_to_percentage "$RELATIVE_ERROR")
    if (( $(is_relative_error_within_tolerance "$RELATIVE_ERROR" "$FUZZ") )) then
        # Test passed: print stats and clean up the files.
        echo "Fuzzy comparison PASSED; error of ${PERCENTAGE_ERROR}% is within ${FUZZ}% tolerance."
    else
        # Test failed!
        echo "Fuzzy comparison FAILED; error of ${PERCENTAGE_ERROR}% exceeds ${FUZZ}% tolerance."
        keep_outputs
        exit 1
    fi
fi

# if additional test file is specified, check existence and execute the command
if [ -n "${TEST_SCRIPT}" ]; then
    script=${TEST_SCRIPT%%;*}
    arguments=${TEST_SCRIPT#*;}
    IFS_OLD=$IFS IFS=';' arguments_array=($arguments) IFS=$IFS_OLD

    if [ ! -f "${script}" ]; then
        echo "Error: Additional test script file '${script}' not found."
        exit 1
    fi

    case ${script} in
        *.py)
            interpreter=python3
            ;;
        *)
            interpreter=bash
            ;;
    esac

    if ! $interpreter ${script} "${arguments_array[@]}"; then
        echo "Error: Additional test script failed."
        echo "Full call: $interpreter ${script} $(printf "\"%s\" " "${arguments_array[@]}")"
        keep_outputs
        exit 1
    fi
fi

clean_outputs

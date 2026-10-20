# SPDX-License-Identifier: GPL-2.0-or-later
#
# Bash functions useful for fuzzy bitmap comparisons. This file is a part of Inkscape.
#
# Authors:
#   Rafael Siejakowski <rs@rs-math.net>
#
# Copyright (C) 2023 Authors
#
# Released under GNU GPL v2+, read the file 'COPYING' for more information.
#

# Ensure consistancy between comparison functions with the same bash
# elements.
#
get_outputs()
{
    PNG_FILENAME="$1"
    PNG_REFERENCE="${PNG_FILENAME%%.png}-reference.png"
    PNG_COMPARE="${PNG_FILENAME%%.png}-compare.png"
    if [[ "${PNG_FILENAME}" = -* ]]; then
        echo "Error: Test output file '$PNG_FILENAME' starts with dash"
        exit 1
    fi
    if [ ! -d `dirname "${PNG_FILENAME}"` ]; then
        mkdir -p `dirname "${PNG_FILENAME}"`
    fi
    # This is used to keep a copy of the output in case of failure
    OUTPUT_FAILED="${OUTPUT_FILENAME%%.*}-failed.${OUTPUT_FILENAME##*.}"
}
clean_outputs()
{
    if [ "${PNG_FILENAME}" ]; then
        rm -f "${PNG_FILENAME}" "${PNG_REFERENCE}" "${PNG_COMPARE}"
    fi
    if [[ -f "${OUTPUT_FAILED}" ]]; then
        rm -f "${OUTPUT_FAILED}"
    fi
}
keep_outputs()
{
    cp "${OUTPUT_FILENAME}" "${OUTPUT_FAILED}"
}

ensure_command()
{
    command -v $1 >/dev/null 2>&1 || { echo >&2 "Required command '$1' not found. Aborting."; exit 1; }
}

export LANG=C # Needed to force . as the decimal separator
ensure_command "bc"

# Parse out the relative difference between two images from the command line output
# of an ImageMagick compare command. In case of error, crash out of the script to ensure the test fails.
#
# Arguments:
# $1 - commandline output from a compare command with RMSE metric.
#
# Output:
# The parsed relative error, as a floating point number.
#
get_compare_result()
{
    local COMPARE_OUTPUT="$1"
    RELATIVE_ERROR=${COMPARE_OUTPUT#*(}
    RELATIVE_ERROR=${RELATIVE_ERROR%)*}
    if [[ "x$RELATIVE_ERROR" == "x" ]]
    then
        echo "Warning: Could not parse out the relative error from ImageMagick output." >&2
        exit 42
    fi
    echo "$RELATIVE_ERROR"
}

# Check whether a floating point number is less than or equal to a percentage value.
# In case of error, crash out of the script.
#
# Arguments:
# $1 - a floating pointing number between 0.0 and 1.0
# $2 - a number between 0.0 and 100.0 representing a percentage. Scientific notation not allowed.
#
# Output:
# 1 if and only if $1 * 100 <= $2 else 0.
#
is_relative_error_within_tolerance()
{
    local CONDITION=$(printf "%.12f * 100 <= $2" "$1")
    WITHIN_TOLERANCE=$(echo "${CONDITION}" | bc)
    if [[ $? -ne 0 || ( $WITHIN_TOLERANCE -ne 0 && $WITHIN_TOLERANCE -ne 1 ) ]]
    then
        echo "Warning: An error occurred running 'bc'." >&2
        exit 42
    fi
    echo "$WITHIN_TOLERANCE"
}

# Multiply a floating point number by 100.
#
# Arguments:
# $1 - a floating point number.
#
# Output:
# The result of multiplying the passed number by 100, rounded to 2 digits after the decimal point.
fraction_to_percentage()
{
    local FORMULA=$(printf "%.5f * 100" "$1")
    echo "$FORMULA" | bc
}


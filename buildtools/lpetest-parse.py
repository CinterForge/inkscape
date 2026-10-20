#!/usr/bin/env python3
# coding=utf-8
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Script to evaluate LPE test output and update reference files.
# 
# Copyright (C) 2023-2024 Authors
# 
# Authors:
#   PBS <pbs3141@gmail.com>
#   Martin Owens <doctormo@gmail.com>
#
# Released under GNU GPL v2+, read the file 'COPYING' for more information.

import os, re, sys, argparse

description = '''Evaluate LPE test output and update reference files.

Instructions:
 1. Download the "complete raw" job log from one of the test pipelines and place it in the same directory as this script, named joblog.txt. Alternatively, collect the output from running the test suite.
 2. Run the script and examine the output.
 3. If there are no regressions, say yes when it offers to update the testfiles/lpe_tests/* files.
 
Output format:
  The output consists of the old paths 'a.txt', new paths 'b.txt', and visual comparison 'path-comparison.svg'.
  For each LPE test failure,
  - The old path is written on a single line to 'a.txt'.
  - The new path is written on a single line to 'b.txt'.
  - A group is written to 'path-comparison.svg' containing the old path in yellow and the new path in cyan,
    with additive blend mode. Non-matching areas therefore appear in either yellow or cyan.'''

open_testfiles = {}

def yesnoprompt():
    while True:
        txt = input("[y/n]: ").lower()
        if txt in ('y', 'yes'): return True
        if txt in ('n', 'no'): return False
        sys.stdout.write("\nPlease specify yes or no.\n")

def update_reference_files(path, a, b, svg, id):
    id = id.split("(")[0]
    m = re.search("testfiles", svg)
    if m is None:
        sys.stderr.write(f"Warning: Ignoring file '{svg}'\n")
        return

    # Open and cache the contents of the file
    name = os.path.join(path, svg[m.start():])
    if name not in open_testfiles:
        with open(name, "r") as tmpf:
            open_testfiles[name] = tmpf.read()

    contents = open_testfiles[name]
    m = re.search(fr'\bid *= *"{id}"', contents)
    if m is None:
        sys.stderr.write(f"Warning: Ignoring id {id}\n")
        return

    i = max(
        contents.rfind("<path", 0, m.start()),
        contents.rfind("<ellipse", 0, m.start())
    )
    if i < 0:
        sys.stderr.write(f"Warning: Couldn't find start of path for {id}\n")
        return

    m = re.compile(r'\bd *= *"').search(contents, i)
    if m is None:
        sys.stderr.write(f"Warning: Couldn't find d attribute for {id}\n")
        return

    i = m.end()
    j = contents.find('"', i)
    if j == -1:
        sys.stderr.write(f"Warning: Couldn't find end of d attribute for {id}\n")
        return

    contents = contents[:i] + b + contents[j:]
    open_testfiles[name] = contents

def found(cmpf, af, bf, inkscape, count, a, b, svg, id):
    cmpf.write(f"""  <g>
    <path style="fill:#ffff00;stroke:none" d="{a}" id="good{count}" />
    <path style="fill:#00ffff;stroke:none;mix-blend-mode:lighten" d="{b}" id="bad{count}" />
  </g>
""")
    af.write(f"{a}\n")
    bf.write(f"{b}\n")
    update_reference_files(inkscape, a, b, svg, id)

def main():
    parser = argparse.ArgumentParser(prog='lpetest-parse.py', description=description, formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('--log', default='joblog.txt', metavar='FILE', help='Input file containing test log')
    parser.add_argument('--cmp', default='path-comparison.svg', metavar='FILE', help='Output file for visualisation of path differences')
    parser.add_argument('--a', default='a.txt', metavar='FILE', help='Output file for original paths')
    parser.add_argument('--b', default='b.txt', metavar='FILE', help='Output file for new paths')
    parser.add_argument('--inkscape', default=None, metavar='DIR', help='Inkscape project root directory')
    args = parser.parse_args()

    path = args.inkscape
    if not path:
        path = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

    if not os.path.isdir(os.path.join(path, '.git')):
        sys.stderr.write(f"Project not found in '{path}'\n\n")
        sys.stderr.write("Please run this from the Inkscape project root directory, or pass it as --inkscape\n")
        sys.exit(-1)

    if not os.path.isfile(args.log):
        sys.stderr.write(f"Test log not found in '{args.log}'\n\n")
        sys.stderr.write("Please run the suite or download log file from a CI pipeline first\n")
        sys.exit(-2)

    with open(args.log, "r") as logf:
        log = logf.read()

    with open(args.cmp, "w") as cmpf, open(args.a, "w") as af, open(args.b, "w") as bf:
        cmpf.write("<svg>\n")
        
        data = {}
        count = 0
        for tag, value in re.findall(r"\s*\d+:\s+(?P<name>svg|id|a|b):\s*\d+:\s+(?P<value>.+)", log):
            data[tag] = value
            if tag == "b":
                count += 1
                found(cmpf, af, bf, path, count, **data)

        cmpf.write("</svg>\n")

    if len(open_testfiles) > 0:
        print("Overwrite these files?")
        for name in open_testfiles.keys():
            print(name)

        if yesnoprompt():
            for name, contents in open_testfiles.items():
                with open(name, "w") as out:
                    out.write(contents)

if __name__ == "__main__":
    main()

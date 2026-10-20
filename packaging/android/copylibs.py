#! /bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later

description = 'Recursively copy dependencies of a library or executable'

import os, subprocess, shutil, argparse
from elftools.elf.elffile import ELFFile

# Get the library search paths from pkg-config.
def get_pkgconfig_search_paths():
    out = subprocess.run('pkg-config --libs-only-L `pkg-config --list-package-names --env-only`', shell=True, check=True, capture_output=True).stdout
    return [x[2:] for x in out.decode('utf-8').split() if x.startswith('-L')]

# Find all libraries in the given search paths.
# Returns the result as a mapping from library names to full paths.
def find_libs(search_paths):
    libs = {}
    for search_path in search_paths:
        for path, dirs, files in os.walk(search_path):
            for f in files:
                if f.endswith('.so'):
                    libs[f] = os.path.join(path, f)
    return libs

# Given a library or executable, return the names of its library dependencies.
def get_imported_libs(path):
    with open(path, 'rb') as f:
        e = ELFFile(f)
        s = e.get_section_by_name('.dynamic')
        return [t.needed for t in s.iter_tags() if t.entry['d_tag'] == 'DT_NEEDED']

# Given a library or executable, recursively add the names of its library dependencies to imported_libs.
def get_imported_libs_recursive(path, all_libs, imported_libs):
    for lib in get_imported_libs(path):
        if lib in imported_libs: continue
        imported_libs.append(lib)
        loc = all_libs.get(lib)
        if loc != None:
            get_imported_libs_recursive(loc, all_libs, imported_libs)

def main():
    parser = argparse.ArgumentParser(prog='copylibs.py', description=description)
    parser.add_argument('-o', '--outputdir', metavar='DIR', help='Directory to copy the libraries to (default: don\'t copy, just list)')
    parser.add_argument('-s', '--search', metavar='DIR', default=[], action='append', help='Specify an additional library search path')
    parser.add_argument('-l', '--link', help='Create symlinks instead of copying', action='store_true')
    parser.add_argument('file', nargs='+', help='The input executable or library')
    args = parser.parse_args()

    # Find all libraries in the search paths.
    search_paths = get_pkgconfig_search_paths() + args.search
    all_libs = find_libs(search_paths)

    # Find all libraries needed by the input files.
    needed_libs = []
    for f in args.file:
        get_imported_libs_recursive(f, all_libs, needed_libs)

    # For each needed lib, find its actual location and copy/symlink to output directory, or report not found
    for lib in needed_libs:
        loc = all_libs.get(lib)
        print(lib, '->', loc)
        if loc != None and args.outputdir != None:
            if args.link:
                os.symlink(loc, os.path.join(args.outputdir, lib))
            else:
                shutil.copy(loc, args.outputdir)

if __name__ == "__main__":
    main()

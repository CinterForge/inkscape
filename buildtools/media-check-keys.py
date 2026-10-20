#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Keys checker:
#
#  * Are there bad xml elements / parsing
#  * Does it contain non-key xml elements
#  * Does the keys file reference something unknown
#  * Does it repeat empty actions
#  * Are there keys formatted as verbs
#  * Are some of the keys missing
#  * Does it include the default xml
#
# Author: Martin Owens <doctormo@geek-2.com>
# Licensed under GPL version 2 or any later version, read the file "COPYING" for more information.

import fnmatch
import os
import sys

from collections import defaultdict

from lxml import etree

KEYS_PATH = os.path.join('.', 'share', 'keys')
DEFAULT = 'inkscape.xml'

IGNORE_MISSING_KEYS = ['org.']

class Keys:
    """Open and parse a keys xml file"""
    def __init__(self, filename):
        self.filename = filename
        self.mods = set()
        self.keys = set()
        self.olds = set()
        self.ticks = defaultdict(list)
        self.errors = False
        self.parse(etree.parse(filename))

    def parse(self, doc):
        """Parse the document into checkable concerns"""
        for child in doc.getroot().getchildren():
            try:
                if child.tag == "modifier":
                    name = child.attrib['action']
                    self.mods.add(name)
                    self.ticks[name].append(child.attrib.get('modifiers'))
                elif child.tag == "bind":
                    name = child.attrib['gaction']
                    self.keys.add(name)
                    self.ticks[name].append(child.attrib.get('keys'))
                    if 'key' in child.attrib or 'modifiers' in child.attrib:
                        self.olds.add(name)
                elif child.tag == "{http://www.w3.org/2001/XInclude}include":
                    self.parse_include(child.attrib['href'])
                elif isinstance(child.tag, str):
                    sys.stderr.write(f"Unrecognised tag in keys file {child.tag}\n")
                    self.errors = True
            except KeyError as err:
                sys.stderr.write(f"Missing attribute g/action in {self.filename}\n")
                self.errors = True

    def parse_include(self, file):
        """Parse in the linked file"""
        other = Keys(os.path.join(os.path.dirname(self.filename), file))
        self.mods = self.mods.union(other.mods)
        self.keys = self.keys.union(other.keys)

    @classmethod
    def others(cls):
        """Load all non default keys"""
        for name in os.listdir(KEYS_PATH):
            filename = os.path.join(KEYS_PATH, name)
            if name == DEFAULT:
                continue
            if not os.path.isfile(filename) or not filename.endswith('.xml'):
                continue
            yield name, cls(filename)

    @classmethod
    def default(cls):
        """Load default keys"""
        return cls(os.path.join(KEYS_PATH, DEFAULT))


if __name__ == '__main__':
    sys.stderr.write("\n\n==== CHECKING KEYBOARD FILES ====\n\n")
    data = defaultdict(set)
    names = set()

    errors = False
    that = Keys.default()


    for name, this in Keys.others():
        sys.stderr.write(f"Checking '{name}'\n")

        for old in this.olds:
            sys.stderr.write(f" ! Old formatted key binding {old}\n")

        add = []
        if '-' not in sys.argv:
            for key in this.keys ^ (that.keys & this.keys):
                sys.stderr.write(f" + Unknown extra key {key}\n")
                errors = True

            for mod in this.mods ^ (that.mods & this.mods):
                sys.stderr.write(f" + Unknown extra modifier {mod}\n")
                errors = True

            for tick, lst in this.ticks.items():
                if len(lst) > 1 and None in lst:
                    sys.stderr.write(f" * Multiple empty references to {tick}\n")

        if '+' not in sys.argv:
            for key in that.keys ^ (that.keys & this.keys):
                if [ig for ig in IGNORE_MISSING_KEYS if key.startswith(ig)]:
                    continue
                if '-' in sys.argv:
                    add.append(f"<bind gaction=\"{key}\" />")
                else:
                    sys.stderr.write(f" - Missing key {key}\n")
                errors = True

            for mod in that.mods ^ (that.mods & this.mods):
                if '-' in sys.argv:
                    add.append(f"<modifier action=\"{mod}\" />")
                else:
                    sys.stderr.write(f" - Missing modifier {mod}\n")
                errors = True

        for item in sorted(add):
            sys.stderr.write(f"  {item}\n")
        sys.stderr.write("\n")

    if errors:
        sys.exit(5)

# vi:sw=4:expandtab:
